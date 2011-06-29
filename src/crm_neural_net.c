//  crm_neural_net.c
//
//  derived from crm_osb_hyperspace.c by Joe Langeway, rehacked by 
//  Bill Yerazunis.  Since this code is directly derived from
//  crm_osb_hyperspace.c and produced for the crm114 project so:
//  
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.  
//
//  Other licenses may be negotiated; contact Bill or Joe for details.  
//
/////////////////////////////////////////////////////////////////////
//
//     crm_neural_net.c - a nueral net classifier
//    
//     Original spec by Bill Yerazunis, original code by Joe Langeway,
//     recode for CRM114 use by Bill Yerazunis. 
//
//     This file of code (crm_neural_net* and subsidiary routines) is
//     dual-licensed to both William S. Yerazunis and Joe Langeway,
//     including the right to reuse this code in any way desired,
//     including the right to relicense it under any other terms as
//     desired.
//
//////////////////////////////////////////////////////////////////////

/*
 * This file is part of on going research and should not be considered
 * a finished product, a reliable tool, an example of good software
 * engineering, or a reflection of any quality of Joe's besides his
 * tendancy towards int hours.
 *
 *
 * Here's what's really going on:
 *
 * This is a three-layer feed-forward neural net, fully connected, with
 * 64K input channels in the retina, 64 neurons in the first layer, 64
 * neurons in the second (hidden) layer, and 2 neurons in the output
 * layer.  Using the -s or -S command line parameter will change these
 * default values in a (relatively) smart way.
 *
 * The "retina" is (by default) 64K channels, each channel corresponds
 * to a retina feature.  Usually the retina features are just the
 * hashed tokens modulo down to the retina size, so for a 64K channel
 * retina, the low-order 16 bits of the hash get used.  << option - it's
 * possible to set things up to get more than one retina channel excited
 * by one hashed token feature, but that's not the default config. >>
 *
 * Each of the 64K retina channels feeds to each of the first-layer
 * neurons, so that's 64K weights inputting to each neuron.  By default
 * there are 64 neurons in this first layer.  In that sense, it's
 * really a big crossbar.  [[ note to self: consider non-crossbar
 * configurations of neurons, say in an ECC-syndrome or block pattern
 * or checkerboard or walsh-hadamard or gray-codeset or even only the
 * near-diagonal terms are allowed to be nonzero. ]]
 *
 * Anyway, after the first layer of 64 neurons does it's summing of
 * channel times weight-per-channel, it applies the sigmoid function
 * and the result goes to the second (hidden) layer of neurons.  By
 * default, there are also 64 neurons in this second layer and they
 * are also connected to the first layer in a crossbar - every neuron
 * output in the first layer is one of 64 weighted and summed inputs
 * to the second layer.  This second layer also applies the sigmoid
 * function to it's sum.
 *
 * The final layer is just *two* neurons, each of these neurons gets
 * the input of all 64 second layer neurons, weighted.  The reason for
 * two neurons in the final layer rather than one is "degrees of freedom".
 *
 * Consider a system with only one neuron in the final layer; we would
 * train it to output a 1 for "yes, member of my class" and 0 for "no,
 * not a member".  But, how can such a system yield any response of
 * certainty in value?  It can't, with only a single output.
 *
 * By adding a second neuron in the output that is trained conversely
 * (1 for "not in class" and 0 for "member of class") , we also obtain a
 * "not in class" as an affirmative and independent degree of freedom;
 * thus our neural net can yield a two-dimensional result in even a
 * single statistics file situation.
 *
 * Training of this network is by back-propagation; see below for the
 * details on that (but consider it a simple allocation of blame) whenever
 * an incorrect result occurs).
 *
 * This code does have two "kinks" - stochastic updating, and
 * net-nuking.
 *
 * Stochastic updating, simply put, when the classic
 * backprop update rule says to update a weight by some value x,
 * instead we update by 2 * x * frand(), where frand is a uniform
 * random value between 0.0 and 1.0.  This seems very effective in
 * keeping us from getting our network stuck in a local minimum.
 * Net-nuking is allowing the user program to command the network to
 * start from a random state and retrain.  This is a fail-safe
 * maneuver so that local minima can again be (hopefully) missed.
 *
 * The counterpart of net-nuking is "learning without retrain", where
 * we accept a text as a class member example but don't retrain the
 * network yet, on the knowledge that many more texts will soon follow
 * and a retrain without those texts is a waste of CPU cycles.
 * Learning without retraining is set by the < append > flag, and
 * nuking the net is triggered by the < fromstart > flag.
 * One can also continue learning without a net-nuke by learning
 * an empty text.
 *
 * Other (minor) kinks - you'll see "NN_SPARSE_RETINA" - that means
 * that the retina has been split into two parts.  The first part of
 * the retina gets positive excitations (feature was present),  The second
 * half is 1:1 with the first half but records negative excitations (feature
 * was definitely NOT PRESENT).
 *
 * You will also see NN_N_PUMPS - this means that a token feature may excite
 * more than one channel.  By default, a token only excites one channel; but
 * the function that converts 32-bit feature tokens into channel numbers can
 * emit more than one channel to excite per incoming feature.
 *
 *
 *****************************************************************************
 * (Updated info per march 2008)
 *
  Here's what's going on:

  It's three-layer feed-forward neural net, trained by accumulating weight 
  updates, calculated with simple gradiant descent and back propagation, to 
  apply once for all documents.

  NB:  the <bychunk> flag changes this behavior to train the results of each
  document as the document is encountered.  This is _usually_ a good idea.

  The <fromstart> means "reinitialize the net".  Use this if things get 
  wedged.

  The <append> flag means _do not retrain_. Instead, just log this data
  into the file and return ASAP (use this to load up a bunch of data
  and then train it all, which is computationally cheaper)

 */

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"



#if !defined (CRM_WITHOUT_NEURAL_NET)



#define HEADER_SIZE 1024

#define STOCHASTIC
//#define TRAIN_PER_DOCUMENT	
//#define ACCUMULATE_TRAINING

//    DESCRIPTION OF A NEURAL NETWORK
//
//   retina_size is number of inputs to the first layer; feature IDs need
//                 to be moduloed down to this value.
//   first_layer_size is the number neurons in the first layer.  Every neuron
//                 sums retina_size weighted values from each of the 
//                 retina inputs
//   hidden_layer_size is the number of neurons in the hidden layer.  Each of
//                 these neurons sums first_layer_size weighted outputs
//                  of the first layer
//   ... and of course, in this implementation, the final layer is just TWO
//                 neurons (in-class and out-of-class), summing up 
//                 hidden_layer_size weighted outputs.


typedef struct mythical_neural_net_header
{
    int32_t retina_size;
    int32_t first_layer_size;
    int32_t hidden_layer_size;
} NEURAL_NET_HEAD_STRUCT;

typedef struct mythical_neural_net
{
  //             The W's are the weights
    float        *Win; // dynamically sized multidimensional array
    float        *Whid; // dynamically sized multidimensional array
    float        *Wout; // dynamically sized multidimensional array

  //                these are the activation intensities
  float *retina;
    float         *first_layer;
    float         *hidden_layer;
    float          output_layer[2];
  //                      these are the learning deltas
    float         *delta_first_layer;
    float         *delta_hidden_layer;
    float          delta_output_layer[2];
    crmhash_t     *docs_start;
    crmhash_t     *docs_end;
    int32_t        retina_size;
    int32_t        first_layer_size;
    int32_t        hidden_layer_size;
    void          *file_origin;
} NEURAL_NET_STRUCT;

//   Three little functions to turn bare pointers to the neural
//   network coefficient arrays into *floats so they can be read
//   or written.  This hack is because C doesn't do multidimensional
//   arrays with runtime defined dimensions very well.  
//
//    Just to remind you- nn is the neural net, "neuron" is the neuron being 
//    activated, and "channel" is the channel on that neuron.  (note that 
//    this means actual storage is in "odometer" format - the last arg 
//    varying fastest.
//  
inline static float *arefWin(NEURAL_NET_STRUCT *nn, int neuron, int channel)
{
   if (neuron < 0 || neuron > nn->first_layer_size)
    fprintf (stderr, "bad neuron number %d in first layer calc\n", neuron);
  if (channel < 0 || channel > nn->retina_size)
    fprintf (stderr, "bad channel number %d in first layer calc\n", channel);
    if (internal_trace)
    {
    fprintf (stderr, "nn = %p, Win = %p, neuron = %d, channel = %d\n",
  	   nn, nn->Win, neuron, channel);
	}
	return &nn->Win [(neuron * nn->retina_size) + channel];
}

inline static float avalWin(NEURAL_NET_STRUCT *nn, int neuron, int channel)
{
    if (neuron < 0 || neuron > nn->first_layer_size)
    fprintf (stderr, "bad neuron number %d in first layer calc\n", neuron);
  if (channel < 0 || channel > nn->retina_size)
    fprintf (stderr, "bad channel number %d in first layer calc\n", channel);
    if (internal_trace)
    {
    fprintf (stderr, "nn = %p, Win = %p, neuron = %d, channel = %d\n",
  	   nn, nn->Win, neuron, channel);
	}
	return nn->Win [(neuron * nn->retina_size) + channel];
}


//     Same deal, nn is the net, "neuron" is the neuron number, "channel" 
//      is the channel on that neuron.
inline static float *arefWhid(NEURAL_NET_STRUCT *nn, int neuron, int channel)
{
  if (neuron < 0 || neuron > nn->hidden_layer_size)
    fprintf (stderr, "bad neuron number %d in hidden layer calc\n", neuron);
  if (channel < 0 || channel > nn->first_layer_size)
    fprintf (stderr, "bad channel number %d in hidden layer calc\n", channel);
  return & (nn->Whid[( neuron * nn->first_layer_size) + channel]);
}

//     Final layer, same deal;, nn is the net, "neuron" is the neuron 
//      number (here, 0 or 1), "channel" is the channel on that neuron.
inline static float *arefWout(NEURAL_NET_STRUCT *nn, int neuron, int channel)
{
  if (neuron < 0 || neuron > 1)
    fprintf (stderr, "bad neuron number %d in final layer calc\n", neuron);
  if (channel < 0 || channel > nn->hidden_layer_size)
    fprintf (stderr, "bad channel number %d in final layer calc\n", channel);
  return &(nn->Wout [(neuron * nn->hidden_layer_size) + channel]);
}

inline static float sign (float x)
{
  if (x < 0.0) return -1;
  return 1;
}

//    Stochastic_factor is a "noise term" to keep the net shaken up a bit;
//    this prevents getting stuck in local minima (one of which can occur
//    when a weight approaches zero, because then the motion becomes *very*
//    small (alpha, the motion factor, is usually << 1.0) and it's not
//    possible for the weight value to "jump over" zero and
//    go negative.  This causes that weight to lock; if enough weights lock,
//    the net fails to converge.
//
//    There are two parameters used (both are static local vars!).  One is
//    the maximum noise added (recommended to be about the same value as 
//    alpha) and the other is the drop rate, which is how fast the noise 
//    diminishes with increasing learning cycles.  This is measured in 
//    terms of the soft cycle limit and epoch number; perhaps it would be
//    better if the drop was actually a 1/R curve (Boltzmann curve) but
//    this works well.  
//
//    Note: the following also worked pretty well:
//     ((0.05 * (double)rand() / (double) RAND_MAX) - 0.025)

inline static double rand0to1()
{
  return ((double)rand() / (double)RAND_MAX);
}

inline static float stochastic_factor (float stoch_noise,
				       int soft_cycle_limit, 
				       int epoch_num) 
{
  float v;
  
  v = ( rand0to1() - 0.5) * stoch_noise;
  //  v = ((((double)rand ()) / ((double) RAND_MAX))) * stoch_noise;
  //  v = v * ( 1 / ( epoch_num + 1));		
  //  v = v * 
  //    (1 - (epoch_num / soft_cycle_limit));
  return (v);
};

//     Gain noise is noise applied to the training impulse, rather than
//     gain applied to the weights
inline static double gain_noise_factor ()
{
  return ( rand0to1() * NN_DEFAULT_GAIN_NOISE);
};


//  These are used _only_ when a new net needs to be created;  
//  the actual net dimensions used usually come from the file header
static int retina_size = NN_RETINA_SIZE;
static int first_layer_size = NN_FIRST_LAYER_SIZE;
static int hidden_layer_size = NN_HIDDEN_LAYER_SIZE;

/*

Another (as yet unimplemented) idea is to stripe the retina in a
pattern such that each neuron is "up against" every other neuron at
least once, but not as many times as the current "full crossbar"
system, which takes a int time to train.

One way to do this is to label the points onto a fully connected graph
nodes, and then to take the graph nodes in pairwise (or larger) groups
such as, on edges, faces, or hyperfaces, so that every graph node is
taken against every other nodeand hence every input at one level is
"against" every other input.  However, we haven't implemented this.

*/



static int make_new_backing_file(const char *filename)
{
    int i;
    FILE *f;
    NEURAL_NET_HEAD_STRUCT h;
    CRM_PORTA_HEADER_INFO classifier_info = { 0 };
  float a;

    f = fopen(filename, "wb");
    if (!f)
    {
        nonfatalerror("unable to create neural network backing file", "filename");
        return -1;
    }

    classifier_info.classifier_bits = CRM_NEURAL_NET;

    if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't write header to file %s; errno=%d(%s)\n",
                filename, errno, errno_descr(errno));
        fclose(f);
        return -1;
    }
  if(sparse_spectrum_file_length)
  {
    first_layer_size = sqrt (sparse_spectrum_file_length / 1024);
    if (first_layer_size < 4) first_layer_size = 4;
    hidden_layer_size = first_layer_size;
    retina_size = first_layer_size * 1024  ;
    if (retina_size < 1024) retina_size = 1024;
  }
  if (internal_trace || user_trace )
  {
    fprintf (stderr, "Input sparse_spectrum_file_length = %d. \n"
	     "new neural net dimensions:\n retina width=%d, "
	     "first layer neurons = %d, hidden layer neurons = %d\n",
	     sparse_spectrum_file_length, retina_size, 
	     first_layer_size, hidden_layer_size);
  }

    h.retina_size = retina_size;
    h.first_layer_size = first_layer_size;
    h.hidden_layer_size = hidden_layer_size;
    if (1 != fwrite(&h, sizeof(NEURAL_NET_HEAD_STRUCT), 1, f))
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't write Neural Net header to file %s; errno=%d(%s)\n",
                filename, errno, errno_descr(errno));
        fclose(f);
        return -1;
    }

    if (file_memset(f, 0, HEADER_SIZE - sizeof(NEURAL_NET_HEAD_STRUCT)))
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't write to file %s; errno=%d(%s)\n",
                filename, errno, errno_descr(errno));
        fclose(f);
        return -1;
    }


  //      and write random small floating points into the 
  //      weighting.
  i = (retina_size * first_layer_size)
    + (first_layer_size * hidden_layer_size)
    + (hidden_layer_size * 2);
  if (internal_trace)
    {
      fprintf (stderr, "Putting out %d coefficients.\n", i);
      fprintf (stderr, "Initial weight ");
      i--;
      a = rand0to1 () * NN_INITIALIZATION_NOISE_MAGNITUDE;
      fprintf (stderr, "%lf ", a);
        if (1 != fwrite(&a, sizeof(a), 1, f))
        {
            fatalerror_ex(SRC_LOC(),
                    "\n Couldn't write Neural Net Coefficients to file %s; errno=%d(%s)\n",
                    filename, errno, errno_descr(errno));
            fclose(f);
            return -1;
        }
      i--;
      a = rand0to1 () * NN_INITIALIZATION_NOISE_MAGNITUDE;
      fprintf (stderr, "%lf ", a);
        if (1 != fwrite(&a, sizeof(a), 1, f))
        {
            fatalerror_ex(SRC_LOC(),
                    "\n Couldn't write Neural Net Coefficients to file %s; errno=%d(%s)\n",
                    filename, errno, errno_descr(errno));
            fclose(f);
            return -1;
        }
      i--;
      a = rand0to1 () * NN_INITIALIZATION_NOISE_MAGNITUDE;
      fprintf (stderr, "%lf ", a);
        if (1 != fwrite(&a, sizeof(a), 1, f))
        {
            fatalerror_ex(SRC_LOC(),
                    "\n Couldn't write Neural Net Coefficients to file %s; errno=%d(%s)\n",
                    filename, errno, errno_descr(errno));
            fclose(f);
            return -1;
        }
      fprintf (stderr, "\n");
    };
    while (i--)
    {
      a = rand0to1 () * NN_INITIALIZATION_NOISE_MAGNITUDE;
        if (1 != fwrite(&a, sizeof(a), 1, f))
        {
            fatalerror_ex(SRC_LOC(),
                    "\n Couldn't write Neural Net Coefficients to file %s; errno=%d(%s)\n",
                    filename, errno, errno_descr(errno));
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

static int map_file(NEURAL_NET_STRUCT *nn, char *filename)
{
    int i;
    NEURAL_NET_HEAD_STRUCT *h;
    struct stat statee;
    float *w;
    int filesize_on_disk;

    if (stat(filename, &statee))
    {
        nonfatalerror("unable to map neural network backing file", "filename");
        return -1;
    }
    filesize_on_disk = statee.st_size;
    nn->file_origin = crm_mmap_file(filename,
            0,
            filesize_on_disk,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            CRM_MADV_RANDOM,
            &filesize_on_disk);
    if (nn->file_origin == MAP_FAILED)
    {
        nonfatalerror("unable to map neural network backing file", "filename");
        return -1;
    }

    //    Fill in our NN's description from stuff in the header- the sizes,
    //    the weights Win (input layer, Whid (hidden layer) and Wout (output
    //    layer) and the
    //
    //     (note that Win, Whid, and Wout actually pointers to arrays of pointers.
    //      In this way we get the appearence of two dimensional arrays, but are
    //      actually just pointing from each start of a one-d array, UNLIKE
    //      when you just create a int vector and use subscript arithmetic)
    //     Note also that most of these (the ones "malloced" below) are
    //     transient and are NOT part of the disk image.
    //
    //  aim the h-struct at the mem-mapped file
    h = nn->file_origin;
    //   copy the important sizes
    nn->retina_size = h->retina_size;
    nn->first_layer_size = h->first_layer_size;
    nn->hidden_layer_size = h->hidden_layer_size;

  if (internal_trace)
{
    fprintf (stderr, "Neural net dimensions: retina width=%d, "
	     "first layer neurons = %d, hidden layer neurons = %d\n",
	     retina_size, first_layer_size, hidden_layer_size);
}
  //  These are the sums of the weighted input activations for these layers
  nn->retina = calloc (h->retina_size, sizeof(nn->retina[0]));
    nn->first_layer = calloc(h->first_layer_size, sizeof(nn->first_layer[0]));
    nn->hidden_layer = calloc(h->hidden_layer_size, sizeof(nn->hidden_layer[0]));
  //  These are the deltas used for learning (only).
    nn->delta_first_layer = calloc(h->first_layer_size, sizeof(nn->delta_first_layer[0]));
    nn->delta_hidden_layer = calloc(h->hidden_layer_size, sizeof(nn->delta_hidden_layer[0]));


  // remember output layer is statically allocated!
  
  //skip w over the header
    w = (float *)((char *)h + HEADER_SIZE);
  nn->Win = w;
  nn->Whid = w + h->retina_size * h->first_layer_size           ;
  
  nn->Wout = w + h->retina_size * h->first_layer_size
               + h->first_layer_size * h->hidden_layer_size     ;
  
    //      This is where the saved documents start (not the actual text, but
    //      rather the sorted bags of retina projections.  For the default case,
    //      with a 64K channel retina, this is the hashes mod 64K.  For other
    //      configurations of the network, this will be something different.  In
    //      all cases though the individual projections are Unsigned Longs.
    nn->docs_start = (crmhash_t *)w
                     + h->retina_size * h->first_layer_size
                     + h->first_layer_size * h->hidden_layer_size
                     + h->hidden_layer_size * 2;
    nn->docs_end = (crmhash_t *)((char *)h + filesize_on_disk);
    return 0;
}

static void unmap_file(NEURAL_NET_STRUCT *nn, char *filename)
{
    free(nn->retina);
    free(nn->first_layer);
    free(nn->hidden_layer);
    free(nn->delta_first_layer);
    free(nn->delta_hidden_layer);
    crm_munmap_file(nn->file_origin);

#if 0  /* now touch-fixed inside the munmap call already! */
#if defined (HAVE_MMAP) || defined (HAVE_MUNMAP)
    //    Because mmap/munmap doesn't set atime, nor set the "modified"
    //    flag, some network filesystems will fail to mark the file as
    //    modified and so their cacheing will make a mistake.
    //
    //    The fix is to do a trivial read/write on the .css ile, to force
    //    the filesystem to repropagate it's caches.
    //
    crm_touch(learnfilename);
#endif
#endif
}

// cache this if this thing is too slow, we'll need crazy interpolation though
// Actually, it's not bad; we spend far more time in frand() than here.
static double logistic(double a)
{
  float y;
  if(isnan(a))
    {
      //char *foo;
      //foo = malloc (32);
      //fprintf (stderr, "Logistic of a NAN\n");
      //foo = NULL;
      //strcpy ("hello, world", foo);
      fatalerror("Tried to compute the logistic function of a NAN!!", ""); 
      return 0.5; //perhaps we should escape all the way
    }
  
  //#define INTERPOLATE_LOGISTIC
#ifndef INTERPOLATE_LOGISTIC
    y = 1.0 / (1.0 + exp(-a));
    if(isnan(y))
    {
      fatalerror("Computed a NAN as a RESULT of the logistic function!", "");
      return 0.5;
    }
  return y;
#else
  //    Code to use a lookup table rather than the exp function
  //    on every pass.  This has a worst case error of about 1.19%
  //    which is just fine for us here.
  {
    double adub;
    int lo;
    const float logistic_lookup[21] = { .0000454,
					.0001234,
					.0003354,
					.0009111,
					.0024726,
					.0066929,
					.0179862,
					.0474259,
					.1192029,
					.2689414,
					.5000000,
					.7310586,
					.8807971,
					.9525741,
					.9820138,
					.9933071,
					.9975274,
					.9990889,
					.9996646,
					.9998766,
					.9999546};
    if (a > 10 || a < -10) 
     {
       fprintf (stderr, " L:%lf", a);
       return ( 1.0 / (1.0 + exp (-a))); 
     }
    else
      fprintf (stderr, "-");
    //if (a < -5.0) return 0.001;
    //if (a >  5.0) return 0.999;
    lo = floor (a);
    adub = a - lo;
    //    fprintf (stderr, "a = %lf, lo = %d, adub = %lf\n",
    //     a, lo, adub);
    y = logistic_lookup[lo + 11] * adub 
      + logistic_lookup[lo + 11 + 1] * (1.0 - adub);
    return y;
  };
#endif
}


//    A totally ad-hoc way to calculate pR.  (well, since all pR's
//     are really ad-hoc underneath, it's perfectly reasonable).
//
//  GROT GROT GROT this is piecewise linear; we may want to make this
//  a smoother one later... but that's a hard problem and not particularly
//  pressing issue right now.
static double get_pR(double p)
{
    double a = fabs(p - 0.5);
  // scale 0..0.1 to 0..1.0
    if (a < 0.1)
        a *= 10.0;
  //  scale 0.1...0.2 to 1.0...10.0
    else if (a < 0.2)
        a = 1.0 + 90.0 * (a - 0.1);
  //   scale 0.2...0.3 to 10.0...100.0
    else if (a < 0.3)
        a = 10.0 + 900.0 * (a - 0.2);
  //    scale 0.3... to 100.0...and up
    else
        a = 100.0 + 1000.0 * (a - 0.3);
    return p >= 0.5 ? a : -a;
}


// if any bag[i] > nn->retina_size you're doomed
//
//    Actually evaluate the neural net, given
//    the "bag" of features.  

static void do_net (NEURAL_NET_STRUCT *nn, crmhash_t *bag)
{
  int i;
  int neuron, channel;

  //    Initialize the activation levels to zeroes everywhere

  //   First layer:
  for(neuron = 0; neuron < nn->first_layer_size; neuron++)
    nn->first_layer[neuron] = 0.00;

  //   Second (hidden) layer:
  for(neuron = 0; neuron < nn->hidden_layer_size; neuron++)
    nn->hidden_layer[neuron] = 0.00;

  //   Final layer:
  nn->output_layer[0] = 0.00;
  nn->output_layer[1] = 0.00;

  //    Sum up the activations on the first layer of the net.
  //    Note that this code (amazingly) works fine for both _non_-projected
  //    as well as projected feature bags, by inclusion of the modulo on
  //    the retina size.
  //  
  //    Note that the sentinel to "stop" is a 0 or 1 in the bag array,
  //    which is the marker for "next featureset".  Also, note that the
  //    features in the bag have already been "uniqued" in the vector
  //    tokenizer, if UNIQUE is desired.

    if (internal_trace)
    {
      fprintf (stderr, "First six items in the bag are: \n "
	       "     %d %d %d %d %d %d\n",
	       (int)bag[0], (int)bag[1], (int)bag[2], (int)bag[3], (int)bag[4], (int)bag[5]);
    }

  //   Empty the retina:
  for (channel = 0; channel < nn->retina_size; channel++)
    nn->retina[channel] = 0.00;

  //    use i=2 to skip checksum and times-trained counter.
  for(i = 2; bag[i] > 1; i++)  
    {
      // for each feature in the bag
      channel = bag[i] % nn-> retina_size;
      //     Channel 0 is reserved as "constants", so if the modulo
      //     puts a stimulus onto the 0th retina channel, we actually 
      //     push it over one channel, to retina channel 1.
      if (channel == 0) channel = 1;
      nn->retina[channel] += 1.0;
    };
  //     debugging check
  if (internal_trace)
    fprintf (stderr, "Running do_net %p on doc at %p length %d\n", 
	     nn, bag, i);

  //    Now we actually calculate the neuron activation values.
  //
  //    First, the column 0 "always activated" constants, since channel
  //    zero is verboten as an actual channel (we roll channel 0 activations
  //    over onto channel 1)
  //

  nn->retina[0] = 1.0;

  //   Major confusion prevention debugging statement.
  if ( 0 )
    for (channel = 0; channel < nn->retina_size; channel++)
      if (nn->retina[channel] > 0)
	fprintf (stderr, " %d", channel);
  
  for ( neuron = 0; neuron < nn->first_layer_size; neuron++)
    for (channel = 0; channel < nn->retina_size; channel++)
      //  sum the activations on the first layer for this channel
      nn->first_layer[neuron] += 
	//	avalWin(nn, neuron, channel)
	nn->Win[(neuron*nn->retina_size) + channel]
	* nn->retina[channel];

  //   Do a nonlinear function (logistic) in-place on the first layer outputs.
  for(neuron = 0; neuron < nn->first_layer_size; neuron++)
    {
      nn->first_layer[neuron] = logistic(nn->first_layer[neuron]);
    };
 
  //    For each neuron output in the first layer, generate activation
  //    levels for the second (hidden) layer inputs.
  for( channel = 0; channel < nn->first_layer_size; channel++)
    {
      for(neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	nn->hidden_layer[neuron] += 
	  *arefWhid(nn, neuron, channel)
	  * nn->first_layer[channel];
    };
  
  //   Do our nonlinear function (logistic) here on the second layer outputs.
  for(neuron = 0; neuron < nn->hidden_layer_size; neuron++)
    nn->hidden_layer[neuron] = logistic(nn->hidden_layer[neuron]);
  
  //     Generate the values in the final output layer (both neurons)
  //     Note that there is no renormalization in these neurons.
  for(channel = 0; channel < nn->hidden_layer_size; channel++)
    {
      nn->output_layer[0] += *arefWout(nn, 0, channel) 
	* nn->hidden_layer[channel];
      nn->output_layer[1] += *arefWout(nn, 1, channel) 
	* nn->hidden_layer[channel];
    };
  
  nn->output_layer[0] = logistic( nn->output_layer[0] );
  nn->output_layer[1] = logistic( nn->output_layer[1] );
  if (internal_trace || user_trace)
    fprintf (stderr, "Network final outputs: %lf vs %lf\n",
	     nn->output_layer[0], nn->output_layer[1]);
}




#if defined (CRM_WITHOUT_MJT_INLINED_QSORT)

static int compare_hash_vals(const void *a, const void *b)
{
    if (*(crmhash_t *)a < *(crmhash_t *)b)
        return -1;

    if (*(crmhash_t *)a > *(crmhash_t *)b)
        return 1;

    return 0;
}

#else

#define compare_hash_vals(a, b) \
    (*(a) < *(b))

#endif


//     Convert a text into a bag of features, obeying things
//     like the tokenizer regex and the UNIQUE flag.  The result 
//     is a bag of sorted features.  This does _not_ do projection
//     onto the retina, however (see "project_features" for that).
//     Note- in later versions of this code, we no longer project
//     separately.  Instead, we modulo at runtime onto the retina.
//
static int eat_document(ARGPARSE_BLOCK *apb,
        char *text, int text_len, int  *ate,
        regex_t *regee,
        crmhash_t *feature_space, int max_features,
        uint64_t flags, crmhash_t *sum)
{
    crmhash_t /* unsigned int */ hash_pipe[OSB_BAYES_WINDOW_LEN];
#ifdef USE_OLD_TOKENIZER
    static const unsigned int hash_coefs[] =
    {
        1, 3, 5, 11, 23, 47
    };
#endif
	regmatch_t match[1];
    char *t_start;
    int t_len;
    int f;
  int n_features = 0;
  int i, j;

    int unigram, unique, string;
  int next_offset;

    unique = !!(apb->sflags & CRM_UNIQUE); /* convert 64-bit flag to boolean */
    unigram = !!(apb->sflags & CRM_UNIGRAM);
    string = !!(apb->sflags & CRM_STRING);

    if (string)
        unique = 1;

    *sum = 0;
    *ate = 0;

 //#define USE_OLD_TOKENIZER
#ifdef USE_OLD_TOKENIZER
    for (i = 0; i < OSB_BAYES_WINDOW_LEN; i++)
        hash_pipe[i] = 0xdeadbeef;
    while (text_len > 0 && n_features < max_features - 1)
    {
        if (crm_regexec(regee, text, text_len, 1, match, 0, NULL))
        {
            //no match or regex error, we're done
            break;
        }
        else
        {
            t_start = text + match->rm_so;
            t_len = match->rm_eo - match->rm_so;
            if (string)
            {
                text += match->rm_so + 1;
                text_len -= match->rm_so + 1;
                *ate += match->rm_so + 1;
            }
            else
            {
                text += match->rm_eo;
                text_len -= match->rm_eo;
                *ate += match->rm_eo;
            }

            for (i = OSB_BAYES_WINDOW_LEN - 1; i > 0; i--)
                hash_pipe[i] = hash_pipe[i - 1];
            hash_pipe[0] = strnhash(t_start, t_len);
        }
        f = 0;
        if (unigram)
        {
            *sum += feature_space[n_features++] = hash_pipe[0];
        }
        else
        {
            for (i = 1; i < OSB_BAYES_WINDOW_LEN && hash_pipe[i] != 0xdeadbeef; i++)
            {
                *sum += feature_space[n_features++] =
                            hash_pipe[0] + hash_pipe[i] * hash_coefs[i];
            }
        }
    }
#else
  //   Use the flagged vector tokenizer                                        
     
  //    put a zero in as the first element (it'll be our "new document"
  //    sentinel.
  feature_space[0] = 0;
  
  //
  //     GROT GROT GROT note that we _disregard_ the passed-in "regee" (the
  //     previously precompiled regex) and use the automagical code in 
  //     vector_tokenize_selector to get the "right" parse regex from the 
  //     user program's APB.
  crm_vector_tokenize_selector
    (apb,                   // the APB                                         
     text,                   // intput string                                  
     text_len,              // how many bytes                                 
     0,                      // starting offset                                
     NULL,                  // parser regex                                   
     0,                     // parser regex len                               
     NULL,                   // tokenizer coeff array                          
     0,                      // tokenizer pipeline len                         
     0,                      // tokenizer pipeline iterations                  
     &feature_space[1],  // where to put the hashed results
     max_features - 1,        //  max number of hashes                    
     &n_features,             // how many hashes we actually got               
     &next_offset);           // where to start again for more hashes          

  n_features++;    // take into account the previously inserted zero

#endif

  //     GROT GROT GROT
  //     maybe we should force a constant into one column of input
  //     so we can change the threshold of level activation?

  //   Sort the features for speed in matching later - NO NEED 
  //  qsort(feature_space, n_features, sizeof(unsigned int), compare_longs);

  // anything that hashed to 0 or 1 needs to be promoted to 2, because
  //   0 and 1 are our "in-class" and "out-of-class" sentinels.  Also 
  //   calculate the checksum for this document.

  for (i = 0; i < n_features; i++)
    {
      if (feature_space[i] == 0)
	feature_space[i] = 2;
      if (feature_space[i] == 1)
	feature_space[i] = 3;
    }

  //    Do a uniquifying pass.
  if(unique)
    {
      //   Sort the features for uniquifying      
    CRM_ASSERT(sizeof(feature_space[0]) == sizeof(crmhash_t));
    QSORT(crmhash_t, feature_space, n_features, compare_hash_vals);

      //      do "two finger" uniquifying on the sorted result.
      i = 0; 
      for(j = 0; j < n_features; j++)
	if ( feature_space[j] != feature_space [j+1] )
	  feature_space[++i] = feature_space[j];
      n_features = i;
    };

  //  and do the checksum update
  *sum = 0;
  for (i = 0; i < n_features; i++)
    *sum += *sum + feature_space[i];

  //    All done.
    return n_features;
}


static void print_doc_info(NEURAL_NET_STRUCT *nn)
{
    crmhash_t *start = nn->docs_start;
    crmhash_t *end = nn->docs_end;
  int i = 0, out_of_class, good, hash;
    float o[2];
    crmhash_t  *k;

    while (start < end)
    {
        k = start;
    out_of_class = *k++;
        good = *k++;
        hash = *k++;
        while (*k++)
            ;
        do_net(nn, start + 3);
    if (internal_trace)
      {
        fprintf(stderr, "document #%d @ %p is %d int\t(%d,%d,%d)\t(%f,%f)\n",
                i, start, (int)(k - start), out_of_class, good, hash,
		nn->output_layer[0], nn->output_layer[1]);
      };
        start = k;
        i++;
    }
}


//     Nuke the net - basically fill it with small uniformly distributed
//     random weighting coefficients.  This gets the net out of a "stuck
//     on center" condition.  A flag of 0 means "from zero", a flag of 1
//     means "in addition to current state".
//
static void nuke(NEURAL_NET_STRUCT *nn, int flag)
{
  int j;
  float *f;
  float dsn, dsno;

  dsn = NN_DEFAULT_STOCH_NOISE  ;
  dsno = - dsn / 2.0;

  if (internal_trace)
    fprintf (stderr, "********Nuking the net to random values\n");
  
  f = nn->Win;
  for (j = 0; j < nn->retina_size * nn->first_layer_size; j++)
    f[j] = flag*f[j] + dsno + dsn * (float)rand() / (float)RAND_MAX;

  f = nn->Whid;
  for (j = 0; j < nn->first_layer_size * nn-> hidden_layer_size; j++)
    f[j] = flag*f[j] + dsno + dsn * (float)rand() / (float)RAND_MAX;

  f = nn->Wout;
  for (j = 0; j < nn-> hidden_layer_size  * 2; j++)
    f[j] = flag*f[j] + dsno + dsn * (float)rand() / (float)RAND_MAX;
  
}


//   Entry point for learning
//    Basically, we use gradient descent.
//
int crm_neural_net_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    NEURAL_NET_STRUCT NN, *nn = &NN;
    char *filename;
    char htext[MAX_PATTERN];
    int htext_len;
    struct stat statee;

    regex_t regee;
    char regex_text[MAX_PATTERN]; //  the regex pattern
    int regex_text_len;

    crmhash_t /* unsigned int */ bag[NN_MAX_FEATURES];

  crmhash_t sum;

  int  i, j;
  int n_features;
  int old_file_size;
    crmhash_t *new_doc_start, *current_doc, *k, *l;
  int  found_duplicate;
  int n_docs, n_docs_trained, out_of_class, current_doc_len;
  int n_cycles, filesize_on_disk, soft_cycle_limit;

  FILE *f;
  
  float *dWin, *dWhid, *dWout;
  
  float stoch_noise, internal_training_threshold;
  double alpha = NN_DEFAULT_ALPHA;
  double alphalocal;
  
  int this_doc_was_wrong;

  //     in_class_docs and out_class_docs are lookaside lists so
  //     we can alternate training of in-class and out-of-class
  //     examples, so huge batches are not a problem.
  //      GROT GROT GROT this goes in later...  coded in .crm,
  //      it works really well, but not coded into the C yet.
  //
  //  int in_class_docs[MAX_PATTERN], out_class_docs[MAX_PATTERN];
  //  int icd, icd_max, ocd, ocd_max; 

  int neuron, channel;
  
  if(internal_trace)
    internal_trace = 1;
    
  if(internal_trace)
    fprintf(stderr, "entered crm_neural_net_learn\n");
  
  //    Get the filename
    crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    htext_len = apb->p1len;
    htext_len = crm_nexpandvar(htext, htext_len, MAX_PATTERN);

#if 0
    i = 0;
    while (htext[i] < 0x021)
        i++;
    CRM_ASSERT(i < htext_len);
    j = i;
    while (htext[j] >= 0x021)
        j++;
    CRM_ASSERT(j <= htext_len);
#else
 if (!crm_nextword(htext, htext_len, 0, &i, &j) || j == 0)
 {
            int fev = nonfatalerror_ex(SRC_LOC(), 
				"\nYou didn't specify a valid filename: '%.*s'\n", 
					(int)htext_len,
					htext);
            return fev;
 }
 j += i;
    CRM_ASSERT(i < htext_len);
    CRM_ASSERT(j <= htext_len);
#endif

    htext[j] = 0;
    filename = strdup(&htext[i]);

  //     Get the parsing regex  (GROT GROT GROT This becomes obsolete
  //     when the switch to vector tokenization becomes standard.
  //
    crm_get_pgm_arg(regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
    regex_text_len = apb->s1len;
    if (regex_text_len == 0)
    {
        strcpy(regex_text, "[[:graph:]]+");
        regex_text_len = strlen(regex_text);
    }
    regex_text[regex_text_len] = 0;
    regex_text_len = crm_nexpandvar(regex_text, regex_text_len, MAX_PATTERN);
    if (crm_regcomp(&regee, regex_text, regex_text_len, REG_EXTENDED))
    {
        nonfatalerror("Problem compiling the tokenizer regex:", regex_text);
        free(filename);
        return 0;
    }


  //     Set a nominal epoch cycle limiter
  //
  if(apb->sflags & CRM_FROMSTART)    //  FROMSTART reinits the coeffs.
    {
      nuke(nn, 0);
      soft_cycle_limit = NN_MAX_TRAINING_CYCLES_FROMSTART;
    } 
  else
    {
      soft_cycle_limit = NN_MAX_TRAINING_CYCLES;
    };

  //    Get alpha, the stochastic noise level, and the cycle limit from 
  //      the s2 parameter
  //
  {
    char s2_buf[MAX_PATTERN];
    int s2_len;

    alpha = NN_DEFAULT_ALPHA;
    stoch_noise = NN_DEFAULT_STOCH_NOISE;
    internal_training_threshold = NN_INTERNAL_TRAINING_THRESHOLD;

    crm_get_pgm_arg (s2_buf, MAX_PATTERN, apb->s2start, apb->s2len);
    s2_len = apb->s2len;
    s2_len = crm_nexpandvar(s2_buf, s2_len, MAX_PATTERN);
    if (s2_len > 0)
	{
      if (4 != sscanf (s2_buf, "%lf %f %d %f", 
	      &alpha, 
	      &stoch_noise, 
	      &soft_cycle_limit, 
	      &internal_training_threshold))
        {
            nonfatalerror("Failed to decode the 4 Neural Net setup parameters [learn]: ", s2_buf);
        }
    if (alpha < 0.0) alpha = NN_DEFAULT_ALPHA; /* [i_a] moved up here by 3 lines, or it would've been useless */
    if (alpha < 0.000001) alpha = 0.000001;
    if (alpha > 1.0) alpha = 1.0;
    if (stoch_noise > 1.0) stoch_noise = 1.0;
    if (stoch_noise < 0.0) stoch_noise = NN_DEFAULT_STOCH_NOISE;
    if (soft_cycle_limit < 0) 
      soft_cycle_limit = NN_MAX_TRAINING_CYCLES_FROMSTART;
    if (internal_training_threshold < 0.0) internal_training_threshold = NN_INTERNAL_TRAINING_THRESHOLD;
	}

    //    fprintf (stderr, "**Alpha = %lf, noise = %f cycle limit = %d\n", 
    //     alpha, stoch_noise, soft_cycle_limit);
  }

  //    Convert the text form of the document into a bag of features
  //    according to the parsing regex, and compute the checksum "sum".
  //
  n_features = eat_document ( apb, txtptr + txtstart, txtlen, &i,
            &regee, bag, WIDTHOF(bag), apb->sflags, &sum);

  found_duplicate = 0;
  //    Put the document's features into the file by appending to the
  //    end of the file.  Note that we have a header - a 0 for normal
  //    (in-class) learning, and a 1 for "not in class" learning
  //
  //    Does the file exist?
  //  GROT GROT GROT  use map_file to decide file existence!
    if (stat(filename, &statee))
    {
      //   Nope, it doesn't.  Make a new one.
        if (make_new_backing_file(filename))
        {
            free(filename);
            return -1;
        }
        stat(filename, &statee);
      if (internal_trace)
	fprintf (stderr, "Initial file size: %ld\n", (long int)statee.st_size);
        k = NULL;
    }
    else
    {
      //   File exists; map it into memory
        if (map_file(nn, filename))
        {
            //we already threw a nonfatalerror
            free(filename);
            return -1;
        }
      
      //    Scan the file for documents with the matching checksum
      //    so we can selectively forget / refute documents.
      //      (GROT GROT GROT - I hate this method of refutation- it
      //      depends on absolutely identical documents.  On the other
      //      hand, it's how a NN can be told of negative examples, which
      //      back-propagation needs. - wsy.  ).
      found_duplicate = 0;
      for(k = nn->docs_start; 
	  k < nn->docs_end 
	    && (k[0] = 0 || k[0] == 1)  //   Find a start-of-doc sentinel
	    && k[2] != sum;             //  does the checksum match?
	  k++);
	
      if (k[2] == sum)
	found_duplicate = 1;
      //   Did we find a matching sum
      if(found_duplicate)
	{
	  k[1] = 0;
	  if (apb->sflags & CRM_REFUTE)
	    {
	      if (internal_trace)
		fprintf (stderr, "Marking this doc as out-of-class.\n");
	      k[1] = 1;
	    };
	  if (apb->sflags & CRM_ABSENT)
	    {
	      //   erasing something out of the
	      //   input set.   Cut it out of the 
	      //   knowledge base.
	      crmhash_t *dest;      
	      int len;
	      if (internal_trace)
		fprintf (stderr, "Obliterating data...");
	      
	      dest = k;
	      //   Find the next zero sentinel
	      for(k += 3; *k; k++)
			  ;
	      //   and copy from here to end of file over our
	      //    incorrectly attributed data, overwriting it.
	      len = (k - dest) * sizeof(k[0]);
	      if (internal_trace)
		  {
		fprintf (stderr, "start %p length %d\n",
			 dest, len);
		  }
	      for (; k< nn->docs_end; k++)
		{
		  *dest = *k;
		  dest++;
		};
	      //
	      //    now unmap the file, msync it, unmap it, and truncate
	      //    it.  Then remap it back in; viola- the bad data's gone.
	      unmap_file (nn, filename);
	      truncate (filename, statee.st_size - len);
	      map_file (nn, filename);
	    }
	  else
	    {
	      if(internal_trace)
		fprintf(stderr, "***Learning same file twice.  W.T.F. \n");
	    };
	};
        retina_size = nn->retina_size;
    };
  
  if (internal_trace)
    fprintf (stderr, "Dealt with a duplicate at %p\n", 
	     k);
  //   then this data file has never been seen before so append it and remap
  //
  if(! found_duplicate) 
    {                         // adding to the old file
      if (internal_trace) fprintf (stderr, "Appending new data.\n");
      old_file_size = statee.st_size;
      if(internal_trace)
	fprintf(stderr, "NN: new file data will start at offset %d\n", 
		old_file_size);

      // unmap the file so we can write-append to it.
      crm_force_munmap_filename(filename);

      //   make sure that there's something to add.
      if(n_features < 2)
	{
	  if(internal_trace)
	    fprintf(stderr, 
		    "NN: Can't add a null example to the net.\n");
	} 
      else
	{
	  //by now retina_size is known for sure
            FILE *f;

            f = fopen(filename, "ab+");
            if (f == NULL)
            {
                int fev = fatalerror_ex(SRC_LOC(),
                        "\n Couldn't open your new CSS file %s for append; errno=%d(%s)\n",
                        filename,
                        errno,
                        errno_descr(errno));
                free(filename);
                return fev;
            }
            else
            {
                int32_t val;
                int retv;

                //     And make sure the file pointer is at EOF.
                (void)fseek(f, 0, SEEK_END);

                retv = 0;
                if (ftell(f) == 0)
                {
                    CRM_PORTA_HEADER_INFO classifier_info = { 0 };

                    classifier_info.classifier_bits = CRM_NEURAL_NET;

                    if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
                    {
                        nonfatalerror("Couldn't write the header to the .hypsvm file named ",
                                filename);
                        fclose(f);
                        free(filename);
                        return -1;
                    }
                }
	  out_of_class = !!(apb->sflags & CRM_REFUTE);
	  if (out_of_class != 0) out_of_class = 1;   // change bitpat to 1 bit
	  if(internal_trace)
	    {
	      fprintf (stderr, "Sense writing %d\n", out_of_class);
	      if(out_of_class)
		fprintf(stderr, "learning out of class\n");
	      else
		fprintf(stderr, "learning in class\n");
	    }

	  //     Offset 0 is the sentinel - always either 0 or 1
	  //   Offset 0 -  Write 0 or 1 (in-class or out-class)

	  val = out_of_class;
                retv += fwrite(&val, sizeof(val), 1, f);    //  in-class(0) or out-of-class(1)
	  
	  //  Offset 1 - Write the number of passes without a 
	  //  retrain on this one
                i = 0;                                      //number of times we went without training this guy
                retv += fwrite(&val, sizeof(val), 1, f);    // number of times notrain
	  
	  //  Offset 2 - Write the sum of the document feature hashes.
                retv += fwrite(&sum, sizeof(sum), 1, f);    // hash of whole doc- for fastfind
	  
	  
	  //       ALERT ALERT ALERT  Changed below!
	  //    Finally, write the actual mapped retina values
	  //    GROT GROT GROT this is 256 Kbytes per text; we can
	  //    probably do better by going smaller.  
	  // ret = malloc(sizeof(unsigned int) * retina_size);
	  // if(!ret)
	  // {
	  //   fatalerror("Unable to malloc!", "neural net 3");
	  // }
	  // rn = project_features(bag, n, ret, retina_size);
	  // fwrite(ret, rn, sizeof(int), f);
	  // free(ret);
	  //
	  //       ALERT ALERT ALERT
	  //    CHANGED to save the bag, _not_ the projection!
	  //    This means we must project during training, however
	  //    it also means we don't bust the CPU cache nearly as 
	  //    badly!   
                retv += fwrite(bag, sizeof(bag[0]), n_features, f); // the actual data
	  if (internal_trace)
	    fprintf (stderr, 
		 "Appending 3 longs of sentinel and %d longs of features\n",
		     n_features);
                if (retv != 1 + 1 + 1 + n_features)
                {
                    fatalerror("Couldn't write the solution to the .hypsvm file named ",
                            filename);
                    fclose(f);
                    free(filename);
                    return -1;
                }
	  //    Close the file and we've now updated the disk image.

                fclose(f);
            }
        }

        stat(filename, &statee);
      filesize_on_disk = statee.st_size;
      if(internal_trace)
        fprintf(stderr, "NN: statted filesize is now %ld.\n", (long int)statee.st_size);
      
      //   MMap the file back into memory and fix up the nn->blah structs
        if (map_file(nn, filename))
        {
            //nonfatalerror("Couldn't mmap file!", filename);
            free(filename);
            return -1;
        }

      if (internal_trace)
	fprintf (stderr, "Neural network mapped at %p\n", nn);

	  CRM_ASSERT(old_file_size % sizeof(crmhash_t) == 0);
      new_doc_start = 
	(crmhash_t *) ((char *)(nn->file_origin) + old_file_size);

      //    Print out a telltale to see if we're in the right place.
      if (internal_trace)
	{ 
	  fprintf (stderr, "\nAfter mmap, offsets of weight arrays"
		   " are:\n   Win: %p, Whid: %p, Wout: %p\n",
		   nn->Win, nn->Whid, nn->Wout);
	  fprintf (stderr, "First weights (in/hid/out): %lf, %lf, %lf\n", 
		   nn->Win[0], nn->Whid[0], nn->Wout[0]);
	  fprintf (stderr, "Weight ptrs from map start : "
		   "Win: %p, Whid: %p, Wout: %p\n",
		   &nn->Win[0],
	           &nn->Whid[0],
		   &nn->Wout[0]);
	  fprintf (stderr, "Weight ptrs (arefWxxx funcs: "
		   "Win: %p, Whid: %p, Wout: %p\n",
		   arefWin(nn, 0, 0),
		   arefWhid(nn, 0, 0),
		   arefWout(nn, 0, 0));
	};

      //    If we're in APPEND mode, we don't train yet.  So we're
      //    complete at this point and can return to the caller.
      //
      if(apb->sflags & CRM_APPEND)
	{
	  if (internal_trace)
	    fprintf (stderr, 
		   "Append mode- exiting neural_learn without recalc\n");
	  unmap_file (nn, filename);
	  return -1;
	}
    }
    else
    {
        new_doc_start = k;
    }

#ifdef YOU_CAN_UNDERSTAND_THIS_BECAUSE_I_CANT
    n_docs = 0;
    for (k = nn->docs_start; k < nn->docs_end;)
    {
      out_of_class = *k++;
      if( ((apb->sflags & CRM_REFUTE) && out_of_class) 
	  || !( (apb->sflags & CRM_REFUTE) || out_of_class) )
        {
            *k++ += 1;
        }
        else
        {
            k++;
        }
        k++;
        while (*k++)
            ;
        n_docs++;
    }
#else  
  n_docs = 0;
  for(k = nn->docs_start; k < nn->docs_end; k++)
    {
      if (*k < 2)
      n_docs++;
    };
#endif


  n_cycles = 0;
  
  n_docs_trained = 1;
  

  //    Malloc the internal allocations for back propagation.  These
  //    are the deltas used in the "blame game" of back-propagation itself.
  //
  dWin = calloc(nn->retina_size * nn->first_layer_size, sizeof(dWin[0]));
  dWhid = calloc(nn->first_layer_size * nn->hidden_layer_size, sizeof(dWhid[0]));
  dWout = calloc(nn->hidden_layer_size * 2, sizeof(dWout[0]));
  
  //    Check - did we malloc successfully?
  if(!dWin || !dWhid || !dWout)
    {
      if(dWin)
	free(dWin);
      if(dWhid)
	free(dWhid);
      if(dWout)
	free(dWout);
      unmap_file(nn, filename);
      fatalerror("unable to malloc!", "Neural network 3");
      return -1;
    }
  
  //    Now the learning loop itself
  n_docs_trained = 1;
  while(n_cycles < soft_cycle_limit && n_docs_trained > 0)
    {
      float e1, e2, e3, e4;
      if (internal_trace)
	fprintf (stderr, "\n\n**** Start of epoch %d on net at %p\n", 
		 n_cycles, nn);
      if (user_trace)
	fprintf (stderr, "\nE %4f %5d ", 
		 alpha * (1.0 + gain_noise_factor()), 
		 n_cycles);
      n_docs_trained = 0;

      // alpha = alpha * (1.0 - (3.0 / soft_cycle_limit));

      //    Zero out any accumulated delta errors, in the reverse
      //    (back-propagation) order, so we can template this code
      //    again for the actual backprop.
      //
      //       mid-to-out layer deltas first
      for( channel = 0; channel < nn->hidden_layer_size; channel++)
	for (neuron = 0; neuron < 2; neuron++)
	dWout[channel * 2 + neuron] = 0.0;

      //       in-to-mid deltas next
      for( channel = 0; channel < nn->first_layer_size; channel++)
	for(neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	  dWhid[channel * nn->hidden_layer_size + neuron] = 0.0;

      //       Retina-to-in layer deltas last.
      for(channel = 0; channel < nn->retina_size; channel++)
	for(neuron = 0; neuron < nn->first_layer_size; neuron++)
	  dWin[channel * nn->first_layer_size + neuron] = 0.0;
      
      //    Add stochastic noise to the network... this happens
      //    only once per epoch no matter what the protocol...
      //
      if (stoch_noise > 0.0)
	{
	  //   First, noodge the output layer's input-side weights:
	  for (neuron = 0; neuron < 2; neuron++)
	    for(channel = 0; channel < nn->hidden_layer_size; channel++)
	      {
		*arefWout(nn, neuron, channel) +=
		  stochastic_factor (stoch_noise, soft_cycle_limit, n_cycles)
		  * alpha;
	      };
	  
	  //     Then for the hidden layer's input weights
	  for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	    for(channel = 0; channel < nn->first_layer_size; channel++)
	      {
		*arefWhid(nn, neuron, channel) +=
		  stochastic_factor (stoch_noise, soft_cycle_limit, n_cycles)
		  * alpha;
	      };
	  
	  //     Finally, the input layer's input weights
	  for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	    for(channel = 0; channel < nn->retina_size; channel++)
	      {
		*arefWin(nn, neuron, channel) +=
		  stochastic_factor (stoch_noise, soft_cycle_limit, n_cycles)
		  * alpha ;
	      };
	};

      //   Loop around and train each document.
      for(k = nn->docs_start; k < nn->docs_end;)  // the per-document loop
	{
	  //  Start on the 0/1 (in-class/out-of-class) sentinel
	  out_of_class = k[0];      
	  
	  if (internal_trace)
	    {
	      fprintf (stderr, " Doc %p out_of_class %d \n", 
		       k, 
		       out_of_class);
	      fprintf (stderr, 
		       "First weights (in/hid/out): %lf, %lf, %lf\n", 
		       nn->Win[0], nn->Whid[0], nn->Wout[0]);
	    };

	  //  save a pointer to the "OK this many times" counter
	  k++;
	  l = k;
	  
	  //     Skip over the doc checksum
	  k++; //to skip doc hash
	  
	  //      Find end of the current doc 
	  current_doc = k;
	  current_doc_len = 0;
	  while (k < nn->docs_end && *k > 1)
	    {
	      k++;
	      current_doc_len++;
	    };
	  //    k now points to the next document
	  if (internal_trace)
	  {
	    fprintf (stderr, 
		     "Current doc %p next doc start %p next sense %lx\n",
		     current_doc, 
		     k,
		     (unsigned long int) *k);
	  }

     	  //      Run the net on the document
	  do_net(nn, current_doc);
	  if( internal_trace )
	  {
		  fprintf(stderr, "-- doc @ %p got network outputs of %f v. %f\n",
		    current_doc,
		    nn->output_layer[0], nn->output_layer[1]);
	  }

	  //    Now, the test- output channel 0 is "in class", channel
	  //    1 is "not in class".  We see if the output of the
	  //    net is acceptable here, and if not, backprop our errors.
	  // 
	  //     Note that the SRI document says 'Always backprop every
	  //     training sample on every training epoch."  Maybe that's a
	  //     good idea, maybe not.  Maybe it causes overfitting. 
	  //     It's a good question either way.
	  e1 = nn->output_layer[0] - (0.5 + internal_training_threshold);
	  e2 = nn->output_layer[1] - (0.5 - internal_training_threshold);
	  e3 = nn->output_layer[0] - (0.5 - internal_training_threshold);
	  e4 = nn->output_layer[1] - (0.5 + internal_training_threshold);

	  //    Do the error bounds comparison
	  this_doc_was_wrong = 0;
	  if(
	     ( ! out_of_class &&
	       (( e1 < 0 ) || ( e2 > 0 )) 
	       )
	     ||
	     ( out_of_class && 
	       (( e3 > 0 ) || (e4 < 0 ))
	       )
	     )
	    {
	      n_docs_trained ++;
	      this_doc_was_wrong = 1;
	      *l = 0;      // reset the "OK this many times" pointer.
	    };

	if (user_trace)
	  {
	    if (this_doc_was_wrong)
	      {
		if ( ! out_of_class )
		  { 
		    if (nn->output_layer[0] > nn->output_layer[1])
		      fprintf (stderr, "+");
		    else
		      fprintf (stderr, "#");
		  }
		else 
		  {
		    if (nn->output_layer[0] < nn->output_layer[1])
		      fprintf (stderr, "x");
		    else
		      fprintf (stderr, "X");
		  };
	      }
	    else
	      {
		fprintf (stderr, " ");
	      };
	  };
	

	  
//#define TRAIN_ALWAYS 1
#define TRAIN_ALWAYS 0
	  if  (this_doc_was_wrong 
	       || TRAIN_ALWAYS 
	       || (!(apb->sflags & CRM_BYCHUNK) ))
      {    
	//    this block is the "actually train this document" section
	//   If we got here, we're training, either because tne neural 
	//   net got it wrong, or because we're in TRAIN_ALWAYS.  
	//   So, it's time to run the ugly backprop.
	//
	if(internal_trace)
	{
		fprintf(stderr, " Need backprop on doc at %p\n", 
				 current_doc );
	}

		//       Start setting the errors.  Work backwards, from the
	//       known desired states.
	//
	//     Now, for a bit of tricky math.  One could train 
	//     with heuristics (a la the Winnow algorithm) or one could
	//     train with a gradient descent algorithm.  
	//
	//      Luckily, the gradient descent is actually quite simple;
	//      it turns out the partial derivative of the logistic 
	//      function dL(x)/dx is just  (1-L(x)) * L(x)
	//  
	//   See http://www.speech.sri.com/people/anand/771/html/node37.html
	//   for details of this math.  SRI: here means "from the SRI paper"
	//
	//     SRI: del_j = -(target - output)*(1 - output)*(output)
	//
	if (internal_trace)
	  fprintf (stderr, "Generating error deltas for output layer\n");

	//  1.0 and 0.0 are the desired final layer outputs
	nn->delta_output_layer[0] = 1.0;
	nn->delta_output_layer[1] = 0.0;
	if (out_of_class)
	  for (neuron = 0; neuron < 2; neuron++)
	    {
	      nn->delta_output_layer[neuron] =
		1 - nn->delta_output_layer [neuron];
	    };
	
	if (internal_trace)
	  fprintf (stderr, "Output layer desired values: %lf, %lf\n",
		   nn->delta_output_layer[0],
		   nn->delta_output_layer[1]);


	//    Now generate the values "in place" from the desired values
	//    MAYBE TYPO in the SRI document- it says this is 
	//    SRI: -del_k = (t_j - o_j) (1 - o_j) o_j) and it is unclear if
	//    the minus sign should _not_ be there.
#define MISSINGMINUS -
#define SRI_BACKPROP 

	for (neuron = 0; neuron < 2; neuron++)
	  nn->delta_output_layer[neuron] =
	     MISSINGMINUS ( 
		(nn->delta_output_layer[neuron]            //  target - actual
		 - nn->output_layer[neuron] )
		* (1.0 - nn->output_layer[neuron])         // 1 - actual
		* (nn->output_layer[neuron])               // actual
		 );

	if (internal_trace)
	  fprintf (stderr, "Output layer output error delta-sub_j: %lf, %lf\n",
		   nn->delta_output_layer[0],
		   nn->delta_output_layer[1]);
	
	//       Now we calculate the delta weight we want to apply to the
	//       middle layer's inputs, again, this is "in place".
	//
	//    The corrected del_j formula for hidden and input neurons is:
	//     del_j = o_j (1 - o_j) SUM (del_k * w_kj) 
	//     (where the sum is over all neurons downstream of neuron j)
	if (internal_trace)
	  fprintf (stderr, "Doing the hidden layer.\n");
	
	if (internal_trace)
	  fprintf (stderr, "Now doing the sum of the del_k * w_kj\n");
	
	//     Initialize the hidden layer deltas
	for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	  nn->delta_hidden_layer[neuron] = 0;
	
	//    Calculate the SUM del_k * w_kj "in place"
	for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	  for (channel = 0; channel < 2; channel++)
	    nn->delta_hidden_layer[neuron] += 
	      nn->delta_output_layer[channel]
	      * (*arefWout(nn, channel, neuron));
	
	//     Now multiply by the o_j * (1-o_j) term
	for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	  nn->delta_hidden_layer[neuron] =
	    MISSINGMINUS (nn->delta_hidden_layer[neuron] 
	       * nn->hidden_layer[neuron]
	       * (1.0 - nn->hidden_layer[neuron]));
	
	if (internal_trace)
	  fprintf (stderr, "First three hidden delta_subj's: %lf %lf %lf\n",
		   nn->delta_hidden_layer[0],
		   nn->delta_hidden_layer[1],
		   nn->delta_hidden_layer[2]);

	
	//   Finally, do the calculation for the input layer.  Same
	//    deal as before.
	//
	//    The corrected del_j formula for hidden and input neurons is:
	//     del_j = o_j (1 - o_j) SUM (del_k * w_kj) 
	//     (where the sum is over all neurons downstream of neuron j)
	if (internal_trace)
	  fprintf (stderr, "Doing the input layer.\n");
	
	if (internal_trace)
	  fprintf (stderr, "Now doing the sum of the del_k * w_kj\n");
	
	//     Initialize the input layer deltas
	for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	  nn->delta_first_layer[neuron] = 0;
	
	//    Calculate the SUM  "in place" (the error mag term)
	for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	  for (channel = 0; channel < nn->hidden_layer_size; channel++)
	    nn->delta_first_layer[neuron] += 
	      nn->delta_hidden_layer[channel]
	      * (*arefWhid(nn, channel, neuron));  // BACKWARDS!!!
	
	//     Now multiply by the o_j * (1-)_j) term
	for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	  nn->delta_first_layer[neuron] =
	    MISSINGMINUS ( nn->delta_first_layer[neuron] 
		* nn->first_layer[neuron]
		* (1.0 - nn->first_layer[neuron]));
		
	if (internal_trace)
	  fprintf (stderr, "First three input delta_subj's: %lf %lf %lf\n",
		   nn->delta_first_layer[0],
		   nn->delta_first_layer[1],
		   nn->delta_first_layer[2]);
	
	//    The SRI document suggests that the right thing to do 
	//    is to update after each training example is calculated.
	//    So, we'll try that.  :)   RESULT: It works fine for _one_
	//    document, but not multiple documents tend to oscillate
	//    back and forth.  Conclusion: sum up the deltas and error
	//   
	//
	//   We have the del_k's all calculated for all three
	//    layers, and we can use them to calculate the desired
	//     change in weights for each layer.  
	//      These weight changes do not take place immediately;
	//       we do them at the end of a training epoch.

	alphalocal = alpha * (1.0 + gain_noise_factor());

	//#ifdef TRAIN_PER_DOCUMENT
	if (apb->sflags & CRM_BYCHUNK)
	  {

	    //   First, the output layer's input-side weights:
	    for (neuron = 0; neuron < 2; neuron++)
	      for(channel = 0; channel < nn->hidden_layer_size; channel++)
		{
		  *arefWout(nn, neuron, channel) =
		    *arefWout (nn, neuron, channel)
		    + ( alphalocal
			* nn->delta_output_layer[neuron]
			* nn->hidden_layer[channel]);
		};
	    
	    //     Then for the hidden layer's input weights
	    for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	      for(channel = 0; channel < nn->first_layer_size; channel++)
		{
		  *arefWhid(nn, neuron, channel) =
		    *arefWhid (nn, neuron, channel)
		    + ( alphalocal
			* nn->delta_hidden_layer[neuron]
			* nn->first_layer[channel]);
		};
	    
	    //     Finally, the input layer's input weights
	    for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	      for(channel = 0; channel < nn->hidden_layer_size; channel++)
		{
		  *arefWin(nn, neuron, channel) =
		    *arefWin (nn, neuron, channel)
		    + ( alphalocal
			* nn->delta_first_layer[neuron]
			* nn->retina[channel]);
		};
	    //#endif
	  }
	else
	  //#ifdef ACCUMULATE_TRAINING
	  {

	    //   First, the output layer's input-side weights:
	    for (neuron = 0; neuron < 2; neuron++)
	      for(channel = 0; channel < nn->hidden_layer_size; channel++)
		{
		  dWout[channel*2 + neuron] +=
		    ( alphalocal
		      * nn->delta_output_layer[neuron]
		      * nn->hidden_layer[channel]);
		};
	    
	    //     Then for the hidden layer's input weights
	    for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	      for(channel = 0; channel < nn->first_layer_size; channel++)
		{
		  dWhid [channel * nn->hidden_layer_size + neuron] +=
		    ( alphalocal
		      * nn->delta_hidden_layer[neuron]
		      * nn->first_layer[channel]);
		};
	    
	    //     Finally, the input layer's input weights
	    for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	      for(channel = 0; channel < nn->hidden_layer_size; channel++)
		{
		  dWin [ channel * nn->first_layer_size + neuron] +=
		    ( alphalocal
		      * nn->delta_first_layer[neuron]
		      *  nn->retina[channel]);
		}; 
	    //#endif
	  };

      }       // end of train this document
	  else
	    {
	      if(internal_trace)
		fprintf(stderr, "Doc %p / %d OK, looks good, not trained\n",
			current_doc, out_of_class);       
	    };      //      END OF THE PER-DOCUMENT LOOP

	  if (internal_trace)
	    fprintf (stderr, 
		     "Moving to next document at k=%p to docs_end=%p\n",
		     k, nn->docs_end);
	};
      n_cycles++;
      if (internal_trace)
	fprintf (stderr, "All documents processed.\n"
		 "Now first weight coeffs: %lf %lf, %lf\n", 
		 nn->Win[0], nn->Whid[0], nn->Wout[0]);
      
      //    If we're _not_ doing immediate-per-document training...
      if (! (apb->sflags & CRM_BYCHUNK))
	{
	  
	  if (internal_trace)
	    fprintf (stderr, "putting accumulated deltas out.\n");
	  
	  //   First, the output layer's input-side weights:
	  for (neuron = 0; neuron < 2; neuron++)
	    for(channel = 0; channel < nn->hidden_layer_size; channel++)
	      {
		*arefWout(nn, neuron, channel) +=
		  dWout [ channel * 2 + neuron ];
	      };
	  
	  //     Then for the hidden layer's input weights
	  for (neuron = 0; neuron < nn->hidden_layer_size; neuron++)
	    for(channel = 0; channel < nn->first_layer_size; channel++)
	      {
		*arefWhid(nn, neuron, channel) +=
		  dWhid [channel * nn->hidden_layer_size + neuron];
	      };
	  
	  //     Finally, the input layer's input weights
	  for (neuron = 0; neuron < nn->first_layer_size; neuron++)
	    for(channel = 0; channel < nn->retina_size; channel++)
	      {
		*arefWin(nn, neuron, channel) +=
		  dWin[ channel * nn->first_layer_size + neuron];
	      };
	};
      
      if (internal_trace)
	fprintf (stderr, "End of epoch;\n"
		 "after training first weight coeffs: %lf %lf, %lf", 
		 nn->Win[0], nn->Whid[0], nn->Wout[0]);    

      //   Sometimes, it's better to dropkick.
      if (n_cycles % (soft_cycle_limit / NN_FROMSTART_PUNTING ) == 0)
      	{
	  fprintf (stderr, "punt...");
      	  nuke (nn, 0);
      	};

    };        //    End of training epoch loop - back to linear code
  
  
  free(dWin);
  free(dWhid);
  free(dWout);
  
  if(n_cycles >= soft_cycle_limit)
    nonfatalerror( "neural: failed to converge within the training limit.  ",
			 "Beware your results.\n You might want to consider"
		     " a larger network as well.");
  
        if (internal_trace)
            fprintf(stderr, "\nn_cycles = %d\n", n_cycles);

  //    Do we microgroom?  
  //    GROT GROT GROT don't do this yet - this code isn't right.
  //    GROT GROT GROT
  if(apb->sflags & CRM_MICROGROOM && 0)
        {
            int trunced = 0;
            
			for (k = nn->docs_start; k < nn->docs_end;)
            {
                current_doc = k;
                k += 3;
                while (*k++)
                    ;
                if (current_doc[1] > NN_MICROGROOM_THRESHOLD)
                {
                    memmove(current_doc, k, sizeof(current_doc[0]) * (nn->docs_end - k));
                    nn->docs_end -= k - current_doc;
                    k = current_doc;
                    trunced = (nn->docs_end - (crmhash_t *)nn->file_origin);
                    n_docs--;
                }
            }
            if (trunced)
            {
                if (internal_trace)
                {
                    print_doc_info(nn);
                }
                crm_force_munmap_filename(filename);
                truncate(filename, trunced * sizeof(k[0]));
                if (internal_trace)
                {
                    fprintf(stderr, "\nleaving neural net learn after truncating"
                                    ", n_docs = %d\n", n_docs);
                }
                free(filename);
                return 0;
            }
        }
        if (internal_trace)
        {
            print_doc_info(nn);
        }
    

    unmap_file(nn, filename);

    if (internal_trace)
    {
        fprintf(stderr, "\nleaving neural net learn"
                        ", n_docs = %d\n", n_docs);
    }

    free(filename);
    return 0;
}

int crm_neural_net_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    char filenames_field[MAX_PATTERN];
    int filenames_field_len;
    char filenames[MAX_CLASSIFIERS][MAX_FILE_NAME_LEN];

    NEURAL_NET_STRUCT NN, *nn = &NN;

    regex_t regee;
    char regex_text[MAX_PATTERN];     //  the regex pattern
    int regex_text_len;

    crmhash_t bag[NN_MAX_FEATURES], sum;

  int i, j, k, n, n_classifiers, out_pos; 
  int fail_on = MAX_CLASSIFIERS;

    float output[MAX_CLASSIFIERS][2];
    double p[MAX_CLASSIFIERS], pR[MAX_CLASSIFIERS], suc_p, suc_pR, tot;

    char out_var[MAX_PATTERN];
    int out_var_len;


    if (internal_trace)
        fprintf(stderr, "entered crm_neural_net_classify\n");


    //grab filenames field
    crm_get_pgm_arg(filenames_field, MAX_PATTERN, apb->p1start, apb->p1len);
    filenames_field_len = apb->p1len;
    filenames_field_len =
        crm_nexpandvar(filenames_field, filenames_field_len, MAX_PATTERN);

    //grab output variable name
    crm_get_pgm_arg(out_var, MAX_PATTERN, apb->p2start, apb->p2len);
    out_var_len = apb->p2len;
    out_var_len = crm_nexpandvar(out_var, out_var_len, MAX_PATTERN);

  //get tokenizing regex   GROT GROT GROT superceded by
  //   use of the VECTOR TOKENIZER
  //
    crm_get_pgm_arg(regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
    regex_text_len = apb->s1len;
    if (regex_text_len == 0)
    {
        strcpy(regex_text, "[[:graph:]]+");
        regex_text_len = strlen(regex_text);
    }
    regex_text[regex_text_len] = 0;
    regex_text_len = crm_nexpandvar(regex_text, regex_text_len, MAX_PATTERN);
    if (crm_regcomp(&regee, regex_text, regex_text_len, REG_EXTENDED))
    {
        nonfatalerror("Problem compiling this regex:", regex_text);
        return 0;
    }

    // a tiny automata for your troubles to grab the names of our classifier
    // files and figure out what side of the "|" they're on, hey Bill, why isn't
    // this in the stringy stuff?
    for (i = 0, j = 0, k = 0; i < filenames_field_len && j < MAX_CLASSIFIERS; i++)
    {
        if (filenames_field[i] == '\\') //allow escaped in case filename is wierd
        {
            filenames[j][k++] = filenames_field[++i];
        }
        else if (crm_isspace(filenames_field[i]) && k > 0)
        {
            //white space terminates filenames
            filenames[j][k] = 0;
            k = 0;
            j++;
        }
        else if (filenames_field[i] == '|')
        {
            //found the bar, terminate filename if we're in one
            if (k > 0)
            {
                k = 0;
                j++;
            }
            fail_on = j;
        }
        else if (crm_isgraph(filenames_field[i]))
        {
            //just copy char otherwise
            filenames[j][k++] = filenames_field[i];
        }
    }

    if (j < MAX_CLASSIFIERS)
        filenames[j][k] = 0;
    if (k > 0)
        n_classifiers = j + 1;
    else
        n_classifiers = j;

    if (fail_on > n_classifiers)
        fail_on = n_classifiers;

    if (internal_trace)
    {
        fprintf(stderr, "Diagnostic: fail_on = %d n_classifiers = %d\n",
                fail_on, n_classifiers);
        for (i = 0; i < n_classifiers; i++)
            fprintf(stderr, "filenames[%d] = >%s<\n", i, filenames[i]);
    }

    n = eat_document(apb, txtptr + txtstart, txtlen, &j,
            &regee, bag, WIDTHOF(bag), apb->sflags, &sum);

    if (internal_trace)
    {
        fprintf(stderr,
                "files: %d, Filename0: >%s<  Filename1: >%s<  Filename2: >%s<\n",
                n_classifiers, filenames[0], filenames[1], filenames[2]);
    }

    //loop over classifiers and calc scores
    for (i = 0; i < n_classifiers; i++)
    {
        if (internal_trace)
        {
            fprintf(stderr,  "Now running filenames[%d] = >%s<\n",
                    i, filenames[i]);
        }
        if (map_file(nn, filenames[i]))
        {
            nonfatalerror("Couldn't mmap file!", filenames[i]);
          output[i][0] = 0.5;
          output[i][1] = 0.5;
            continue;
        }
      //   ***  now we do projection in real time because it's smaller!
      //     this allows different sized nn's to be compared.
      // ret = malloc( sizeof(unsigned int) * nn->retina_size);
      // rn = project_features(bag, n, ret, nn->retina_size);
      do_net(nn, bag);
      output[i][0] = nn->output_layer[0];
      output[i][1] = nn->output_layer[1];
      // free(ret);
        unmap_file(nn, filenames[i]);
        if (internal_trace)
        {
	fprintf(stderr, "Network outputs on file %s:\t%f\t%f\n", 
                    filenames[i], output[i][0], output[i][1]);
        }
    }
  
  //    Calculate the winners and losers.  
  
  //    Get total activation output for all of the networks first:
    tot = 0.0;
    for (i = 0; i < n_classifiers; i++)
    {
      //    Normalize the classifiers to a [0...1] range  
        tot += p[i] = 0.5 * (1.0 + output[i][0] - output[i][1]);
    }
  
  if(internal_trace)
    fprintf(stderr, "tot = %f\n", tot);
  
    for (i = 0; i < n_classifiers; i++)
    {
      //     response fraction of this class.
        p[i] /= tot;
      if(internal_trace)
	fprintf(stderr, "p[%d] = %f (normalized)\n", i, p[i]);
    }
  
  //      Find the one "best matching" network
    j = 0;
    for (i = 1; i < n_classifiers; i++)
    {
        if (p[i] > p[j])
            j = i;
    }
  
  //      Find the overall winning side - success, or fail
    suc_p = 0.0;
    for (i = 0; i < fail_on; i++)
    {
        suc_p += p[i];
    }
  
  //      Calculate pR's for all of the classifiers.
    for (i = 0; i < n_classifiers; i++)
    {
        pR[i] = get_pR(p[i]);
    }
    suc_pR = get_pR(suc_p);
    out_pos = 0;

        //test for nan as well
    if (suc_p > 0.5)
    {
        out_pos += sprintf(outbuf + out_pos,
                "CLASSIFY succeeds; success probability: %f  pR: %6.4f\n",
                suc_p, suc_pR);
    }
    else
    {
        out_pos += sprintf(outbuf + out_pos,
                "CLASSIFY fails; success probability: %f  pR: %6.4f\n",
                suc_p, suc_pR);
    }

    out_pos += sprintf(outbuf + out_pos,
            "Best match to file #%d (%s) prob: %6.4f  pR: %6.4f  \n",
            j,
            filenames[j],
            p[j], pR[j]);

    out_pos += sprintf(outbuf + out_pos,
            "Total features in input file: %d\n",
            n);

    for (i = 0; i < n_classifiers; i++)
    {
        out_pos += sprintf(outbuf + out_pos,
                "#%d (%s): prob: %3.2e, pR: %6.2f\n",
                i, filenames[i], p[i], pR[i]);
    }

    if (out_var_len)
        crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_pos);

    if (suc_p <= 0.5)
    {
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
    }
    return 0;
}

#else /* CRM_WITHOUT_NEURAL_NET */

int crm_neural_net_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}

#endif /* CRM_WITHOUT_NEURAL_NET */




int crm_neural_net_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}


int crm_neural_net_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "Neural Net");
}

