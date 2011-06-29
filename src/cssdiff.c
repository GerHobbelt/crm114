//	cssutil.c - utility for munging css files, version X0.1

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

int main (int argc, char **argv)
{

  long i,j,k;    //  some random counters, when we need a loop
  long hfsize, hfsize1, hfsize2;

  long f1, f2;
  long sim, diff, dom1, dom2, hclash, kclash;

  {
    struct stat statbuf;    //  filestat buffer
    FEATUREBUCKET_STRUCT *h1, *h2;              //  the text of the hash file
    //   filename is argv [1]
    //             and stat it to get it's length
    if(!argv[1] || !argv[2])
      {
        fprintf (stdout, "Usage: cssdiff <cssfile1> <cssfile2>\n");
        return (EXIT_SUCCESS);
      };
    //             quick check- does the first file even exist?
    k = stat (argv[1], &statbuf);
    if (k != 0)
      {
	fprintf (stderr, "\n CSS file '%s' not found. \n", argv[1]);
	exit (EXIT_FAILURE);
      };
    //
    hfsize = statbuf.st_size;
    //         mmap the hash file into memory so we can bitwhack it
    h1 = (FEATUREBUCKET_STRUCT *) crm_mmap_file (argv[1],
						 0, hfsize,
						 PROT_READ | PROT_WRITE,
						 MAP_SHARED,
						 NULL);

    if (h1 == MAP_FAILED)
      {
	fprintf (stderr, "\n MMAP failed on file %s\n",
		 argv[1]);
	exit (EXIT_FAILURE);
      };
    hfsize1 = statbuf.st_size / sizeof (FEATUREBUCKET_STRUCT);

    //
    //  and repeat the process for the second file:
    k = stat (argv[2], &statbuf);
    //             quick check- does the file even exist?
    if (k != 0)
      {
	fprintf (stderr, "\n.CSS file '%s' not found.\n", argv[2]);
	exit (EXIT_FAILURE);
      };

    hfsize2 = statbuf.st_size;
    //         mmap the hash file into memory so we can bitwhack it
    h2 = (FEATUREBUCKET_STRUCT *) crm_mmap_file (argv[2],
						 0, hfsize2,
						 PROT_READ | PROT_WRITE,
						 MAP_SHARED,
						 NULL);
    if (h2 == MAP_FAILED)
      {
	fprintf (stderr, "\n MMAP failed on file %s\n",
		 argv[2]);
	exit (EXIT_FAILURE);
      };

    hfsize2 = hfsize2 / sizeof (FEATUREBUCKET_STRUCT);

    fprintf (stderr, "Sparse spectra file %s has %ld bins total\n",
	     argv[1], hfsize1);


    fprintf (stdout, "Sparse spectra file %s has %ld bins total\n",
	     argv[2], hfsize2);

    //
    //
    if (hfsize1 != hfsize2)
      {
	fprintf (stderr,
		 "\n.CSS files %s, %s :\n lengths differ: %ld vs %ld.\n",
		 argv[1],argv[2], hfsize1, hfsize2);
	fprintf (stderr, "\n This is not a fatal error, but be warned.\n");
      };

    f1 = 0;
    f2 = 0;
    sim  = 0;
    diff = 0;
    dom1 = 0;
    dom2 = 0;
    hclash = 0;
    kclash = 0;
    //
    //   The algorithm - for each file,
    //                      for each bucket in each file
    //                          find corresponding bucket in other file
    //                              increment dom1 or dom2 as appropriate
    //                              always increment sim and diff
    //                          end
    //                      end
    //                      divide sim and diff by 2, as they are doublecounted
    //                      print statistics and exit.
    //
    // start at 1 - no need to check bin 0 (version).
    for ( i = 1; i < hfsize1; i++)
      {
	if (   h1[i].key != 0 )
	  {
	    f1 += h1[i].value;
	    k = h1[i].hash % hfsize2;
	    if (k == 0)
	      k = 1;
	    while (h2[k].value != 0 &&
		   (h2[k].hash != h1[i].hash
		    || h2[k].key != h1[i].key))
	      {
		k++;
		if (k >= hfsize2) k = 1;
	      };

	    //   Now we've found the corresponding (or vacant) slot in
	    //   h2.  Do our tallies...
	    j = h1[i].value ;
	    if (j > h2[k].value ) j = h2[k].value;
	    sim +=  j;

	    j = h1[i].value - h2[k].value;
	    if (j < 0) j = -j;
	    diff += j;

	    j = h1[i].value - h2[k].value;
	    if (j < 0) j = 0;
	    dom1 += j;
	  };
      };
    //
    //      And repeat for file 2.
    for ( i = 1; i < hfsize2; i++)
      {
	if (   h2[i].key != 0 )
	  {
	    f2 += h2[i].value;
	    k = h2[i].hash % hfsize1;
	    if (k == 0)
		k = 1;
	      while (h1[k].value != 0 &&
		     (h1[k].hash != h2[i].hash
		      || h1[k].key != h2[i].key))
		{
		  k++;
		  if (k >= hfsize1) k = 1;
		};

	      //   Now we've found the corresponding (or vacant) slot in
	      //   h1.  Do our tallies...
	      j = h2[i].value ;
	      if (j > h1[k].value ) j = h1[k].value;
	      sim +=  j;

	      j = h1[k].value - h2[i].value;
	      if (j < 0) j = -j;
	      diff += j;

	      j = h2[i].value - h1[k].value;
	      if (j < 0) j = 0;
	      dom2 += j;
	  };
      };

    fprintf (stdout, "\n File 1 total features            : %12ld", f1);
    fprintf (stdout, "\n File 2 total features            : %12ld\n", f2);

    fprintf (stdout, "\n Similarities between files       : %12ld", sim/2);
    fprintf (stdout, "\n Differences between files        : %12ld\n", diff/2);

    fprintf (stdout, "\n File 1 dominates file 2          : %12ld", dom1);
    fprintf (stdout, "\n File 2 dominates file 1          : %12ld\n", dom2);

  }
  return 0;
}
