//
//   Files that we include from the system.

//   Unix declarations follow:

#ifdef POSIX
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <sys/times.h>
#include <signal.h>
#if 0
#include <inttypes.h> /* for fixed width C99 integer types for use as binary file storage units */
#include <arpa/inet.h> /* for htonl() and ntohl() macros for use as binary file storage unit formatters */
#include <netinet/in.h> 
#endif
#include <assert.h>
#include <tre/regex.h>
// #include <regex.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif


//     Windows declarations follow

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
/* #include <ntstatus.h> */ /* DuplicateHandle() --> STATUS_SUCCESS */
/* DuplicateHandle() */
#include <stdlib.h>
#include <crtdbg.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
// #include <tre/regex.h>
#include <regex.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <assert.h>
#include "getopt.h"

#define snprintf _snprintf
#define stat _stat
#define strncasecmp(a,b,n) strnicmp((a), (b), (n))
#define strcasecmp(a,b) stricmp((a), (b))

typedef long int clock_t;
clock_t times(void *buf);

#define MAP_FAILED NULL
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_PRIVATE 2

typedef int pid_t;

// #define sqrtf sqrt  /* [i_a] */
#define crm_logl(x) logl(x)  /* [i_a] */

#define msync(a, b, c) FlushViewOfFile(a, b)
#define MS_SYNC 0

int truncate(const char *filepath, long filesize); /* [i_a] Win32 doesn't come with a truncate() function! */

#endif





#ifndef REG_LITERAL
#define REG_LITERAL   (REG_NOSUB << 1)
#endif



//   The following mumbo-jumbo needed for BSD to compile cleanly, because
//    BSD's logl function is not defined in all builds!  What a crock!
#ifdef logl
#define crm_logl(x) logl(x)
#else
#define crm_logl(x) log(x)
#endif

#ifndef sqrtf
#define sqrtf(x) sqrt(x)
#endif
//     End BSD crapola.


/* like sizeof() but this returns the number of /elements/ instead of bytes: */
#define NUMBEROF(item)			(sizeof(item)/sizeof((item)[0]))

#define CRM_MIN(a, b)			((a) <= (b) ? (a) : (b))
#define CRM_MAX(a, b)			((a) >= (b) ? (a) : (b))


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
