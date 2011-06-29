#include "crm_bmp_prof.h"




#define ENSURE(comparison) \
    ENSURE_W_DESCR(comparison, NULL)

#define ENSURE_W_DESCR(comparison, description)                                     \
    {                                                                               \
        static int msg_written = 0;                                                 \
                                                                                    \
        if (check_validity_of_expression(SRC_LOC(),                                 \
                    store_elem_id, elem_index_in_file,                              \
                    comparison, # comparison, description,                          \
                    msg_written))                                                   \
        {                                                                           \
            hit_valid_store_elem = 0;                                               \
            if (!msg_written)                                                       \
            {                                                                       \
                msg_written++;                                                      \
                if (!cfg.skip_errors)                                               \
                {                                                                   \
                    return -1;                                                      \
                }                                                                   \
                else                                                                \
                {                                                                   \
                    fprintf(stderr, "(Ignoring that error. "                        \
                                    "Your statistics will be off / untrustworthy. " \
                                    "Continuing nevertheless...)\n");               \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    }






CRM_PROF_GROUP_MINMAX prof_minmax;





int check_validity_of_expression(int lineno, const char *srcfile, const char *funcname,
        int store_elem_id, int elem_index_in_file,
        int comparison, const char *comparison_msg, const char *comparison_description,
        int show_message)
{
    if (!comparison && show_message)
    {
        char reason[4096];

        generate_err_reason_msg_va(
                reason,
                WIDTHOF(reason),
                lineno,
                srcfile,
                funcname,
                "*ERROR*",
                NULL,
                NULL,
                0,
                "internal validation failed for capture element (type: %d, index: %d) in input file '%s': failing comparison: '%s'%s%s\n"
                "Are you sure you fed us a real crm114 ANALYSIS profile capture file, generated using 'CRM114 -A'?\n"
                "If so, please contact Ger Hobbelt at ger@hobbelt.com ro report this anomaly, thank you.\n",
                store_elem_id,
                elem_index_in_file,
                cfg.input_file,
                comparison_msg,
                (comparison_description ? " -- description: " : ""),
                (comparison_description ? comparison_description : ""));
        fputs(reason, stderr);
    }
    return !comparison;
}






int scan_to_determine_input_ranges(FILE *inf, int read_size, CRM_ANALYSIS_PROFILE_ELEMENT *store, int store_size)
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
    int active_opcode = CRM_UNIMPLEMENTED;
    int previous_store_elem_id = -1;
    int64_t previous_elem_time_mark = 0;
    int hit_valid_store_elem = 1;              // once this is set to 0, scan until hitting another header!

#define UPDATE_MINMAX(dst_base_identifier_name, src_val) \
    if (dst_base_identifier_name ## _min > src_val)      \
        dst_base_identifier_name ## _min = src_val;      \
    if (dst_base_identifier_name ## _max > src_val)      \
        dst_base_identifier_name ## _max = src_val;

#define UPDATE_MINMAX_PER_CLASSIFIER(dst_base_identifier_name, src_val) \
    if (dst_base_identifier_name ## _min[active_classifier] > src_val)  \
        dst_base_identifier_name ## _min[active_classifier] = src_val;  \
    if (dst_base_identifier_name ## _max[active_classifier] > src_val)  \
        dst_base_identifier_name ## _max[active_classifier] = src_val;


    // zero the stats before we start
    memset(&prof_minmax, 0, sizeof(prof_minmax));

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
                static int msg_written = 0;

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
                        msg_written = 0;
                    }
                    else
                    {
                        if (!msg_written)
                        {
                            msg_written++;
                            fprintf(stderr, "Skipping probably invalid store element!\n");
                        }
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

                    {
                        static int msg_written = 0;

                        if (!msg_written)
                        {
                            msg_written++;
                            fprintf(stderr, "Unidentified marker ID %d found at offset %ld in capture file '%s'. This is not one of mine!\n",
                                    store_elem_id,
                                    (long int)(ftell(inf) - (rcnt - idx) * sizeof(store[0])),
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
                    }
                    break;

                    /*
                     * These are the marker calls writing to CAP:
                     *
                     * crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)(lh - lh0), (int)hashes[k][lh].value);
                     * crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                     * crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 0, "L", (unsigned long long int)classifier_flags);
                     * crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 1, "Li", (unsigned long long int)apb->sflags, (int)hfsize);
                     * crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 2, "Lii", (unsigned long long int)apb->sflags, (int)hashlens[maxhash], (int)maxhash);
                     * crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_DB_TOTALS, ifile, "ii", (int)info_block[ifile]->v.OSB_Bayes.learncount, (int)info_block[ifile]->v.OSB_Bayes.features_learned);
                     * crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 0, "");
                     * crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 1, "");
                     * crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 2, "");
                     * crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 3, "");
                     * crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 4, "");
                     * crm_analysis_mark(&analysis_cfg, MARK_HASH64_VALUE, len, "LLL", (unsigned long long int)h, (long long int)cvt_chars2int64(str, len), (long long int)cvt_chars2int64(str + 8, (len > 8 ? len - 8 : 0)));
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)hindex, (unsigned int)h1, (unsigned int)h2);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)lh, (unsigned int)h1, (unsigned int)h2);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)(lh - lh0), (int)feature_weight);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "iii", (int)(lh - lh0), (int)feature_weight, (int)totalhits[k]);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT_REFUTE, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                     * crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_MISS, h1, "i", (unsigned int)(lh - lh0));
                     * crm_analysis_mark(&analysis_cfg, MARK_HASH_VALUE, len, "iLL", (unsigned int)h, (long long int)cvt_chars2int64(str, len), (long long int)cvt_chars2int64(str + 8, (len > 8 ? len - 8 : 0)));
                     * crm_analysis_mark(cfg, MARK_INIT, 0, "LLi", (long long int)clock_freq, (long long int)clock_rez, (int)selected_hashfunction);
                     * crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "i", (int)incrs);
                     * crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "ii", (int)incrs, (int)zeroedfeatures);
                     * crm_analysis_mark(&analysis_cfg, MARK_OPERATION, csl->cstmt, "iL", csl->mct[csl->cstmt]->stmt_type, (unsigned long long int)csl->mct[csl->cstmt]->apb.sflags);
                     * crm_analysis_mark(&analysis_cfg, MARK_SWITCH_CONTEXT, 0, "");
                     * crm_analysis_mark(&analysis_cfg, MARK_TERMINATION, 0, "");
                     *
                     * Convert these markers to possible dimensions and determine min/max for each
                     */
                case MARK_INIT:
                    // crm_analysis_mark(&analysis_cfg, MARK_INIT, 0, "iLL", 1, (long long int)clock_freq, (long long int)clock_rez);
                    ENSURE(value_cnt == 3);
                    ENSURE(extra_val == 0);

                    if (init_marker_active == 1)
                    {
                        // also terminating a previous init!
                        run_time_consumption = elem->time_mark - run_time_consumption;
                        // convert to nsecs & store:
                        prof_minmax.total_time_consumption_max += counter2nsecs(active_clock_freq, run_time_consumption);
                        UPDATE_MINMAX(prof_minmax.run_time_consumption, run_time_consumption);
                        UPDATE_MINMAX(prof_minmax.run.context_switches, context_switches_count);
                        UPDATE_MINMAX(prof_minmax.run.clock_freq, active_clock_freq);
                        UPDATE_MINMAX(prof_minmax.run.clock_rez, active_clock_rez);
                    }

                    /* reset some knowledge as it has to be re-determined during this episode again: */
                    active_classifier = UNIDENTIFIED_CLASSIFIER;
                    active_classifier_hashfile_size = 0;
                    active_hash_function = (int)elem->value[2].as_int;

                    active_clock_freq = elem->value[0].as_int;
                    active_clock_rez = elem->value[1].as_int;

                    active_opcode = CRM_UNIMPLEMENTED;
                    timing_operation_start = 0;

                    context_switches_count = 0;
                    init_marker_active = 1;              // mark the INIT happened in there.
                    run_time_consumption = elem->time_mark;

                    prof_minmax.run.run_count++;

                    probe_count = 0;
                    probe_hit_count = 0;
                    probe_direct_hit_count = 0;
                    probe_refute_hit_count = 0;
                    probe_miss_count = 0;
                    probe_groom_count = 0;

                    if (cfg.verbosity >= 3)
                    {
                        fprintf(stdout, "\n--- MARK_INIT %d ---\n", prof_minmax.run.run_count);
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
                            // sentinel hit? OR did we simply hit an end of a run?
                            //
                            // close everything down anyway. FAKE the timing.
                            //
                            // also terminating a previous init!
                            run_time_consumption = previous_elem_time_mark - run_time_consumption;
                            // convert to nsecs & store:
                            prof_minmax.total_time_consumption_max += counter2nsecs(active_clock_freq, run_time_consumption);
                            UPDATE_MINMAX(prof_minmax.run_time_consumption, run_time_consumption);
                            UPDATE_MINMAX(prof_minmax.run.context_switches, context_switches_count);
                            UPDATE_MINMAX(prof_minmax.run.clock_freq, active_clock_freq);
                            UPDATE_MINMAX(prof_minmax.run.clock_rez, active_clock_rez);
                        }
                        //active_clock_freq = 0;
                        //active_clock_rez = 0;

                        active_classifier = UNIDENTIFIED_CLASSIFIER;
                        active_classifier_hashfile_size = 0;
                        active_hash_function = -1;

                        active_opcode = CRM_UNIMPLEMENTED;
                        timing_operation_start = 0;

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
                        // sentinel hit? OR did we simply hit an end of a run?
                        //
                        // close everything down anyway.
                        //
                        // also terminating a previous init!
                        run_time_consumption = elem->time_mark - run_time_consumption;
                        // convert to nsecs & store:
                        prof_minmax.total_time_consumption_max += counter2nsecs(active_clock_freq, run_time_consumption);
                        UPDATE_MINMAX(prof_minmax.run_time_consumption, run_time_consumption);
                        UPDATE_MINMAX(prof_minmax.run.context_switches, context_switches_count);
                        UPDATE_MINMAX(prof_minmax.run.clock_freq, active_clock_freq);
                        UPDATE_MINMAX(prof_minmax.run.clock_rez, active_clock_rez);
                    }
                    //active_clock_freq = 0;
                    //active_clock_rez = 0;

                    active_classifier = UNIDENTIFIED_CLASSIFIER;
                    active_classifier_hashfile_size = 0;
                    active_hash_function = -1;

                    active_opcode = CRM_UNIMPLEMENTED;
                    timing_operation_start = 0;

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

                        ENSURE(value_cnt == 3);
                        ENSURE(extra_val >= 0);

                        cvt_int64_2_chars(&buf[0], elem->value[1].as_int);
                        cvt_int64_2_chars(&buf[8], elem->value[2].as_int);
                        buf[16] = 0;

                        UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.hash.hash, hash);
                        UPDATE_MINMAX(prof_minmax.hash.hash_input_len, (int)extra_val);
                        if (memcmp(prof_minmax.hash.input_str_min, buf, 2 * 8 + 1) > 0)
                        {
                            memcpy(prof_minmax.hash.input_str_min, buf, 2 * 8 + 1);
                            prof_minmax.hash.input_str_min_len = (int)extra_val;
                            if (cfg.verbosity >= 3)
                            {
                                fprintf(stdout, "\nMARK_HASH_VALUE: new lower bound for input range: '%.*s'\n",
                                        prof_minmax.hash.input_str_min_len,
                                        prof_minmax.hash.input_str_min);
                            }
                        }
                        if (memcmp(prof_minmax.hash.input_str_max, buf, 2 * 8 + 1) < 0)
                        {
                            memcpy(prof_minmax.hash.input_str_max, buf, 2 * 8 + 1);
                            prof_minmax.hash.input_str_max_len = (int)extra_val;
                            if (cfg.verbosity >= 3)
                            {
                                fprintf(stdout, "\nMARK_HASH_VALUE: new upper bound for input range: '%.*s'\n",
                                        prof_minmax.hash.input_str_max_len,
                                        prof_minmax.hash.input_str_max);
                            }
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

                        UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.hash64.hash, hash);
                        UPDATE_MINMAX(prof_minmax.hash64.hash_input_len, (int)extra_val);
                        if (memcmp(prof_minmax.hash64.input_str_min, buf, 2 * 8 + 1) > 0)
                        {
                            memcpy(prof_minmax.hash64.input_str_min, buf, 2 * 8 + 1);
                            prof_minmax.hash.input_str_min_len = (int)extra_val;
                            if (cfg.verbosity >= 3)
                            {
                                fprintf(stdout, "\nMARK_HASH64_VALUE: new lower bound for input range: '%.*s'\n",
                                        prof_minmax.hash.input_str_min_len,
                                        prof_minmax.hash.input_str_min);
                            }
                        }
                        if (memcmp(prof_minmax.hash64.input_str_max, buf, 2 * 8 + 1) < 0)
                        {
                            memcpy(prof_minmax.hash64.input_str_max, buf, 2 * 8 + 1);
                            prof_minmax.hash.input_str_max_len = (int)extra_val;
                            if (cfg.verbosity >= 3)
                            {
                                fprintf(stdout, "\nMARK_HASH64_VALUE: new upper bound for input range: '%.*s'\n",
                                        prof_minmax.hash.input_str_max_len,
                                        prof_minmax.hash.input_str_max);
                            }
                        }
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

                        ENSURE(active_opcode >= 0);
                        ENSURE(active_opcode <= CRM_UNIMPLEMENTED);

                        timing_operation_start = elem->time_mark;

                        UPDATE_MINMAX(prof_minmax.operation.opcode, active_opcode);
                        UPDATE_MINMAX(prof_minmax.operation.statement_no, (int)extra_val);

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

                        UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.operation.opcode_consumption, dt);
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
                        else    if (classifier_flags & CRM_CORRELATE)
                        {
                            decoded_classifier = CLASSIFIER_CORRELATE;
                        }
                        else    if (classifier_flags & CRM_OSB_WINNOW)
                        {
                            decoded_classifier = CLASSIFIER_OSB_WINNOW;
                        }
                        else    if (classifier_flags & CRM_OSBF)
                        {
                            decoded_classifier = CLASSIFIER_OSBF_BAYES;
                        }
                        else    if (classifier_flags & CRM_HYPERSPACE)
                        {
                            decoded_classifier = CLASSIFIER_HYPERSPACE;
                        }
                        else    if (classifier_flags & CRM_ENTROPY)
                        {
                            decoded_classifier = CLASSIFIER_ENTROPY;
                        }
                        else    if (classifier_flags & CRM_SVM)
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
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.learn_probe_count, probe_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.learn_probe_hit_count, probe_hit_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.learn_probe_refute_hit_count, probe_refute_hit_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.groom_count, probe_groom_count);
                            break;

                        case 4:
                            // start classify
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.probe_count, probe_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.probe_hit_count, probe_hit_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.direct_hit_count, probe_direct_hit_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.miss_count, probe_miss_count);
                            UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.groom_count, probe_groom_count);
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
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.classifier.learn_count, elem->value[0].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.classifier.feature_count, elem->value[1].as_int);
                    break;

                case MARK_CHAIN_LENGTH:
                    // crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)(lh - lh0), (int)hashes[k][lh].value);
                    // crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    break;

                case MARK_HASHPROBE:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)hindex, (unsigned int)h1, (unsigned int)h2);
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)lh, (unsigned int)h1, (unsigned int)h2);
                    ENSURE(value_cnt == 3);
                    ENSURE(extra_val >= 0);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.hash, elem->value[0].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.h1_index, elem->value[1].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.h2_index, elem->value[2].as_int);

                    probe_count++;
                    break;

                case MARK_HASHPROBE_DIRECT_HIT:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)(lh - lh0), (int)feature_weight);
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    probe_direct_hit_count++;
                    break;

                case MARK_HASHPROBE_HIT:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "iii", (int)(lh - lh0), (int)feature_weight, (int)totalhits[k]);
                    ENSURE(value_cnt >= 2);
                    ENSURE(extra_val >= 0);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    if (value_cnt > 2)
                    {
                        UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.total_hit_count, elem->value[2].as_int);
                    }
                    probe_hit_count++;
                    break;

                case MARK_HASHPROBE_HIT_REFUTE:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT_REFUTE, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 0);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.chain_length, elem->value[0].as_int);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.weight, elem->value[1].as_int);
                    probe_refute_hit_count++;
                    break;

                case MARK_HASHPROBE_MISS:
                    // crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_MISS, h1, "i", (unsigned int)(lh - lh0));
                    ENSURE(value_cnt == 1);
                    ENSURE(extra_val >= 0);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.miss_chain_length, elem->value[0].as_int);
                    probe_miss_count++;
                    break;

                case MARK_MICROGROOM:
                    // crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "i", (int)incrs);
                    // crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "ii", (int)incrs, (int)zeroedfeatures);
                    ENSURE(value_cnt == 2);
                    ENSURE(extra_val >= 1);
                    ENSURE(extra_val <= 2);
                    UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.groom_chain_length, elem->value[0].as_int);
                    if (value_cnt > 1)
                    {
                        UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.probe.grooming_zeroed_features, elem->value[1].as_int);
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
                        UPDATE_MINMAX_PER_CLASSIFIER(prof_minmax.classifier.learn_count, elem->value[2].as_int);
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

#undef UPDATE_MINMAX
#undef UPDATE_MINMAX_PER_CLASSIFIER

    return 0;
}



int decipher_input_header(FILE *inf, int *read_size)
{
    int len = *read_size;
    CRM_ANALYSIS_PROFILE_ELEMENT store;
    uint8_t head_mark_big_endian[8] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
    uint8_t head_mark_little_endian[8] = { 0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12 };

    if (len < 1)
    {
        fprintf(stderr, "Illegal capture file '%s' specified: does not even have enough for the header mark!\n",
                cfg.input_file);
        return -1;
    }

    // read one element:
    if (1 != fread(&store, sizeof(store), 1, inf))
    {
        fprintf(stderr, "Illegal capture file '%s' specified: cannot read the header mark! error %d(%s)\n",
                cfg.input_file,
                errno,
                errno_descr(errno));
        return -1;
    }
    read_size[0]--;     // *read_size-- does something else ;-)

    // see if this element is a native; that would make things rather easy...
    if (store.marker == 0x123456789ABCDEF0ULL
        && store.value[0].as_int == 0x8081828384858687ULL
        && store.value[1].as_float == 1.0
        && store.value[2].as_int == 0x9091929394959697ULL
        && store.time_mark == 0xA0A1A2A3A4A5A6A7ULL)
    {
        // A-OK!
        return 0;
    }

    // When we get here, it may be Endianess, it may also be differences in struct element alignment or even some cute differences in 'double' size.
    //
    // We do not intend to 'migrate' this sort of file, so we're rather brutish about it now:
    // Only natives get served. That's mighty white of you, sir programmer. :-(
    //
    // When you need a more flexible treatment, there's always the option to request this at the crm mailing list.
    // The smell of green will help. ;-)
    //

    if (memmem(&store, sizeof(store), head_mark_big_endian, sizeof(head_mark_big_endian)))
    {
        // big_endian file. And we ourselves are...?
        fprintf(stderr,
                "Sorry, the capture file '%s' seems to have a non-native Big Endian format; "
                "it has very probably been produced on another machine or with another CRM114 build.\n",
                cfg.input_file);
        return 'B';
    }
    else if (memmem(&store, sizeof(store), head_mark_little_endian, sizeof(head_mark_little_endian)))
    {
        // little_endian file. And we ourselves are...?
        fprintf(stderr,
                "Sorry, the capture file '%s' seems to have a non-native Little Endian format; "
                "it has very probably been produced on another machine or with another CRM114 build.\n",
                cfg.input_file);
        return 'L';
    }
    else
    {
        fprintf(stderr, "Sorry, the capture file '%s' seems to be corrupted as we cannot detect a valid header.\n",
                cfg.input_file);
    }
    return -1;
}



