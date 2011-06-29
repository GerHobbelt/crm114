//
//   Files that we include from the system.

#ifndef __CRM114_SYSINCLUDES_H__
#define __CRM114_SYSINCLUDES_H__


#if defined(HAVE_CONFIG_H)
#include "config.h"
#elif defined(WIN32)
#include "config_win32.h"
#else
#error "please run ./configure in the crm114 root directory. You should have a config.h by then or you're on an unsupported system where you've got to roll your own."
#endif

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
#else
/*
   [i_a] GROT GROT GROT - cleanup the crm114 source to properly 'say' what this
   is all about, because it has NOTHING to do with any POSIXness whatsoever!
 */
// #define POSIX
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef STDC_HEADERS

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>

#else

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#elif defined(HAVE_VARARGS_H)
#include <varargs.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#endif

#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_CRT_EXTERNS_H
#include <crt_externs.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#else
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#elif defined(HAVE_TIME_H)
#include <time.h>
#endif
#endif

#if defined(HAVE_SYS_WAIT_H)
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif


#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

/* for fixed width C99 integer types for use as binary file storage units */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
/* for htonl() and ntohl() macros for use as binary file storage unit formatters */
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

/* Microsoftish stuff: */
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_CRTDBG_H
#include <crtdbg.h>
#endif
#ifdef HAVE_LMCONS_H
#include <lmcons.h>
#endif

/* getopt() support? */
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif
#ifdef HAVE_GETOPT_EX_H
#include <getopt_ex.h>
#else
#include "getopt_ex.h"
#endif


/* REs support */
#ifndef HAVE_REGEX
#error "crm114 MUST be compiled with regex support. It's a regex mutilator, remember? Try to run './configure --disable-extended-compile-checks --with-regex=tre' and run make clean && make again. Please report your findings at the developer mailing list: crm114-developers@lists.sourceforge.net"
#endif

#if defined(HAVE_TRE_REGEX)

#if defined(HAVE_TRE_REGEX_H)
#include <tre/regex.h>
#elif defined(HAVE_REGEX_H)
#include <regex.h>
#else
#error "the TRE regex library doesn't seem to come with any known headerfile?  :-S   Try to add '--with-regex-includes=DIR' to your ./configure and run make clean && make again. Please report your findings at the developer mailing list: crm114-developers@lists.sourceforge.net"
#endif

#else

#if defined(HAVE_REGEX_H)
#include <regex.h>
#else
#error "your regex library of choice doesn't seem to come with any known headerfile?  :-S       Try to add '--with-regex-includes=DIR' to your ./configure and run make clean && make again. Please report your findings at the developer mailing list: crm114-developers@lists.sourceforge.net"
#endif

#endif




/* this is for non-MS systems; goes with O_RDWR, O_RDONLY, etc. */
#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined(HAVE__SNPRINTF) && !defined(HAVE_SNPRINTF)
#undef snprintf
#define snprintf _snprintf
#endif
#if defined(HAVE__STAT) && !defined(HAVE_STAT)
#undef stat
#define stat(path, buf) _stat(path, buf)
#endif
#if defined(HAVE_STRNICMP) && !defined(HAVE_STRNCASECMP)
#undef strncasecmp
#define strncasecmp(a,b,n) strnicmp((a), (b), (n))
#endif
#if defined(HAVE_STRICMP) && !defined(HAVE_STRCASECMP)
#undef strcasecmp
#define strcasecmp(a,b) stricmp((a), (b))
#endif
#if !defined(HAVE_UTIME) && defined(HAVE__UTIME)
#undef utime
#define utime(filename, timestruct)      _utime(filename, timestruct)
#endif


#if defined(PREFER_PORTABLE_MEMMOVE) || !defined(HAVE_MEMMOVE)
void *crm_memmove(void *dst, const void *src, size_t len);
#undef memmove
#define memmove(dst, src, len)  crm_memmove(dst, src, len)
#endif


#if defined(PREFER_PORTABLE_SNPRINTF)
#error "provide a proper snprintf() implementation, please"
#endif


#if defined(HAVE_STAT_EMPTY_STRING_BUG)
static inline int crm_stat(const char *path, struct stat *buf)
{
  if (!path || !*path || !buf)
  {
    errno = EINVAL;
    return -1;
  }
  return stat(path, buf);
}
#undef stat
#define stat(path, buf)         crm_stat(path, buf)
#endif



#ifndef HAVE_CLOCK_T
typedef long int clock_t;
#endif

#ifndef HAVE_TIMES
struct tms {
  clock_t tms_utime;  // user time
  clock_t tms_stime;  // system time
  clock_t tms_cutime; // user time of children
  clock_t tms_cstime; // system time of children
};
#endif

#ifndef HAVE_TIMES
clock_t times(struct tms *buf);
#endif



#if !defined(MAP_FAILED)
#define MAP_FAILED NULL
#endif
#if !defined(PROT_READ)
#define PROT_READ 1
#define PROT_WRITE 2
#endif
#if !defined(MAP_SHARED)
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#endif



#if !defined(MS_SYNC)
#define MS_SYNC 0
#endif

#ifndef HAVE_TRUNCATE
int truncate(const char *filepath, long filesize); /* [i_a] Win32 doesn't come with a truncate() function! */
#endif


/* readline() support, anyone? */
/*
   see also:

     http://autoconf-archive.cryp.to/vl_lib_readline.html
*/
#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
//extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
//char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
/* no readline */
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else /* !defined(HAVE_HISTORY_H) */
//extern void add_history ();
//extern int write_history ();
//extern int read_history ();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
/* no history */
#endif /* HAVE_READLINE_HISTORY */







#ifndef REG_LITERAL
#define REG_LITERAL   (REG_NOSUB << 1)
#endif





/* like sizeof() but this returns the number of /elements/ instead of bytes: */
#define WIDTHOF(item)                  (sizeof(item)/sizeof((item)[0]))


#define CRM_MIN(a, b)                   ((a) <= (b) ? (a) : (b))
#define CRM_MAX(a, b)                   ((a) >= (b) ? (a) : (b))


#ifndef HAVE_STRMOV
char *strmov(char *dst, const char *src);
#endif

#if 0
/*
   This anonymous union allows a compiler to detect and report typedef errors

   Copied from http://www.netrino.com/Articles/FixedWidthIntegers/index.php
 */

static union
{
        char   int8_t_incorrect[sizeof(  int8_t) == 1];
        char  uint8_t_incorrect[sizeof( uint8_t) == 1];
        char  int16_t_incorrect[sizeof( int16_t) == 2];
        char uint16_t_incorrect[sizeof(uint16_t) == 2];
        char  int32_t_incorrect[sizeof( int32_t) == 4];
        char uint32_t_incorrect[sizeof(uint32_t) == 4];
};
#endif



/* TODO: apply __CHAR_UNSIGNED__ here? */
/*
   regular char is _signed_ char, so it will sign-expand to int when calling isgraph() et al:
   that may cause Trouble (with capital T) on some less forgiving systems.

   (e.g. isgraph(c) should be generally true for c > 127. Guess what happens to a char?
   --> sign extended to int::  c = ASCII(130) == char(-126) == int(-126) is NOT >= 128, so
   will produce FALSE as a result. Bummer.

   These inline functions will prevent this from happening by forcing the use of _Unsigned_
   char.
 */

static inline int crm_isalnum(unsigned char c) { return isalnum(c); }
static inline int crm_isalpha(unsigned char c) { return isalpha(c); }

#ifdef HAVE_ISASCII
static inline int crm_isascii(unsigned char c) { return isascii(c); }
#else
static inline int crm_isascii(unsigned char c) { return !(c >> 7); }
#endif

#ifdef HAVE_ISBLANK
static inline int crm_isblank(unsigned char c) { return isblank(c); }
#else
static inline int crm_isblank(unsigned char c) { return ((c == ' ') || (c == '\t')); }
#endif

static inline int crm_iscntrl(unsigned char c) { return iscntrl(c); }
static inline int crm_isdigit(unsigned char c) { return isdigit(c); }
static inline int crm_isgraph(unsigned char c) { return isgraph(c); }
static inline int crm_islower(unsigned char c) { return islower(c); }
static inline int crm_isprint(unsigned char c) { return isprint(c); }
static inline int crm_ispunct(unsigned char c) { return ispunct(c); }
static inline int crm_isspace(unsigned char c) { return isspace(c); }
static inline int crm_isupper(unsigned char c) { return isupper(c); }
static inline int crm_isxdigit(unsigned char c) { return isxdigit(c); }

// and to make sure no-one will use the system funcs:

#undef isalnum
#undef isalpha
#undef isascii
#undef isblank
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit

#define isalnum(c)              error_you_must_use_the_crm_isalnum_equivalent_call!
#define isalpha(c)              error_you_must_use_the_crm_isalpha_equivalent_call!
#define isascii(c)              error_you_must_use_the_crm_isascii_equivalent_call!
#define isblank(c)              error_you_must_use_the_crm_isblank_equivalent_call!
#define iscntrl(c)              error_you_must_use_the_crm_iscntrl_equivalent_call!
#define isdigit(c)              error_you_must_use_the_crm_isdigit_equivalent_call!
#define isgraph(c)              error_you_must_use_the_crm_isgraph_equivalent_call!
#define islower(c)              error_you_must_use_the_crm_islower_equivalent_call!
#define isprint(c)              error_you_must_use_the_crm_isprint_equivalent_call!
#define ispunct(c)              error_you_must_use_the_crm_ispunct_equivalent_call!
#define isspace(c)              error_you_must_use_the_crm_isspace_equivalent_call!
#define isupper(c)              error_you_must_use_the_crm_isupper_equivalent_call!
#define isxdigit(c)             error_you_must_use_the_crm_isxdigit_equivalent_call!


/* log(2) */
#define CRM_LN_2                0.69314718055994530941723212145818
/* log(10) */
#define CRM_LN_10               2.3025850929940456840179914546844

/* log10(2) */
#define CRM_LOG10_2             0.30102999566398119521373889472449



#if !defined(HAVE_LOGL) && defined(HAVE_LOG)
#define logl(val)       log(val)
#endif

#if !defined(HAVE_LOG2) && defined(HAVE_LOG)
#define log2(val)       (log(val) / CRM_LN_2)
#endif

#if !defined(HAVE_LOG10) && defined(HAVE_LOG)
#define log10(val)      (log(val) / CRM_LN_10)
#endif



#if defined(HAVE_ENVIRON)
extern char **environ;
#elif defined(HAVE___ENVIRON)
extern char **__environ;

#define environ         __environ
#elif defined(HAVE_CRT_EXTERNS_H)
/* derived from http://www.gnu-pascal.de/crystal/gpc/en/mail11031.html */
#define environ (*_NSGetEnviron())
#elif defined(WIN32)
#define environ         _environ
#endif




/*
   equivalent to touch(3).

  Because mmap/munmap doesn't set atime, nor set the "modified"
  flag, some network filesystems will fail to mark the file as
  modified and so their cacheing will make a mistake.
*/
void crm_touch(const char *filename);



#endif /* __CRM114_SYSINCLUDES_H__ */

