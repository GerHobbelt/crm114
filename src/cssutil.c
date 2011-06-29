//	cssutil.c - utility for munging css files, version X0.1

// Copyright 2001-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

char version[] = "1.2";

void
helptext ()
{
  fprintf (stdout,
	   "cssutil version %s - generic css file utility.\n"
	   "Usage: cssutil [options]... css-file\n"

	   "		-b   - brief; print only summary\n"
	   "		-h   - print this help\n"
	   "		-q   - quite mode; no warning messages\n"
	   "		-r   - report then exit (no menu)\n"
	   "		-s css-size  - if no css file found, create new\n"
	   "			       one with this many buckets.\n"
	   "		-S css-size  - same as -s, but round up to next\n"
	   "			       2^n + 1 boundary.\n"
	   "		-v   - print version and exit\n"
	   "		-D   - dump css file to stdout in CSV format.\n"
           "		-R csv-file  - create and restore css from CSV\n",
	   VERSION);
}

int
main (int argc, char **argv)
{

  long i, k;			//  some random counters, when we need a loop
  long v;
  long sparse_spectrum_file_length = DEFAULT_SPARSE_SPECTRUM_FILE_LENGTH;
  long user_set_css_length = 0;
  long hfsize;
  long long sum;		// sum of the hits... can be _big_.
  // int hfd;
  int brief = 0, quiet = 0, dump = 0, restore = 0;
  int opt, fields;
  int report_only = 0;

  long *bcounts;
  long maxchain;
  long curchain;
  long totchain;
  long fbuckets;
  long nchains;
  long zvbins;
  long ofbins;

  long histbins;  // how many bins for the histogram

  char cmdstr[255];
  char cssfile[255];
  char csvfile[255];
  unsigned char cmdchr[2];
  char crapchr[2];
  float cmdval;
  int zloop, cmdloop;
  long learns_index, features_index;
  long docs_learned = -1;
  long features_learned = -1;

  //    the following for crm114.h's happiness

  char *newinputbuf;
  newinputbuf = (char *) &hfsize;

  histbins = FEATUREBUCKET_VALUE_MAX;
  if (histbins > FEATUREBUCKET_HISTOGRAM_MAX)
    histbins = FEATUREBUCKET_HISTOGRAM_MAX;
  bcounts = malloc (sizeof (unsigned long) * (histbins + 2) );

  {
    struct stat statbuf;		//  filestat buffer
    FEATUREBUCKET_STRUCT *hashes;	//  the text of the hash file

    // parse cmdline options
    while ((opt = getopt (argc, argv, "bDhR:rqs:S:v")) != -1)
      {
	switch (opt)
	  {
	  case 'b':
	    brief = 1;		// brief, no 'bin value ...' lines
	    break;
	  case 'D':
	    dump = 1;		// dump css file, no cmd menu
	    break;
	  case 'q':
	    quiet = 1;		// quiet mode, no warning messages
	    break;
	  case 'R':
	    {
	      FILE *f;
	      unsigned long key, hash, value;

	      // count lines to determine number of buckets and check CSV format
	      if ((f = fopen (optarg, "rb")) != NULL)
		{
		  sparse_spectrum_file_length = 0;
		  while (!feof (f))
		    if (fscanf (f, "%lu;%lu;%lu\n", &key, &hash, &value) == 3)
		      sparse_spectrum_file_length++;
		    else
		      {
			fprintf (stderr,
				 "\n %s is not in the right CSV format.\n",
				 optarg);
			exit (EXIT_FAILURE);
		      }
		  fclose (f);
		  strcpy (csvfile, optarg);
		}
	      else
		{
		  fprintf (stderr,
			   "\n Couldn't open csv file %s; errno=%d.\n",
			   optarg, errno);
		  exit (EXIT_FAILURE);
		}
	    }
	    restore = 1;	// restore css file, no cmd menu
	    break;
	  case 'r':
	    report_only = 1;	// print stats only, no cmd menu.
	    break;
	  case 's':		// set css size to option value
	  case 'S':		// same as above but round up to next 2^n+1
	    if (sscanf (optarg, "%ld", &sparse_spectrum_file_length))
	      {
		if (!quiet)
		  fprintf (stderr,
			   "\nOverride css creation length to %ld\n",
			   sparse_spectrum_file_length);
		user_set_css_length = 1;
	      }
	    else
	      {
		fprintf (stderr,
			 "On -%c flag: Missing or incomprehensible number of buckets.\n",
			 opt);
		exit (EXIT_FAILURE);
	      }
	    if (opt == 'S')	// round up to next 2^n+1
	      {
		int k;

		k = (long) floor (log10 (sparse_spectrum_file_length - 1)
				  / log10 (2.0));
		while ((2 << k) + 1 < sparse_spectrum_file_length)
		  k++;
		sparse_spectrum_file_length = (2 << k) + 1;
		user_set_css_length = 1;
	      }
	    break;
	  case 'v':
	    fprintf (stderr, " This is cssutil, version %s\n", version);
	    fprintf (stderr, " Copyright 2001-2006 W.S.Yerazunis.\n");
	    fprintf (stderr,
		     " This software is licensed under the GPL with ABSOLUTELY NO WARRANTY\n");
	    exit (EXIT_SUCCESS);
	  default:
	    helptext ();
	    exit (EXIT_SUCCESS);
	    break;
	  }
      }

    if (optind < argc)
      strncpy (cssfile, argv[optind], sizeof (cssfile));
    else
      {
	helptext ();
	exit (EXIT_SUCCESS);
      }

    //       and stat it to get it's length
    k = stat (cssfile, &statbuf);
    //       quick check- does the file even exist?
    if (k == 0)
      {
	if (restore)
	  {
	    fprintf (stderr,
		     "\n.CSS file %s exists! Restore operation aborted.\n",
		     cssfile);
	    exit (EXIT_FAILURE);
	  }
	hfsize = statbuf.st_size;
	if (!quiet && user_set_css_length)
	  fprintf (stderr,
		   "\n.CSS file %s exists; -s, -S options ignored.\n",
		   cssfile);
      }
    else
      {
	//      file didn't exist... create it
	if (!quiet && !restore)
	  fprintf (stdout, "\nHad to create .CSS file %s\n", cssfile);
	if (crm_create_cssfile
	    (cssfile, sparse_spectrum_file_length, 0, 0, 0) != EXIT_SUCCESS)
	  exit (EXIT_FAILURE);
	k = stat (cssfile, &statbuf);
	hfsize = statbuf.st_size;
      }
    //
    //   mmap the hash file into memory so we can bitwhack it
    hashes = (FEATUREBUCKET_STRUCT *) crm_mmap_file (cssfile,
						     0, hfsize,
						     PROT_READ | PROT_WRITE,
						     MAP_SHARED,
						     NULL);
    if (hashes == MAP_FAILED)
      {
	fprintf (stderr,
		 "\n Couldn't open RW file %s; errno=%d .\n", cssfile, errno);
	exit (EXIT_FAILURE);
      }

    //   from now on, hfsize is buckets, not bytes.
    hfsize = statbuf.st_size / sizeof (FEATUREBUCKET_STRUCT);

#ifdef OSB_LEARNCOUNTS
    //       If LEARNCOUNTS is enabled, we normalize with documents-learned.
    //
    //       We use the reserved h2 == 0 setup for the learncount.
    //
    {
      char* litf = "Learnings in this file";
      char* fitf = "Features in this file";
      unsigned int hcode, h1, h2;
      //
      hcode = strnhash (litf, strlen ( litf ));
      h1 = hcode % hfsize;
      h2 = 0;
      if (hashes[h1].hash != hcode)
	{
	  // initialize the file?
	  if (hashes[h1].hash == 0 && hashes[h1].key == 0)
	    {
	      hashes[h1].hash = hcode;
	      hashes[h1].key = 0;
	      hashes[h1].value = 1;
	      learns_index = h1;
	    }
	  else
	    {
	      //fatalerror (" This file should have learncounts, but doesn't!",
	      //  " The slot is busy, too.  It's hosed.  Time to die.");
	      //goto regcomp_failed;
	      fprintf (stderr, "\n Minor Caution - this file has the learncount slot in use.\n This is not a problem for Markovian classification, but it will have some\n issues with an OSB classfier.\n");
	    };
	}
      //      fprintf (stderr, "This file has had %ld documents learned!\n",
      //	       hashes[h1].value);
      docs_learned = hashes[h1].value;
      hcode = strnhash (fitf, strlen ( fitf ));
      h1 = hcode % hfsize;
      h2 = 0;
      if (hashes[h1].hash != hcode)
	{
	  // initialize the file?
	  if (hashes[h1].hash == 0 && hashes[h1].key == 0)
	    {
	      hashes[h1].hash = hcode;
	      hashes[h1].key = 0;
	      hashes[h1].value = 1;
	      features_index = h1;
	    }
	  else
	    {
	      //fatalerror (" This file should have learncounts, but doesn't!",
	      //  " The slot is busy, too.  It's hosed.  Time to die.");
	      //goto regcomp_failed ;
	      fprintf (stderr, "\n Minor Caution - this file has the featurecount slot in use.\n This is not a problem for Markovian classification, but it will have some\n issues with an OSB classfier.\n");
	    };
	}
      //fprintf (stderr, "This file has had %ld features learned!\n",
      //	       hashes[h1].value);
      features_learned = hashes[h1].value;
    };

#endif	// OSB_LEARNCOUNTS

    if (dump)
      {
	// dump the css file
	for (i = 0; i < hfsize; i++)
	  {
	    printf ("%u;%u;%u\n",
		    hashes[i].key, hashes[i].hash, hashes[i].value);
	  }
      }

    if (restore)
      {
	FILE *f;

	// restore the css file  - note that if we DIDN'T create
	//  it already, then this will fail.
	//
	if ((f = fopen (csvfile, "rb")) == NULL)
	  {
	    fprintf (stderr, "\n Couldn't open csv file %s; errno=%d.\n",
		     csvfile, errno);
	    exit (EXIT_FAILURE);
	  }
	for (i = 0; i < hfsize; i++)
	  {
	    dontcare = fscanf (f, "%u;%u;%u\n",
		    &(hashes[i].key), &(hashes[i].hash), &(hashes[i].value));
	  }
	fclose (f);
      }

    zloop = 1;
    while (zloop == 1 && !restore && !dump)
      {
	zloop = 0;
	//crm_packcss (hashes, hfsize, 1, hfsize-1);
	sum = 0;
	maxchain = 0;
	curchain = 0;
	totchain = 0;
	fbuckets = 0;
	nchains = 0;
	zvbins = 0;
	ofbins = 0;
	//   calculate maximum overflow chain length
	for (i = 1; i < hfsize; i++)
	  {
	    if (hashes[i].key != 0)
	      {
		//  only count the non-special buckets for feature count
		sum = sum + hashes[i].value;
		//
		fbuckets++;
		curchain++;
		if (hashes[i].value == 0)
		  zvbins++;
		if (hashes[i].value >= FEATUREBUCKET_VALUE_MAX)
		  ofbins++;
	      }
	    else
	      {
		if (curchain > 0)
		  {
		    nchains++;
		    totchain += curchain;
	    	    if (curchain > maxchain)
		      maxchain = curchain;
		    curchain = 0;
		  }
	      }
	  }

	fprintf (stdout, "\n Sparse spectra file %s statistics: \n", cssfile);
	fprintf (stdout, "\n Total available buckets          : %12ld ",
		 hfsize);
	fprintf (stdout, "\n Total buckets in use             : %12ld  ",
		 fbuckets);
	fprintf (stdout, "\n Total in-use zero-count buckets  : %12ld  ",
		 zvbins);
	fprintf (stdout, "\n Total buckets with value >= max  : %12ld  ",
		 ofbins);
	fprintf (stdout, "\n Total hashed datums in file      : %12lld", sum);
	fprintf (stdout, "\n Documents learned                : %12ld  ",
		 docs_learned);
	fprintf (stdout, "\n Features learned                 : %12ld  ",
		 features_learned);
	fprintf (stdout, "\n Average datums per bucket        : %12.2f",
		 (fbuckets > 0) ? (sum * 1.0) / (fbuckets * 1.0) : 0);
	fprintf (stdout, "\n Maximum length of overflow chain : %12ld  ",
		 maxchain);
	fprintf (stdout, "\n Average length of overflow chain : %12.2f ",
		 (nchains > 0) ? (totchain * 1.0) / (nchains * 1.0) : 0 );
	fprintf (stdout, "\n Average packing density          : %12.2f\n",
		 (fbuckets * 1.0) / (hfsize * 1.0));

	// set up histograms
	for (i = 0; i < histbins; i++)
	  bcounts[i] = 0;
	for (v = 1; v < hfsize; v++)
	  {
	    if (hashes[v].value < histbins)
	      {
		bcounts[hashes[v].value]++;
	      }
	    else
	      {
		bcounts[histbins]++;  // note that bcounts is len(histbins+2)
	      }
	  }

	if (!brief)
	  for (i = 0; i < histbins; i++)
	    {
	      if (bcounts[i] > 0)
		{
		  if (i < histbins)
		    {
		      fprintf (stdout, "\n bin value %8ld found %9ld times",
			       i, bcounts[i]);
		    }
		  else
		    {
		      fprintf (stdout, "\n bin value %8ld or more found %9ld times",
			       i, bcounts[i]);
		    }
		}
	    }

	fprintf (stdout, "\n");
	cmdloop = 1;
	while (!report_only && cmdloop)
	  {
	    // clear command buffer
	    cmdchr[0] = '\0';
	    fprintf (stdout, "Options:\n");
	    fprintf (stdout, "   Z n - zero bins at or below a value\n");
	    fprintf (stdout, "   S n - subtract a constant from all bins\n");
	    fprintf (stdout, "   D n - divide all bins by a constant\n");
	    fprintf (stdout, "   R - rescan\n");
	    fprintf (stdout, "   P - pack\n");
	    fprintf (stdout, "   Q - quit\n");
	    fprintf (stdout, ">>> ");
	    clearerr (stdin);
	    dontcare = fscanf (stdin, "%[^\n]", cmdstr);
	    dontcare = fscanf (stdin, "%c", crapchr);
	    fields = sscanf (cmdstr, "%s %f", cmdchr, &cmdval);
	    if (strlen ( (char *) cmdchr) != 1)
	      {
		fprintf (stdout, "Unknown command: %s\n", cmdchr);
		continue;
	      }
	    switch (tolower ((int)cmdchr[0]))
	      {
	      case 'z':
		if (fields != 2)
		  fprintf (stdout,
			   "Z command requires a numeric argument!\n");
		else
		  {
		    fprintf (stdout, "Working...");
		    for (i = 1; i < hfsize; i++)
		      if (hashes[i].value <= cmdval)
			hashes[i].value = 0;
		    fprintf (stdout, "done.\n");
		  }
		break;
	      case 's':
		if (fields != 2)
		  fprintf (stdout,
			   "S command requires a numeric argument!\n");
		else
		  {
		    fprintf (stdout, "Working...");
		    for (i = 1; i < hfsize; i++)
		      {
			if (hashes[i].value > (int) cmdval)
			  {
			    hashes[i].value = hashes[i].value - cmdval;
			  }
			else
			  {
			    hashes[i].value = 0;
			  }
		      }
		    fprintf (stdout, "done.\n");
		  }
		break;
	      case 'd':
		if (fields != 2)
		  fprintf (stdout,
			   "D command requires a numeric argument!\n");
		else if (cmdval == 0)
		  fprintf (stdout, "You can't divide by zero, nimrod!\n");
		else
		  {
		    fprintf (stdout, "Working...");
		    for (i = 1; i < hfsize; i++)
		      hashes[i].value = hashes[i].value / cmdval;
		    fprintf (stdout, "done.\n");
		  }
		break;
	      case 'r':
		zloop = 1;
		cmdloop = 0;
		break;
	      case 'p':
		fprintf (stdout, "Working...");
		crm_packcss (hashes, NULL, hfsize, 1, hfsize - 1);
		zloop = 1;
		cmdloop = 0;
		break;
	      case 'q':
		fprintf (stdout, "Bye! \n");
		cmdloop = 0;
		break;
	      default:
		fprintf (stdout, "Unknown command: %c\n", cmdchr[0]);
		break;
	      }
	  }
      }

    crm_munmap_file ((void *) hashes);
  }
  return 0;
}
