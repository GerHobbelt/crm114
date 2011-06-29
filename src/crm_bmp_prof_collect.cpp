#include "crm_bmp_prof.h"





int collect_and_display_requested_data(FILE *inf, int read_size, CRM_ANALYSIS_PROFILE_ELEMENT *store, int store_size)
{
    int i;
    int init_marker_active = 0;
    int64_t run_time_consumption = 0;
    int context_switches_count = 0;
    int64_t active_clock_freq = 0;
    int64_t active_clock_rez = 0;
    int active_classifier = UNIDENTIFIED_CLASSIFIER;
    int active_classifier_hashfile_size = 0;
    int active_hash_function = 0;
    int64_t timing_operation_start = 0;
    int probe_count = 0;
    int probe_hit_count = 0;
    int probe_direct_hit_count = 0;
    int probe_refute_hit_count = 0;
    int probe_miss_count = 0;
    int probe_groom_count = 0;
    int run_count = 0;
    int active_opcode = -1;
    int previous_store_elem_id = -1;
    int64_t previous_elem_time_mark = 0;
    int hit_valid_store_elem = 1;              // once this is set to 0, scan until hitting another header!


    for (i = 0; i < read_size + 1 /* one extra slot for sentinel marker */;)
    {
        int num_to_load = CRM_MIN(store_size, read_size - i);
        int rcnt = 0;
        int idx;

        if (num_to_load > 0)
        {
            rcnt = (int)fread(store, sizeof(store[0]), num_to_load, inf);
        }
        if (rcnt != num_to_load)
        {
            fprintf(stderr, "Cannot read %d elements from the capture file '%s': %d elements have been read & error %d(%s)\n",
                    num_to_load,
                    cfg.input_file,
                    rcnt,
                    errno,
                    errno_descr(errno));
            return -1;
        }

        // make sure we write one extra MARK_TERMINATION mark as a sentinel so
        // all timings, etc. are completed for the very last run in the capture,
        // assuming that capture was not capable of a MARK_TERMINATION mark itself:
        if (rcnt < store_size)
        {
            CRM_ANALYSIS_PROFILE_ELEMENT *elem = &store[rcnt];

            memset(elem, 0, sizeof(elem[0]));
            elem->marker = (0xDEADBEEFULL << 16) | MARK_TERMINATION;
            rcnt++;
        }

        for (idx = 0; idx < rcnt; idx++)
        {
            CRM_ANALYSIS_PROFILE_ELEMENT *elem = &store[idx];
            int elem_index_in_file = i + idx;

            // first, let's decipher the contents of this one store[] element then:
            int store_elem_id = (int)(elem->marker & 0xFFF);
            int value_cnt = (int)((elem->marker >> 12) & 0xF);
            int64_t extra_val = elem->marker >> 16;
            // the other bits of the store[elem] can stay as-is.

            if (!hit_valid_store_elem)
            {
                switch (store_elem_id)
                {
                default:
                    // see if this element is a native; that would make things rather easy...
                    if (elem->marker == 0x123456789ABCDEF0ULL
                       && elem->value[0].as_int == 0x8081828384858687ULL
                       && elem->value[1].as_float == 1.0
                       && elem->value[2].as_int == 0x9091929394959697ULL
                       && elem->time_mark == 0xA0A1A2A3A4A5A6A7ULL)
                    {
                        // A-OK!
                        hit_valid_store_elem = 1;
                    }
                    break;

                case MARK_NUL_SENTINEL:
                    // special case: if not preceeded by a TERMINATION marker, it means the CRM114 session was aborted or crashed.
                    break;
                }
            }
            else
            {
                ENSURE(value_cnt <= 3);

                switch (store_elem_id)
                {
                default:
                    hit_valid_store_elem = 0;

                    if (!cfg.skip_errors)
                    {
                        return -1;
                    }
                    break;

                case MARK_INIT:
                    // crm_analysis_mark(&analysis_cfg, MARK_INIT, 0, "iLL", 1, (long long int)clock_freq, (long long int)clock_rez);
                    ENSURE(value_cnt == 3);
                    ENSURE(extra_val == 0);

                    if (init_marker_active == 1)
                    {
                        int64_t time_used;

                        // also terminating a previous init!
                        run_time_consumption = elem->time_mark - run_time_consumption;
                        // convert to nsecs:
                        time_used = counter2nsecs(active_clock_freq, run_time_consumption);
                    }

                    /* reset some knowledge as it has to be re-determined during this episode again: */
                    active_classifier = UNIDENTIFIED_CLASSIFIER;
                    active_classifier_hashfile_size = 0;
                    active_hash_function = (int)elem->value[2].as_int;

                    active_clock_freq = elem->value[0].as_int;
                    active_clock_rez = elem->value[1].as_int;

                    active_opcode = -1;
                    //timing_operation_start = 0;

                    context_switches_count = 0;
                    init_marker_active = 1;              // mark the INIT happened in there.
                    run_time_consumption = elem->time_mark;

                    run_count++;

                    probe_count = 0;
                    probe_hit_count = 0;
                    probe_direct_hit_count = 0;
                    probe_refute_hit_count = 0;
                    probe_miss_count = 0;
                    probe_groom_count = 0;

                    if (cfg.verbosity >= 3)
                    {
                        fprintf(stdout, "\n--- MARK_INIT %d ---\n", run_count);
                    }
                    break;

                case MARK_SWITCH_CONTEXT:
                    // crm_analysis_mark(&analysis_cfg, MARK_SWITCH_CONTEXT, 0, "");
                    ENSURE(value_cnt == 0);
                    ENSURE(extra_val == 0);
                    ENSURE_W_DESCR(init_marker_active == 1, "checks if an active MARK_INIT came before this MARK_SWITCH_CONTEXT");

                    context_switches_count++;
                    break;

                case MARK_NUL_SENTINEL:
                    // special case: if not preceeded by a TERMINATION marker, it means the CRM114 session was aborted or crashed.
                    hit_valid_store_elem = 0;

                    if (previous_store_elem_id != MARK_TERMINATION)
                    {
                        static int msg_written = 0;

                        if (!msg_written)
                        {
                            msg_written++;
                            fprintf(
                                stderr,
                                "Aparently, a CRM114 profiling session has been rudely aborted!\n"
                                "Expected TERMINATION marker ID %d, but instead got previous marker ID %d at offset %ld in capture file '%s'.\n",
                                MARK_TERMINATION,
                                previous_store_elem_id,
                                (long int)(ftell(inf) - (rcnt - idx + 1) * sizeof(store[0])),
                                // previous store elem!!!
                                cfg.input_file);
                            if (!cfg.skip_errors)
                            {
                                return -1;
                            }
                            else
                            {
                                fprintf(stderr, "(Ignoring that error. Your statistics will be off / untrustworthy. Continuing nevertheless...)\n");
                            }
                        }

                        // act as a 'fake' MARK_TERMINATION; though we don't have precise timing, so we fake that too.
                        if (cfg.verbosity >= 3)
                        {
                            fprintf(stdout, "\n--- MARK_TERMINATION ---\n");
                        }

                        if (init_marker_active == 1)
                        {
                            int64_t time_used;

                            // sentinel hit? OR did we simply hit an end of a run?
                            //
                            // close everything down anyway. FAKE the timing.
                            //
                            // also terminating a previous init!
                            run_time_consumption = previous_elem_time_mark - run_time_consumption;
                            // convert to nsecs & store:
                            time_used = counter2nsecs(active_clock_freq, run_time_consumption);
                        }
                        //active_clock_freq = 0;
                        //active_clock_rez = 0;

                        active_classifier = UNIDENTIFIED_CLASSIFIER;
                        active_classifier_hashfile_size = 0;
                        active_hash_function = -1;

                        active_opcode = -1;
                        //timing_operation_start = 0;

                        context_switches_count = 0;
                        init_marker_active = 0;                         // de-activate INIT/RUN
                        run_time_consumption = 0;                       // reset time mark too:
                    }
                    break;

                case MARK_TERMINATION:
                    // crm_analysis_mark(&analysis_cfg, MARK_TERMINATION, 0, "");
                    ENSURE(value_cnt == 0);
                    ENSURE(extra_val == 0 || extra_val == 0xDEADBEEFULL);

                    if (cfg.verbosity >= 3)
                    {
                        fprintf(stdout, "\n--- MARK_TERMINATION ---\n");
                    }

                    if (extra_val == 0xDEADBEEFULL || init_marker_active == 1)
                    {
                        int64_t time_used;

                        // sentinel hit? OR did we simply hit an end of a run?
                        //
                        // close everything down anyway.
                        //
                        // also terminating a previous init!
                        run_time_consumption = elem->time_mark - run_time_consumption;
                        // convert to nsecs & store:
                        time_used = counter2nsecs(active_clock_freq, run_time_consumption);
                    }
                    //active_clock_freq = 0;
                    //active_clock_rez = 0;

                    active_classifier = UNIDENTIFIED_CLASSIFIER;
                    active_classifier_hashfile_size = 0;
                    active_hash_function = -1;

                    active_opcode = -1;
                    //timing_operation_start = 0;

                    context_switches_count = 0;
                    init_marker_active = 0;                             // de-activate INIT/RUN
                    run_time_consumption = elem->time_mark;             // reset time mark too:
                    break;

                case MARK_DEBUG_INTERACTION:
                    // crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 0, "");
                    // crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 1, "");
                    // crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 2, "");
                    // crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 3, "");
                    // crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 4, "");
                    break;

                case MARK_HASH_VALUE:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASH_VALUE, len, "iLL", (unsigned int)h, (long long int)cvt_chars2int64(str, len), (long long int)cvt_chars2int64(str + 8, (len > 8 ? len - 8 : 0)));
                    {
                        char buf[2 * 8 + 1];
                        crmhash_t hash = (crmhash_t)elem->value[0].as_int;
                        unsigned int hash_divisor;
                        int id;
                        int pos;

                        ENSURE(value_cnt == 3);
                        ENSURE(extra_val >= 0);

                        cvt_int64_2_chars(&buf[0], elem->value[1].as_int);
                        cvt_int64_2_chars(&buf[8], elem->value[2].as_int);
                        buf[16] = 0;

                        //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.hash.hash, hash);
                        //UPDATE_MINMAX(prof_minmax.hash.hash_input_len, (int)extra_val);
                        //if (memcmp(prof_minmax.hash.input_str_min, buf, 2 * 8 + 1) > 0)

                        hash_divisor = (unsigned int)((1LL << 32) / HASH_DISTRIBUTION_GRANULARITY);
                        id = hash / hash_divisor;
                        if (id < 0)
                        {
                            id = 0;
                        }
                        else if (id >= HASH_DISTRIBUTION_GRANULARITY)
                        {
                            id = HASH_DISTRIBUTION_GRANULARITY - 1;
                        }
                        report_data.hash_distro_counts[id]++;

                        hash_divisor = (unsigned int)((1LL << 32) / HASH_PER_CHAR_GRANULARITY);
                        for (pos = 0; pos < 16; pos++)
                        {
                            // assume [[:graph:]]* regex for tokenizing the crm114 input: NUL bytes do not happen then.
                            if (!buf[pos])
                                break;

                            id = hash / hash_divisor;
                            if (id < 0)
                            {
                                id = 0;
                            }
                            else if (id >= HASH_PER_CHAR_GRANULARITY)
                            {
                                id = HASH_PER_CHAR_GRANULARITY - 1;
                            }
                            report_data.hash_per_char_counts[(unsigned char)buf[pos]][pos][id]++;
                        }
                    }
                    break;

                case MARK_HASH64_VALUE:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASH64_VALUE, len, "LLL", (unsigned long long int)h, (long long int)cvt_chars2int64(str, len), (long long int)cvt_chars2int64(str + 8, (len > 8 ? len - 8 : 0)));
                    {
                        char buf[2 * 8 + 1];
                        crmhash64_t hash = (crmhash64_t)elem->value[0].as_int;

                        ENSURE(value_cnt == 3);
                        ENSURE(extra_val >= 0);

                        cvt_int64_2_chars(&buf[0], elem->value[1].as_int);
                        cvt_int64_2_chars(&buf[8], elem->value[2].as_int);
                        buf[16] = 0;

                        //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.hash64.hash, hash);
                        //UPDATE_MINMAX(prof_minmax.hash64.hash_input_len, (int)extra_val);
                        //if (memcmp(prof_minmax.hash64.input_str_min, buf, 2 * 8 + 1) > 0)
                    }
                    break;

                case MARK_VT_HASH_VALUE:
                case MARK_HASH_CONTINUATION:
                    // TBD
                    // TODO
                    break;

                case MARK_OPERATION:
                    // crm_analysis_mark(&analysis_cfg, MARK_OPERATION, csl->cstmt, "iL", csl->mct[csl->cstmt]->stmt_type, (unsigned long long int)csl->mct[csl->cstmt]->apb.sflags);
                    // crm_analysis_mark(&analysis_cfg, MARK_OPERATION, tstmt, "");
                    if (value_cnt == 2)
                    {
                        // start of statement:
                        uint64_t opcode_flags;
                        active_opcode = (int)elem->value[0].as_int;

                        timing_operation_start = elem->time_mark;

#if 0
                        if (active_opcode >= 0 && active_opcode <= CRM_UNIMPLEMENTED)
                        {
                            report_data.opcode_counts[active_opcode]++;
                        }
                        else
                        {
                            report_data.opcode_counts[CRM_UNIMPLEMENTED + 1]++;
                        }
#endif
                        //UPDATE_MINMAX(prof_minmax.operation.statement_no, (int)extra_val);

                        opcode_flags = elem->value[1].as_int;

                        probe_count = 0;
                        probe_hit_count = 0;
                        probe_direct_hit_count = 0;
                        probe_refute_hit_count = 0;
                        probe_miss_count = 0;
                        probe_groom_count = 0;
                    }
                    else
                    {
                        // end of statement:
                        uint64_t dt;

                        ENSURE(value_cnt == 0);

                        dt = elem->time_mark - timing_operation_start;

                        if (active_opcode >= 0 && active_opcode <= CRM_UNIMPLEMENTED)
                        {
                            report_data.opcode_times[active_opcode] += counter2nsecs(active_clock_freq, dt);
                            report_data.opcode_counts[active_opcode]++;
                        }
                        else
                        {
                            // fake it: we've got corrupted timing anyhow!
                            dt = elem->time_mark - timing_operation_start;
                            if (dt > 1000000000)
                                dt = 1000000;

                            report_data.opcode_times[CRM_UNIMPLEMENTED + 1] += counter2nsecs(active_clock_freq, dt);
                            report_data.opcode_counts[CRM_UNIMPLEMENTED + 1]++;
                        }
                    }
                    break;

                case MARK_CLASSIFIER:
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 0, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 1, "Li", (unsigned long long int)apb->sflags, (int)hfsize);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 2, "Lii", (unsigned long long int)apb->sflags, (int)hashlens[maxhash], (int)maxhash);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 3, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 4, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 5, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 6, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 7, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 8, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 9, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 10, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 11, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 12, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 13, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 14, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 15, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 16, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 17, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 18, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 19, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 20, "L", (unsigned long long int)classifier_flags);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 21, "L", (unsigned long long int)classifier_flags);
                    ENSURE(value_cnt >= 1);
                    ENSURE(extra_val >= 0);
                    ENSURE(extra_val <= 21);
                    {
                        uint64_t classifier_flags = (uint64_t)elem->value[0].as_int;
                        CRM_PROF_CLASSIFIER decoded_classifier = UNIDENTIFIED_CLASSIFIER;

                        if (classifier_flags & CRM_OSB_BAYES)
                        {
                            decoded_classifier = CLASSIFIER_OSB_BAYES;
                        }
                        else if (classifier_flags & CRM_CORRELATE)
                        {
                            decoded_classifier = CLASSIFIER_CORRELATE;
                        }
                        else if (classifier_flags & CRM_OSB_WINNOW)
                        {
                            decoded_classifier = CLASSIFIER_OSB_WINNOW;
                        }
                        else if (classifier_flags & CRM_OSBF)
                        {
                            decoded_classifier = CLASSIFIER_OSBF_BAYES;
                        }
                        else if (classifier_flags & CRM_HYPERSPACE)
                        {
                            decoded_classifier = CLASSIFIER_HYPERSPACE;
                        }
                        else if (classifier_flags & CRM_ENTROPY)
                        {
                            decoded_classifier = CLASSIFIER_ENTROPY;
                        }
                        else if (classifier_flags & CRM_SVM)
                        {
                            decoded_classifier = CLASSIFIER_SVM;
                        }
                        else if (classifier_flags & CRM_SKS)
                        {
                            decoded_classifier = CLASSIFIER_SKS;
                        }
                        else if (classifier_flags & CRM_FSCM)
                        {
                            decoded_classifier = CLASSIFIER_FSCM;
                        }
                        else if (classifier_flags & CRM_NEURAL_NET)
                        {
                            decoded_classifier = CLASSIFIER_NEURAL_NET;
                        }
                        else if (classifier_flags & CRM_MARKOVIAN)
                        {
                            decoded_classifier = CLASSIFIER_MARKOVIAN;
                        }
                        else
                        {
                            //    Default with no classifier specified is Markov
                            decoded_classifier = CLASSIFIER_MARKOVIAN;
                        }
                        active_classifier = decoded_classifier;
                        active_classifier_hashfile_size = 0;
                        // unknown hashfile size yet; maybe this marker is a classifier marker type 1 or 2: those carry hashfile size:
                        if (value_cnt >= 2)
                        {
                            // yes, this marker does know the size:
                            active_classifier_hashfile_size = (int)elem->value[1].as_int;
                        }

                        switch (extra_val)
                        {
                        case 0:
                            // start learn
                            break;

                        case 3:
                            // end learn
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.learn_probe_count, probe_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.learn_probe_hit_count, probe_hit_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.learn_probe_refute_hit_count, probe_refute_hit_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.groom_count, probe_groom_count);
                            break;

                        case 4:
                            // start classify
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.probe_count, probe_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.probe_hit_count, probe_hit_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.direct_hit_count, probe_direct_hit_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.miss_count, probe_miss_count);
                            //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.groom_count, probe_groom_count);
                            break;

                        case 5:
                            // end classify
                            break;

                        case 6:
                        case 8:
                        case 10:
                        case 12:
                        case 14:
                        case 16:
                        case 18:
                        case 20:
                            // CSS tool commands start
                            break;

                        case 7:
                        case 9:
                        case 11:
                        case 13:
                        case 15:
                        case 17:
                        case 19:
                        case 21:
                            // CSS tool commands end
                            break;
                        }
                        probe_count = 0;
                        probe_hit_count = 0;
                        probe_direct_hit_count = 0;
                        probe_refute_hit_count = 0;
                        probe_miss_count = 0;
                        probe_groom_count = 0;
                    }
                    break;

                case MARK_CLASSIFIER_DB_TOTALS:
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_DB_TOTALS, ifile, "ii", (int)info_block[ifile]->v.OSB_Bayes.learncount, (int)info_block[ifile]->v.OSB_Bayes.features_learned);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    ENSURE(extra_val <= 16);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.classifier.learn_count, elem->value[0].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.classifier.feature_count, elem->value[1].as_int);
                    break;

                case MARK_CHAIN_LENGTH:
                    // crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)(lh - lh0), (int)hashes[k][lh].value);
                    // crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    break;

                case MARK_HASHPROBE:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)hindex, (unsigned int)h1, (unsigned int)h2);
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)lh, (unsigned int)h1, (unsigned int)h2);
                    ENSURE(value_cnt == 3);
                    ENSURE(extra_val >= 0);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.hash, elem->value[0].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.h1_index, elem->value[1].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.h2_index, elem->value[2].as_int);

                    probe_count++;
                    break;

                case MARK_HASHPROBE_DIRECT_HIT:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)(lh - lh0), (int)feature_weight);
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    probe_direct_hit_count++;
                    break;

                case MARK_HASHPROBE_HIT:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "iii", (int)(lh - lh0), (int)feature_weight, (int)totalhits[k]);
                    ENSURE(value_cnt >= 2);
                    ENSURE(extra_val >= 0);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    if (value_cnt > 2)
                    {
                        //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.total_hit_count, elem->value[2].as_int);
                    }
                    probe_hit_count++;
                    break;

                case MARK_HASHPROBE_HIT_REFUTE:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT_REFUTE, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    probe_refute_hit_count++;
                    break;

                case MARK_HASHPROBE_MISS:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_MISS, h1, "i", (unsigned int)(lh - lh0));
                    ENSURE(value_cnt == 1);
                    ENSURE(extra_val >= 0);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.miss_chain_length, elem->value[0].as_int);
                    probe_miss_count++;
                    break;

                case MARK_MICROGROOM:
                    // crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "i", (int)incrs);
                    // crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "ii", (int)incrs, (int)zeroedfeatures);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 1);
                    ENSURE(extra_val <= 2);
                    //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.groom_chain_length, elem->value[0].as_int);
                    if (value_cnt > 1)
                    {
                        //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.grooming_zeroed_features, elem->value[1].as_int);
                    }
                    probe_groom_count++;
                    break;

                case MARK_CLASSIFIER_PARAMS:
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 1, "idi", (int)ifile, (double)cpcorr[ifile], (int)total_learns);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 2, "iii", (int)htf, (int)skip_this_feature, (int)feature_weight);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 3, "idd", (int)k, ptc[k], pltc[k]);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 4, "ddd", renorm, (double)hits[k], (double)htf);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 5, "idd", (int)k, ptc[k], chi2[k]);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 6, "did", (double)expected, (int)unk_features, (double)actual);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 7, "idd", (int)k, tprob, ptc[k]);
                    // crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 8, "dd", tprob, overall_pR);
                    ENSURE(value_cnt >= 2);
                    ENSURE(extra_val >= 1);
                    ENSURE(extra_val <= 8);
                    switch (extra_val)
                    {
                    case 1:
                        //UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.classifier.learn_count, elem->value[2].as_int);
                        break;

                    default:
                        break;
                    }
                    break;

                case MARK_CSS_STATS_GROUP:
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 1, "iii", hfsize, fbuckets, zvbins);
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 2, "iii", ofbins, docs_learned, features_learned);
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 3, "Ldd", (long long int)sum, avg_datums_per_bucket, avg_pack_density);
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 4, "idi", maxchain, avg_ovchain_length, specials);
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 5, "i", specials_in_chains);
                    ENSURE(value_cnt >= 1);
                    ENSURE(extra_val >= 1);
                    ENSURE(extra_val <= 5);
                    break;

                case MARK_CSS_STATS_HISTOGRAM:
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_HISTOGRAM, ev, "iii", val[0], val[1], val[2]);
                    // crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_HISTOGRAM, ev, (vc == 2 ? "ii" : "i"), val[0], val[1]);
                    ENSURE(value_cnt >= 1);
                    break;
                }

                if (hit_valid_store_elem)
                {
                    previous_store_elem_id = store_elem_id;
                    previous_elem_time_mark = elem->time_mark;
                }
            }
        }
        i += rcnt;
    }

    return 0;
}



