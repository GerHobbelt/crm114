
#include "crm_bmp_prof.h"







int produce_opcode_coverage_report(CRM_ANALYSIS_REPORT_DATA *report_data, FILE *of, int compressed_report)
{
    int i;
    double total_time = 0.0;
	double total_time_per_piece = 0.0;
	int total_count = 0;

    fprintf(of, "CRM script opcode test coverage%s:\n"
                "===============================%s=\n"
                "\n"
                "     opcode        # hits  time spent (seconds)       %%   a piece (seconds)       %%\n"
                "-----------------------------------------------------------------------------------\n",
            (compressed_report ? " (only items which have been tested)" : ""),
            (compressed_report ? "====================================" : ""));
    for (i = 0; i < WIDTHOF(report_data->opcode_counts); i++)
    {
        const STMT_DEF_TYPE *stmt_def = get_stmt_def(i);

        if ((report_data->opcode_counts[i] > 0 && i != CRM_UNIMPLEMENTED+1) 
			|| (!compressed_report && stmt_def->stmt_code != CRM_BOGUS))
		{
        total_time += report_data->opcode_times[i];
		if (report_data->opcode_counts[i] > 0)
		{
		total_time_per_piece += ((double)report_data->opcode_times[i]) / report_data->opcode_counts[i];
		}
		total_count += report_data->opcode_counts[i];
		}
    }
    total_time /= 100.0;
    total_time_per_piece /= 100.0;
    for (i = 0; i < WIDTHOF(report_data->opcode_counts); i++)
    {
        const STMT_DEF_TYPE *stmt_def = get_stmt_def(i);

        // in uncompressed report, only show the bogus/unknown lines when there's actually some time spent to report there.
        if ((report_data->opcode_counts[i] > 0 && i != CRM_UNIMPLEMENTED+1) 
			|| (!compressed_report && stmt_def->stmt_code != CRM_BOGUS))
		{
    fprintf(of, "%11.11s %13d ", stmt_def->stmt_name, report_data->opcode_counts[i]);
            if (report_data->opcode_counts[i] > 0)
            {
                fprintf_nsecs2dhmsss(of, 5, 6, report_data->opcode_times[i]);
                fprintf(of, " %7.3f ", report_data->opcode_times[i] / total_time);
                fprintf_nsecs2dhmsss(of, 3, 6, report_data->opcode_times[i] / report_data->opcode_counts[i]);
                fprintf(of, " %7.3f\n", ((double)report_data->opcode_times[i]) / (report_data->opcode_counts[i] * total_time_per_piece));
            }
            else
            {
                fprintf(of, "%21s %7s %19s %7s\n", "-", "-", "-", "-");
            }
        }
    }
    fprintf(of, "-----------------------------------------------------------------------------------\n");
	fprintf(of, "%11s %13d ", "Totals:", total_count);
                fprintf_nsecs2dhmsss(of, 5, 6, (int64_t)(total_time * 100.0));
                fprintf(of, " %7.3f %19s %7s\n\n\n", 100.0, "(N.A.)", "(N.A.)");

    return 0;
}



int produce_hash_distribution_report(CRM_ANALYSIS_REPORT_DATA *report_data, FILE *of)
{
    int i;
    int avg_max_count = 0;
    int max_max_count = 0;
    int width = 64;
    int height = 64;
    int *avg_img;
    int *max_img;
    int idx_divisor;
    double avg_quantize_divisor;
    double max_quantize_divisor;
    double avg_log2_quantize_divisor;
    double max_log2_quantize_divisor;

    avg_img = (int *)calloc(width * height, sizeof(avg_img[0]));
    max_img = (int *)calloc(width * height, sizeof(max_img[0]));

    fprintf(of, "CRM hash distribution (avg/max):\n"
                "================================\n"
                "\n");

    idx_divisor = WIDTHOF(report_data->hash_distro_counts) /* HASH_DISTRIBUTION_GRANULARITY */ / (width * height);
    for (i = 0; i < WIDTHOF(report_data->hash_distro_counts); i++)
    {
        int idx;

        idx = i / idx_divisor;

        CRM_ASSERT(idx < width * height);
        avg_img[idx] += report_data->hash_distro_counts[i];
        if (max_img[idx] < report_data->hash_distro_counts[i])
        {
            max_img[idx] = report_data->hash_distro_counts[i];
        }
    }
    for (i = width * height; --i >= 0;)
    {
        if (avg_max_count < avg_img[i])
        {
            avg_max_count = avg_img[i];
        }
        if (max_max_count < max_img[i])
        {
            max_max_count = max_img[i];
        }
    }
    avg_quantize_divisor = (avg_max_count + 1) / 12.0;              // sizeof(".-0123456789#") - 2
    avg_log2_quantize_divisor = log2(avg_max_count + 1.0) / 12.0;     // sizeof(".-0123456789#") - 2

    fprintf(of, "AVG: max: %d, quantize divisor: %f\n", avg_max_count, (double)avg_quantize_divisor);

    if (avg_quantize_divisor)
    {
        int x;
        int y;

        for (y = height; --y >= 0;)
        {
            for (x = 0; x < width; x++)
            {
                int idx = y + height * x;
                int qv;

                if (avg_img[idx])
                {
                    qv = 1 + (int)(avg_img[idx] / avg_quantize_divisor);
                }
                else
                {
                    qv = 0;
                }
                if (qv > 12)
                {
                    qv = 12;
                }

                fputc(".-0123456789#"[qv], of);
            }
            fputc(' ', of);
            for (x = 0; x < width; x++)
            {
                int idx = y + height * x;
                int qv;

                if (avg_img[idx])
                {
                    qv = 1 + (int)(log2((double)avg_img[idx]) / avg_log2_quantize_divisor);
                }
                else
                {
                    qv = 0;
                }
                if (qv > 12)
                {
                    qv = 12;
                }

                fputc(".-0123456789#"[qv], of);
            }
            fputc('\n', of);
        }
    }

    max_quantize_divisor = (max_max_count + 1) / 12.0;              // sizeof(".-0123456789#") - 2
    max_log2_quantize_divisor = log2(max_max_count + 1.0) / 12.0;     // sizeof(".-0123456789#") - 2

    fprintf(of, "\n"
                "PEAK: max: %d, quantize divisor: %f\n", max_max_count, (double)max_quantize_divisor);

    if (max_quantize_divisor)
    {
        int x;
        int y;

        for (y = height; --y >= 0;)
        {
            for (x = 0; x < width; x++)
            {
                int idx = y + height * x;
                int qv;

                if (max_img[idx])
                {
                    qv = 1 + (int)(max_img[idx] / max_quantize_divisor);
                }
                else
                {
                    qv = 0;
                }
                if (qv > 12)
                {
                    qv = 12;
                }

                fputc(".-0123456789#"[qv], of);
            }
            fputc(' ', of);
            for (x = 0; x < width; x++)
            {
                int idx = y + height * x;
                int qv;

                if (max_img[idx])
                {
                    qv = 1 + (int)(log2((double)max_img[idx]) / max_log2_quantize_divisor);
                }
                else
                {
                    qv = 0;
                }
                if (qv > 12)
                {
                    qv = 12;
                }

                fputc(".-0123456789#"[qv], of);
            }
            fputc('\n', of);
        }
    }

    free(avg_img);
    free(max_img);

    fprintf(of, "\n"
                "----------------------\n"
                "\n"
                "\n");

    return 0;
}



int produce_reports(CRM_ANALYSIS_REPORT_DATA *report_data)
{
    FILE *of = stdout;
    int ret;

    ret = produce_opcode_coverage_report(report_data, of, 1);
    if (ret)
    {
        return ret;
    }

    ret = produce_opcode_coverage_report(report_data, of, 0);      // uncompressed report: show all opcodes!
    if (ret)
    {
        return ret;
    }

    ret = produce_hash_distribution_report(report_data, of);
    if (ret)
    {
        return ret;
    }

    return ret;
}



CRM_ANALYSIS_REPORT_DATA report_data = { { 0 } };

