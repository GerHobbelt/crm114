#ifndef __CRM114_BMP_ASSISTED_ANALYSIS_TOOL_H__
#define __CRM114_BMP_ASSISTED_ANALYSIS_TOOL_H__

//  include some standard files
#include "crm114_sysincludes.h"

#undef CRM_WITHOUT_BMP_ASSISTED_ANALYSIS

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"





// to ease counting and postprocessing using scaling, a floating point type is advised for the basic counter.
typedef double analysis_counter_t;





typedef struct
{
    int x_width;
    int y_height;

    // mode flags:
    //
    // Note that Y is always 'data'.
    unsigned int x_is_time    : 1;       // y is data range; x is time range - if not set, data is spread over the square of X by Y
    unsigned int take_delta   : 1;       // use the sample delta instead of the absolute input value
    unsigned int plot_bidir   : 1;       // plot zero (= 'offset_for_data') at middle Y position
    unsigned int data_is_log2 : 1;       // plot the data in log2() instead of linear

    // trackers:

    // possible input: previous values for delta calc. A Hash should fit in a double for our granularity of value plot
    double previous_value;

    // range:
    double minimum_value;
    double maximum_value;

    // NOTE that 'offsets' are applied AFTER minimum/maximum range limiting!
    double offset_for_data;     // offset applied to absolute value before plotting it in bidir: 'offset' will end at middle Y height in the image.
    double log2_shift;          // add this to the log2() value as negative log2() will be limited to zero. This lifts the log curve UP or DOWN.


    // time
    int current_time;      // starts at zero and counts up with each 'event'.
    int time_range;        // maximum time when saampling should stop as this equals x-max for the 'image'

    // additional 'intermediates' calculated by the analysis code to help speed up the plotting of the data.
    //
    // ANYTHING AFTER THIS LINE YOU DO NOT NEED TO SET BEFORE CALLING THE CREATE METHOD
    double x_max_edge;
    double value_scale;
    int    counterspace_size;   // number of counters allocated following this struct.
    int    x_divisor;
    double y_single_scale;
    double y_square_scale;
    double y_middle_offset;
    double x_square_scale;
    double x_middle_offset;

    double x_display_scale;
    double y_display_scale;

    double x_display_size;       // double type copies of x_width and y_height:
    double y_display_size;

    // reference to the 'image' containing the x_size * y_size counters.
    analysis_counter_t *counters;
} CRM_ANALYSIS_INFOBLOCK;





#include "crm_pshpack2.h"
typedef struct
{
#define BMP_FILE_ID                  0x4D42  // 'B' 'M' ASCII is Little Endian 16-bit int
    uint16_t type_id;
    uint32_t file_size;                         // file size in bytes
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t bitmap_data_offset;       // offset in bytes to pixel data
} BmpFileHeader;
#include "crm_poppack.h"

#include "crm_pshpack1.h"
typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} BmpPixel;
#include "crm_poppack.h"

#include "crm_pshpack2.h"
typedef struct
{
    uint32_t header_size; // size of this header (should be 40 bytes precisely!)
    int32_t  image_width;
    int32_t  image_height;
    uint16_t color_planes;        // must be 1
    uint16_t bits_per_pixel;      // 1, 4, 8, 16, 24, 32 -- we use 32 only!
    uint32_t compression_method;  // 0: none
    uint32_t size_of_pixel_data;
    int32_t  horizontal_resolution;      // pixels per meter
    int32_t  vertical_resolution;        // pixels per meter
    uint32_t number_of_colors;           // number of colors in palette; 0: true color
    uint32_t number_of_important_colors; // should be 0
} BmpInfoHeader;
#include "crm_poppack.h"



#include "crm_pshpack2.h"
typedef struct
{
    BmpFileHeader header;
    BmpInfoHeader info;
    BmpPixel      bitmap[4];
} BmpFileStructure;
#include "crm_poppack.h"



typedef struct
{
    double blue;
    double green;
    double red;
    double alpha;
} RGBpixel;


#if defined (WORDS_BIGENDIAN)

static inline uint16_t n16_to_le(int v)
{
    return (v << 8) | ((v >> 8) & 0xFF);
}

static inline uint32_t n32_to_le(int v)
{
    return (v << 24) | ((v >> 24) & 0xFF) | ((v >> 16) & 0xFF00) | ((v >> 8) & 0xFF0000);
}

#else

static inline uint16_t n16_to_le(int v)
{
    return (uint16_t)v;
}

static inline uint32_t n32_to_le(int v)
{
    return (uint32_t)v;
}

#endif




#define MAX_DIMENSIONS                  6  // x/y/color:RGB/time

typedef struct
{
    int verbosity;
    int skip_errors;

    char *input_file;
    char *output_file_template;
} ANALYSIS_CONFIG;







typedef enum
{
    CPDIM_RUNS = 0,         // number of runs stored in CAPture file, based on MARK_INIT - MARK_SWITCH_CONTEXT, check: MARK_TERMINATION
    CPDIM_OPERATIONS,       // which crm opcodes have been executed, based on MARK_OPERATION; timing excludes MARK_DEBUG_INTERACTION blocks
    CPDIM_CLASSIFIERS,      /* which classifiers have been executed, based on MARK_CLASSIFIER,
                             *                             also registers if and when a 'learn' or a 'classify' was executed. */
    CPDIM_HASHPROBE,       // which hash probes were executed for each classifier, also register chain lengths, direct and delayed hits, misses, etc.
    CPDIM_HASH_VALUES,     // which inputs were fed to the hash function?

    CPDIM_TOTAL_GROUPS
} CRM_PROF_GROUP;

typedef enum
{
    UNIDENTIFIED_CLASSIFIER = 0,
    CLASSIFIER_MARKOVIAN,
    CLASSIFIER_OSB_BAYES,
    CLASSIFIER_CORRELATE,
    CLASSIFIER_OSB_WINNOW,
    CLASSIFIER_ENTROPY,
    CLASSIFIER_OSBF_BAYES,
    CLASSIFIER_HYPERSPACE,
    CLASSIFIER_SKS,
    CLASSIFIER_SVM,
    CLASSIFIER_FSCM,
    CLASSIFIER_NEURAL_NET,

    CLASSIFIER_COUNT
} CRM_PROF_CLASSIFIER;

typedef struct
{
    // all times in nsec.

    // each group at least comes with timing measurements:
    // per element (and we'll keep 'element' arbitrary right now: it really depends on the group_id
    int64_t element_time_consumption_min;
    int64_t element_time_consumption_max;

    // per run (init to termination)
    int64_t run_time_consumption_min;
    int64_t run_time_consumption_max;

    // and the grand total for the capture
    int64_t total_time_consumption_min;
    int64_t total_time_consumption_max;

    // each group of course comes with its own additional 'ranges':
    struct
    {
        // number of context switches per run
        int context_switches_min;
        int context_switches_max;

        // clocks used:
        int64_t clock_freq_min;
        int64_t clock_rez_min;
        int64_t clock_freq_max;
        int64_t clock_rez_max;

        // number of runs in capture
        int run_count;
    } run;

    struct
    {
        // opcode ID range: which opcodes were actually executed, eh?
        int opcode_min;
        int opcode_max;

        // and how much did each opcode eat?
        int64_t opcode_consumption_min[CRM_UNIMPLEMENTED + 1];
        int64_t opcode_consumption_max[CRM_UNIMPLEMENTED + 1];

        // and how often was each opcode executed? grand total only.
        int opcode_exec_count_min[CRM_UNIMPLEMENTED + 1];
        int opcode_exec_count_max[CRM_UNIMPLEMENTED + 1];

        // line/statement numbers for the tokenized script(s) run:
        int statement_no_min;
        int statement_no_max;
    } operation;

    struct
    {
        // exec counts
        int exec_count_min[CLASSIFIER_COUNT];
        int exec_count_max[CLASSIFIER_COUNT];

        // learn counts
        int exec_learn_count_min[CLASSIFIER_COUNT];
        int exec_learn_count_max[CLASSIFIER_COUNT];

        // classify counts
        int exec_classify_count_min[CLASSIFIER_COUNT];
        int exec_classify_count_max[CLASSIFIER_COUNT];

        // database sizes used:
        int db_size_min[CLASSIFIER_COUNT];
        int db_size_max[CLASSIFIER_COUNT];

        // database learn counts:
        int learn_count_min[CLASSIFIER_COUNT];
        int learn_count_max[CLASSIFIER_COUNT];

        // database feature counts:
        int feature_count_min[CLASSIFIER_COUNT];
        int feature_count_max[CLASSIFIER_COUNT];

        uint64_t flags_mask_min[CLASSIFIER_COUNT];
        uint64_t flags_mask_max[CLASSIFIER_COUNT];
    } classifier;

    struct
    {
        // hashvalues used for a probe:
        crmhash_t hash_min[CLASSIFIER_COUNT];
        crmhash_t hash_max[CLASSIFIER_COUNT];

        // which h1 indexes resulted from this?
        crmhash_t h1_index_min[CLASSIFIER_COUNT];
        crmhash_t h1_index_max[CLASSIFIER_COUNT];
        // and h2?
        crmhash_t h2_index_min[CLASSIFIER_COUNT];
        crmhash_t h2_index_max[CLASSIFIER_COUNT];

        // and how large were the documents fed to the classifiers?
        int probe_count_min[CLASSIFIER_COUNT];
        int probe_count_max[CLASSIFIER_COUNT];

        // and how large were the documents fed to the classifiers?
        int learn_probe_count_min[CLASSIFIER_COUNT];
        int learn_probe_count_max[CLASSIFIER_COUNT];

        // how often did we perform a microgroom?
        int groom_count_min[CLASSIFIER_COUNT];
        int groom_count_max[CLASSIFIER_COUNT];

        // and the number of removed (groomed) features
        int grooming_zeroed_features_min[CLASSIFIER_COUNT];
        int grooming_zeroed_features_max[CLASSIFIER_COUNT];

        // how often did we have a hit?
        int learn_probe_hit_count_min[CLASSIFIER_COUNT];
        int learn_probe_hit_count_max[CLASSIFIER_COUNT];

        // how often did we have a hit?
        int probe_hit_count_min[CLASSIFIER_COUNT];
        int probe_hit_count_max[CLASSIFIER_COUNT];

        // how often did we have a direct hit?
        int direct_hit_count_min[CLASSIFIER_COUNT];
        int direct_hit_count_max[CLASSIFIER_COUNT];

        // and the alternative for a hit: after another (N) probes?
        int secondary_hit_count_min[CLASSIFIER_COUNT];
        int secondary_hit_count_max[CLASSIFIER_COUNT];

        // and the misses?
        int miss_count_min[CLASSIFIER_COUNT];
        int miss_count_max[CLASSIFIER_COUNT];

        // and the total hit count
        int total_hit_count_min[CLASSIFIER_COUNT];
        int total_hit_count_max[CLASSIFIER_COUNT];

        // any refuted items out there?
        int learn_probe_refute_hit_count_min[CLASSIFIER_COUNT];
        int learn_probe_refute_hit_count_max[CLASSIFIER_COUNT];

        // number of refutes?
        int refute_count_min[CLASSIFIER_COUNT];
        int refute_count_max[CLASSIFIER_COUNT];

        // what weights were assigned the individual hashprobe hits?
        int weight_min[CLASSIFIER_COUNT];
        int weight_max[CLASSIFIER_COUNT];

        // and our chain lengths - at least the lengths we've seen?
        int chain_length_min[CLASSIFIER_COUNT];
        int chain_length_max[CLASSIFIER_COUNT];

        // and our chain lengths - at least the lengths we've seen?
        int groom_chain_length_min[CLASSIFIER_COUNT];
        int groom_chain_length_max[CLASSIFIER_COUNT];

        // split up into: the ones for learn episodes ...
        int learn_chain_length_min[CLASSIFIER_COUNT];
        int learn_chain_length_max[CLASSIFIER_COUNT];

        // ... secondary hits, ...
        int secondary_hit_chain_length_min[CLASSIFIER_COUNT];
        int secondary_hit_chain_length_max[CLASSIFIER_COUNT];

        // ... and misses.
        int miss_chain_length_min[CLASSIFIER_COUNT];
        int miss_chain_length_max[CLASSIFIER_COUNT];
    } probe;

    struct
    {
        // the lowest and hishest input string [starts] fed to the hash function:
        char input_str_min[2 * 8 + 1];
        int input_str_min_len;
        char input_str_max[2 * 8 + 1];
        int input_str_max_len;

        // input lengths:
        int hash_input_len_min;
        int hash_input_len_max;

        // and the resulting hashvalues
        crmhash_t hash_min[CLASSIFIER_COUNT];
        crmhash_t hash_max[CLASSIFIER_COUNT];
    } hash;

    struct
    {
        // the lowest and hishest input string [starts] fed to the hash function:
        char input_str_min[2 * 8 + 1];
        int input_str_min_len;
        char input_str_max[2 * 8 + 1];
        int input_str_max_len;

        // input lengths:
        int hash_input_len_min;
        int hash_input_len_max;

        // and the resulting hashvalues
        crmhash64_t hash_min[CLASSIFIER_COUNT];
        crmhash64_t hash_max[CLASSIFIER_COUNT];
    } hash64;
} CRM_PROF_GROUP_MINMAX;




typedef struct
{
    int     opcode_counts[CRM_UNIMPLEMENTED + 2];
    int64_t opcode_times[CRM_UNIMPLEMENTED + 2];

#define HASH_DISTRIBUTION_GRANULARITY (2048 * 2048)
    unsigned int hash_distro_counts[HASH_DISTRIBUTION_GRANULARITY];

#define HASH_PER_CHAR_GRANULARITY 32768
    unsigned int hash_per_char_counts[1 << 8][16][HASH_PER_CHAR_GRANULARITY];

    int *chain_length_counts;
    int  chain_length_count_size;
} CRM_ANALYSIS_REPORT_DATA;



typedef struct
{
    char    *stmt_name;
    int      stmt_code;
    unsigned is_executable : 1;
} STMT_DEF_TYPE;







extern CRM_ANALYSIS_PROFILE_CONFIG analysis_cfg;

extern ANALYSIS_CONFIG cfg;

extern CRM_PROF_GROUP_MINMAX prof_minmax;

extern CRM_ANALYSIS_REPORT_DATA report_data;






static inline void cvt_int64_2_chars(char *dst, int64_t val)
{
    union
    {
        char    c[8];
        int64_t ll;
    } v = { 0 };
    v.ll = val;
    memcpy(dst, &v.c[0], 8);
}







CRM_ANALYSIS_INFOBLOCK *crm_create_analysis_image(CRM_ANALYSIS_INFOBLOCK *info);

int calculate_BMP_image_size(int x_width, int y_height);
int create_BMP_file_header(BmpFileStructure *dst, int bufsize, int x_width, int y_height);
int plot_BMP_pixel(BmpFileStructure *dst, CRM_ANALYSIS_INFOBLOCK *info, double x, double y, RGBpixel *pixel);


int scan_to_determine_input_ranges(FILE *inf, int read_size, CRM_ANALYSIS_PROFILE_ELEMENT *store, int store_size);
int decipher_input_header(FILE *inf, int *read_size);
int collect_and_display_requested_data(FILE *inf, int read_size, CRM_ANALYSIS_PROFILE_ELEMENT *store, int store_size);

int produce_opcode_coverage_report(CRM_ANALYSIS_REPORT_DATA *report_data, FILE *of, int compressed_report);
int produce_hash_distribution_report(CRM_ANALYSIS_REPORT_DATA *report_data, FILE *of);
int produce_reports(CRM_ANALYSIS_REPORT_DATA *report_data);

int64_t counter2nsecs(int64_t active_counter_frequency, int64_t run_time_consumption);
void fprintf_nsecs2dhmsss(FILE *of, int day_width, int subsecond_width, int64_t nsecs);
const STMT_DEF_TYPE *get_stmt_def(int opcode);




// collect / scan commons:

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



int check_validity_of_expression(int lineno, const char *srcfile, const char *funcname,
        int store_elem_id, int elem_index_in_file,
        int comparison, const char *comparison_msg, const char *comparison_description,
        int show_message);






/*
 * ====================================================================
 *
 * Abstract Base Classes: basic hierarchy
 *
 * ====================================================================
 */

/*
 * Keeps track of profile data (input feed) and offers access to it
 * for perusal by the Collectors and DataFilters.
 */
class AugmentedDataFeedBase
{
public:
    virtual CRM_ANALYSIS_PROFILE_ELEMENT *NextElem() = 0;
    virtual CRM_ANALYSIS_PROFILE_ELEMENT *RelativeElem(int offset) = 0;
    virtual void ResetToStart() = 0;

private:
    AugmentedDataFeedBase() { };
    virtual ~AugmentedDataFeedBase() { };
};


class StatsCollectorBase;

/*
 * When a Collector collects something (or a Filter detects a certain
 * transition) a ReportEvent occurs: this is fed to a ReportFilter
 * chain to see if any reports are interested in this event. If they
 * are, execute them.
 *
 * Note that the Event does not pass along who triggered it, either
 * a collector or a DataFilter. This is not important anyhow, as
 * a report only needs to be 'set off': it will know which bits of data
 * to use, thanks to the data Collectors registered with each report.
 */
class ReportEventBase
{
public:
    virtual int Event() = 0;

private:
    ReportEventBase() { };
    virtual ~ReportEventBase() { };
};



/*
 * A report can be produced at any time.
 *
 * It requires a specific set of (processed) data sources: it will
 * retrieve any desired data from the registered Collectors.
 */
class ReportBase
{
public:
    virtual void Report() = 0;

    virtual void RegisterRequiredCollection(StatsCollectorBase&collection, int purpose) = 0;

private:
    ReportBase() { };
    virtual ~ReportBase() { };
};


/*
 * A Collector or DataFilter can trigger a ReportEvent, which is fed to this
 * chain of ReportEventFilters (which can be chained to create AND and OR constructs).
 *
 * When a ReportEventFilter chain 'selects'/passes a ReportEvent, it will
 * cause the registered Reports to be produced this time.
 */
class ReportEventFilterBase
{
public:
    virtual int Report(ReportEventBase&report_event) = 0;

    virtual void RegisterReport(ReportBase&report) = 0;
    virtual void ChainFilter(ReportEventFilterBase&subsequent_filter) = 0;

private:
    ReportEventFilterBase() { };
    virtual ~ReportEventFilterBase() { };
};


/*
 * Defines a collector, which will process and store this bit of profile data from the feed.
 *
 * Note that Collectors MAY produce ReportEvents, which should be sent to
 * the registered ReportEventFilters to see if any reports have to be
 * produced this time. Reports can thus be 'triggered' by filters AND
 * collectors.
 *
 * Reports use the Collectors registered with them to retrieve the data required
 * to produce that particular report.
 */
class StatsCollectorBase
{
public:
    virtual void Collect(AugmentedDataFeedBase&feed) = 0;

    virtual void RegisterReportEventFilter(ReportEventFilterBase&report_filter) = 0;

private:
    StatsCollectorBase() { };
    virtual ~StatsCollectorBase() { };
};

/*
 * Defines a filter, which is fed profile data from the feed.
 *
 * 'Collectors' registered with this filter will receive any data which passes.
 *
 * Also, filters may be chained to create AND and OR filter flow
 * structures. (OR is created using 'either-or' filters in the chain)
 *
 * Note that Filters MAY produce ReportEvents, which should be sent to
 * the registered ReportEventFilters to see if any reports have to be
 * produced this time. Reports can thus be 'triggered' by filters AND
 * collectors.
 *
 * The hierarchy of classes culminates in this the DataFilters:
 * given a feed, (through registered collectors, etc.) these pick the
 * interesting data elements, collect them, process them and
 * immediately produce reports based on this (processed) data.
 *
 * This input-is-driving-output hierarchy has been designed to maximize
 * collect and report flexibility.
 * This way, for instance, reports can be generated multiple times while
 * processing a single feed: think report=image, where a new image is
 * produced following every LEARN operation, showing the new hash table
 * occupancy and chain lengths statistics. This image series can be
 * postprocessed into a video, showing how the data is collected and spread
 * by CRM114 classifiers in semi-'real time'.
 *
 * Why a data PUSH hierarchy instead of a output-to-input PULL
 * hierarchy? Because here we have ONE input and a (dynamic) large
 * number of parallel outputs: that means a PUSH system has its
 * hierarchy laid out in the same 'direction' (hierarchy = triangle;
 * input is the top/root, a run-time determined number of leaves in the
 * tree represent the various types of requested reports).
 *
 * When processes produce a single output, based on multiple inputs, a
 * PULL hierarchy is better (easier to implement and maintain), but that
 * is not the case here.
 */
class DataFilterBase
{
public:
    virtual int Filter(AugmentedDataFeedBase&feed) = 0;

    virtual void RegisterCollector(StatsCollectorBase&collector) = 0;
    virtual void ChainFilter(DataFilterBase&subsequent_filter) = 0;

    virtual void RegisterReportEventFilter(ReportEventFilterBase&report_filter) = 0;

private:
    DataFilterBase() { };
    virtual ~DataFilterBase() { };
};




/*
 * ====================================================================
 *
 * Derived Classes: particular implementations for multiple purposes
 *
 * ====================================================================
 */



#endif

