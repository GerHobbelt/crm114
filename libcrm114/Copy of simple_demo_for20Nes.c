//  Simple example program for libcrm114.
//
// Copyright 2010 Kurt Hackenberg & William S. Yerazunis, each individually
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

#include "crm114_sysincludes.h"
#include "crm114_config.h"
#include "crm114_structs.h"
#include "crm114_lib.h"
#include "crm114_internal.h"

#include "texts.h"		/* large megatest texts, ~100KB total */
#include "libsvm/libsvm-2.91/svm.h"

//     TIMECHECK lets you run timings on the things going on inside
//     this demo.  You probably want to leave it turned on; it does little harm.
#define TIMECHECK 1
#if TIMECHECK
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#endif


//
//     For this simple example, we'll just use four short excerpts
//     from the original works:
//



int main (int argc, char *argv[])
{

  //
  //    The essential data declarations you will need:
  //
  CRM114_CONTROLBLOCK *p_cb;
  CRM114_DATABLOCK *p_db;
  CRM114_MATCHRESULT result;
  CRM114_ERR err;

  //    Here's the classifier flags we can use (optionally)
  //*************PICK ONE PICK ONE PICK ONE PICK ONE ******************
  // static const long long my_classifier_flags = CRM114_OSB;
 //  static const long long my_classifier_flags_append = (CRM114_SVM |CRM114_APPEND) ;
  // static const long long my_classifier_flags_solve = CRM114_SVM;
   //static const long long my_classifier_flags = (CRM114_SVM | CRM114_STRING);
  // static const long long my_classifier_flags = (CRM114_LIBSVM);
  
static const long long my_classifier_flags_append = (CRM114_LIBSVM | CRM114_APPEND);
static const long long my_classifier_flags_solve = CRM114_LIBSVM;
  
//static const long long my_classifier_flags_append = CRM114_HYPERSPACE;
//static const long long my_classifier_flags_solve = CRM114_HYPERSPACE;
  
  // static const long long my_classifier_flags = CRM114_FSCM;
  // static const long long my_classifier_flags = CRM114_HYPERSPACE;
  // static const long long my_classifier_flags = CRM114_ENTROPY;            // toroid
  // static const long long my_classifier_flags = (CRM114_ENTROPY | CRM114_UNIQUE );  // dynamic mesh
  // static const long long my_classifier_flags = (CRM114_ENTROPY | CRM114_UNIQUE | CRM114_CROSSLINK);  // dynamic mesh + reuse

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
  if ( crm114_cb_setflags (p_cb, my_classifier_flags_append ) != CRM114_OK)
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

  printf (" Setting the class names to 'Alice' and 'Macbeth'\n");
  strcpy (p_cb->class[0].name, "Alice");
  strcpy (p_cb->class[1].name, "Macbeth");
  //strcpy (p_cb->class[2].name, "Hound");

  printf (" Setting our desired space to a total of 8 megabytes \n");
  p_cb->datablock_size = 8000000;


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
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif

printf("Reading the documents!\n");

char trainfilename[DIRECTORY_NAME_LENGTH];
FILE *fptr;
FILE *fp;
int whichclass=0;

static char model_outputfile_name[1024];



//training file that lists the path for the files to be used in training phase

strcpy(trainfilename,argv[1]);

fptr= fopen(trainfilename,"r");

	char line [ DIRECTORY_NAME_LENGTH ]; /* or other suitable maximum line size */
	char filename[DIRECTORY_NAME_LENGTH];
	 char * pch;
	
	while ( fgets ( line, sizeof line, fptr ) != NULL ) /* read a line */
	{
		
		pch = strtok (line,"\n");
		 while (pch != NULL)
  			{
    			//printf ("%s\n",pch);
    			strcpy(filename,pch);
    			break;
  			}
		
		
		
		
		
		
		fscanf(fptr, "%d", &whichclass);
		fgetc(fptr);
		
		if(strcmp(filename,"")==0)
			{
				continue;
			}
		else
			{
				fp=fopen(filename,"r");
			}
		if(fp==NULL)
		     	{
		     		continue;
		     	}
		else
		 		{
		    			char buffer[MAX_EMAIL_SIZE];
					
						fread (buffer, MAX_EMAIL_SIZE-1, 1,fp); //Assumes that the email is all read in once.
							{	
								//printf("Learning class %d. \n", whichclass);
								
								err = crm114_learn_text((CRM114_DATABLOCK**)&p_db,whichclass,buffer, strlen (buffer) );			
							}
						fclose(fp);
				}
		     	
	}
	
	fclose(fptr);
	
	#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif 

	

//this is to call to solve
 if ( crm114_cb_setflags (&(p_db->cb), my_classifier_flags_solve ) != CRM114_OK)
      {
      printf ("Couldn't set flags!  Must exit!\n");
      exit(0);
    };
	 
	   err = crm114_learn_text(&p_db, 1, "", 0);
	   

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
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

//#define READ_WRITE_TEXT
#ifdef READ_WRITE_TEXT
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

  

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif 
#endif 




  //    *** OPTIONAL ***  if you want to learn a third class (class #2) do it here
  //printf (" Starting to learn the 'Hound of the Baskervilles' text\n");
  //err = crm114_learn_text(&p_db, 2,
  //			  Hound,
  //			  strlen (Hound) );

int trueClass=0;
int total=0,correct=0;
printf ("\n Classifying!\n");
  
  char testfilename[DIRECTORY_NAME_LENGTH];
  strcpy(testfilename,argv[2]);
  
  fptr= fopen(testfilename,"r");
  
  while ( fgets ( line, sizeof line, fptr ) != NULL ) /* read a line */
	{
		pch = strtok (line,"\n");
		 while (pch != NULL)
  			{
    			printf ("%s\n",pch);
    			strcpy(filename,pch);
    			break;
  			}
		
		fscanf(fptr, "%d", &trueClass);
		fgetc(fptr);
		
		if(strcmp(filename,"")==0)
			{
				continue;
			}
		else
			{
				fp=fopen(filename,"r");
			}
		
		if(fp==NULL)
		     	{
		     		continue;
		     	}
		else
		 		{
		    			char buffer[MAX_EMAIL_SIZE];
					
						fread (buffer, MAX_EMAIL_SIZE-1, 1,fp); //Assumes that the email is all read in once.
							{	
								  if ((err = crm114_classify_text(p_db,
				  												buffer, 
				  												strlen (buffer),
				  												&result))
      															== CRM114_OK)
    							{ 
    								crm114_show_result( "Results:", &result);
    								if(trueClass == result.bestmatch_index)
		     									correct++;
		     						total++; 
    								
    								}
    							else exit (err);			
							}
						fclose(fp);
				}
		
	}
	
	fclose(fptr);
	
	printf("The accuracy is %.5f\n", (double)correct/total);
  
   
#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif

  printf (" Freeing the data block and control block\n");
  free (p_db);
  free (p_cb);
  
  //test_connection();

  exit (err);
}
