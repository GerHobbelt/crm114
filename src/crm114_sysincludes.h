//  crm114_sysincludes.h - Files that we include from the system.

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//   Files that we include from the system.
#ifndef __CRM114_SYSINCLUDES_H__
#define __CRM114_SYSINCLUDES_H__

// autoconf hooks
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// UNIX and Windows include files
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Only TRE regex library is currently supported
#include <tre/regex.h>

// Normally declared from tre/regex.h
//#ifndef REG_LITERAL
//#define REG_LITERAL   (REG_NOSUB << 1)
//#endif

// Detect if compilation is occurring in a Microsoft compiler
#if (defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64))
#define CRM_WINDOWS
#endif

#ifndef CRM_WINDOWS
//   UNIX and Linux specific declarations follow:
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <unistd.h>

#else	// CRM_WINDOWS
//   Windows specific declarations follow
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <direct.h>
#include <io.h>
#include <windows.h>
#include "getopt.h"

#define snprintf _snprintf
#define stat _stat
#define strncasecmp(a,b,n) strnicmp((a), (b), (n))
#define strcasecmp(a,b) stricmp((a), (b))
typedef long int clock_t;
#define MAP_FAILED NULL
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_PRIVATE 2
typedef int pid_t;
#define sqrtf sqrt
#define msync(a, b, c) FlushViewOfFile(a, b)
#define MS_SYNC 0

#endif	// CRM_WINDOWS

#endif	// !__CRM114_SYSINCLUDES_H__
