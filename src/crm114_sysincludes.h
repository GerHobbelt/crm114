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
#include <locale.h>
#include <sys/times.h>
#include <signal.h> 
#include <tre/regex.h>
// #include <regex.h>
#endif


//     Windows declarations follow

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
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
#include <locale.h>
#include <signal.h>
// #include <tre/regex.h>
#include <regex.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
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
#define sqrtf sqrt
#define msync(a, b, c) FlushViewOfFile(a, b)
#define MS_SYNC 0
#endif


#ifndef REG_LITERAL
#define REG_LITERAL   (REG_NOSUB << 1)
#endif

