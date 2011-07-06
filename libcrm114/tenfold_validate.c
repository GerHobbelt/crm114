//  Tenfold_validation example program for libcrm114.
//
// Copyright 2010 Kurt Hackenberg & Huseyin Oktay & William S. Yerazunis, each individually
// with full rights to relicense.
//
//   This file is part of the CRM114 Library.
//
//   The CRM114 Library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   The CRM114 Library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with the CRM114 Library.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include "crm114_sysincludes.h"
#include "crm114_config.h"
#include "crm114_structs.h"
#include "crm114_lib.h"
#include "crm114_internal.h"

#include "libsvm/libsvm-2.91/svm.h"

//     TIMECHECK lets you run timings on the things going on inside
//     this demo.  You probably want to leave it turned on; it does little harm.
#define TIMECHECK 1
#if TIMECHECK
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#endif


int main (int argc, char *argv[])
{

  //
  //    The essential data declarations you will need:
  //
  CRM114_CONTROLBLOCK *p_cb;
  CRM114_DATABLOCK *p_db;
  CRM114_MATCHRESULT result;
  CRM114_ERR err;

#define MAXDATAFILES 20000
#define MAXFILENAMELEN 256
  char listfilename [MAXFILENAMELEN];
  int datafileclass[MAXDATAFILES];
  char datafiles [MAXDATAFILES][MAXFILENAMELEN];
  int learningmode;
  float ssttt_thresh;
  // **************************************************************
  //
  //    Parse the command line
  //

  //   First thing is the name of the "files file"  This is a file in the TREC
  //   format that specifies the class number of a file, then a filename (absolute
  //   or relative to the current working directory; it just gets fed directly 
  //   into a fopen() call.

  if (argc < 2)
    {
      fprintf (stderr, "Sorry, the proper use is:\n"
	      "   %s <file_of_files> <classifier> <options>\n"
	      " where file_of_files is in TREC format\n", argv[0]);
      exit (0);
    };
  strncpy (listfilename, argv[1], 1023); 
  
  FILE *listfp;
  listfp = fopen (listfilename, "r");
  if (listfp == NULL) 
    {
      fprintf (stderr, "Sorry, couldn't open the list-of-files file %s .  Bye.\n", 
	       listfilename);
      exit (0);
    };

  printf ("Reading index file %s.\n", listfilename);
  //   See what the datafiles are...
  int i;
  int nfiles = 0;
  int maxclass = 0;
  int thisclass = 0;
  int itemsread = 1;
  char filename[1024];
  while (itemsread > 0 && nfiles < MAXDATAFILES)
    {
      itemsread = fscanf (listfp, "%d %1023s",  
			  &datafileclass[nfiles], 
			  datafiles[nfiles]);
      nfiles++;
    }
  fclose (listfp);
  nfiles--;
  //   Scan the input file to see how many classes there are.
  //   Assume classes start at zero to make our life easy.
  for (i = 0; i < nfiles; i++)
    if (maxclass < datafileclass[i])
      maxclass = datafileclass[i];
  maxclass++;

  printf ("Got %d classes, with %d files total.\n", maxclass, nfiles);
					 
  //     and parse the rest of the command line
  long long my_classifier_flags = CRM114_OSB;  // default is OSB
  int timechecker = 0;
  int savefile = 0;
  learningmode = 0;
  int iarg;
  for (iarg = 1; iarg < argc; iarg++)
    {
      //    baseline classifier flags
      if (strncmp (argv[iarg],"osb", 3) == 0 ) my_classifier_flags = CRM114_OSB;
      if (strncmp (argv[iarg],"svm", 3) == 0 ) my_classifier_flags = CRM114_SVM | CRM114_APPEND;
      if (strncmp (argv[iarg],"libsvm", 6) == 0 ) my_classifier_flags = CRM114_LIBSVM | CRM114_APPEND;
      if (strncmp (argv[iarg],"hyp", 3) == 0 ) my_classifier_flags = CRM114_HYPERSPACE;
      if (strncmp (argv[iarg],"hyperspace", 10) == 0 ) my_classifier_flags = CRM114_HYPERSPACE;
      if (strncmp (argv[iarg],"fscm", 4) == 0 ) my_classifier_flags = CRM114_FSCM;
      if (strncmp (argv[iarg],"entropy", 7) == 0 ) my_classifier_flags = CRM114_ENTROPY;
      //    ...and optional classifier modifiers.
      if (strncmp (argv[iarg],"string", 6) == 0 ) my_classifier_flags = my_classifier_flags | CRM114_STRING;
      if (strncmp (argv[iarg],"unigram", 7) == 0 ) my_classifier_flags = my_classifier_flags | CRM114_UNIGRAM;
      if (strncmp (argv[iarg],"unique", 6) == 0 ) my_classifier_flags = my_classifier_flags | CRM114_UNIQUE;
      if (strncmp (argv[iarg],"crosslink", 9) == 0 ) my_classifier_flags = my_classifier_flags | CRM114_CROSSLINK;
      if (strncmp (argv[iarg],"append", 6) == 0 ) my_classifier_flags = my_classifier_flags | CRM114_APPEND;
      if (strncmp (argv[iarg],"microgroom", 10)== 0) my_classifier_flags = my_classifier_flags | CRM114_MICROGROOM;
      //   ... and optional behavioral modifiers.
      if (strncmp (argv[iarg],"time", 4) == 0 ) timechecker = 1;
      if (strncmp (argv[iarg],"save", 4) == 0 ) savefile = 1;
      if (strncmp (argv[iarg],"toe", 3) == 0) learningmode = 1;
      if (strncmp (argv[iarg],"ssttt", 5) == 0) 
	{
	  learningmode = 2;
	  ssttt_thresh = 5.0;
	  if (sscanf (argv[iarg+1], "%f", &ssttt_thresh))
	    iarg++; 
	};
    };


  //     Here's a regex we'll use, just for demonstration sake
  //static const char my_regex[] =
  //{
  //  "[a-zA-Z]+"
  //  ""          //  use this to get default regex
  //};

  //    Here's a valid pipeline (3 words in succession)
  //static const int my_pipeline[UNIFIED_ITERS_MAX][UNIFIED_WINDOW_MAX] =
  //{ {3, 5, 7} };


  //   You'll need these only if you want to do timing tests.
#if TIMECHECK
  struct tms start_time, end_time;
  struct timeval start_val, end_val;
  times ((void *) &start_time);
  gettimeofday ( (void *) &start_val, NULL);
#endif

  printf (" Creating a CB (control block) \n");
  if (((p_cb) = crm114_new_cb()) == NULL)
    {
      printf ("Couldn't allocate!  Must exit!\n");
      exit(0) ;
    };

  //    *** OPTIONAL *** Change the classifier type
  printf (" Setting the classifier flags and style. \n");
  if ( crm114_cb_setflags (p_cb, my_classifier_flags ) != CRM114_OK)
    {
      printf ("Couldn't set flags!  Must exit!\n");
      exit(0);
    };
  

  printf (" Setting the classifier defaults for this style classifier.\n");
  crm114_cb_setclassdefaults (p_cb);
  
  //   *** OPTIONAL ***  Change the default regex
  //printf (" Override the default regex to '[a-zA-Z]+' (in my_regex) \n");
  //if (crm114_cb_setregex (p_cb, my_regex, strlen (my_regex)) != CRM114_OK)
  //  {
  //    printf ("Couldn't set regex!  Must exit!\n");
  //    exit(0);
  //  };
  
  
  //   *** OPTIONAL *** Change the pipeline from the default
  //printf (" Override the pipeline to be 1 phase of 3 successive words\n");
  //if (crm114_cb_setpipeline (p_cb, 3, 1, my_pipeline) != CRM114_OK)
  //  {
  //    printf ("Couldn't set pipeline!  Must exit!\n");
  //    exit(0);
  //  };



  //   *** OPTIONAL *** Increase the number of classes
  //printf (" Setting the number of classes to 3\n");
  //p_cb->how_many_classes = 2;

  p_cb->how_many_classes = maxclass;

  //printf (" Setting the class names to 'Alice' and 'Macbeth'\n");
  //strcpy (p_cb->class[0].name, "Alice");
  //strcpy (p_cb->class[1].name, "Macbeth");
  //strcpy (p_cb->class[2].name, "Hound");
  
  printf ("Setting the class names.\n");
  for (i = 0; i < maxclass; i++)
    sprintf (p_cb->class[i].name, "Class%d", i);

  printf (" Setting our desired space to a total of 64 megabytes \n");
  p_cb->datablock_size = 64000000;
    
  printf (" Set up the CB internal state for this configuration \n");
  crm114_cb_setblockdefaults(p_cb);
  
  printf (" Use the CB to create a DB (data block) \n");
  if ((p_db = crm114_new_db (p_cb)) == NULL)
    { printf ("Couldn't create the datablock!  Must exit!\n");
      exit(0);
    };

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  if (timechecker)
    printf (
	    "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
	    end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
	    (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
	    (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif


  //  printf (" Starting to learn the 'Alice in Wonderland' text\n");
  //err = crm114_learn_text(&p_db, 0,
  //			  Alice,
  //			  strlen (Alice) );
  
  FILE *datafp;
#define MAXTEXTLEN 65536
  char text[MAXTEXTLEN];
  size_t textlen;
  int tmpclass;

  //  Randomize the order in which we're learning the files?  Actually,
  //  not needed.  You can just:
  //
  //     sort -R < index.txt > shuffled_index.txt
  //
  //  to get the files in a random order; this is advisable for TOE training.

  int partition;
  for (partition = 0; partition < 10; partition++)
    {
      printf ("Start of partition %d. \n Deleting the datablock.\n", partition);
      //  Delete and re-create the db...
      free (p_db);
      printf (" Use the CB to create a DB (data block) \n");
      if ((p_db = crm114_new_db (p_cb)) == NULL)
	{ printf ("Couldn't create the datablock!  Must exit!\n");
	  exit(0);
	};
      printf ("New datablock created.  Proceeding with LEARNs.\n");
      
      for (i = 0; i < nfiles; i++)
	{
	  if (i % 10 != partition) 
	    {
	      datafp = fopen (datafiles[i], "r");
	      if (datafp == NULL)
		{
		  fprintf (stderr, "... nope.  Couldn't open '%s'.\n", datafiles[i]);
		  break;
		}
	      textlen = fread(text, sizeof(char), MAXTEXTLEN, datafp);
	      text[textlen] = '\0' ;
	      if (learningmode == 0)
		{
		  // TEFT learning... train everything
		  printf ("Learning text # %d, '%s' (class %d) length %d \n",  
			  i, datafiles[i], datafileclass[i], (int) strlen (text));
		  err = crm114_learn_text(&p_db, datafileclass[i],
					  text,
					  textlen );
		  if (err != CRM114_OK)
		    fprintf (stderr, "Learn failed, code = %d \n", err);
		}
	      if (learningmode == 1)
		{
		  // TOE learning- train only errors.
		  printf ("Testing text # %d, '%s' (class %d) length %d \n",  
			  i, datafiles[i], datafileclass[i], (int) strlen (text));
		  err = crm114_classify_text (p_db, text, textlen, &result);
		  if (err != CRM114_OK)
		    fprintf (stderr, "Test classify failed, code = %d \n", err);
		  if (datafileclass[i] != result.bestmatch_index)
		    {
		      printf ("Learning text # %d, '%s' (class %d) length %d \n",  
			      i, datafiles[i], datafileclass[i], (int) strlen (text));
		      err = crm114_learn_text(&p_db, datafileclass[i],
					      text,
					      textlen );
		      if (err != CRM114_OK)
			fprintf (stderr, "Learn failed, code = %d \n", err);
		    }
		}
	      if (learningmode == 2)
		{
		  // SSTTT learning- train errors and marginals.
		  printf ("Testing text # %d, '%s' (class %d) length %d \n",  
			  i, datafiles[i], datafileclass[i], (int) strlen (text));
		  err = crm114_classify_text (p_db, text, textlen, &result);
		  if (err != CRM114_OK)
		    fprintf (stderr, "Test classify failed, code = %d \n", err);
		  if (result.class[result.bestmatch_index].pR < ssttt_thresh)
		    {
		      printf ("Learning text # %d, '%s' (class %d) length %d \n",  
			      i, datafiles[i], datafileclass[i], (int) strlen (text));
		      err = crm114_learn_text(&p_db, datafileclass[i],
					      text,
					      textlen );
		      if (err != CRM114_OK)
			fprintf (stderr, "Learn failed, code = %d \n", err);
		    }
		}


	      fclose (datafp);
	    }
	  else
	    { printf ("Reserving text %d for testing\n", i);};
	  
	}
      
      //   If we've been CRM114_APPENDing, in learns, now we need to run one more
      //   LEARN to actually execute the solver.
      if ( my_classifier_flags & CRM114_APPEND )
	{
	  printf (" Running a deferred solver\n");
	  p_db->cb.classifier_flags = my_classifier_flags ^ CRM114_APPEND ; 
	  err = crm114_learn_text (&p_db, 0, "", strlen (""));
	  if (err != CRM114_OK)
	    {
	      printf ("Solver failed! (error code %d)\n", err);
	      exit(0);
	    }
	};
      
#if TIMECHECK
      times ((void *) &end_time);
      gettimeofday ((void *) &end_val, NULL);
      if (timechecker)
	printf (
		"Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
		end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
		(end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
		(end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif
      
      
      //    *** OPTIONAL *** Here's how to read and write the datablocks as 
      //    ASCII text files.  This is NOT recommended for storage (it's ~5x bigger
      //    than the actual datablock, and takes longer to read in as well, 
      //    but rather as a way to debug datablocks, or move a db in a portable fashion
      //    between 32- and 64-bit machines and between Linux and Windows.  
      //
      //    ********* CAUTION *********** It is NOT yet implemented 
      //    for all classifiers (only Markov/OSB, SVM, PCA, Hyperspace, FSCM, and
      //    Bit Entropy so far.  It is NOT implemented yet for neural net, OSBF,
      //    Winnow, or correlation, but those haven't been ported over yet anyway).
      //  
      
#define READ_WRITE_TEXT
#ifdef READ_WRITE_TEXT
      
      if (savefile)
	{
	  printf (" Writing our datablock as 'simple_demo_datablock.txt'.\n");
	  crm114_db_write_text (p_db, "simple_demo_datablock.txt");
	  
	  //  printf (" Freeing the old datablock memory space\n");
	  
	  printf ("Zeroing old datablock!  Address was %ld\n", (unsigned long) p_db);
	  { 
	    int i;
	    for (i = 0; i < p_db->cb.datablock_size; i++)
	      ((char *)p_db)[i] = 0;
	  }
	  
	  //  free (p_db);
	  
	  printf (" Reading the text form back in.\n");
	  p_db = crm114_db_read_text ("simple_demo_datablock.txt");
	  printf ("Created new datablock.  Datablock address is now %ld\n", (unsigned long) p_db);
	};
      
      
#if TIMECHECK
      times ((void *) &end_time);
      gettimeofday ((void *) &end_val, NULL);
      if (timechecker)
	printf (
		"Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
		end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
		(end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
		(end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif 
#endif 
      
      //     Now, run the classifies on the remaining files
      
      for (i = 0; i < nfiles; i++)
	{
	  if (i % 10 == partition) 
	    {
	      datafp = fopen (datafiles[i], "r");
	      if (datafp == NULL)
		{
		  fprintf (stderr, "... nope.  Couldn't open '%s'.\n", datafiles[i]);
		  break;
		}
	      textlen = fread(text, sizeof(char), MAXTEXTLEN, datafp);
	      text[textlen] = '\0' ;
	      printf ("Classifying text # %d, '%s' (class %d) length %d \n",  
		      i, datafiles[i], datafileclass[i], (int) strlen (text));
	      err = crm114_classify_text (p_db, text, textlen, &result);
	      if (err != CRM114_OK)
		{
		  fprintf (stderr, "Classify failed, code = %d \n", err);
		}
	      else
		{
		  printf ("Correct class: %d, result class: %d ... ", 
			  datafileclass[i], result.bestmatch_index);
		  if (datafileclass[i] == result.bestmatch_index )
		    { printf (" RIGHT!\n"); } else { printf (" WRONG!\n"); };
		}
	      fclose (datafp);
	    }
	  else
	    { 
	      // printf ("Used text %d for learning, skipping.\n", i);
	    };
	  
	}
    }
  
#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  if (timechecker)
    printf (
	    "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
	    end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
	    (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
	    (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif
  
  printf (" Freeing the data block and control block\n");
  free (p_db);
  free (p_cb);
  
  exit (err);
}
