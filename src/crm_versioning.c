//  crm_versioning.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007 William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//
//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"


#if !defined (CRM_WITHOUT_VERSIONING_HEADER)

//////////////////////////////////////////////////////////////////
//
//                            Versioning
//                            ==========
//
//    CRM114 files can be fitted with a header block, which should ideally
//    be one (or more) mmap()ped pages so as not to influence mmap()
//    performance in any way, while this header block can be used to store
//    any version and platform related info useful to CRM114 users.
//
//    These information particles can be used to:
//
//    - detect / validate the proper use of various CRM file contents
//      (different classifiers)
//
//    - detect if CRM114 is using a 'current version' of the data [format];
//      where applicable CRM114 can thus provide an upgrade path OR report
//      the impossibility of such (e.g. when the classifier has been removed
//      or changed in incompatible ways)
//
//    - help tools to properly recognize and report on CRM114 data files
//
//    - help tools to convert CRM114 files from other versions and/or
//      platforms (e.g. for automated upgrades and cross-platform
//      synchronization)
//
//
//    Basic Design
//    ------------
//
//    The Header Block is sized at precisely 8 KByte, which should be
//    sufficient to store any machine- and human-readable info which should
//    accompany the actual data stored in the CSS file proper.
//
//    The Header Block size is set to 8KB because it is assumed that most
//    (if not all) systems out there have a mmap() page size for which 8K
//    will be an integer multiple >= 1. (See also the getpagesize() code in
//    CRM114.)
//
//    To facilitate both machines and humans perusing the file, the 8K
//    header is split into two sections, each of size 4K: the Header block
//    always starts with a 4K human readable ASCII text area, followed by 4K
//    of machine-readable fields and byte sequences. The initial 'human
//    readable text' section is also used to store some file type
//    recognition data, which can be used by file type/MIME type detection
//    tools (a bit similar to ZIP files which always start with the
//    character sequence 'PK' for instance; for file to MIME type
//    conversions using initial byte sequences see the UNIX man page
//    mime.types(5) for example).
//
//    Furthermore, the initial 4K section is constructed in such a way that
//    the human readable text stored herein will be displayed on screen
//    when:
//
//    - the CRM file is e'x'ecutable on UNIX and 'run' as is: it will behave
//      like a very basic shell script.
//
//    - the CRM file is fed to UNIX tools like 'head'
//
//    - the CRM file is 'type'd on MSDOS or Windows compatible systems.
//
//    We hope that the mechanism/structure used also works out favorably on
//    non-UNIX MACintosh boxes and other platforms.
//
//    The second 4K block is intended for machine-only use:
//
//    - version, classifier and other info is stored here for tools to
//      detect and decide on upgrade/transformation paths available.
//
//    - 'marker' sequences are stored by the originating platform to allow
//      tools able to copy/transform the data in a cross-platform/compiler
//      portable manner to detect relevant info such as integer/float size,
//      format, Endianess, etc.
//
//    - additional statistics and or creation info may be stored in this
//      section to aid reporting and analysis tools
//
//
//    Header block format / layout
//    ----------------------------
//
//    The human readable text area consists of ASCII text, separated into
//    multiple lines like any text file using the UNIX LF character sequence
//    to indicate the end of a line (all OS's, including MSDOS, Windows and
//    MAC OS/X, can often handle this text format automatically, while many
//    [bare bones] UNIX boxes will struggle to display the MSDOS (CR LF) or
//    MAC (CR-only) text formats). Unfortunately, some tools on WINDOWS
//    (Notepad, ...) do not handle the LF-only line termination well, but
//    this is decided to be the lesser nuisance.
//
//    The text is terminated by a mix of EOL/EOF signaling characters to
//    fool simple text file tools:
//
//    Sentinel (5)            A sequence of these bytes: EOL (ASCII(26)),
//                            ETX (ASCII(4)), EOT (ASCII(5)),
//                            FF (ASCII(12)), FS (ASCII(28))
//
//    The text is then padded with ASCII NUL (ASCII(0)) bytes to completely
//    fill the initial 4K block.
//
//    NOTE: The first two lines of text (i.e. all text up to the second LF)
//    is used to aid automated file type recognition tools and will include
//    some basic file format info in ':' colon separated fixed width text
//    fields. These initial bytes can be used by MIME type detection tools,
//    etc.
//
//
//    Initial bytes / file recognition marker
//    ---------------------------------------
//
//    The initial bytes of the header block are always these 11 characters
//    (for simplified automated file type recognition) where '\n' represents
//    a single LF (ASCII(10dec)) character:
//
//    "#!\n#CRM114:"
//
//    These are immediately followed by a 10 character, space padded, text,
//    identifying the classifier which creates/uses this file, terminated by
//    a colon ':' as the eleventh character:
//
//    "SKS       :"
//    "SVM       :"
//    "WINNOW    :"
//    "CLUMP     :"
//    "FSCM      :"
//    "SCM       :"
//    "MARKOV    :"
//    "NEURAL    :"
//    "HYPERSPACE:"
//    "OSBF      :"
//    "OSB-BAYES :"
//    "BITENTROPY:"
//    "CORRELATE :"
//
//    Next, the CRM version used to create this file is included as a 35
//    character fixed width, space padded, field, e.g.:
//
//    "20060611-SomewhatTamedBeast       :"
//
//    Platform specifics follow next: each element, its width and sample
//    field content are listed in the table below. All fields are space
//    padded to the left, just like the fields described so far:
//
//    Field           Width             Example(s)
//                    (excl. colon)
//
//    Format Version  4                 "0001:" --> the exception to the
//                                                  space padding rule: a
//                                                  zero-padded number!
//
//    Host Type       20                "windows-MS:" (host type as
//                                         determined by the ./configure
//                                         script. If unknown, use "???" or
//                                         "unknown" instead)
//
//    Future versions of CRM114 may add additional fields, so any software
//    scanning these files should be line oriented, despite the fixed width
//    of the individual fields.
//
//    Additional fields will be located following the last field described above.
//
//    The third line is there for the sole purpose of cajoling UNIX boxes
//    into displaying the remaining text when the file is run as executable
//    ('x' access rights):
//
//      cat <<-EOF\n
//
//    where once again '\n' represents a single LF character.
//
//    The next lines in the human readable text area are for general
//    information purposes. These may include statistics information or
//    other types of memo data, e.g. owner & purpose info for your personal
//    / company use.
//
//    WARNING: *none* of these lines may start with the character sequence:
//
//               'EOF'
//
//             We suggest using two leading spacing for each line of text to
//             prevent this issue from ever occurring.
//
//    To allow storage of multiple text sections, which can be decoded to
//    some degree at least by automated systems, each section will be start
//    with a header line, followed by a line which only contains as many '='
//    characters as the length of the previous 'header' line. Next, an empty
//    line will precede the content, which will be terminated By a double
//    empty line.
//
//    NOTE: content MAY contain double empty lines: the double-LF sequence
//          is only considered to be a terminator when followed immediately
//          by an EOF sequence OR a header line (plus '=' line) as described
//          above.
//
//    An example:
//
//        CRM114 Header Block: Trial Info
//        ===============================
//
//        This is a valid blurb of human-readable content, stored in a
//        CRM114 header block.
//
//
//        As you can see, it can contain series of empty lines, like the
//        double occurrence just above, because this paragraph does NOT look
//        like a 'header line', such as the one that started all this.
//
//        Enjoy CRM114!
//
//
//        A Header without Content
//        ========================
//      EOF
//
//
//    End of Text
//    -----------
//
//    The human readable header block is terminated by the 10 character
//    sequence
//
//      EOF\n
//      exit;\n
//
//    as shown also in the example above, followed by the 5 character
//    Sentinel (EOL ETX EOT FF FS) as mentioned earlier in this text.
//
//
//
//
//    The Machine Readable Section
//    ----------------------------
//
//    The initial fields just mimic the initial line of the human-readable
//    section. However, additional fields are available to report and detect
//    platform specifics of the CRM114 binary used to create this file.
//
//    NOTES:
//
//      We may decide to store file statistics, etc. in this section for
//      ease of analysis/reporting/validation of the file (statistics listed
//      in this block should of course match the statistics that can be
//      derived from the actual file content itself! If not, we've got a
//      corrupted file on our hands!)
//
//      (Design Note: the initial concept reserved the last 128 bytes of
//      this 4K block for padding and a CRC32 checksum (padding used to make
//      sure the checksum would never be all zeroes, which was a magic value
//      to signal 'no checksum available'). However, this checksum business
//      has been discarded altogether, because a checksum for only this
//      header block isn't all that worthy anyway: the actual CRM classifier
//      data is important but may still be corrupted despite that checksum,
//      and calculating the checksum is also considered to be way too much
//      overhead for too little gain, especially when this header block is
//      used to store information which may be updated during every (learn)
//      run of CRM114. Bye bye, checksum!
//
//      The last 128 bytes of this 4K block are reserved and must be filled
//      with all zero(0) bytes.
//
//
//    Fields in the binary section and platform specifics
//    ---------------------------------------------------
//
//    The 4K machine readable block does carry the (fixed width) fields
//    listed below, each of which is stored in 'network byte order' (Big
//    Endian). All Fields are aligned at a 16 byte boundary to ensure easy
//    access for any platform.
//
//    A special initial block ('Marker') is used to report and detect
//    platform specifics such as
//
//    - int32_t / int64_t / float / double alignment within C structures
//      (which depends on both the platform/CPU and the C compiler used to
//      create the CRM114 binary)
//
//    - Endianess (Big / Little / Mixed) for int32_t and int64_t
//
//    - Endianess and format for float and double
//
//    - sizes of the integer and floating point types will be reported using
//      additional fields, but do not reside in the 'Marker' block; the
//      'Marker' block stores 'magical values' to report Endianess and such
//      and is mostly useful for cross-[platform/compiler build] upgrade
//      tools for CRM114.
//
//    As we assume a worst case alignment of 16 bytes per integer, each of
//    the elements above will be calculated at 16 bytes cost a piece ==>
//
//    - alignment (at 2 fields per item to test this):        4 * 2 * 16
//    - intXX_t Endianess                                         2 * 16
//    - floating point Endian/format at
//        value1/value2/PosInf/NegInf/NaN/... (8):            2 * 8 * 16
//
//
//    -->                                        total:        = 26 * 16
//
//    The fields in the 4K machine readable section (all fields are padded
//    with NUL ASCII(0) bytes to fulfil the 16 byte alignment requirement):
//
//    Field (Width)           Description
//
//    Header (6)              The character sequence 'CRM114'
//
//
//    Marker (26*16)          As described above:
//
//                            Alignment is tested by using 'struct'ures
//                            which have one character HEX(FF), followed by
//                            the type to detect the alignment for. Magic
//                            integer values are HEX(5A) for each byte, so
//                            no Endianess issues should ensue here (yet).
//
//                            Endianess is detected by writing
//                            HEX(0FEDCBA987654321) derived sequences,
//                            limited in bytes by the size of the type for
//                            the generating platform/compiler.
//
//                            The fields in the 'Marker' section are listed
//                            now:
//
//      M:Alignment
//
//        int32_t (32)        HEX(FF) char and a int32_t with magic value
//                            mentioned above. Magic will be truncated to
//                            int32_t size by the compiler.
//
//        int64_t (32)        HEX(FF) char and a int64_t with magic value
//                            mentioned above. Magic will be truncated to
//                            int32_t size by the compiler.
//
//        float (32)          HEX(FF) char and a float with magic value
//                            15365221879119872 (which should produce the
//                            HEX bytes 5A5A5A5A for IEEE754 compliant
//                            systems). Detect alignment by looking for the
//                            first non-NUL byte as IEEE compliancy is NOT
//                            guaranteed on all systems)
//
//        double (32)         HEX(FF) char and a double with magic value
//                            10843961455707782 (which should produce the
//                            HEX bytes 4343434343434343 for IEEE754
//                            compliant systems).
//
//
//      M:Endianess
//
//        int32_t (16)        Writes int32_t-truncated HEX value
//                            HEX(0FEDCBA987654321) in this field. Endianess
//                            is determined by the order of the bytes that
//                            represent this value (Big Endian 32 bit e.g.
//                            87,65,43,21, while Little Endian 32-bit will
//                            produce 21,43,65,87).
//
//        int64_t (16)        Writes int64_t-truncated HEX value
//                            HEX(0FEDCBA987654321) in this field. Endianess
//                            is determined as above. Should produce 8 bytes
//                            0F,ED,... (when Big Endian)
//
//        float:value1 (16)   Writes float magic value -65016332288, which
//                            should generate the value HEX(D1723468); byte
//                            order determines float Endianess (which may
//                            differ from integer Endianess!)
//
//        float:value2 (16)   Writes float magic value 1305747233177600,
//                            which should generate the value HEX(5894723F);
//                            byte order determines float Endianess (which
//                            may differ from integer Endianess!) This extra
//                            value is used to help tools on other platforms
//                            detect floating point format and Endianess.
//
//        float:filler (96)   Six 16 byte blocks with NUL bytes only.
//                            Reserved for future use.
//
//        double:value1 (16)  Writes double magic value -2246777488905480,
//                            which should generate the value
//                            HEX(C31FEDBA98765420); byte order determines
//                            double Endianess (which may differ from
//                            integer Endianess!)
//
//        double:value2 (16)  Writes double magic value 282803858680781.90,
//                            which should generate the value
//                            HEX(42F01356789ABCDE); byte order determines
//                            double Endianess (which may differ from
//                            integer Endianess!) This extra value is used
//                            to help tools on other platforms detect
//                            doubleing point format and Endianess.
//
//        double:filler (96)  Six 16 byte blocks with NUL bytes only.
//                            Reserved for future use.
//
//
//
//    Header II (6)           The character sequence 'CRM114', padded by NUL
//                            bytes. (This field can be used to make sure
//                            the header was constructed properly; if the
//                            '16-byte alignment for everything' assumption
//                            was broken on the originating system, the
//                            distance between this and the previous Header
//                            marker can be used to deduce the _actual_
//                            alignment.)
//
//    Host Type (32)          Host type as determined by the ./configure
//                            script. If unknown, use "???" or "unknown"
//                            instead.
//
//    Version (48)            A copy of the equivalent field in the first
//                            line of the human readable section, though
//                            THIS time it is NOT space-padded, but NUL(0)
//                            padded, while the last character always MUST
//                            be a NUL(0) to make sure this byte sequence
//                            can be processed like a regular 'C' string. An
//                            example:
//
//                              "20060611-SomewhatTamedBeast"
//
//                            Note that this field allows for a Version
//                            string of up to 47 characters (plus NUL
//                            sentinel).
//
//
//    Platform specifics are next: each element, its width and sample field
//    content are listed in the table below. All fields are int32_t integer
//    native format, unless otherwise specified.
//
//    WARNING: this means that cross-platform tools should *first* decode
//             the Endianess and alignment sections above, before attempting
//             to decode this data. We are aware this complicates the
//             decoder logic, but simplifies the writer. And since the
//             decoder must be able to cope with the alignment and Endianess
//             differences anyway (if the tool is to process the data stored
//             in this file), the decoder complexity does not change for the
//             worse after all.
//
//    NOTE: Note that the '16 byte alignment' also applies for these fields!
//
//    Field           Width (bytes)     Example(s)
//
//    Format Version  4                 1 (initial header format version as
//                                         described here)
//
//    (Endianess      0                 must be 'auto-detected' using the
//                                      sections above
//
//    INT Word Size   4                 4 --> sizeof(int)
//
//    LONG Size       4                 8 --> sizeof(long int)
//
//    LONG LONG Size  4                 8 --> sizeof(long long int)
//                                      0 --> 'long long int' is unknown type on
//                                            this box
//
//    INT32 Size      4                 4 --> sizeof(int32_t)
//
//    INT64 Size      4                 8 --> sizeof(int64_t)
//
//    INT32 Alignment 0                 must be 'auto-detected' using the
//                                      sections above
//
//    INT64 Alignment 0                 must be 'auto-detected' using the
//                                      sections above
//
//    FLOAT Size      4                 4 --> sizeof(float)
//
//    DOUBLE Size     4                 8 --> sizeof(double)
//
//    (FLOAT/double IEEE Endianess
//                    0                 must be 'auto-detected' using the
//                                      sections above
//
//
//    Next is the classifier (and its options) which was used to create this
//    file:
//
//    Classifier      8                 The binary equivalent of the
//                                      corresponding field in the first
//                                      line of the human readable section,
//                                      though THIS time it is encoded as a
//                                      64-bit bitfield. It will contain one
//                                      or more of the flags defined in
//                                      crm114_structs.h, e.g.
//
//                                        CRM_OSB_WINNOW
//
//    Classifier Arguments
//                    512               This depends on the classifier and
//                                      platform: arbitrary content.
//
//
//
// Endianess and IEEE754 floating point references for further reading:
//
// http://babbage.cs.qc.edu/courses/cs341/IEEE-754references.html
// http://www.math.utah.edu/~beebe/software/ieee/
// http://aspn.activestate.com/ASPN/docs/ActivePython/2.5/peps/pep-0754.html
// http://en.wikipedia.org/wiki/Endianness
// http://docs.sun.com/source/806-3567/data.represent.html
//



/*
 * checksum is not useful and only causes large overhead when we want to store
 * real-time statistics in the header (8K CRC block for every access! :-( )
 */





typedef union
{
    uint32_t hex;
    float    f;
} crm_i2f_t;

typedef union
{
    uint64_t hex;
    double   d;
} crm_i2d_t;



#endif



/*
 * Return !0 if 'f' is a CRM file with portability header; return 0 otherwise.
 *
 * NOTE: file 'f' must be fseek()able to make this work.
 */
int is_crm_headered_file(FILE *f)
{
    int old_pos;
    char sequence[CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH + 1];
    int ret = 0;

    if (!f)
        return 0;

    old_pos = ftell(f);
    if (!fseek(f, 0, SEEK_SET))
    {
        memset(sequence, 0, sizeof(sequence));
        ret = fread(sequence, sizeof(sequence[0]), WIDTHOF(sequence) - 1, f);
        sequence[WIDTHOF(sequence) - 1] = 0;

		ret = is_crm_headered_mmapped_file(sequence, ret);
    }
    (void)fseek(f, old_pos, SEEK_SET);

    return ret;
}



/*
 * Return !0 if 'buf' points to a mmap()ed CRM file with portability header; return 0 otherwise.
 */
int is_crm_headered_mmapped_file(void *buf, size_t length)
{
	int ret = 0;

    if (length > CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH)
	{
		ret = (0 == memcmp(CRM_PORTABILITY_HEADER_SEQUENCE, buf, CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH));
    }

    return ret;
}



static const char *hrmsg =
    "                        Versioning\n"
    "                        ==========\n"
    "\n"
    "CRM114 files can be fitted with a header block, which should ideally\n"
    "be one (or more) mmap()ped pages so as not to influence mmap()\n"
    "performance in any way, while this header block can be used to store\n"
    "any version and platform related info useful to CRM114 users.\n"
    "\n"
    "These information particles can be used to:\n"
    "\n"
    "- detect / validate the proper use of various CRM file contents\n"
    "  (different classifiers)\n"
    "\n"
    "- detect if CRM114 is using a 'current version' of the data [format];\n"
    "  where applicable CRM114 can thus provide an upgrade path OR report\n"
    "  the impossibility of such (e.g. when the classifier has been removed\n"
    "  or changed in incompatible ways)\n"
    "\n"
    "- help tools to properly recognize and report on CRM114 data files\n"
    "\n"
    "- help tools to convert CRM114 files from other versions and/or\n"
    "  platforms (e.g. for automated upgrades and cross-platform\n"
    "  synchronization)\n"
    "\n"
    "\n"
    "Basic Design\n"
    "------------\n"
    "\n"
    "The Header Block is sized at precisely 8 KByte, which should be\n"
    "sufficient to store any machine- and human-readable info which should\n"
    "accompany the actual data stored in the CSS file proper.\n"
    "\n"
    "The Header Block size is set to 8KB because it is assumed that most\n"
    "(if not all) systems out there have a mmap() page size for which 8K\n"
    "will be an integer multiple >= 1. (See also the getpagesize() code in\n"
    "CRM114.)\n"
    "\n"
    "To facilitate both machines and humans perusing the file, the 8K\n"
    "header is split into two sections, each of size 4K: the Header block\n"
    "always starts with a 4K human readable ASCII text area, followed by 4K\n"
    "of machine-readable fields and byte sequences. The initial 'human\n"
    "readable text' section is also used to store some file type\n"
    "recognition data, which can be used by file type/MIME type detection\n"
    "tools (a bit similar to ZIP files which always start with the\n"
    "character sequence 'PK' for instance; for file to MIME type\n"
    "conversions using initial byte sequences see the UNIX man page\n"
    "mime.types(5) for example).\n"
    "\n"
    "Furthermore, the initial 4K section is constructed in such a way that\n"
    "the human readable text stored herein will be displayed on screen\n"
    "when:\n"
    "\n"
    "- the CRM file is e'x'ecutable on UNIX and 'run' as is: it will behave\n"
    "  like a very basic shell script.\n"
    "\n"
    "- the CRM file is fed to UNIX tools like 'head'\n"
    "\n"
    "- the CRM file is 'type'd on MSDOS or Windows compatible systems.\n"
    "\n"
    "We hope that the mechanism/structure used also works out favorably on\n"
    "non-UNIX MACintosh boxes and other platforms.\n"
    "\n"
    "The second 4K block is intended for machine-only use:\n"
    "\n"
    "- version, classifier and other info is stored here for tools to\n"
    "  detect and decide on upgrade/transformation paths available.\n"
    "\n"
    "- 'marker' sequences are stored by the originating platform to allow\n"
    "  tools able to copy/transform the data in a cross-platform/compiler\n"
    "  portable manner to detect relevant info such as integer/float size,\n"
    "  format, Endianess, etc.\n"
    "\n"
    "- additional statistics and or creation info may be stored in this\n"
    "  section to aid reporting and analysis tools\n"
;


/*
 * Write the CRM portability header block to 'f'.
 *
 * When not NULL, the 'human_readable_message' will be included as part of the header.
 *
 * Return 0 on success.
 *
 * NOTE: the file position (offset) of 'f' is assumed to be 0.
 */
int fwrite_crm_headerblock(FILE *f, CRM_PORTA_HEADER_INFO *classifier_info, const char *human_readable_message)
{
    const char *class_id = "\?\?\?";
    int ret;
    int cnt = 0;
    crm_porta_bin_header_block binhdr;

    if (!f)
        return -1;

#if !defined (CRM_WITHOUT_VERSIONING_HEADER)
    /* first, write the ID elements and human readable info */
    if (classifier_info->classifier_bits & CRM_SKS)
    {
        class_id = "SKS";
    }
    else if (classifier_info->classifier_bits & CRM_SVM)
    {
        class_id = "SVM";
    }
    else if (classifier_info->classifier_bits & CRM_OSB_WINNOW)
    {
        class_id = "WINNOW";
    }
    else if (classifier_info->classifier_bits & CRM_CLUMP)
    {
        class_id = "CLUMP";
    }
    else if (classifier_info->classifier_bits & CRM_FSCM)
    {
        class_id = "FSCM";
    }
    else if (classifier_info->classifier_bits & CRM_MARKOVIAN)
    {
        class_id = "MARKOV";
    }
    else if (classifier_info->classifier_bits & CRM_NEURAL_NET)
    {
        class_id = "NEURAL";
    }
    else if (classifier_info->classifier_bits & CRM_HYPERSPACE)
    {
        class_id = "HYPERSPACE";
    }
    else if (classifier_info->classifier_bits & CRM_OSBF)
    {
        class_id = "OSBF";
    }
    else if (classifier_info->classifier_bits & CRM_ENTROPY)
    {
        class_id = "BITENTROPY";
    }
    else if (classifier_info->classifier_bits & CRM_CORRELATE)
    {
        class_id = "CORRELATE";
    }
    else if (classifier_info->classifier_bits & CRM_OSB_BAYES)
    {
        class_id = "OSB-BAYES";
    }

    ret = fprintf(f, "%s%-10.10s:%-35.35s:%04d:%-20.20s:\ncat <<-EOF\n",
            CRM_PORTABILITY_HEADER_SEQUENCE,
            class_id,
            VERSION,
            1,
            HOSTTYPE);
    if (ret <= 0)
        return -1;

    cnt += ret;
    if (!human_readable_message)
        human_readable_message = hrmsg;
    if (human_readable_message)
    {
        int remain = CRM114_TEXT_HEADERBLOCK_SIZE - cnt - 4 - CRM_PORTABILITY_HEADER_TEXT_SENTINEL_LENGTH - 1 - 1;
        ret = fprintf(f, "%.*s\n", remain, human_readable_message);
        if (ret <= 0)
            return -1;

        cnt += ret;
    }
    ret = fprintf(f, "EOF\nexit;\n%s", CRM_PORTABILITY_HEADER_TEXT_SENTINEL);
    if (ret <= 0)
        return -1;

    cnt += ret;

    /* Now we have to pad the human readable section to 4K: */
    if (cnt < CRM114_TEXT_HEADERBLOCK_SIZE)
    {
        int remain = CRM114_TEXT_HEADERBLOCK_SIZE - cnt;

        ret = file_memset(f, 0, remain);
        if (ret != 0)
            return -1;

        cnt += remain;
    }

    /* -=# the Binary Section: #=- */

    memset(&binhdr, 0, sizeof(binhdr));
    //    Header (6)              The character sequence 'CRM114'
    memcpy(binhdr.crm_identifier, "CRM114", 6);
    //      M:Alignment
    //        int32_t (32)
    binhdr.i32_a.i32.marker[0] = 0xFF;
    binhdr.i32_a.i32.i32 = (int32_t)0x5A5A5A5A5A5A5A5ALL;
    //        int64_t (32)
    binhdr.i64_a.i64.marker[0] = 0xFF;
    binhdr.i64_a.i64.i64 = 0x5A5A5A5A5A5A5A5ALL;
    //        float (32)
    binhdr.float_a.f.marker[0] = 0xFF;
    binhdr.float_a.f.f = 15365221879119872.0;
    //        double (32)
    binhdr.double_a.d.marker[0] = 0xFF;
    binhdr.double_a.d.d = 10843961455707782.0;
    //      M:Endianess
    //        int32_t (16)
    binhdr.i32_e.i32 = (int32_t)0x0FEDCBA987654321LL;
    //        int64_t (16)
    binhdr.i64_e.i64 = 0x0FEDCBA987654321LL;
    //        float:value1 (16)
    binhdr.float_e_v1.f = -65016332288.0;
    //        float:value2 (16)
    binhdr.float_e_v2.f = 1305747233177600.0;
    //        float:filler (96)
    //---
    //        double:
    binhdr.double_e_v1.d = -2246777488905480.0;
    binhdr.double_e_v2.d = 282803858680781.90;

    //    Header2 (6)              The character sequence 'CRM114'
    memcpy(binhdr.crm_identifier_2, "CRM114", 6);
    //    Host Type (32)
    memcpy(binhdr.host_type, HOSTTYPE, CRM_MIN(strlen(HOSTTYPE), WIDTHOF(binhdr.host_type)));
    binhdr.host_type[WIDTHOF(binhdr.host_type) - 1] = 0;
    //    Version (48)
    memcpy(binhdr.version_indentifier, VERSION, CRM_MIN(strlen(VERSION), WIDTHOF(binhdr.version_indentifier)));
    binhdr.version_indentifier[WIDTHOF(binhdr.version_indentifier) - 1] = 0;

    //    Format Version  4
    binhdr.format_version.i32 = 1;
    //    (Endianess      0

    //    INT Word Size   4                 4 --> sizeof(int)
    binhdr.int_size.i32 = sizeof(int);
    //    LONG Size       4                 8 --> sizeof(long int)
    binhdr.long_size.i32 = sizeof(long int);
    //    LONG LONG Size  4                 8 --> sizeof(long long int)
    //                                      0 --> 'long long int' is unknown type on this box
    binhdr.long_long_size.i32 = sizeof(long long int);
    //    INT32 Size      4                 4 --> sizeof(int32_t)
    binhdr.i32_size.i32 = sizeof(int32_t);
    //    INT64 Size      4                 8 --> sizeof(int64_t)
    binhdr.i64_size.i32 = sizeof(int64_t);
    //    INT32 Alignment 0                 must be 'auto-detected' using the sections above
    //    INT64 Alignment 0                 must be 'auto-detected' using the sections above
    //    FLOAT Size      4                 4 --> sizeof(float)
    binhdr.float_size.i32 = sizeof(float);
    //    DOUBLE Size     4                 8 --> sizeof(double)
    binhdr.double_size.i32 = sizeof(double);
    //    (FLOAT/DOUBLE
    //     IEEE Endianess 0                 must be 'auto-detected' using the sections above

    //    Classifier      8                 e.g. CRM_OSB_WINNOW
    //    Classifier Arguments
    //                    512               This depends on the classifier and platform: arbitrary content.
    memcpy(&binhdr.classifier_info, classifier_info, sizeof(binhdr.classifier_info));

    ret = fwrite(&binhdr, sizeof(binhdr), 1, f);
    if (ret != 1)
        return -1;

    cnt += sizeof(binhdr);

    /* Now we have to pad the human readable section to 4K: */
    if (cnt < CRM114_HEADERBLOCK_SIZE)
    {
        int remain = CRM114_HEADERBLOCK_SIZE - cnt;

        ret = file_memset(f, 0, remain);
        if (ret != 0)
            return -1;

        cnt += remain;
    }

    return cnt != CRM114_HEADERBLOCK_SIZE;

#else
    /* don't support, so don't write it. */
    return 0;

#endif
}

/*
 * mmap() support routine:
 *
 * detect if '*ptr' points at a valid CRM portability header block, and if so,
 * adjust both '*ptr' and '*len' to the data following the header: reduce
 * '*len' by 8K and move '*ptr' 8K forward.
 *
 * Return 1 if the portability header was found,
 * 0 if it wasn't,
 * or a NEGATIVE value on error.
 *
 * NOTE: 'len' MAY be NULL. 'ptr' MUST NOT be NULL.
 */
int crm_correct_for_version_header(void **ptr, int *len)
{
    char *s;
    int ret;

    if (!ptr)
        return -1;

    if (*ptr == MAP_FAILED)
        return -1;

    /* very small file? which doesn't even have enough room for the header anyway? */
    if (len && *len < CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH)
        return 0;

    s = (char *)*ptr;

    ret = (0 == memcmp(CRM_PORTABILITY_HEADER_SEQUENCE, s, CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH));
    if (ret)
    {
#if !defined (CRM_WITHOUT_VERSIONING_HEADER)
        s += CRM114_HEADERBLOCK_SIZE;
        if (len)
        {
            if (*len < CRM114_HEADERBLOCK_SIZE)
            {
                fatalerror("The CRM114 'headered' CSS file is corrupt: file size is too small.",
                        "Please recover a backup with a correct version/portability header or seek help.");
                return -1;
            }
            else
            {
                *len -= CRM114_HEADERBLOCK_SIZE;
            }
        }
        *ptr = s;
#else
        fatalerror("This CRM114 build cannot cope with the new 'headered' CSS file format.",
                "Please rebuild with version/portability header support or seek a prebuild CRM114 binary that can.");
        ret = -1;   /* cannot cope with version-headered files */
#endif
    }
    return ret;
}



/*
 * Convert 'src' into a (fully) decoded portability header record 'dst'.
 *
 * Return 0 on success, otherwise a non-zero return value.
 *
 * NOTE: the 'dst.human_readable_message' structure element will be allocated on the heap
 *       using malloc() when such an element exists in the 'src' byte block.
 *
 * If 'fast_only_native' is set to TRUE, this routine supports the latest header version for
 * this (native) platform ONLY. Any other version, platform, etc. will then result in a decode
 * error.
 * This is useful to quickly check the approriateness of any CRM data file for the current task.
 */
int crm_decode_header(void *src, int64_t acceptable_classifiers, int fast_only_native, CRM_DECODED_PORTA_HEADER_INFO *dst)
{
    const char *s = src;
    const char *p;

    if (!src)
        return -1;

    if (!dst)
        return -1;

    memset(dst, 0, sizeof(*dst));

    if (0 != memcmp(CRM_PORTABILITY_HEADER_SEQUENCE, src, CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH))
        return 1;

    if (fast_only_native)
    {
        crm_porta_bin_header_block *d = &dst->binary_section;
        memcpy(d, s + CRM114_TEXT_HEADERBLOCK_SIZE, sizeof(dst->binary_section));

        if (0 != memcmp(d->crm_identifier, "CRM114", 6))
            return 2;

        //      M:Alignment
        //        int32_t (32)
        if (d->i32_a.i32.marker[0] != 0xFF)
            return 3;

        if (d->i32_a.i32.i32 != (int32_t)0x5A5A5A5A5A5A5A5ALL)
            return 3;

        //        int64_t (32)
        if (d->i64_a.i64.marker[0] != 0xFF)
            return 3;

        if (d->i64_a.i64.i64 != 0x5A5A5A5A5A5A5A5ALL)
            return 3;

        //        float (32)
        if (d->float_a.f.marker[0] != 0xFF)
            return 3;

        if (d->float_a.f.f != 15365221879119872.0)
            return 3;

        //        double (32)
        if (d->double_a.d.marker[0] != 0xFF)
            return 3;

        if (d->double_a.d.d != 10843961455707782.0)
            return 3;

        //      M:Endianess
        //        int32_t (16)
        if (d->i32_e.i32 != (int32_t)0x0FEDCBA987654321LL)
            return 4;

        //        int64_t (16)
        if (d->i64_e.i64 != 0x0FEDCBA987654321LL)
            return 4;

        //        float:value1 (16)
        if (d->float_e_v1.f != -65016332288.0)
            return 5;

        //        float:value2 (16)
        if (d->float_e_v2.f != 1305747233177600.0)
            return 5;

        //        float:filler (96)
        //---
        //        double:
        if (d->double_e_v1.d != -2246777488905480.0)
            return 5;

        if (d->double_e_v2.d != 282803858680781.90)
            return 5;

        //    Header2 (6)              The character sequence 'CRM114'
        if (0 != memcmp(d->crm_identifier_2, "CRM114", 6))
            return 6;

        //    Host Type (32)
        if (0 != memcmp(d->host_type, HOSTTYPE, CRM_MIN(strlen(HOSTTYPE), WIDTHOF(d->host_type) - 1)))
            return 7;

        //    Version (48)
        if (0 != memcmp(d->version_indentifier, VERSION, CRM_MIN(strlen(VERSION), WIDTHOF(d->version_indentifier) - 1)))
            return 7;

        //    Format Version  4
        if (d->format_version.i32 != 1)
            return 8;

        dst->header_version = d->format_version.i32;

        //    (Endianess      0

        //    INT Word Size   4                 4 --> sizeof(int)
        if (d->int_size.i32 != sizeof(int))
            return 9;

        //    LONG Size       8                 8 --> sizeof(long int)
        if (d->long_size.i32 != sizeof(long int))
            return 9;

        //    LONG LONG Size  8                 8 --> sizeof(long long int)
        //                                      0 --> 'long long int' is unknown type on this box
        if (d->long_long_size.i32 != sizeof(long long int))
            return 9;

        //    INT32 Size      4                 4 --> sizeof(int32_t)
        if (d->i32_size.i32 != sizeof(int32_t))
            return 9;

        //    INT64 Size      8                 8 --> sizeof(int64_t)
        if (d->i64_size.i32 != sizeof(int64_t))
            return 9;

        //    INT32 Alignment 0                 must be 'auto-detected' using the sections above
        //    INT64 Alignment 0                 must be 'auto-detected' using the sections above
        //    FLOAT Size      4                 4 --> sizeof(float)
        if (d->float_size.i32 != sizeof(float))
            return 9;

        //    DOUBLE Size     4                 8 --> sizeof(double)
        if (d->double_size.i32 != sizeof(double))
            return 9;

        //    (FLOAT/DOUBLE
        //     IEEE Endianess 0                 must be 'auto-detected' using the sections above

        //    Classifier      8                 e.g. CRM_OSB_WINNOW
        //    Classifier Arguments
        //                    512               This depends on the classifier and platform: arbitrary content.
        if (acceptable_classifiers)
        {
            if (!(acceptable_classifiers & d->classifier_info.classifier_bits))
                return 10;
        }



        p = strstr(s, ":\ncat <<-EOF\n");
        if (p)
        {
            const char *e;
            int len;

            p += WIDTHOF(":\ncat <<-EOF\n") - 1;

            e = strstr(p, "EOF\nexit;\n" CRM_PORTABILITY_HEADER_TEXT_SENTINEL);
            if (!e)
                return 11;

            len = e - p;
            if (len > 0)
            {
                dst->human_readable_message = calloc(len + 1, sizeof(dst->human_readable_message[0]));
                if (!dst->human_readable_message)
                    return -1;

                memcpy(dst->human_readable_message, p, len);
            }
            dst->human_readable_message[len] = 0;
        }

        dst->text_section_size = CRM114_TEXT_HEADERBLOCK_SIZE;
        dst->binary_section_size = CRM114_MACHINE_HEADERBLOCK_SIZE;

        dst->integer_endianess = 0;
        dst->floating_point_endianess = 0;

        return 0;
    }

    return -2;
}







