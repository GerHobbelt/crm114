//  crm_neural_net.c

//  by Joe Langeway derived from crm_osb_hyperspace.c and produced for the
//  crm114 so:
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact Bill for details.
//
/////////////////////////////////////////////////////////////////////
//
//     crm_neural_net.c - a nueral net classifier
//
//     Original spec by Bill Yerazunis, original code by Joe Langeway,
//     recode for CRM114 use by Bill Yerazunis.
//
//     This code section (crm_neural_net and subsidiary routines) is
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
 * tendancy towards long hours.
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

typedef struct mythical_neural_net_header
{
  int32_t retina_size;
  int32_t first_layer_size;
  int32_t hidden_layer_size;
} NEURAL_NET_HEAD_STRUCT;

typedef struct mythical_neural_net
{
  float        **Win;
  float        **Whid;
  float        **Wout;
  float         *first_layer;
  float         *hidden_layer;
  float          output_layer[2];
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

static double alpha = 0.02;
//implement these!
static long retina_size = NN_RETINA_SIZE;
static long first_layer_size = NN_FIRST_LAYER_SIZE;
static long hidden_layer_size = NN_HIDDEN_LAYER_SIZE;


//      Create a new neural net statistics file from nothing...
//
//
static int make_new_backing_file(char *filename)
{
  long i;
  FILE *f;
  NEURAL_NET_HEAD_STRUCT h;

  f = fopen(filename, "wb");
  if (!f)
  {
    nonfatalerror("unable to create neural network backing file", "filename");
    return -1;
  }
  h.retina_size = retina_size;
  h.first_layer_size = first_layer_size;
  h.hidden_layer_size = hidden_layer_size;
  fwrite(&h, 1, sizeof(NEURAL_NET_HEAD_STRUCT), f);
  for (i = sizeof(NEURAL_NET_HEAD_STRUCT); i < HEADER_SIZE; i++)
    fputc('\0', f);

  //   the actual size required for the neural network coeffs.  Note that
  //   this leaves ZERO padding between the coeffs and the "stored texts".
  i = retina_size * first_layer_size + first_layer_size * hidden_layer_size +
      hidden_layer_size * 2;
  while (i--)
  {
    float a = ((0.2 * rand()) / RAND_MAX) - 0.1;
    fwrite(&a, 1, sizeof(a), f);
  }
  fclose(f);
  return 0;
}

static int map_file(NEURAL_NET_STRUCT *nn, char *filename)
{
  long i;
  NEURAL_NET_HEAD_STRUCT *h;
  struct stat statee;
  float *w;

  if (stat(filename, &statee))
  {
    nonfatalerror("unable to map neural network backing file", "filename");
    return -1;
  }
  nn->file_origin = crm_mmap_file
                    (filename,
                     0, statee.st_size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     NULL);
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
  //      when you just create a long vector and use subscript arithmetic)
  //     Note also that most of these (the ones "malloced" below) are
  //     transient and are NOT part of the disk image.
  //
  //  aim the h-struct at the mem-mapped file
  h = nn->file_origin;
  //   copy the important sizes
  nn->retina_size = h->retina_size;
  nn->first_layer_size = h->first_layer_size;
  nn->hidden_layer_size = h->hidden_layer_size;
  //  and make space for Win, Whid, and Wout
  //   GROT GROT GROT why?  Why not use the file versions?
  nn->Win = calloc(h->retina_size, sizeof(nn->Win[0]));
  nn->Whid = calloc(h->first_layer_size, sizeof(nn->Whid[0]));
  nn->Wout = calloc(h->hidden_layer_size, sizeof(nn->Wout[0]));
  //   and the activation values (these aren't needed to be kept.
  nn->first_layer = calloc(h->first_layer_size, sizeof(nn->first_layer[0]));
  nn->hidden_layer = calloc(h->hidden_layer_size, sizeof(nn->hidden_layer[0]));
  nn->delta_first_layer = calloc(h->first_layer_size, sizeof(nn->delta_first_layer[0]));
  nn->delta_hidden_layer = calloc(h->hidden_layer_size, sizeof(nn->delta_hidden_layer[0]));

  if (!nn->Win || !nn->Whid || !nn->Wout)
  {
    nonfatalerror("unable to malloc, tell Joe to make the NN use macros"
                  " instead of caching easily computable addresses",
                  "filename");
    return -1;
  }

  //    Set up some aux indexes.
  //
  w = (float *)((char *)h + HEADER_SIZE);

  //     fill in the pointers for Win[i] so that C will think it's
  //     really a two-dimensional array.
  for (i = 0; i < h->retina_size; i++)
  {
    nn->Win[i] = w + i * h->first_layer_size;
  }

  //     fill in the pointers for Whid[i] the same way- now C thinks
  //     it's a 2-D array
  for (i = 0; i < h->first_layer_size; i++)
  {
    nn->Whid[i] = w + h->retina_size * h->first_layer_size
                  + i * h->hidden_layer_size;
  }

  //      and the same pointers for the output layer.  Yes, this gets
  //      hairy because this is now the third thing in the array
  for (i = 0; i < h->hidden_layer_size; i++)
  {
    nn->Wout[i] = w + h->retina_size * h->first_layer_size
                  + h->first_layer_size * h->hidden_layer_size
                  + i * 2;
  }

  //      This is where the saved documents start (not the actual text, but
  //      rather the sorted bags of retina projections.  For the default case,
  //      with a 64K channel retina, this is the hashes mod 64K.  For other
  //      configurations of the network, this will be something different.  In
  //      all cases though the individual projections are Unsigned Longs.
  nn->docs_start = (crmhash_t *)w
                   + h->retina_size * h->first_layer_size
                   + h->first_layer_size * h->hidden_layer_size
                   + h->hidden_layer_size * 2;
  nn->docs_end = (crmhash_t *)((char *)h + statee.st_size);
  return 0;
}

static void unmap_file(NEURAL_NET_STRUCT *nn, char *filename)
{
  free(nn->Win);
  free(nn->Whid);
  free(nn->Wout);
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
  return 1.0 / (1.0 + exp(-a) + 0.000001);
}

//  GROT GROT GROT this is piecewise linear; we may want to make this
//  a smoother one later... but that's a hard problem and not particularly
//  pressing issue right now.
static double get_pR(double p)
{
  double a = fabs(p - 0.5);

  if (a < 0.1)
    a *= 10.0;
  else if (a < 0.2)
    a = 1.0 + 90.0 * (a - 0.1);
  else if (a < 0.3)
    a = 10.0 + 900.0 * (a - 0.2);
  else
    a = 100.0 + 1000.0 * (a - 0.3);
  return p >= 0.5 ? a : -a;
}


// if any bag[i] > nn->retina_size you're doomed
//
static void do_net
(NEURAL_NET_STRUCT *nn, crmhash_t *bag, long n, float *output_layer)
{
  long i, j, k, absent_shift = nn->retina_size / 2;

  for (i = 0; i < nn->first_layer_size; i++)
    nn->first_layer[i] = 0.0;
  for (i = 0; i < nn->hidden_layer_size; i++)
    nn->hidden_layer[i] = 0.0;
  nn->output_layer[0] = nn->output_layer[1] = 0.0;
  k = 0;
  if (NN_SPARSE_RETINA)
  {
    for (i = 0; i < absent_shift; i++)
    {
      if (bag[k] == i)
      {
        for (j = 0; j < nn->first_layer_size; j++)
          nn->first_layer[j] += nn->Win[i][j];
        k++;
      }
      else
      {
        for (j = 0; j < nn->first_layer_size; j++)
        {
          nn->first_layer[j] += nn->Win[i + absent_shift][j];
        }
      }
    }
  }
  else
  {
    for (i = 0; i < n; i++)
    {
      for (j = 0; j < nn->first_layer_size; j++)
      {
        nn->first_layer[j] += nn->Win[bag[i]][j];
      }
    }
  }
  for (i = 0; i < nn->first_layer_size; i++)
  {
    nn->first_layer[i] = logistic(nn->first_layer[i]);
    for (j = 0; j < nn->hidden_layer_size; j++)
    {
      nn->hidden_layer[j] += nn->Whid[i][j] * nn->first_layer[i];
    }
  }
  for (i = 0; i < nn->hidden_layer_size; i++)
  {
    nn->hidden_layer[i] = logistic(nn->hidden_layer[i]);
    nn->output_layer[0] += nn->Wout[i][0] * nn->hidden_layer[i];
    nn->output_layer[1] += nn->Wout[i][1] * nn->hidden_layer[i];
  }
  output_layer[0] = logistic(nn->output_layer[0]);
  output_layer[1] = logistic(nn->output_layer[1]);
  if (0 && internal_trace)
  {
    fprintf(stderr, "ran do_net on doc at %p and got (%f, %f)\n", bag,
            output_layer[0], output_layer[1]);
  }
}


static int do_net_learn_back_prop_with_noise(NEURAL_NET_STRUCT *nn,
                                             crmhash_t  *bag, long n,
                                             long in_class)
{
  long i, j, k, absent_shift;
  double scoro;

  //  if we use half the retina nodes as inversions of the others than we need to
  //  know how much half is, and we don't go above this for "normal" (not
  //  inverted) channels.
  absent_shift = nn->retina_size / 2;

  //      Zero out the first layer activations
  for (i = 0; i < nn->first_layer_size; i++)
    nn->first_layer[i] = 0.0;

  //      Zero out the middle layer activations
  for (i = 0; i < nn->hidden_layer_size; i++)
    nn->hidden_layer[i] = 0.0;

  //       Zero out the output layer activations
  nn->output_layer[0] = 0.0;
  nn->output_layer[1] = 0.0;

  //  if we use sparse retina stuff it's so we can use half the retina
  //  nodes as inversions of the others
  k = 0;
  if (NN_SPARSE_RETINA)
  {
    //loop through every possible feature and see if it is high or low, set
    //the inversion node high if the primary is low
    for (i = 0; i < absent_shift; i++)
    {
      //the bag is sorted, so it's this one if it's there at all
      if (bag[k] == i)
      {
        //     propagate input signals to first layer
        for (j = 0; j < nn->first_layer_size; j++)
        {
          while (bag[k++] == i)
          {
            nn->first_layer[j] += nn->Win[i][j];
          }
        }
      }
      else
      {
        //     propagate signal to first layer
        for (j = 0; j < nn->first_layer_size; j++)
        {
          nn->first_layer[j] += nn->Win[i + absent_shift][j];
        }
      }
    }
  }
  else
  {
    //  just take the input as bag of activations and propagate to
    //  the first layer activation values.
    for (i = 0; i < n; i++)
    {
      for (j = 0; j < nn->first_layer_size; j++)
      {
        nn->first_layer[j] += nn->Win[bag[i]][j];
      }
    }
  }

  //    Turn activations in first layer into sigmoid of activations
  for (i = 0; i < nn->first_layer_size; i++)
  {
    nn->first_layer[i] = logistic(nn->first_layer[i]);
  }

  //    Calculate activations in the second (hidden) layer
  for (i = 0; i < nn->first_layer_size; i++)
  {
    for (j = 0; j < nn->hidden_layer_size; j++)
    {
      nn->hidden_layer[j] += nn->Whid[i][j] * nn->first_layer[i];
    }
  }

  //   Turn hidden layer activation sums into sigmoid of hidden activations
  for (i = 0; i < nn->hidden_layer_size; i++)
  {
    nn->hidden_layer[i] = logistic(nn->hidden_layer[i]);
  }

  //    Calculate final (output) layer activation sums
  for (i = 0; i < nn->hidden_layer_size; i++)
  {
    nn->output_layer[0] += nn->Wout[i][0] * nn->hidden_layer[i];
    nn->output_layer[1] += nn->Wout[i][1] * nn->hidden_layer[i];
  }

  //     Finally logistic-ize the output activation sums
  nn->output_layer[0] = logistic(nn->output_layer[0]);
  nn->output_layer[1] = logistic(nn->output_layer[1]);

  //     And create a final score.   nn->output_layer[0] is the "in-class"
  //     signal; nn->output_layer[1] is the "out of class" signal, so just
  //     subtracting them would yield a -1...+1 range.  This rescales it to
  //     0.0 ... +1.0 range.
  scoro = 0.5 * (1.0 + nn->output_layer[0] - nn->output_layer[1]);

  if (user_trace && internal_trace)
  {
    fprintf(stderr, "learning doc at %p and scored %f\t%f\t%f\n", bag,
            scoro, nn->output_layer[0], nn->output_layer[1]);
  }


  //  Calculate delta on the output delta layer.  This is the amount of total
  //   output error that the particular neuron was responsible for.
  //
  //  This isn't just the delta per se, this is also the derivative of the
  //  sigmoid output function.  This is gradient descent learning, on the
  //  weights used by the output layer to weight-off the middle layer.

  if (scoro < 0.5 + NN_INTERNAL_TRAINING_THRESHOLD && in_class)
  {
    nn->delta_output_layer[0] =
      (1.0 - nn->output_layer[0])       // this term is the target minus actual
                                        // value we got (i.e. actual error)
      * nn->output_layer[0] * (1.0 - nn->output_layer[0]);       // This term is
    // the derivative of the sigmoid function
    // at the actual value (i.e. the slope
    // of the sigmoid, but computed from the
    // output of the sigmoid, not the input!
    //  ... dy/dx given y, not dy/dx given x)
    //
    //  We multiply these together in a bizarre
    //  (mis)application of Newton's method
    //  called the delta rule to find out how
    //  much we really need to tweak the input
    //  weights of this layer of the net to
    //  get the desired result,
    //
    //  same thing here... and below three times as well for the
    //    prior layers
    nn->delta_output_layer[1] = (0.0 - nn->output_layer[1]) *
                                nn->output_layer[1] * (1.0 - nn->output_layer[1]);
  }
  else if (scoro > 0.5 - NN_INTERNAL_TRAINING_THRESHOLD && !in_class)
  {
    //  Repeat of the above, but "out of class".  All the math is the same,
    //  except that the target values are 0 and 1 instead of 1 and 0.
    //   Note that the derivative term doesn't change; that's why this
    //   code looks asymmetric.
    nn->delta_output_layer[0] = (0.0 - nn->output_layer[0]) *
                                nn->output_layer[0] * (1.0 - nn->output_layer[0]);
    nn->delta_output_layer[1] = (1.0 - nn->output_layer[1]) *
                                nn->output_layer[1] * (1.0 - nn->output_layer[1]);
  }
  else     //   Else we're good on this document, no need to change weights
  {
    if (0 && internal_trace)
    {
      fprintf(stderr, "not learning doc at %p which scored %f\t%f\t%f\n",
              bag, scoro, nn->output_layer[0], nn->output_layer[1]);
    }
    return 0;
  }

  //     Note that to use crm_frand as a multiplier, we should double
  //      the value of alpha from 0.01 to 0.02 .  This alpha value is an
  //       empirical value; however stochastic learning has very different
  //        behavior than straight constant-value gradient descent.  Feel free
  //         to experiment if you're bored.
  for (i = 0; i < nn->hidden_layer_size; i++)
  {
    nn->Wout[i][0] +=
      crm_frand() * alpha * nn->delta_output_layer[0] * nn->hidden_layer[i];
    nn->Wout[i][1] +=
      crm_frand() * alpha * nn->delta_output_layer[1] * nn->hidden_layer[i];
  }

  //      Now we use the above crazy looking delta rule on the hidden layer.
  //       Because we don't have actual target values, we have to split the
  //        "target" value into the part coming from Wout[i][0] and the part
  //          coming Wout[i][1] and assume that those are representative.
  for (i = 0; i < nn->hidden_layer_size; i++)
  {
    nn->delta_hidden_layer[i] = 0.0;
    //  what part of the error is from output 0 (in-class)...
    nn->delta_hidden_layer[i] += nn->delta_output_layer[0] * nn->Wout[i][0];
    //   what part of the error is from output 1 (out-of-class)...
    nn->delta_hidden_layer[i] += nn->delta_output_layer[1] * nn->Wout[i][1];
    //    .... times the dy/dx given y at this point on the sigmoid gives
    //     us the +/- blame for this neuron
    nn->delta_hidden_layer[i] *=
      nn->hidden_layer[i] * (1.0 - nn->hidden_layer[i]);
  }

  //     now run through the hidden layer neurons and stochastically apply
  //      the blame correction factor on the weights used by the middle
  //       layer to weight-out the outputs of the first layer, so we multiply
  //        those deltas by the actual outputs of the first layer.
  for (i = 0; i < nn->first_layer_size; i++)
  {
    for (j = 0; j < nn->hidden_layer_size; j++)
    {
      nn->Whid[i][j] +=
        crm_frand() * alpha * nn->delta_hidden_layer[j] * nn->first_layer[i];
    }
  }

  //     And once more again, for the first layer, calculate the deltas desired
  //     to get the results we expect for the first layer.
  for (i = 0; i < nn->first_layer_size; i++)
  {
    nn->delta_first_layer[i] = 0.0;
    for (j = 0; j < nn->hidden_layer_size; j++)
    {
      nn->delta_first_layer[i] += nn->delta_hidden_layer[j] * nn->Whid[i][j];
    }
    nn->delta_first_layer[i] *= nn->first_layer[i] * (1.0 - nn->first_layer[i]);
  }

  //    and once more, apply those deltas to the first layer's weights
  //     as it sees the input retina channels.
  if (NN_SPARSE_RETINA)
  {
    for (i = 0; i < absent_shift; i++)
    {
      if (bag[k] == i)
      {
        for (j = 0; j < nn->first_layer_size; j++)
        {
          while (bag[k++] == i)
          {
            nn->Win[i][j] += crm_frand() * alpha * nn->delta_first_layer[j];
          }
        }
      }
      else
      {
        for (j = 0; j < nn->first_layer_size; j++)
        {
          nn->Win[i + absent_shift][j] +=
            crm_frand() * alpha * nn->delta_first_layer[j];
        }
      }
    }
  }
  else
    for (i = 0; i < n; i++)
      for (j = 0; j < nn->first_layer_size; j++)
        nn->Win[bag[i]][j] += crm_frand() * alpha * nn->delta_first_layer[j];

  if (internal_trace)
  {
    fputc(in_class ? 'x' : 'o', stderr);
  }
  return 1;
}

//       Nuke the net from orbit.  It's the only way to be sure.
//            GROT GROT GROT GROT
//        Note to the reader: if you DO NOT use stochastic updates, then
//         +X and -X as weights is a really _bad_ idea, because it means
//          that all of the weights that started the same way in a particular
//           neuron always stay the same way!  Stochastic updates fixes this,
//            so if you don't use stochastic updates FIX THIS TO GIVE RANDOMS
static void nuke(NEURAL_NET_STRUCT *nn)
{
  long j;
  float *f;

  j = nn->retina_size * nn->first_layer_size
      + nn->first_layer_size * nn->hidden_layer_size
      + nn->hidden_layer_size * 2;
  f = nn->Win[0];
  //   use a high-order bit of rand to get reasonable randomicity
  while (j--)
  {
    *f++ = rand() & 16384 ? 0.1 : -0.1;             // use crm_frand instead here if you
    // don't want to use stochastic
    // learning.
  }
}


static int compare_hash_vals(const void *a, const void *b)
{
  if (*(crmhash_t *)a < *(crmhash_t *)b)
    return -1;

  if (*(crmhash_t *)a > *(crmhash_t *)b)
    return 1;

  return 0;
}

//         GROT GROT GROT
//      This is the feature extractor.  In goes text, out comes a
//      sorted bag of hashes (not projected yet).  This whole thing
//      can and should be replaced by the vector tokenizer soon.
//         GROT GROT GROT
//
static int eat_document(ARGPARSE_BLOCK *apb,
                        char *text, long text_len, long *ate,
                        regex_t *regee,
                        crmhash_t *feature_space, long max_features,
                        uint64_t flags, crmhash_t *sum)
{
  long n_features = 0, i, j;
  crmhash_t /* unsigned long */ hash_pipe[OSB_BAYES_WINDOW_LEN];
  unsigned long hash_coefs[] =
  {
    1, 3, 5, 11, 23, 47
  };
  regmatch_t match[1];
  char *t_start;
  long t_len;
  long f;
  int unigram, unique, string;

  unique = !!(apb->sflags & CRM_UNIQUE);   /* convert 64-bit flag to boolean */
  unigram = !!(apb->sflags & CRM_UNIGRAM);
  string = !!(apb->sflags & CRM_STRING);

  if (string)
    unique = 1;

  *sum = 0;
  *ate = 0;

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
  // We sort the features so we get sequential memory access hereafter.
  qsort(feature_space, n_features, sizeof(feature_space[0]), compare_hash_vals);

  for (i = 0; i < n_features && feature_space[i] == 0; i++)
    feature_space[i] = 1;

  if (unique)
  {
    i = 0;
    j = 0;
    for (j = 0; j < n_features; j++)
    {
      if (feature_space[i] != feature_space[j])
      {
        feature_space[++i] = feature_space[j];
      }
    }
    feature_space[++i] = 0;
    n_features = i + 1;     //the zero counts
  }
  else
  {
    feature_space[n_features++] = 0;
  }
  return n_features;
}


//       Given a bag of features, project them onto the retina, yielding
//      a bag of retina excitations.  Note that this is is very close to
//     but NOT exactly modulo, as if you do sparse retinas or multipumps,
//    that all changes.
int project_features(crmhash_t *feat, int nf, crmhash_t *ret, int max)
{
  int i, j, k;
  crmhash_t a;
  int soft_max;

  j = 0;
  soft_max = NN_SPARSE_RETINA ? max / 2 : max;
  for (i = 0; i < nf && j < max; i++)
  {
    a = feat[i];
    ret[j++] = a % soft_max;
    a /= max;
    for (k = 1; k < NN_N_PUMPS && a && j < max; k++)
    {
      ret[j++] = a % soft_max;
      a /= soft_max;
    }
  }
  qsort(ret, j, sizeof(ret[0]), compare_hash_vals);
  for (k = 0; k < j && ret[k] == 0; k++)
    ret[k] = 1;
  ret[j++] = 0;
  return j;
}

static void print_doc_info(NEURAL_NET_STRUCT *nn)
{
  crmhash_t *start = nn->docs_start;
  crmhash_t *end = nn->docs_end;
  int i = 0, in_class, good, hash;
  float o[2];
  crmhash_t  *k;

  while (start < end)
  {
    k = start;
    in_class = *k++;
    good = *k++;
    hash = *k++;
    while (*k++)
      ;
    do_net(nn, start + 3, k - start - 3, o);
    fprintf(stderr, "document #%d @ %p is %d long\t(%d,%d,%d)\t(%f,%f)\n",
            i, start, (int)(k - start), in_class, good, hash, o[0], o[1]);
    start = k;
    i++;
  }
}


//entry point for learning
int crm_neural_net_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                         char *txtptr, long txtstart, long txtlen)
{
  NEURAL_NET_STRUCT NN, *nn = &NN;
  char *filename;
  char htext[MAX_PATTERN];
  long htext_len;
  struct stat statee;
  long append_only;

  regex_t regee;
  char regex_text[MAX_PATTERN];   //  the regex pattern
  long regex_text_len;

  crmhash_t /* unsigned long */ bag[32768], *ret, sum;

  long i, j, n, rn;
  crmhash_t *new_doc_start, *current_doc, *k, *l;
  long n_docs, n_docs_without_training, in_class, current_doc_len;
  long n_cycles, filesize_on_disk, soft_cycle_limit;

  //   regmatch_t matchee[2];
  //  char param_text[MAX_PATTERN];
  // long param_text_len;

  if (internal_trace)
    fprintf(stderr, "entered crm_neural_net_learn\n");

  //        Append-only mode means don't actually run the long
  //        training, because more examples are soon to arrive.
  append_only = 0;
  if (apb->sflags & CRM_APPEND)
  {
    if (user_trace)
      fprintf(stderr, "Append-only mode activated\n");
    append_only = 1;
  }

  //     Set reasonable sizes for the various parts of the NN, assuming
  //     that the sparse_spectrum_file_length is vaguely reasonable.
  //
  if (sparse_spectrum_file_length)
  {
    retina_size = 1024 * (int)sqrt(sparse_spectrum_file_length / 1024);
    first_layer_size = (int)sqrt(sparse_spectrum_file_length / 1024);
    hidden_layer_size = first_layer_size;
  }

  //parse out backing file name
  crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
  htext_len = apb->p1len;
  htext_len = crm_nexpandvar(htext, htext_len, MAX_PATTERN);

  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;
  htext[j] = 0;
  filename = strdup(&htext[i]);

  crm_get_pgm_arg(regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
  regex_text_len = apb->s1len;
  if (regex_text_len == 0)
  {
    strcpy(regex_text, "[[:graph:]]+");
    regex_text_len = strlen(regex_text);
  }
  regex_text[regex_text_len] = '\0';
  regex_text_len = crm_nexpandvar(regex_text, regex_text_len, MAX_PATTERN);
  if (crm_regcomp(&regee, regex_text, regex_text_len, REG_EXTENDED))
  {
    nonfatalerror("Problem compiling the tokenizer regex:", regex_text);
    free(filename);
    return 0;
  }
  n = eat_document(apb, txtptr + txtstart, txtlen, &i,
                   &regee, bag, WIDTHOF(bag), apb->sflags, &sum);

  //    Now we've got features, save them in the backing file.
  //
  if (stat(filename, &statee))
  {
    make_new_backing_file(filename);
    stat(filename, &statee);
    k = NULL;
  }
  else
  {
    if (map_file(nn, filename))
    {
      //we already threw a nonfatalerror
      free(filename);
      return -1;
    }
    for (k = nn->docs_start; k < nn->docs_end && k[2] != sum; k++)
    {
      for (k += 3; *k; k++)
        ;
    }
    if (k < nn->docs_end)
    {
      k[2] = !(apb->sflags & CRM_REFUTE);
#if 0 /* [i_a] unused code */
      j = (char *)k - (char *)nn;
#endif
      if (internal_trace)
        fprintf(stderr, "learning same file twice\n");
    }
    else
    {
      k = NULL;
    }
    retina_size = nn->retina_size;
  }

  //    By now retina_size is known for sure, so we can project the
  //    bagged features onto the retina.
  //
  ret = calloc(retina_size, sizeof(ret[0]));
  rn = project_features(bag, n, ret, retina_size);


  if (k == NULL)   //then this files never been seen before so append it and remap
  {
    j = statee.st_size;
    if (internal_trace)
      fprintf(stderr, "NN: new file will start at offset %ld\n", j);
    crm_force_munmap_filename(filename);
    if (n < 2)
    {
      if (internal_trace)
      {
        fprintf(stderr,
                "NN: not going to train the network on a null string thankyou.\n");
      }
    }
    else
    {
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

        val = !(apb->sflags & CRM_REFUTE);
        if (internal_trace)
        {
          if (val)
            fprintf(stderr, "learning in class\n");
          else
            fprintf(stderr, "learning out of class\n");
        }
        fwrite(&val, 1, sizeof(val), f);           //  in-class(0) or out-of-class(1)
        i = 0;                                     //number of times we went without training this guy
        fwrite(&val, 1, sizeof(val), f);           // number of times notrain
        fwrite(&sum, 1, sizeof(sum), f);           // hash of whole doc- for fastfind
        fwrite(ret, rn, sizeof(ret[0]), f);        // the actual data

        fclose(f);
      }
    }

    stat(filename, &statee);
    filesize_on_disk = statee.st_size;
    if (internal_trace)
      fprintf(stderr, "NN: filesize is now %ld.\n", statee.st_size);

    if (map_file(nn, filename))
    {
      //nonfatalerror("Couldn't mmap file!", filename);
      return -1;
    }
    new_doc_start = (crmhash_t  *)((char *)(nn->file_origin) + j);
  }
  else
  {
    new_doc_start = k;
  }

  n_docs = 0;
  for (k = nn->docs_start; k < nn->docs_end;)
  {
    in_class = *k++;
    if (((apb->sflags & CRM_REFUTE) && in_class)
        || !((apb->sflags & CRM_REFUTE) || in_class))
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

  //    Do the actual learning-looping only if append _wasn't_ specified.
  //
  if (append_only == 0)
  {
    n_docs_without_training = 0;
    n_cycles = 0;
    k = new_doc_start;

    if (apb->sflags & CRM_FROMSTART)
    {
      nuke(nn);
      soft_cycle_limit = NN_MAX_TRAINING_CYCLES_FROMSTART;
    }
    else
    {
      soft_cycle_limit = NN_MAX_TRAINING_CYCLES;
    }

    while (n_docs_without_training < n_docs && n_cycles < soft_cycle_limit)
    {
      in_class = *k++;
      l = k++;
      k++;           //to skip doc hash
      current_doc = k;
      current_doc_len = 0;
      while (*k++)
      {
        current_doc_len++;
      }
      if (do_net_learn_back_prop_with_noise(
            nn, current_doc, current_doc_len - 1, in_class))
      {
        if (((apb->sflags & CRM_REFUTE) && in_class)
            || !((apb->sflags & CRM_REFUTE) || in_class))
        {
          *l = 0;
        }
        n_docs_without_training = 0;
      }
      else
      {
        n_docs_without_training++;
      }

      if (k >= nn->docs_end)
      {
        k = nn->docs_start;
        n_cycles++;
      }
      if (internal_trace && k == new_doc_start)
        fputc('|', stderr);
    }
    if (n_cycles == soft_cycle_limit)
    {
      if (internal_trace)
      {
        fprintf(stderr, "neural: failed to converge after %d training cycles\n",
                NN_MAX_TRAINING_CYCLES);
      }
    }

    if (internal_trace)
      fprintf(stderr, "\nn_cycles = %ld\n", n_cycles);
    if (apb->sflags & CRM_MICROGROOM)
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
                          ", n_docs = %ld\n", n_docs);
        }
        free(filename);
        return 0;
      }
    }
    if (internal_trace)
    {
      print_doc_info(nn);
    }
  }

  unmap_file(nn, filename);

  if (internal_trace)
  {
    fprintf(stderr, "\nleaving neural net learn"
                    ", n_docs = %ld\n", n_docs);
  }

  free(filename);
  return 0;
}

int crm_neural_net_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                            char *txtptr, long txtstart, long txtlen)
{
  char filenames_field[MAX_PATTERN];
  long filenames_field_len;
  char filenames[MAX_CLASSIFIERS][MAX_FILE_NAME_LEN];

  NEURAL_NET_STRUCT NN, *nn = &NN;

  regex_t regee;
  char regex_text[MAX_PATTERN];       //  the regex pattern
  long regex_text_len;

  crmhash_t /* unsigned long */ bag[32768], *ret, sum;

  long i, j, k, n, rn, fail_on = MAX_CLASSIFIERS, n_classifiers, out_pos;

  float output[MAX_CLASSIFIERS][2];
  double p[MAX_CLASSIFIERS], pR[MAX_CLASSIFIERS], suc_p, suc_pR, tot;

  char out_var[MAX_PATTERN];
  long out_var_len;


  if (internal_trace)
    internal_trace = 1;

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

  //get tokenizing regex
  crm_get_pgm_arg(regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
  regex_text_len = apb->s1len;
  if (regex_text_len == 0)
  {
    strcpy(regex_text, "[[:graph:]]+");
    regex_text_len = strlen(regex_text);
  }
  regex_text[regex_text_len] = '\0';
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
    if (filenames_field[i] == '\\')     //allow escaped in case filename is wierd
    {
      filenames[j][k++] = filenames_field[++i];
    }
    else if (crm_isspace(filenames_field[i]) && k > 0)
    {
      //white space terminates filenames
      filenames[j][k] = '\0';
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
    filenames[j][k] = '\0';
  if (k > 0)
    n_classifiers = j + 1;
  else
    n_classifiers = j;

  if (fail_on > n_classifiers)
    fail_on = n_classifiers;

  if (internal_trace)
  {
    fprintf(stderr, "Diagnostic: fail_on = %ld n_classifiers = %ld\n",
            fail_on, n_classifiers);
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "filenames[%ld] = >%s<\n", i, filenames[i]);
  }

  n = eat_document(apb, txtptr + txtstart, txtlen, &j,
                   &regee, bag, WIDTHOF(bag), apb->sflags, &sum);

  if (internal_trace)
  {
    fprintf(stderr,
            "files: %ld, Filename0: >%s<  Filename1: >%s<  Filename2: >%s<\n",
            n_classifiers, filenames[0], filenames[1], filenames[2]);
  }

  //loop over classifiers and calc scores
  for (i = 0; i < n_classifiers; i++)
  {
    if (internal_trace)
    {
      fprintf(stderr,  "Now running filenames[%ld] = >%s<\n",
              i, filenames[i]);
    }
    if (map_file(nn, filenames[i]))
    {
      nonfatalerror("Couldn't mmap file!", filenames[i]);
      output[i][0] = 0.0;
      output[i][1] = 0.0;
      continue;
    }
    ret = calloc(nn->retina_size, sizeof(ret[0]));
    rn = project_features(bag, n, ret, nn->retina_size);
    do_net(nn, ret, rn, output[i]);
    free(ret);
    unmap_file(nn, filenames[i]);
    if (internal_trace)
    {
      fprintf(stderr, "%s:\t%f\t%f\n",
              filenames[i], output[i][0], output[i][1]);
    }
  }
  tot = 0.0;
  for (i = 0; i < n_classifiers; i++)
  {
    tot += p[i] = 0.5 * (1.0 + output[i][0] - output[i][1]);
  }
  for (i = 0; i < n_classifiers; i++)
  {
    p[i] /= tot;
  }
  j = 0;
  for (i = 1; i < n_classifiers; i++)
  {
    if (p[i] > p[j])
      j = i;
  }
  suc_p = 0.0;
  for (i = 0; i < fail_on; i++)
  {
    suc_p += p[i];
  }
  for (i = 0; i < n_classifiers; i++)
  {
    pR[i] = get_pR(p[i]);
  }
  suc_pR = get_pR(suc_p);
  out_pos = 0;

  if (suc_p > 0.5)
  {
    //test for nan as well
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
                     "Best match to file #%ld (%s) prob: %6.4f  pR: %6.4f  \n",
                     j,
                     filenames[j],
                     p[j], pR[j]);

  out_pos += sprintf(outbuf + out_pos,
                     "Total features in input file: %ld\n",
                     n);

  for (i = 0; i < n_classifiers; i++)
  {
    out_pos += sprintf(outbuf + out_pos,
                       "#%ld (%s): prob: %3.2e, pR: %6.2f\n",
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
                         char *txtptr, long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "FSCM");
}


int crm_neural_net_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                            char *txtptr, long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "FSCM");
}

#endif /* CRM_WITHOUT_NEURAL_NET */

