//  crm_str_funcs.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2006  William S. Yerazunis, all rights reserved.
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

//    the command line argc, argv
extern int prog_argc;
extern char **prog_argv;

//    the auxilliary input buffer (for WINDOW input)
extern char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used 
//    wherever needed.  These are sized to the same size as the data window.
extern char *inbuf;
extern char *outbuf;
extern char *tempbuf;


//     strnhash - generate the hash of a string of length N
//     goals - fast, works well with short vars includng 
//     letter pairs and palindromes, not crypto strong, generates
//     hashes that tend toward relative primality against common
//     hash table lengths (so taking the output of this function
//     modulo the hash table length gives a relatively uniform distribution
//
//     In timing tests, this hash function can hash over 10 megabytes
//     per second (using as text the full 2.4.9 linux kernel source)
//     hashing individual whitespace-delimited tokens, on a Transmeta
//     666 MHz.

/*****    OLD VERSION NOT 64-BIT PORTABLE DON'T USE ME *********
long strnhash (char *str, long len)
{
  long i;
  long hval;
  char *hstr;
  char chtmp;

  // initialize hval
  hval= len;

  hstr = (char *) &hval;

  //  for each character in the incoming text:

  for ( i = 0; i < len; i++)
    {
      //    xor in the current byte against each byte of hval
      //    (which alone gaurantees that every bit of input will have
      //    an effect on the output)
      //hstr[0] = (hstr[0] & ( ~ str[i] ) ) | ((~ hstr [0]) & str[i]);
      //hstr[1] = (hstr[1] & ( ~ str[i] ) ) | ((~ hstr [1]) & str[i]);
      //hstr[2] = (hstr[2] & ( ~ str[i] ) ) | ((~ hstr [2]) & str[i]);
      //hstr[3] = (hstr[3] & ( ~ str[i] ) ) | ((~ hstr [3]) & str[i]);

      hstr[0] ^= str[i];
      hstr[1] ^= str[i];
      hstr[2] ^= str[i];
      hstr[3] ^= str[i];

      //    add some bits out of the middle as low order bits.
      hval = hval + (( hval >> 12) & 0x0000ffff) ;
		     
      //     swap bytes 0 with 3 
      chtmp = hstr [0];
      hstr[0] = hstr[3];
      hstr [3] = chtmp;

      //    rotate hval 3 bits to the left (thereby making the
      //    3rd msb of the above mess the hsb of the output hash)
      hval = (hval << 3 ) + (hval >> 29);
    }
  return (hval);
}
****/

// This is a more portable hash function, compatible with the original.
// It should return the same value both on 32 and 64 bit architectures.
// The return type was changed to unsigned long hashes, and the other
// parts of the code updated accordingly.
// -- Fidelis

unsigned long strnhash (char *str, long len)
{
  long i;
  // unsigned long hval;
  int32_t hval;
  unsigned long tmp;

  // initialize hval
  hval= len;

  //  for each character in the incoming text:
  for ( i = 0; i < len; i++)
    {
      //    xor in the current byte against each byte of hval
      //    (which alone gaurantees that every bit of input will have
      //    an effect on the output)

      tmp = str[i] & 0xFF;
      tmp = tmp | (tmp << 8) | (tmp << 16) | (tmp << 24);
      hval ^= tmp;

      //    add some bits out of the middle as low order bits.
      hval = hval + (( hval >> 12) & 0x0000ffff) ;

      //     swap most and min significative bytes 
      tmp = (hval << 24) | ((hval >> 24) & 0xff);
      hval &= 0x00ffff00;           // zero most and min significative bytes of hval
      hval |= tmp;                  // OR with swapped bytes

      //    rotate hval 3 bits to the left (thereby making the
      //    3rd msb of the above mess the hsb of the output hash)
      hval = (hval << 3) + (hval >> 29);
    }
  return (hval);
}

////////////////////////////////////////////////////////////////////////////
//
//    Cached mmap stuff.  Adapted from Win32 compatibility code from
//    Barry Jaspan.  Altered to not reveal the difference between a
//    mapped file pointer and one of Barry's 'map' structs.  In this
//    code (unlike Barry's patches), all that is ever seen are
//    pointers to memory (i.e. crm_mmap and crm_munmap have the same
//    API and semantics as with the libc mmap() and munmap() calls),
//    no structs are ever seen by the callers of this code.
//
//     Bugs in the POSIX code are my fault.  Bugs in the WIN32 code are
//     either mine or his.  So there.
//

///////////////////////////////////////////////////////////////////////////
//
//     This code section (from this line to the line below that states
//     that it is the end of the dual-licensed code section) is
//     copyright and owned by William S. Yerazunis.  In return for
//     addition of significant derivative work, Barry Jaspan is hereby
//     granted a full unlimited license to use this code section,
//     including license to relicense under other licenses.
//
////////////////////////////////////////////////////////////////////////////


//     An mmap cell.  This is how we cache.
//
typedef struct prototype_crm_mmap_cell 
{
  char *name;
  long start;
  long requested_len;
  long actual_len;
  time_t modification_time;  // st_mtime - time last modified
  void *addr;
  long prot;            //    prot flags to be used, in the mmap() form
                          //    that is, PROT_*, rather than O_*
  long mode;            //   Mode is things like MAP_SHARED or MAP_LOCKED

  int unmap_count;         //  counter - unmap this after UNMAP_COUNT_MAX
  struct prototype_crm_mmap_cell *next, *prev;
#ifdef POSIX
    int fd;
#endif
#ifdef WIN32
    HANDLE fd, mapping;
#endif
} CRM_MMAP_CELL;


//  We want these to hang around but not be visible outside this file.

static CRM_MMAP_CELL *cache = NULL;  // "volatile" for W32 compile bug


//////////////////////////////////////
//
//     Force an unmap (don't look at the unmap_count, just do it)
//     Watch out tho- this takes a CRM_MMAP_CELL, not a *ptr, so don't
//     call it from anywhere except inside this file.
//
static void crm_unmap_file_internal ( CRM_MMAP_CELL *map)
{
  long munmap_status;

#ifdef POSIX
  if (map->prot & PROT_WRITE)
    msync (map->addr, map->actual_len, MS_ASYNC | MS_INVALIDATE);
  munmap_status = munmap (map->addr, map->actual_len);
  //  fprintf (stderr, "Munmap_status is %ld\n", munmap_status);

     //    Because mmap/munmap doesn't set atime, nor set the "modified"
     //    flag, some network filesystems will fail to mark the file as
     //    modified and so their cacheing will make a mistake.
     //
     //    The fix is that for files that were mmapped writably, to do
     //    a trivial read/write on the mapped file, to force the
     //    filesystem to repropagate it's caches.
     //
  if (map->prot & PROT_WRITE)
  {
    FEATURE_HEADER_STRUCT foo;
    lseek (map->fd, 0, SEEK_SET);
    read (map->fd, &foo, sizeof(foo));
    lseek (map->fd, 0, SEEK_SET);
    write (map->fd, &foo, sizeof(foo));
  }
  
  //     Although the docs say we can close the fd right after mmap, 
  //     while leaving the mmap outstanding even though the fd is closed,
  //     actual testing versus several kernels shows this leads to 
  //     broken behavior.  So, we close here instead.
  //
  close (map->fd);
  //  fprintf (stderr, "U");
#endif


#ifdef WIN32
    FlushViewOfFile(map->addr, 0);
    UnmapViewOfFile(map->addr);
    CloseHandle(map->mapping);
    CloseHandle(map->fd);
#endif

}

/////////////////////////////////////////////////////
//    
//     Hard-unmap by filename.   Do this ONLY if you
//      have changed the file by some means outside of
//      the mmap system (i.e. by writing via fopen/fwrite/fclose).
//
void crm_force_munmap_filename (char *filename)
{
  CRM_MMAP_CELL *p;
  //    Search for the file - if it's already mmaped, unmap it.
  //    Note that this is a while loop and traverses the list.
  for (p = cache; p != NULL; p = p->next)
    {
      if (strcmp(p->name, filename) == 0)
        {
	  //   found it... force an munmap.
	  crm_force_munmap_addr (p->addr);
	  // break;     //  because p may be clobbered during unmap.
        }
    }
}


//////////////////////////////////////////////////////
//
//      Hard-unmap by address.  Do this ONLY if you
//      have changed the file by some means outside of
//      the mmap system (i.e. by writing via fopen/fwrite/fclose).
//
void crm_force_munmap_addr (void *addr)
{
  CRM_MMAP_CELL *p;

  //     step 1- search the mmap cache to see if we actually have this 
  //     mmapped
  //   
  p = cache;
  while ( p != NULL && p->addr != addr)
    p = p->next;

  if ( ! p )
    {
      nonfatalerror5 ("Internal fault - this code has tried to force unmap memory "
		     "that it never mapped in the first place.  ",
		      "Please file a bug report. ", CRM_ENGINE_HERE);
      return;
    }
    
  //   Step 2: we have the mmap cell of interest.  Mark it for real unmapping.
  //
  p->unmap_count = UNMAP_COUNT_MAX + 1;
  
  //   Step 3: use the standard munmap to complete the unmapping
  crm_munmap_file (addr);
  return;
} 
  

//////////////////////////////////////////////////////
//
//      This is the wrapper around the "traditional" file unmap, but
//      does cacheing.  It keeps count of unmappings and only unmaps
//      when it needs to.
//
void crm_munmap_file (void *addr)
{
  CRM_MMAP_CELL *p;

  //     step 1- search the mmap cache to see if we actually have this 
  //     mmapped
  //   
  p = cache;
  while ( p != NULL && p->addr != addr)
    p = p->next;

  if ( ! p )
    {
      nonfatalerror5 ("Internal fault - this code has tried to unmap memory "
		     "that it never mapped in the first place.  ",
		      "Please file a bug report. ", CRM_ENGINE_HERE);
      return;
    }
    
  //   Step 2: we have the mmap cell of interest.  Do the right thing.
  //
  p->unmap_count = (p->unmap_count) + 1;
  if (p->unmap_count > UNMAP_COUNT_MAX) 
    {
      crm_unmap_file_internal (p);
      //
      //    File now unmapped, take the mmap_cell out of the cache
      //    list as well.
      //
      if (p->prev != NULL)
	p->prev->next = p->next;
      else
	cache = p->next;
      if (p->next != NULL)
	p->next->prev = p->prev;
      free(p->name);
      free(p);
    }
  else
    {
      if (p->prot & PROT_WRITE)
	{
#ifdef POSIX
         msync (p->addr, p->actual_len, MS_ASYNC | MS_INVALIDATE);
#endif
#ifdef WIN32
	 //unmap our view of the file, which will lazily write any
	 //changes back to the file
	 UnmapViewOfFile(p->addr);
	 //and remap so we still have it open
	 p->addr = MapViewOfFile(p->mapping, (p->mode &
		 MAP_PRIVATE)?FILE_MAP_COPY:((p->prot &
		 PROT_WRITE)?FILE_MAP_WRITE:FILE_MAP_READ), 0, 0, 0);
	 //if the remap failed for some reason, just free everything
	 //  and get rid of this cached mmap entry.
	 if (p->addr == NULL)
	   {
	     CloseHandle(p->mapping);
	     CloseHandle(p->fd);
	     if (p->prev != NULL)
	       p->prev->next = p->next;
	     else
	       cache = p->next;
	     if (p->next != NULL)
	       p->next->prev = p->prev;
	     free(p->name);
	     free(p);
	   }
#endif
	}
    }
}


/////////////////////////////////////////////////////////
//
//           Force an Unmap on every mmapped memory area we know about
void crm_munmap_all()
{
  while (cache != NULL) 
    {
      cache->unmap_count = UNMAP_COUNT_MAX + 1;
      crm_munmap_file (cache->addr);
    }
}


//////////////////////////////////////////////////////////
//
//           MMap a file in (or get the map from the cache, if possible)
//             (length is how many bytes to get mapped, remember!)
//
//     prot flags are in the mmap() format - that is, PROT_, not O_ like open.
//      (it would be nice if length could be self-generated...)

void *crm_mmap_file (char *filename, 
		     long start, long requested_len, long prot, long mode, 
		     long *actual_len)
{
  CRM_MMAP_CELL *p;
  long pagesize = 0;
  struct stat statbuf;
#ifdef POSIX
  mode_t open_flags;
#endif
#ifdef WIN32
  DWORD open_flags = 0;
  DWORD createmap_flags = 0;
  DWORD openmap_flags = 0;
#endif

  pagesize = 0;
  //    Search for the file - if it's already mmaped, just return it.
  for (p = cache; p != NULL; p = p->next) 
    {
      if (strcmp(p->name, filename) == 0
	  && p->prot == prot
	  && p->mode == mode 
	  && p->start == start
	  && p->requested_len == requested_len) 
	{
	  // check the mtime; if this differs between cache and stat
	  // val, then someone outside our process has played with the
	  // file and we need to unmap it and remap it again.
	  int k;
	  struct stat statbuf;
	  k = stat (filename, &statbuf);
	  if (k != 0 
	      || p->modification_time != statbuf.st_mtime)
	    {
	      // yep, someone played with it. unmap and remap
	      crm_force_munmap_filename (filename);
	    }
	  else
	    {
	      //  nope, it looks clean.  We'll reuse it.
	      if (actual_len)
		*actual_len = p->actual_len;
	      return (p->addr);
	    }
	}
    }
  
  //    No luck - we couldn't find the matching file/start/len/prot/mode
  //    We need to add an mmap cache cell, and mmap the file.
  //  
  p = (void *) malloc( sizeof ( CRM_MMAP_CELL) );
  if (p == NULL)
    {
      untrappableerror5(" Unable to malloc enough memory for mmap cache.  ",
			" This is unrecoverable.  Sorry.", CRM_ENGINE_HERE);
      return MAP_FAILED;
    }
  p->name = strdup(filename);
  p->start = start;
  p->requested_len = requested_len;
  p->prot = prot;
  p->mode = mode;

#ifdef POSIX
  
  open_flags = O_RDWR;
  if ( ! (p->prot & PROT_WRITE) && (p->prot & PROT_READ) ) 
    open_flags = O_RDONLY;
  if ( (p->prot & PROT_WRITE) && !(p->prot & PROT_READ))
    open_flags = O_WRONLY;
  if (internal_trace)
    fprintf (stderr, "MMAP file open mode: %ld\n", (long) open_flags);

  //   if we need to, we stat the file
  if (p->requested_len < 0)
    {
      long k;
      k = stat (p->name, &statbuf);
      if ( k != 0 )
	{
	  free (p->name);
	  free (p);
	  if (actual_len)
	    *actual_len = 0;
	  return (MAP_FAILED);
	}
    }

  if (user_trace)
    fprintf (stderr, "MMAPping file %s for direct memory access.\n", filename);
  p->fd = open (filename, open_flags);
  if (p->fd < 0) 
    {
      close (p->fd);
      free(p->name);
      free(p);
      if (actual_len)
	*actual_len = 0;
      return MAP_FAILED;
    }

  //   If we didn't get a length, fill in the max possible length via statbuf
  p->actual_len = p->requested_len;
  if (p->actual_len < 0)
    p->actual_len = statbuf.st_size - p->start;

  //  and put in the mtime as well
  p->modification_time = statbuf.st_mtime;

  //  fprintf (stderr, "m");
  p->addr = mmap (NULL, 
		  p->actual_len, 
		  p->prot,
		  p->mode,
		  p->fd,
		  p->start);
  //fprintf (stderr, "M");

  //     we can't close the fd now (the docs say yes, testing says no,
  //     we need to wait till we're really done with the mmap.)
  //close(p->fd);

  if (p->addr == MAP_FAILED) 
    {
      close (p->fd);
      free(p->name);
      free(p);
      if (actual_len)
	*actual_len = 0;
      return MAP_FAILED;
    }
  
  
#endif       
#ifdef WIN32
  if (p->mode & MAP_PRIVATE)
    {
      open_flags = GENERIC_READ;
      createmap_flags = PAGE_WRITECOPY;
      openmap_flags = FILE_MAP_COPY;
    }
  else
    {
      if (p->prot & PROT_WRITE)
	{
	  open_flags = GENERIC_WRITE;
	  createmap_flags = PAGE_READWRITE;
	  openmap_flags = FILE_MAP_WRITE;
	}
      if (p->prot & PROT_READ)
	{
	  open_flags |= GENERIC_READ;
	  if (!(p->prot & PROT_WRITE))
	    {
	      createmap_flags = PAGE_READONLY;
	      openmap_flags = FILE_MAP_READ;
	    }
	}
    }
  if (internal_trace)
    fprintf (stderr, "MMAP file open mode: %ld\n", (long) open_flags);

  //  If we need to, we stat the file.
  if (p->requested_len < 0)
    {
      long k;
      k = stat (p->name, &statbuf);
      if (k != 0)
	{
	  free (p->name);
	  free (p);
	  if (actual_len)
	    *actual_len = 0;
	  return (MAP_FAILED);
	};
    };

  if (user_trace)
    fprintf (stderr, "MMAPping file %s for direct memory access.\n", filename);

  p->fd = CreateFile(filename, open_flags, 0,
		     NULL, OPEN_EXISTING, 0, NULL);
  if (p->fd == INVALID_HANDLE_VALUE) 
    {
      free(p->name);
      free(p);
      return NULL;
    }

  p->actual_len = p->requested_len;
  if (p->actual_len < 0)
    p->actual_len = statbuf.st_size - p->start;

  p->mapping = CreateFileMapping(p->fd, 
				 NULL, 
				 createmap_flags, 0, requested_len,
				 NULL);
  if (p->mapping == NULL) 
    {
      CloseHandle(p->fd);
      free(p->name);
      free(p);
      return NULL;
    }
  p->addr = MapViewOfFile(p->mapping, openmap_flags, 0, 0, 0);
  if (p->addr == NULL) 
    {
      CloseHandle(p->mapping);
      CloseHandle(p->fd);
      free(p->name);
      free(p);
      return NULL;
    }
  
  {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    pagesize = info.dwPageSize;
  }
  
  //  Jaspan-san says force-loading every page is a good thing
  //  under Windows.  I know it's a bad thing under Linux,
  //  so we'll only do it under Windows.
  {
    char one_byte;

    char *addr = (char *) p->addr;
    long i;
    for (i = 0; i < p->actual_len; i += pagesize)
      one_byte = addr[i];
  }
#endif

  //   If the caller asked for the length to be passed back, pass it.
  if (actual_len)
    *actual_len = p->actual_len;

	       
  //   Now, insert this fresh mmap into the cache list
  //
  p->unmap_count = 0;
  p->prev = NULL;
  p->next = cache;
  if (cache != NULL)
    cache->prev = p;
  cache = p;
  return p->addr;
}

#ifdef WIN32
clock_t times(TMS_STRUCT *buf)
{
  FILETIME create, exit, kern, user;
  if (GetProcessTimes(GetCurrentProcess(), &create, &exit, &kern, &user)) 
    {
      buf->tms_utime = user.dwLowDateTime;
      buf->tms_stime = kern.dwLowDateTime;
      buf->tms_cutime = 0;
      buf->tms_cstime = 0;
      return GetTickCount();
    }
  return -1;
}
#endif

///////////////////////////////////////////////////////////////////////
//
//         End of section of code dual-licensed to Yerazunis and Jaspan
//
///////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//
//     strntrn - translate characters of a string.
//    
//     Original spec by Bill Yerazunis, original code by Raul Miller,
//     recode for CRM114 use by Bill Yerazunis. 
//
//     This code section (crm_strntrn and subsidiary routines) is
//     dual-licensed to both William S. Yerazunis and Raul Miller,
//     including the right to reuse this code in any way desired,
//     including the right to relicense it under any other terms as
//     desired.
//
//////////////////////////////////////////////////////////////////////
//
//   We start out with two helper routines - one to invert a string,
//   and the other to expand string ranges.
//
//////////////////////////////////////////////////////////////////////
//
//   Given a string of characters, invert it - that is, the string
//   that was originally 0x00 to 0xFF but with all characters that
//   were in the incoming string omitted and the string repacked.
//
//   Returns a pointer to the fresh inversion, or NULL (on error)
//
//   The old string is unharmed.  Be careful of it.
//
//   REMEMBER TO FREE() THE RESULT OR ELSE YOU WILL LEAK MEMORY!!!


unsigned char * crm_strntrn_invert_string (unsigned char *str, 
					   long len, 
					   long *rlen)
{
  unsigned char *outstr;
  long i, j;

  //  create our output string space.  It will never be more than 256
  //  characters.  It might be less.  But we don't care.
  outstr = malloc (256);

  //  error out if there's a problem with MALLOC
  if (!outstr)
    {
      untrappableerror5
	("Can't allocate memory to invert strings for strstrn", "",
	 CRM_ENGINE_HERE);
    }

  //  The string of all characters is the inverse of "" (the empty
  //  string), so a mainline string of "^" inverts here to the string
  //  of all characters from 0x00 to 0xff.
  //
  //  The string "^" (equivalent to total overall string "^^") is the
  //  string of all characters *except* ^; the mainline code suffices
  //  for that situation as well.
  // 
  //  BUT THEN how does one specify the string of a single "^"?  Well,
  //  it's NOT of NOT of "NOT" ("^"), so "^^^" in the original, or
  //  "^^" here, is taken as just a literal "^" (one carat character).
  //
  if (len == 2 && strncmp ((char *)str, "^^", 2) == 0)
    {
      outstr[0] = '^';
      *rlen = 1;
      return (outstr);
    };

  //  No such luck.  Fill our map with "character present".
  //  fill it with 1's  ( :== "character present")
  //
  for (i=0; i < 256; i++)
    outstr[i] = 1;

  //   for each character present in the input string, zero the output string.
  for (i = 0; i < len; i++)
    outstr [ str [i]] = 0;

  //   outstr now is a map of the characters that should be present in the 
  //   final output string.  Since at most this is 1:1 with the map (which may
  //   have zeros) we can just reuse outstr.
  //
  for (i = 0, j = 0 ; i < 256; i++)
    if (outstr[i])
      {
	outstr[j] = i;
	j++;
      };

  //    The final string length is j characters long, in outstr.  
  //    Don't forget to free() it later.  :-)

  //  printf ("Inversion: '%s' RLEN: %d\n", outstr, *rlen);
  *rlen = j;
  return (outstr);
}

//   expand those hyphenated string ranges - input is str, of length len.
//    We return the new string, and the new length in rlen.
//
unsigned char * crm_strntrn_expand_hyphens(unsigned char *str, 
					   long len, 
					   long *rlen) 
{
  long j, k, adj;
  unsigned char* r;

  //    How much space do we need for the expanded-hyphens string
  //    (note that the string might be longer than 256 characters, if
  //    the user specified overlapping ranges, either intentionally
  //    or unintentionally.
  //
  //    On the other hand, if the user used a ^ (invert) as the first
  //    character, then the result is gauranteed to be no longer than
  //    255 characters.
  //
  for (j= 1, adj=0; j < len-1; j++) 
    {
      if ('-' == str[j]) 
	{
	  adj+= abs(str[j+1]-str[j-1])-2;
	}
    }

  //      Get the string length for our expanded strings
  //
  *rlen = adj + len;

  //      Get the space for our expanded string.
  r = malloc ( 1 + *rlen);	/* 1 + to avoid empty problems */
  if (!r) 
    {
      untrappableerror5(
	  "Can't allocate memory to expand hyphens for strstrn", 
	  "", CRM_ENGINE_HERE);
    }

  //   Now expand the string, from "str" into "r"
  //
  
  for (j= 0, k=0; j < len; j++) 
    {
      r[k]= str[j];
      //  are we in a hyphen expression?  Check edge conditions too!
      if ('-' == str[j] && j > 0 && j < len-1) 
	{
	  //  we're in a hyphen expansion
	  if (j && j < len) 
	    {
	      int delta;
	      int m = str[j-1];
	      int n = str[j+1];
	      int c;

	      //  is this an increasing or decreasing range?
	      delta = m < n ? 1 : -1;

	      //  run through the hyphen range.
	      if (m != n) 
		{
		  for (c= m+delta; c != n; c+= delta) 
		    {
		      r[k++]= (unsigned char) c;
		    };
		  r[k++]= n;
		}
	      j+= 1;
	    }
	} 
      else 
	{
	  //    It's not a range, so we just move along.  Move along!
	  k++;
	}
    };

  //  fprintf (stderr, "Resulting range string: %s \n", r);
  //  return the char *string.
  return (r);
}

//   strntrn - translate a string, like tr() but more fun.  
//    This new, improved version not only allows inverted ranges
//     like 9-0 --> 9876543210 but also negation of strings and literals 
//
//      flag of CRM_UNIQUE means "uniquify the incoming string"
//
//      flag of CRM_LITERAL means "don't interpret the alteration string"
//      so "^" and "-" regain their literal meaning
//
//      The modification is "in place", and datastrlen gets modified.
//       This routine returns a long >=0 strlen on success, 
//        and a negative number on failure.

long strntrn (
		  unsigned char *datastr,
		  long *datastrlen,
		  long maxdatastrlen,
		  unsigned char *fromstr,
		  long fromstrlen,
		  unsigned char *tostr,
		  long tostrlen,
		  long flags) 
{
  long len= *datastrlen;
  long flen, tlen;
  unsigned char map[256];
  unsigned char *from = NULL;
  unsigned char *to = NULL;
  long j, k, last;

  //               If tostrlen == 0, we're deleting, except if
  //                 ASLO fromstrlen == 0, in which case we're possibly
  //                   just uniquing or maybe not even that.
  //
  int replace = tostrlen;

  //     Minor optimization - if we're just uniquing, we don't need
  //     to do any of the other stuff.  We can just return now.
  //   
  if (tostrlen == 0 && fromstrlen == 0)
    {
      // fprintf (stderr, "Fast exit from strntrn  \n");
      *datastrlen = len;
      return (len);
    };


  //    If CRM_LITERAL, the strings are ready, otherwise build the
  //    expanded from-string and to-string.
  //
  if (CRM_LITERAL & flags)
    {
      //       Else - we're in literal mode; just copy the 
      //       strings.
      from = malloc (fromstrlen);
      strncpy  ( (char *)from,  (char *)fromstr, fromstrlen);
      flen = fromstrlen;
      to = malloc (tostrlen);
      strncpy ((char *) to, (char *)tostr, tostrlen);
      tlen = tostrlen;
      if (from == NULL || to == NULL) return (-1);
    }
  else
    {
      //  Build the expanded from-string
      if (fromstr[0] != '^')
	{
	  from = crm_strntrn_expand_hyphens(fromstr, fromstrlen, &flen);
	  if (!from) return (-1);
	}
      else
	{
	  unsigned char *temp;
	  long templen;
	  temp = crm_strntrn_expand_hyphens(fromstr+1, fromstrlen-1, &templen);
	  if (!temp) return (-1);
	  from = crm_strntrn_invert_string (temp, templen, &flen);
	  if (!from) return (-1);
	  free (temp);
	};
      
      //     Build the expanded to-string
      //
      if (tostr[0] != '^')
	{
	  to = crm_strntrn_expand_hyphens(tostr, tostrlen, &tlen);
	  if (!to) return (-1);
	}
      else
	{
	  unsigned char *temp;
	  long templen;
	  temp = crm_strntrn_expand_hyphens(tostr+1, tostrlen-1, &templen);
	  if (!temp) return (-1);
	  to = crm_strntrn_invert_string (temp, templen, &tlen);
	  if (!to) return (-1);
	  free (temp);
	};
    };
      
  //  If we're in <unique> mode, squish out any duplicated
  //   characters in the input data first.  We can do this as an in-place
  //    scan of the input string, and we always do it if <unique> is 
  //     specified.
  //
  if (CRM_UNIQUE & flags) 
    {
      unsigned char unique_map [256];

      //                        build the map of the uniqueable characters
      //
      for (j = 0; j < 256; j++)
	unique_map[j] = 1;           // all characters are keepers at first...
      for (j = 0; j < flen; j++)
	unique_map[from[j]] = 0;    //  but some need to be uniqued.
      
      //                          If the character has a 0 the unique map,
      //                          and it's the same as the prior character,
      //                          don't copy it.  Just move along.
      
      for (j= 0, k= 0, last= -1; j < len; j++) 
	{
	  if (datastr[j] != last || unique_map[datastr[j]] ) 
	    {
	      last= datastr[k++]= datastr[j];
	    };
	};
      len= k;
    };

  //     Minor optimization - if we're just uniquing, we don't need

  //     Build the mapping array  
  //
  if (replace) 
    {
      //  This is replacement mode (not deletion mode) so we need
      //   to build the character map.  We
      //    initialize the map as each character maps to itself.
      //
      for (j= 0; j < 256; j++) 
	{
	  map[j]= (unsigned char)j;
	}

      //   go through and mod each character in the from-string to
      //   map into the corresponding character in the to-string
      //   (and start over in to-string if we run out)
      //
      for (j= 0, k=0; j < flen; j++) 
	{
	  map[from[j]]= to[k];
	  //   check- did we run out of characters in to-string, so
	  //    that we need to start over in to-string?
	  k++;
	  if (k >= tlen) 
	    {
	      k= 0;
	    }
	}


      //    Finally, the map is ready.  We go thorugh the 
      //     datastring translating one character at a time.
      //
      for (j= 0; j < len; j++) 
	{
	  datastr[j]= map[datastr[j]];
	}
    } 
  else 
    {
      //  No, we are not in replace mode, rather we are in delete mode
      //  so the map now says whether we're keeping the character or
      //  deleting the character.
      for (j= 0; j < 256; j++) 
	{
	  map[j]= 1;
	}
      for (j= 0; j < flen; j++) 
	{
	  map[from[j]] = 0;
	}
      for (j= 0, k= 0; j < len; j++) 
	{
	  if (map[datastr[j]]) 
	    {
	      datastr[k++]= datastr[j];
	    }
	}
      len= k;
    }
 
  //          drop the storage that we allocated
  //          
  free(from);
  free(to);
  *datastrlen = len;
  return (len);
}

/////////////////////////////////////////////////////////////////
//
//   END of strntrn code (dual-licensed to both Yerazunis
//   and Miller
//
//////////////////////////////////////////////////////////////////
