//  crm_svm.c  - version v1.0
//
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
///////////////////////////////////////////////////////////////////////////
//
//    This code is originally copyright and owned by William
//    S. Yerazunis.  In return for addition of significant derivative
//    work, Yimin Wu is hereby granted a full unlimited license to use
//    this code, includng license to relicense under other licenses.
//
///////////////////////////////////////////////////////////////////////
//
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



#if !defined (CRM_WITHOUT_SVM)



//////////////////////////////////////////////////////////////////////////
//
//                  Support Vector Machine (SVM) Classification
//
//    This is an implementation of a support vector machine classification.
//    The current version only implement one type of SVM called C-Support
//    Vector Classification (C-SVC, Boser et al., 1992; Cortes and Vapnik,
//    1995).
//
//    The dual formulation of C-SVC is to find
//
//            min     0.5 ( \alpha^T Q \alpha) - e^T \alpha
//
//      subject to    y^T \alpha = 0
//                    y_i = +1 or -1
//                    0 <= alpha_i <= C, i=1,...,sizeof(corpus).
//
//    Where "e" is the vector of all ones,
//       Q is the sizeof(corpus) by sizeof(corpus) matrix containing the
//         calculated distances between any two documents (that is,
//           Q_ij = y_i * y_j * kernel(x_i, x_j) which may be HUGE and so
//           we only calculate part of it at any one time.
//       x_i is the feature vector of document i.
//
//    The decision function is
//
//           sgn (sum(y_i * \alpha_i * kernel(x_i, x)) + b)
//
//     In the optimization, we set the kernel parameters at the start and
//     then modify only the weighting parameters till it (hopefully) converges.

////////////////////////////////////////////////////////////////////////////
//
//                   SMO-type Decomposition Method
//
//    Here we used SMO-type decomposition method ( Platt, 1998) to solve
//    the quadratic optimization problem --dual formulation of C-SVC, using
//    the method of Fan, Chen, and Lin ("Working Set Selection using Second
//    Order Information for Training Support Vector Machines", 2005)
//    to select the working set.
//

//   Pick a kernel...
#define LINEAR 0
#define RBF 1
#define POLY 2

//   and a mode for the software to run in (only C-SVC works for now)
#define C_SVC 0
#define ONE_CLASS 1



//    Tau is a small minimum positive number (divide-by-zero noodge)
#define TAU 1e-12

#if 10
typedef double Qitem_t;
#else
typedef float Qitem_t;
#endif


//    We use the same old hyperspace structures for the intermediate storage.
typedef struct mythical_hyperspace_cell
{
    crmhash_t hash;
} HYPERSPACE_FEATUREBUCKET_STRUCT;


//     Parameter block to control the SVM solver.
//
typedef struct mythical_svm_param
{
    int    svm_type;
    int    kernel_type;
    double cache_size;        // in MB
    double eps;               // convergence stop criterion
    double C;                 // parameter in C_SVC
    double nu;                // parameter for One-class SVM
    double max_run_time;      // time control for microgroom (in seconds).
                              // If computing time exceeds max_run_time,
                              // then start microgrooming to delete the
                              //  documents far away from the hyperplane.
    int shrinking;      // use the shrinking heuristics, isn't available now
} SVM_PARAM;


//     And a struct to hold the actual data we're trying to solve.
//
typedef struct mythical_svm_problem
{
    int                               l; //number of documents
    int                              *y; //label of documents -1/+1
    HYPERSPACE_FEATUREBUCKET_STRUCT **x; //  x[i] is the ith document's
                                         //  feature vector
} SVM_PROBLEM;

//    A structure to hold the cache node - these hold one row worth of
//    Q matrix.
//
typedef struct mythical_cache_node
{
    struct mythical_cache_node *prev, *next;
    Qitem_t *data;
    int      len;
} CACHE_NODE;

//    This is the cache representaton of the whole matrix; this is the
//    "first column" and points to the start of each row.
typedef struct mythical_cache
{
    int          l;        // The number of documents in the corpus
    int         size;     // The cache size (bytes)
    CACHE_NODE  *head;
    CACHE_NODE   lru_headnode; //least-recent-use node
} CACHE;

//   This stores the result - alpha is the weighting vector (what we are
//   searching for) and
//
typedef struct mythical_solver
{
    double *alpha;
    double *G;          // Gradient of objective function in each dimension
    double *deci_array; // decision values for all training data
} SOLVER;

//   And a few file-wide static globals:
//
static SVM_PARAM param = { 0 };
static SVM_PROBLEM svm_prob = { 0 };
static CACHE svmcache = { 0 };
static Qitem_t *DiagQ = NULL; //diagonal Qmatrix
static SOLVER solver = { 0 };

////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static const int hctable[] =
{
    1, 7,
    3, 13,
    5, 29,
    11, 51,
    23, 101,
    47, 203,
    97, 407,
    197, 817,
    397, 1637,
    797, 3277
};

#if defined (CRM_WITHOUT_MJT_INLINED_QSORT)

static int hash_compare(void const *a, void const *b)
{
    HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;

    pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *)a;
    pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *)b;
    if (pa->hash < pb->hash)
        return -1;

    if (pa->hash > pb->hash)
        return 1;

    return 0;
}

//   Compare two integers when all we have are void* pointers to them
static int int_compare(void const *a, void const *b)
{
    int *pa, *pb;

    pa = (int *)a;
    pb = (int *)b;
    if (*pa < *pb)
        return -1;

    if (*pa > *pb)
        return 1;

    return 0;
}

//  Compare ids of documents according to its decision value (decending
//  absolute value), which is used for microgrooming
static int descend_int_decision_compare(void const *a, void const *b)
{
    int *pa, *pb;

    pa = (int *)a;
    pb = (int *)b;
    if (solver.deci_array[*pa] < solver.deci_array[*pb])
        return 1;

    if (solver.deci_array[*pa] > solver.deci_array[*pb])
        return -1;

    return 0;
}

#else

#define hash_compare(a, b) \
    ((a)->hash < (b)->hash)

//   Compare two integers when all we have are void* pointers to them
#define int_compare(a, b) \
    (*(a) < *(b))

//  Compare ids of documents according to its decision value (decending
//  absolute value), which is used for microgrooming
#define descend_int_decision_compare(a, b) \
    (solver.deci_array[*(a)] < solver.deci_array[*(b)])

#endif



///////////////////////////////////////////////////////////////////////////
//
//     Cache with least-recent-use strategy
//     This will be used to store the part of the Q matrix that we know
//     about.  We recalculate parts as needed... this lets us solve the
//     problem without requiring enough memory to build the entire Q
//     matrix.
//

static void cache_init(int len, int size, CACHE *svmcache)
{
    svmcache->l = len;
    svmcache->size = size;
    svmcache->head = (CACHE_NODE *)calloc(len, sizeof(svmcache->head[0]));
#if 0 /* [i_a] unused code... */
    size /= sizeof(Qitem_t);
    size -= len * (sizeof(CACHE_NODE) / sizeof(Qitem_t));
    if (size < (2 * len))
        size = 2 * len; //   cache size must at least
    //   as large as two columns of Qmatrix
#endif
    (svmcache->lru_headnode).prev
    = (svmcache->lru_headnode).next
      = &(svmcache->lru_headnode);
}


//    Release the whole Q-matrix cache
//
static void cache_free(CACHE *svmcache)
{
    CACHE_NODE *temp;

    for (temp = (svmcache->lru_headnode).next;
         temp != &(svmcache->lru_headnode);
         temp = temp->next)
        free(temp->data);
    free(svmcache->head);
}


// Delete one node (that is, one row of Q-matrix) from the LRU.
// (we then usually move that row to the front of the LRU.
static void lru_delete(CACHE_NODE *h)
{
    h->prev->next = h->next;
    h->next->prev = h->prev;
}

// Insert to the last position in the cache node list
//
static void lru_insert(CACHE_NODE *h, CACHE *svmcache)
{
    h->next = &(svmcache->lru_headnode);
    h->prev = (svmcache->lru_headnode).prev;
    h->prev->next = h;
    (svmcache->lru_headnode).prev = h;
}

//  Get a line of Q matrix data for certain document, and return the
//  length of cached data.  If it is smaller than the request length,
//  then we need to fill in the uncached data.

static int get_data(CACHE      *svmcache,
        const int               doc_index,
        Qitem_t               **data,
        int                     length)
{
    int result = length;
    CACHE_NODE *doc = svmcache->head + doc_index;

    CRM_ASSERT(doc_index >= 0);
    CRM_ASSERT(doc_index < svmcache->l);
    if (doc->len)
        lru_delete(doc); //least-recent-use strategy

    //need to allocate more space
    if (length > (doc->len))
    {
        //   GROT GROT GROT check this to see if it doesn't leak memory
        // Cache hasn't enough free space, we need to release some old space
        while ((svmcache->size) < (length - doc->len))
        {
            CACHE_NODE *temp = (svmcache->lru_headnode).next;
            lru_delete(temp);
            free(temp->data);
            svmcache->size += temp->len;
            temp->data = 0;
            temp->len = 0;
        }
        //allocate new space
        doc->data = (Qitem_t *)realloc(doc->data, length * sizeof(doc->data[0]));
        if (!doc->data)
        {
            fatalerror("Could not re-allocate enough space for document data ", "");
        }
        else
        {
            memset(doc->data + doc->len, 0, (length - doc->len) * sizeof(doc->data[0]));
            svmcache->size -= (length - doc->len);
            result = doc->len;
            doc->len = length;
        }
    }
    lru_insert(doc, svmcache);
    *data = doc->data;
    return result;
}


// Dot operation of two feature vectors
//
static double dot(void const *a, void const *b)
{
    HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;
    int j = 0;
    int i = 0;
    double sum = 0;

    pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *)a;
    pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *)b;
    while (pa[i].hash != 0 && pb[j].hash != 0)
    {
        if (pa[i].hash == pb[j].hash && pa[i].hash != 0)
        {
            sum++;
            i++;
            j++;
        }
        else
        {
            if (pa[i].hash > pb[j].hash)
                j++;
            else
                i++;
        }
    }
    return sum;
}

//   RBF (Radial Basis Function) kernel
//
static double rbf(void const *a, void const *b)
{
    HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;
    int j = 0;
    int i = 0;
    double sum = 0;

    pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *)a;
    pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *)b;
    while (pa[i].hash != 0 && pb[j].hash != 0)
    {
        if (pa[i].hash > pb[j].hash)
        {
            sum++;
            j++;
        }
        else if (pa[i].hash < pb[j].hash)
        {
            sum++;
            i++;
        }
        else
        {
            i++;
            j++;
        }
    }
    while (pa[i].hash != 0)
    {
        sum++;
        i++;
    }
    while (pb[j].hash != 0)
    {
        sum++;
        j++;
    }
    return exp(-0.00001  * sum);
}


//    Polynomial kernel of two basis vectors
static double poly(void const *a, void const *b)
{
    double gamma = 0.001;
    double sum = 0.0;

    sum = pow(gamma * dot(a, b) + 3, 2.0);
    return sum;
}


// Big switch to pick the kernel operation we want
static double kernel(void const *a, void const *b)
{
    switch (param.kernel_type)
    {
    case LINEAR:
        return dot(a, b);

    case RBF:
        return rbf(a, b);

    case POLY:
        return poly(a, b);

    default:
        return 0;
    }
}



//  Ask the cache for the ith row in the Q matrix for C-Support Vector
//  Classification(C-SVC) and one-class SVM
//
static Qitem_t *get_rowQ(int i, int length)
{
    Qitem_t *rowQ;
    int cached;

    if ((cached = get_data(&svmcache, i, &rowQ, length)) < length)
    {
        int temp;
        for (temp = cached; temp < length; temp++)
        {
            CRM_ASSERT(temp >= 0);
            CRM_ASSERT(temp < svm_prob.l);
            CRM_ASSERT(i >= 0);
            CRM_ASSERT(i < svm_prob.l);
            if (param.svm_type == C_SVC)
            {
                //   multiply by the +1/-1 labels (in the .y structures) to
                //   face the kernel result in the right direction.
                rowQ[temp] = (Qitem_t)svm_prob.y[i]
                             * svm_prob.y[temp]
                             * kernel(svm_prob.x[i], svm_prob.x[temp]);
            }
            else if (param.svm_type == ONE_CLASS)
            {
                rowQ[temp] = (Qitem_t)kernel(svm_prob.x[i], svm_prob.x[temp]);
            }
        }
    }
    return rowQ;
}

//  Request of the diagonal in Qmatrix for C- Support Vector
//  Classification(C-SVC) and one-class SVM
static Qitem_t *get_DiagQ()
{
    Qitem_t *DiagQ = calloc(svm_prob.l, sizeof(DiagQ[0]));
    int i;

    for (i = 0; i < svm_prob.l; i++)
    {
        DiagQ[i] = kernel(svm_prob.x[i], svm_prob.x[i]);
    }
    return DiagQ;
}

//  Initialize the cache and diagonal Qmatrix
static void Q_init(void)
{
    //   Default initialization is the param cache size in megabytes (that's
    //   the 1<<20)
    cache_init(svm_prob.l, (int)(param.cache_size * (1L << 20)), &svmcache);
    DiagQ = get_DiagQ();
}

//   Now, select a working set.  This is based on the paper:
// "An SMO algorithm in Fan et al., JMLR 6(2005), p. 1889--1918"
// Solves:
//
//      min 0.5(\alpha^T Q \alpha) + p^T \alpha
//
//              y^T \alpha = \delta
//              y_i = +1 or -1
//              0 <= alpha <= C
//
//
// Given:
//
//      Q, p, y, C, and an initial feasible point \alpha
//      l is the size of vectors and matrices
//      eps is the stopping tolerance
//
// solution will be put in \alpha
//

//  modification from Fan's paper- select_times keeps us from re-selecting
//   the same document over and over (which was causing us to lock up)

static void selectB(int workset[], int *select_times)
{
    // select i
    int i = -1;
    double G_max;
    double G_min;
    int t;
    int j;
    double obj_min;
    double a, b;
    Qitem_t *Qi;

    //     Select a document that is on the wrong side of the hyperplane
    //    (called a "violating pair" in Fan's paper).  Note that the
    //    margin is not symmetrical - we can select any "positive" class
    //    element with alpha < param.C, but the 'negative' class must
    //    only be greater than 0.  Yimin says this is OK, I say it's
    //    weird.
    G_max = -HUGE_VAL;
    for (t = 0; t < svm_prob.l; t++)
    {
        if ((((svm_prob.y[t] == 1) && (solver.alpha[t] < param.C))
             || ((svm_prob.y[t] == -1) && (solver.alpha[t] > 0)))
            && select_times[t] < 10)
        {
            if (-svm_prob.y[t] * solver.G[t] >= G_max)
            {
                i = t;
                G_max = -svm_prob.y[t] * solver.G[t];
            }
        }
    }

    //  select j as second member of working set;
    j = -1;
    obj_min = HUGE_VAL;
    G_min = HUGE_VAL;
    for (t = 0; t < svm_prob.l; t++)
    {
        if ((((svm_prob.y[t] == -1) && (solver.alpha[t] < param.C))
             || ((svm_prob.y[t] == 1) && (solver.alpha[t] > 0)))
            && select_times[t] < 10)
        {
            b = G_max + svm_prob.y[t] * solver.G[t];
            if (-svm_prob.y[t] * solver.G[t] <= G_min)
                G_min = -svm_prob.y[t] * solver.G[t];
            if (b > 0)
            {
                if (i != -1)
                {
                    Qi = get_rowQ(i, svm_prob.l);
                    a = Qi[i] + DiagQ[t]
                        - 2 * svm_prob.y[i] * svm_prob.y[t] * Qi[t];
                    if (a <= 0)
                        a = TAU;
                    if (-(b * b) / a <= obj_min)
                    {
                        j = t;
                        obj_min = -(b * b) / a;
                    }
                }
            }
        }
    }
    //   Are we done?
    if (G_max - G_min < param.eps)
    {
        workset[0] = -1;
        workset[1] = -1;
    }
    else
    {
        workset[0] = i;
        workset[1] = j;
    }
}


static void solve(void)
{
    int t, workset[2], i, j;
    double a, b, oldi, oldj, sum;
    Qitem_t *Qi, *Qj;
    int *select_times;

    //  Array for storing how many times a particular document has been
    //  selected in working set.
    select_times = calloc(svm_prob.l, sizeof(select_times[0]));
    if (select_times == NULL)
    {
        untrappableerror("Couldn't allocate space for the solver.",
                "[select_items]");
    }

    for (i = 0; i < svm_prob.l; i++)
    {
        select_times[i] = 0;
    }

    CRM_ASSERT(solver.alpha == NULL);
    solver.alpha = calloc(svm_prob.l, sizeof(solver.alpha[0]));
    if (solver.alpha == NULL)
    {
        untrappableerror("Couldn't allocate space for the solver.",
                "[solver.alpha]");
    }
    CRM_ASSERT(solver.G == NULL);
    solver.G = calloc(svm_prob.l, sizeof(solver.G[0]));
    if (solver.G == NULL)
    {
        untrappableerror("Couldn't allocate space for the solver.",
                "[solver.G]");
    }
    if (param.svm_type == C_SVC)
    {
        // initialize alpha to all zero;
        // initialize G to all -1;
        for (t = 0; t < svm_prob.l; t++)
        {
            solver.alpha[t] = 0;
            solver.G[t] = -1;
        }
    }
    else if (param.svm_type == ONE_CLASS)
    {
        int n;

        //initialize the first nu*l elements of alpha to have the value one;
        n = (int)(param.nu * svm_prob.l);
        CRM_ASSERT(n <= svm_prob.l);
        for (i = 0; i < n; i++)
        {
            solver.alpha[i] = 1;
        }
        if (n < svm_prob.l)
        {
            solver.alpha[n] = param.nu * svm_prob.l - n;
        }
        for (i = n + 1; i < svm_prob.l; i++)
        {
            solver.alpha[i] = 0;
        }
        //initialize G to all 0;
        for (i = 0; i < svm_prob.l; i++)
        {
            solver.G[i] = 0;
        }
    }
    while (1)
    {
        selectB(workset, select_times);
        i = workset[0];
        j = workset[1];
        if (i != -1)
            select_times[i]++;
        if (j != -1)
            select_times[j]++;
        if (j == -1)
            break;

        Qi = get_rowQ(i, svm_prob.l);
        Qj = get_rowQ(j, svm_prob.l);

        //  Calculate the incremental step forward.
        a = Qi[i] + DiagQ[j] - 2 * svm_prob.y[i] * svm_prob.y[j] * Qi[j];
        if (a <= 0)
            a = TAU;
        b = -svm_prob.y[i] * solver.G[i] + svm_prob.y[j] * solver.G[j];

        //  update alpha (weight vector)
        oldi = solver.alpha[i];
        oldj = solver.alpha[j];
        solver.alpha[i] += svm_prob.y[i] * b / a;
        solver.alpha[j] -= svm_prob.y[j] * b / a;

        //  Project alpha back to the feasible region(that is, where
        //  where 0 <= alpha <= C )
        sum = svm_prob.y[i] * oldi + svm_prob.y[j] * oldj;
        if (solver.alpha[i] > param.C)
            solver.alpha[i] = param.C;
        if (solver.alpha[i] < 0)
            solver.alpha[i] = 0;
        solver.alpha[j] = svm_prob.y[j]
                          * (sum - svm_prob.y[i] * (solver.alpha[i]));
        if (solver.alpha[j] > param.C)
            solver.alpha[j] = param.C;
        if (solver.alpha[j] < 0)
            solver.alpha[j] = 0;
        solver.alpha[i] = svm_prob.y[i]
                          * (sum - svm_prob.y[j] * (solver.alpha[j]));

        //  update gradient array
        for (t = 0; t < svm_prob.l; t++)
        {
            solver.G[t] += Qi[t] * (solver.alpha[i] - oldi)
                           + Qj[t] * (solver.alpha[j] - oldj);
        }
    }

    free(select_times); /* [i_a] wasn't this missing? */
}

//    Calculate b (hyperplane offset in
//      SUM (y[i] alpha[i] kernel (x[i],x)) + b form)
//       after calculating error margin alpha
static double calc_b()
{
    int count = 0;
    double upper = HUGE_VAL;
    double lower = -HUGE_VAL;
    double sum = 0;
    int i;
    double b;

    for (i = 0; i < svm_prob.l; i++)
    {
        if (svm_prob.y[i] == 1)
        {
            if (solver.alpha[i] == param.C)
            {
                if (solver.G[i] > lower)
                {
                    lower = solver.G[i];
                }
            }
            else if (solver.alpha[i] == 0)
            {
                if (solver.G[i] < upper)
                {
                    upper = solver.G[i];
                }
            }
            else
            {
                count++;
                sum += solver.G[i];
            }
        }
        else
        {
            if (solver.alpha[i] == 0)
            {
                if (-solver.G[i] > lower)
                {
                    lower = -solver.G[i];
                }
            }
            else if (solver.alpha[i] == param.C)
            {
                if (-solver.G[i] < upper)
                {
                    upper = -solver.G[i];
                }
            }
            else
            {
                count++;
                sum -= solver.G[i];
            }
        }
    }
    if (count > 0)
        b = -sum / count;
    else
        b = -(upper + lower) / 2;
    return b;
}

//   Calculate the decision function
static double calc_decision(HYPERSPACE_FEATUREBUCKET_STRUCT *x,
        double                                              *alpha,
        double                                               b)
{
    int i;
    double sum = 0;

    i = 0;
    if (param.svm_type == C_SVC)
    {
        for (i = 0; i < svm_prob.l; i++)
        {
            if (alpha[i] != 0)
            {
                sum += svm_prob.y[i] * alpha[i] * kernel(x, svm_prob.x[i]);
            }
        }
        sum += b;
    }
    else if (param.svm_type == ONE_CLASS)
    {
        for (i = 0; i < svm_prob.l; i++)
        {
            if (alpha[i] != 0)
            {
                sum += alpha[i] * kernel(x, svm_prob.x[i]);
            }
        }
        sum -= b;
    }
    return sum;
}

//  Implementation of Lin's 2003 improved algorithm on Platt's
//  probabilistic outputs for binary SVM
//  Input parameters: deci_array = array of svm decision values
//                    svm.prob
//                    posn = number of positive examples
//                    negn = number of negative examples

//  Outputs: parameters of sigmoid function-- A and B  (AB[0] = A, AB[1] = B)
static void calc_AB(double *AB, double *deci_array, int posn, int negn)
{
    int maxiter, i, j;
    double minstep, sigma, fval, hiTarget, loTarget, *t;
    double fApB, h11, h22, h21, g1, g2, p, q, d1, d2, det, dA, dB, gd, stepsize, newA, newB, newf;

    maxiter = 100;
    minstep = 1e-10;
    sigma = 1e-3;
    fval = 0.0;
    hiTarget = (posn + 1.0) / (posn + 2.0);
    loTarget = 1 / (negn + 2.0);
    t = (double *)calloc(svm_prob.l, sizeof(t[0]));
    for (i = 0; i < svm_prob.l; i++)
    {
        if (svm_prob.y[i] > 0)
            t[i] = hiTarget;
        else
            t[i] = loTarget;
    }
    AB[0] = 0.0;
    AB[1] = log((negn + 1.0) / (posn + 1.0));
    for (i = 0; i < svm_prob.l; i++)
    {
        fApB = deci_array[i] * AB[0] + AB[1];
        if (fApB >= 0)
            fval += t[i] * fApB + log(1 + exp(-fApB));
        else
            fval += (t[i] - 1) * fApB + log(1 + exp(fApB));
    }

    for (j = 0; j < maxiter; j++)
    {
        h11 = h22 = sigma;
        h21 = g1 = g2 = 0.0;
        for (i = 0; i < svm_prob.l; i++)
        {
            fApB = deci_array[i] * AB[0] + AB[1];
            if (fApB >= 0)
            {
                p = exp(-fApB) / (1.0 + exp(-fApB));
                q = 1.0 / (1.0 + exp(-fApB));
            }
            else
            {
                p =  1.0 / (1.0 + exp(fApB));
                q = exp(fApB) / (1.0 + exp(fApB));
            }
            d2 = p * q;
            h11 += deci_array[i] * deci_array[i] * d2;
            h22 += d2;
            h21 += deci_array[i] * d2;
            d1 = t[i] - p;
            g1 += deci_array[i] * d1;
            g2 += d1;
        }
        // Stopping Criterion
        if ((fabs(g1) < 1e-5) && (fabs(g2) < 1e-5))
        {
            break;
        }
        //compute modified Newton directions
        det = h11 * h22 - h21 * h21;
        dA = -(h22 * g1 - h21 * g2) / det;
        dB = -(-h21 * g1 +  h11 * g2) / det;
        gd = g1 * dA + g2 * dB;
        stepsize = 1;
        while (stepsize >= minstep)
        {
            newA = AB[0] + stepsize * dA;
            newB = AB[1] + stepsize * dB;
            newf = 0.0;
            for (i = 0; i < svm_prob.l; i++)
            {
                fApB = deci_array[i] * newA + newB;
                if (fApB >= 0)
                    newf += t[i] * fApB + log(1 + exp(-fApB));
                else
                    newf += (t[i] - 1) * fApB + log(1 + exp(fApB));
            }
            // Check whether sufficient decrease is satisfied
            if (newf < fval + 0.0001 * stepsize * gd)
            {
                AB[0] = newA;
                AB[1] = newB;
                fval = newf;
                break;
            }
            else
                stepsize /= 2.0;
        }
        if (stepsize < minstep)
        {
            if (user_trace)
                fprintf(stderr, "Line search fails in probability estimates\n");
            break;
        }
    }
    if (j >= maxiter)
        if (user_trace)
            fprintf(stderr,
                    "Reaching maximal iterations in  probability estimates\n");
    free(t);
}

static double sigmoid_predict(double decision_value, double A, double B)
{
    double fApB = decision_value * A + B;

    if (fApB >= 0)
    {
        return exp(-fApB) / (1.0 + exp(-fApB));
    }
    else
        return 1.0 / (1 + exp(fApB));
}


int crm_expr_svm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    int i, j, k, h;
    char ftext[MAX_PATTERN];
    int flen;
    char file1[MAX_PATTERN];
    char file2[MAX_PATTERN];
    char file3[MAX_PATTERN];
    FILE *hashf;
    int textoffset;
    int textmaxoffset;
    HYPERSPACE_FEATUREBUCKET_STRUCT *hashes; //  the hashes we'll sort
    int hashcounts;
    int cflags, eflags;
    int sense;
    int microgroom;
    int unique;
    int use_unigram_features;
    char ptext[MAX_PATTERN]; //the regrex pattern
    int plen;
    regex_t regcb;
    regmatch_t match[5];
    struct stat statbuf1;    //  for statting the hash file1
    struct stat statbuf2;    //  for statting the hash file2
    crmhash_t hashpipe[OSB_BAYES_WINDOW_LEN + 1];
    time_t start_timer;
    time_t end_timer;
    double run_time;

    i = 0;
    j = 0;
    k = 0;

    start_timer = time(NULL);

    //            set our cflags, if needed.  The defaults are
    //            "case" and "affirm", (both zero valued).
    //            and "microgroom" disabled.
    cflags = REG_EXTENDED;
    eflags = 0;
    sense = +1;
    if (apb->sflags & CRM_NOCASE)
    {
        cflags = cflags | REG_ICASE;
        eflags = 1;
        if (user_trace)
            fprintf(stderr, "turning oncase-insensitive match\n");
    }
    if (apb->sflags & CRM_REFUTE)
    {
        sense = -sense;
        if (user_trace)
            fprintf(stderr, " refuting learning\n");
    }
    microgroom = 0;
    if (apb->sflags & CRM_MICROGROOM)
    {
        microgroom = 1;
        if (user_trace)
            fprintf(stderr, " enabling microgrooming.\n");
    }

    unique = 0;
    if (apb->sflags & CRM_UNIQUE)
    {
        unique = 1;
        if (user_trace)
            fprintf(stderr, " enabling uniqueifying features.\n");
    }

    use_unigram_features = 0;
    if (apb->sflags & CRM_UNIGRAM)
    {
        use_unigram_features = 1;
        if (user_trace)
            fprintf(stderr, " using only unigram features.\n");
    }

    //   Note that during a LEARN in hyperspace, we do NOT use the mmap of
    //    pre-existing memory.  We just write to the end of the file instead.
    //    malloc up the unsorted hashbucket space
    hashes = calloc(HYPERSPACE_MAX_FEATURE_COUNT,
            sizeof(hashes[0]));
    hashcounts = 0;

    //  Extract the file names for storing svm solver.( file1.svm |
    //  file2.svm | 1vs2_solver.svm )
    crm_get_pgm_arg(ftext, MAX_PATTERN, apb->p1start, apb->p1len);
    flen = apb->p1len;
    flen = crm_nexpandvar(ftext, flen, MAX_PATTERN);

    strcpy(
            ptext,
            "[[:space:]]*([[:graph:]]+)[[:space:]]*\\|[[:space:]]*([[:graph:]]+)[[:space:]]*\\|[[:space:]]*([[:graph:]]+)[[:space:]]*");
    plen = strlen(ptext);
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);
    i = crm_regcomp(&regcb, ptext, plen, cflags);
    if (i != 0)
    {
        crm_regerror(i, &regcb, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto regcomp_failed;
    }
    k = crm_regexec(&regcb, ftext,
            flen, WIDTHOF(match), match, 0, NULL);
    if (k == 0)
    {
        // get three input files.
        memmove(file1, &ftext[match[1].rm_so], (match[1].rm_eo - match[1].rm_so));
        file1[match[1].rm_eo - match[1].rm_so] = 0;
        memmove(file2, &ftext[match[2].rm_so], (match[2].rm_eo - match[2].rm_so));
        file2[match[2].rm_eo - match[2].rm_so] = 0;
        memmove(file3, &ftext[match[3].rm_so], (match[3].rm_eo - match[3].rm_so));
        file3[match[3].rm_eo - match[3].rm_so] = 0;
        if (internal_trace)
            fprintf(stderr, "file1=%s\tfile2=%s\tfile3=%s\n", file1, file2, file3);
    }
    else
    {
        //only has one input file
        if (ptext[0] != 0)
            crm_regfree(&regcb);
#if 0
    i = 0;
    while (ftext[i] < 0x021)
        i++;
    CRM_ASSERT(i < flen);
    j = i;
    while (ftext[j] >= 0x021)
        j++;
    CRM_ASSERT(j <= flen);
#else
 if (!crm_nextword(ftext, flen, 0, &i, &j) || j == 0)
 {
            int fev = nonfatalerror_ex(SRC_LOC(), 
				"\nYou didn't specify a valid filename: '%.*s'\n", 
					(int)flen,
					ftext);
            return fev;
 }
 j += i;
    CRM_ASSERT(i < flen);
    CRM_ASSERT(j <= flen);
#endif
        ftext[j] = 0;
        strcpy(file1, &ftext[i]);
        file2[0] = 0;
        file3[0] = 0;
    }
    //    if (|Text|>0) hide the text into the .svm file

    //     get the "this is a word" regex
    crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    plen = apb->s1len;
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);

    //   compile the word regex
    //
    if (internal_trace)
        fprintf(stderr, "\nWordmatch pattern is %s", ptext);

    i = crm_regcomp(&regcb, ptext, plen, cflags);

    if (i != 0)
    {
        crm_regerror(i, &regcb, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto regcomp_failed;
    }

    //   Now tokenize the input text
    //   We got txtptr, txtstart, and txtlen from the caller.
    //
    textoffset = txtstart;
    textmaxoffset = txtstart + txtlen;
    i = 0;
    j = 0;
    k = 0;

    //   init the hashpipe with 0xDEADBEEF
    for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
    {
        hashpipe[h] = 0xDEADBEEF;
    }

    // if [Text]>0 hide it and append to the file1
    if (txtlen > 0)
    {
        while (k == 0 && textoffset <= textmaxoffset
               && hashcounts < HYPERSPACE_MAX_FEATURE_COUNT)
        {
            int wlen;
            int slen = textmaxoffset - textoffset;
            // if pattern is empty, extract non graph delimited tokens
            // directly ([[graph]]+) instead of calling regexec  (8% faster)
            if (ptext[0] != 0)
            {
                k = crm_regexec(&regcb, &(txtptr[textoffset]),
                        slen, WIDTHOF(match), match, 0, NULL);
            }
            else
            {
                k = 0;
                //         skip non-graphical characters
                match[0].rm_so = 0;
                while (!crm_isgraph(txtptr[textoffset + match[0].rm_so])
                       && textoffset + match[0].rm_so < textmaxoffset)
                    match[0].rm_so++;
                match[0].rm_eo = match[0].rm_so;
                while (crm_isgraph(txtptr[textoffset + match[0].rm_eo])
                       && textoffset + match[0].rm_eo < textmaxoffset)
                    match[0].rm_eo++;
                if (match[0].rm_so == match[0].rm_eo)
                    k = 1;
            }
            if (!(k != 0 || textoffset > textmaxoffset))
            {
                wlen = match[0].rm_eo - match[0].rm_so;
                memmove(tempbuf,
                        &(txtptr[textoffset + match[0].rm_so]),
                        wlen);
                tempbuf[wlen] = 0;
                if (internal_trace)
                {
                    fprintf(stderr,
                            "  Learn #%d t.o. %d strt %d end %d len %d is -%s-\n",
                            i,
                            textoffset,
                            (int)match[0].rm_so,
                            (int)match[0].rm_eo,
                            wlen,
                            tempbuf);
                }
                if (match[0].rm_eo == 0)
                {
                    nonfatalerror("The LEARN pattern matched zero length! ",
                            "\n Forcing an increment to avoid an infinite loop.");
                    match[0].rm_eo = 1;
                }

                //      Shift the hash pipe down one
                //
                for (h = OSB_BAYES_WINDOW_LEN - 1; h > 0; h--)
                {
                    hashpipe[h] = hashpipe[h - 1];
                }
                //  and put new hash into pipeline
                hashpipe[0] = strnhash(tempbuf, wlen);
                if (internal_trace)
                {
                    fprintf(stderr, "  Hashpipe contents: ");
                    for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
                        fprintf(stderr, " 0x%08lX", (unsigned long int)hashpipe[h]);
                    fprintf(stderr, "\n");
                }

                //  and account for the text used up.
                textoffset = textoffset + match[0].rm_eo;
                i++;

                //        is the pipe full enough to do the hashing?
                //  we always run the hashpipe now, even if it's
                //  just full of 0xDEADBEEF.  (was i >=5)
                if (1)
                {
                    crmhash_t h1;
                    crmhash_t h2;
                    int th = 0;       // a counter used for TSS tokenizing
                    int j;
                    //
                    //     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
                    //     (coefficients chosen by requiring superincreasing,
                    //     as well as prime)
                    //
                    th = 0;
                    //
                    if (use_unigram_features == 1)
                    {
                        h1 = hashpipe[0];
                        if (h1 == 0)
                            h1 = 0xdeadbeef;
                        h2 = 0xdeadbeef;
                        if (internal_trace)
                            fprintf(stderr, "Singleton feature: 0x%08lX\n", (unsigned long int)h1);
                        CRM_ASSERT(hashcounts >= 0);
                        CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
                        hashes[hashcounts].hash = h1;
                        hashcounts++;
                    }
                    else
                    {
                        for (j = 1;
                             j < OSB_BAYES_WINDOW_LEN; // OSB_BAYES_WINDOW_LEN;
                             j++)
                        {
                            h1 = hashpipe[0] * hctable[0] +
                                 hashpipe[j] * hctable[j << 1];
                            if (h1 == 0)
                                h1 = 0xdeadbeef;
                            h2 = 0xdeadbeef;
                            if (internal_trace)
                                fprintf(stderr,
                                        "Polynomial %d has h1:0x%08lX  h2:0x%08lX\n",
                                        j, (unsigned long int)h1, (unsigned long int)h2);

                            CRM_ASSERT(hashcounts >= 0);
                            CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
                            hashes[hashcounts].hash = h1;
                            hashcounts++;
                        }
                    }
                }
            }
            else
            {
                if (ptext[0] != 0)
                    crm_regfree(&regcb);
                k = 1;
            }
        }   //   end the while k==0

        CRM_ASSERT(hashcounts >= 0);
        CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
        //mark the end of a feature vector
        hashes[hashcounts].hash = 0;

        //   Now sort the hashes array.
        //
        QSORT(HYPERSPACE_FEATUREBUCKET_STRUCT, hashes, hashcounts,
                hash_compare);

        if (user_trace)
        {
            fprintf(stderr, "sorted hashes:\n");
            for (i = 0; i < hashcounts; i++)
            {
                fprintf(stderr, "hashes[%d]=0x%08lX\n", i, (unsigned long int)hashes[i].hash);
            }
            fprintf(stderr, "Total hashes generated: %d\n", hashcounts);
        }

        //   And uniqueify the hashes array
        //
        if (unique)
        {
			int j;

            i = 0;
            j = 0;

            CRM_ASSERT(hashcounts >= 0);
            CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
            CRM_ASSERT(hashes[hashcounts].hash == 0);
            while (i < hashcounts)
            {
                if (hashes[i].hash != hashes[i + 1].hash)
                {
                    hashes[j] = hashes[i];
                    j++;
                }
                i++;
            }
            hashcounts = j;

            //mark the end of a feature vector
            hashes[hashcounts].hash = 0;
        }

        CRM_ASSERT(hashcounts >= 0);
        CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
        CRM_ASSERT(hashes[hashcounts].hash == 0);

        if (user_trace)
        {
            fprintf(stderr, "Total unique hashes generated: %d\n", hashcounts);
        }

        if (hashcounts > 0 && sense > 0)
        {
            //append the hashed text to file1

            //  Because there are probably retained hashes of the
            //  file, we need to force an unmap-by-name which will allow a remap
            //  with the new file length later on.
            crm_force_munmap_filename(file1);

            if (user_trace)
                fprintf(stderr, "Opening a svm file %s for append.\n", file1);
            hashf = fopen(file1, "ab+");
            if (hashf == NULL)
            {
                fatalerror("For some reason, I was unable to append-open the svm file named ",
                        file1);
                return 0;
            }
            else
            {
                int ret;

                if (user_trace)
                {
                    fprintf(stderr, "Writing to a svm file %s\n", file1);
                }

                //     And make sure the file pointer is at EOF.
                (void)fseek(hashf, 0, SEEK_END);

                ret = 0;
                if (ftell(hashf) == 0)
                {
                    CRM_PORTA_HEADER_INFO classifier_info = { 0 };

                    classifier_info.classifier_bits = CRM_SVM;
		classifier_info.hash_version_in_use = selected_hashfunction;

                    if (0 != fwrite_crm_headerblock(hashf, &classifier_info, NULL))
                    {
                        fatalerror("For some reason, I was unable to write the header to the svm file named ",
                                file1);
                    }
                }

                //    and write the sorted hashes out.
                ret = fwrite(hashes, sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
                        hashcounts + 1,
                        hashf);
                if (ret != hashcounts + 1)
                {
                    fatalerror("For some reason, I was unable to append the feature to the svm file named ",
                            file1);
                }
                fclose(hashf);
            }
        }
        /////////////////////////////////////////////////////////////////////
        //     Start refuting........
        //     What we have to do here is find the set of hashes that matches
        //     the input most closely - and then remove it.
        //
        //     For this, we want the single closest set of hashes.  That
        //     implies highest radiance (see the hyperspace classifier for
        //     details on radiance), so we use the same bit of code
        //     we use down in classification.  We also keep start and
        //     end of the "best match" segment.
        ////////////////////////////////////////////////////////////////////
        if (hashcounts > 0 && sense < 0)
        {
            int beststart, bestend;
            int thisstart, thislen, thisend;
            double bestrad;
            int wrapup;
            double kandu, unotk, knotu, dist, radiance;
            int k, u;
            int file_hashlens;
            HYPERSPACE_FEATUREBUCKET_STRUCT *file_hashes;

            //   Get the file mmapped so we can find the closest match
            //

            struct stat statbuf;    //  for statting the hash file

            //             stat the file to get it's length
            k = stat(file1, &statbuf);

            //              does the file really exist?
            if (k != 0)
            {
                nonfatalerror("Refuting from nonexistent data cannot be done!"
                              " More specifically, this data file doesn't exist: ",
                        file1);
                return 0;
            }
            else
            {
                file_hashlens = statbuf.st_size;
                file_hashes = crm_mmap_file(file1,
                        0,
                        file_hashlens,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        CRM_MADV_RANDOM,
                        &file_hashlens);
                file_hashlens = file_hashlens
                                / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
            }
            wrapup = 0;

            k = u = 0;
            beststart = bestend = 0;
            bestrad = 0.0;
            while (k < file_hashlens)
            {
                int cmp;
                //   Except on the first iteration, we're looking one cell
                //   past the 0x0 start marker.
                kandu = 0;
                knotu = unotk = 10;
                u = 0;
                thisstart = k;
                if (internal_trace)
                    fprintf(stderr,
                            "At featstart, looking at 0x%08lX (next bucket value is 0x%08lX)\n",
                            (unsigned long int)file_hashes[thisstart].hash,
                            (unsigned long int)file_hashes[thisstart + 1].hash);
                while (wrapup == 0)
                {
                    //    it's an in-class feature.
                    // int cmp = hash_compare(&hashes[u], &file_hashes[k]);
                    if (hashes[u].hash < file_hashes[k].hash)
                    {
                        // unknown less, step u forward
                        //   increment on u, because maybe k will match next time
                        unotk++;
                        u++;
                    }
                    else if (hashes[u].hash > file_hashes[k].hash)
                    {
                        // unknown is greater, step k forward
                        //  increment on k, because maybe u will match next time.
                        knotu++;
                        k++;
                    }
                    else
                    {
                        // features matched.
                        //   These aren't the features you're looking for.
                        //   Move along, move along....
                        u++;
                        k++;
                        kandu++;
                    }
                    //   End of the U's?  If so, skip k to the end marker
                    //    and finish.
                    if (u >= hashcounts)
                    {
                        while (k < file_hashlens
                               && file_hashes[k].hash != 0)
                        {
                            k++;
                            knotu++;
                        }
                    }
                    //   End of the K's?  If so, skip U to the end marker
                    if (k >= file_hashlens - 1
                        || file_hashes[k].hash == 0)   //  end of doc features
                    {
                        unotk += hashcounts - u;
                    }

                    //  end of the U's or end of the K's?  If so, end document.
                    if (u >= hashcounts
                        || k >= file_hashlens - 1
                        || file_hashes[k].hash == 0) // this sets end-of-document
                    {
                        wrapup = 1;
                        k++;
                    }
                }
                //  Now the per-document wrapup...
                wrapup = 0;                   // reset wrapup for next file

                //   drop our markers for this particular document.  We are now
                //   looking at the next 0 (or end of file).
                thisend = k - 1;
                thislen = thisend - thisstart + 1;
                if (internal_trace)
                    fprintf(stderr,
                            "At featend, looking at 0x%08lX (next bucket value is 0x%08lX)\n",
                            (unsigned long int)file_hashes[thisend].hash,
                            (unsigned long int)file_hashes[thisend + 1].hash);

                //  end of a document- process accumulations

                //    Proper pythagorean (Euclidean) distance - best in
                //   SpamConf 2006 paper
                dist = sqrt(unotk + knotu);

                // PREV RELEASE VER --> radiance = 1.0 / ((dist * dist )+ 1.0);
                //
                //  This formula was the best found in the MIT `SC 2006 paper.
                radiance = 1.0 / ((dist * dist) + .000001);
                radiance = radiance * kandu;
                radiance = radiance * kandu;

                if (user_trace)
                    fprintf(stderr, "Feature Radiance %f at %d to %d\n",
                            radiance, thisstart, thisend);
                if (radiance >= bestrad)
                {
                    beststart = thisstart;
                    bestend = thisend;
                    bestrad = radiance;
                }
            }
            //  end of the per-document stuff - now chop out the part of the
            //  file between beststart and bestend.

            if (user_trace)
                fprintf(stderr,
                        "Deleting feature from %d to %d (rad %f) of file %s\n",
                        beststart, bestend, bestrad, file1);

            //   Deletion time - move the remaining stuff in the file
            //   up to fill the hole, then msync the file, munmap it, and
            //   then truncate it to the new, correct length.
            {
                int newhashlen, newhashlenbytes;
                newhashlen = file_hashlens - (bestend + 1 - beststart);
                newhashlenbytes = newhashlen
                                  * sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
                memmove(&file_hashes[beststart],
                        &file_hashes[bestend + 1],
                        sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT)
                        * (file_hashlens - bestend));
                crm_force_munmap_filename(file1);

                if (internal_trace)
                    fprintf(stderr,
                            "Truncating file to %d cells ( %d bytes)\n",
                            newhashlen,
                            newhashlenbytes);
                k = truncate(file1,
                        newhashlenbytes);
            }
        }
    }
    //  let go of the hashes.
    free(hashes);
    if (sense < 0)
    {
        // finish refuting....
        return 0;
    }

    //           extract parameters for svm
    crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s2start, apb->s2len);
    plen = apb->s2len;
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);
    if (plen)
    {
        //set default parameters for SVM
        param.svm_type = C_SVC;
        param.kernel_type = LINEAR;
        param.cache_size = 100;  // MB
        param.eps = 1e-3;
        param.C = 1;
        param.nu = 0.5;
        param.max_run_time = 1;
        param.shrinking = 1; //not available right now

        if (8 != sscanf(ptext,
                    "%d %d %lf %lf %lf %lf %lf %d",
                    &(param.svm_type),
                    &(param.kernel_type),
                    &(param.cache_size),
                    &(param.eps),
                    &(param.C),
                    &(param.nu),
                    &(param.max_run_time),
                    &(param.shrinking)))
        {
            nonfatalerror("Failed to decode the 8 SVM setup parameters [learn]: ", ptext);
        }
    }
    else
    {
        //set default parameters for SVM
        param.svm_type = C_SVC;
        param.kernel_type = LINEAR;
        param.cache_size = 100;  // MB
        param.eps = 1e-3;
        param.C = 1;
        param.nu = 0.5;
        param.max_run_time = 1;
        param.shrinking = 1;     //  not available right now
    }

    //if svm_type is ONE_CLASS, then do one class svm
    if (param.svm_type == ONE_CLASS)
    {
        int file1_hashlens;
        HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
        int k1;
        k1 = stat(file1, &statbuf1);
        if (k1 != 0)
        {
            nonfatalerror("Refuting from nonexistent data cannot be done!"
                          "More specifically, it should initialize a new .svm file: ",
                    file1);
            return 0;
        }
        else
        {
            int *y = NULL;
            HYPERSPACE_FEATUREBUCKET_STRUCT **x = NULL;

            file1_hashlens = statbuf1.st_size;
            file1_hashes = crm_mmap_file(file1,
                    0,
                    file1_hashlens,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    CRM_MADV_RANDOM,
                    &file1_hashlens);
            file1_hashlens = file1_hashlens
                             / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
            //find out how many documents in file1
            for (i = 0; i < file1_hashlens; i++)
            {
                if (internal_trace)
                {
                    fprintf(stderr,
                            "\nThe %dth hash value in file1 is 0x%08lX",
                            i, (unsigned long int)file1_hashes[i].hash);
                }
                if (file1_hashes[i].hash == 0)
                {
                    k1++;
                }
            }
            if (internal_trace)
                fprintf(stderr,
                        "\nThe total number of documents in file1 is %u\n",
                        k1);

            //initialize the svm_prob.x, svm_prob.y
            svm_prob.l = k1;
            x = calloc(svm_prob.l, sizeof(x[0]));
            y = calloc(svm_prob.l, sizeof(y[0]));
            for (i = 0; i < k1; i++)
            {
                y[i] = 1;
            }
            svm_prob.y = y;
            x[0] = &(file1_hashes[0]);
            k = 1;
            for (i = 1; i < file1_hashlens - 1; i++)
            {
                if (file1_hashes[i].hash == 0)
                {
                    CRM_ASSERT(k < svm_prob.l);
                    x[k++] = &(file1_hashes[i + 1]);
                    CRM_ASSERT(x[k - 1] != NULL);
                }
            }
            svm_prob.x = x;
            if (internal_trace)
            {
                for (i = 0; i < k; i++)
                {
                    fprintf(stderr, "\nx[%d]=0x%08lX\n", i, (unsigned long int)x[i][1].hash);
                }
            }
            Q_init();
            solve(); //result is in solver

            //free cache
            cache_free(&svmcache);
        }
    }


    // If file2 is not empty, open file1 and file2, calculate hyperplane,
    // and write the solution to file3
    if (file2[0] != 0 && file3[0] != 0)
    {
        int file1_hashlens;
        HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
        int file2_hashlens;
        HYPERSPACE_FEATUREBUCKET_STRUCT *file2_hashes;
        int k1, k2;

        k1 = stat(file1, &statbuf1);
        k2 = stat(file2, &statbuf2);
        if (k1 != 0)
        {
            nonfatalerror("Sorry, there is not enough data to calculate the hyperplane ",
                    file1);
            return 0;
        }
        else if (k2 != 0)
        {
            nonfatalerror("Sorry, there is not enough data to calculate the hyperplane ",
                    file2);
            return 0;
        }
        else
        {
            k1 = 0;
            k2 = 0;
            file1_hashlens = statbuf1.st_size;
            file1_hashes = crm_mmap_file(file1,
                    0,
                    file1_hashlens,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    CRM_MADV_RANDOM,
                    &file1_hashlens);
            file1_hashlens = file1_hashlens
                             / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
            file2_hashlens = statbuf2.st_size;
            file2_hashes = crm_mmap_file(file2,
                    0,
                    file2_hashlens,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    CRM_MADV_RANDOM,
                    &file2_hashlens);
            file2_hashlens = file2_hashlens
                             / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);

            for (i = 0; i < file1_hashlens; i++)
            {
                if (internal_trace)
                    fprintf(stderr,
                            "\nThe %dth hash value in file1 is 0x%08lX",
                            i, (unsigned long int)file1_hashes[i].hash);
                if (file1_hashes[i].hash == 0)
                {
                    k1++;
                }
            }
            if (user_trace)
            {
                fprintf(stderr,
                        "\nThe total number of documents in file1 is %u\n", k1);
            }

            for (i = 0; i < file2_hashlens; i++)
            {
                if (internal_trace)
                    fprintf(stderr,
                            "\nThe %dth hash value in file2 is 0x%08lX",
                            i, (unsigned long int)file2_hashes[i].hash);
                if (file2_hashes[i].hash == 0)
                {
                    k2++;
                }
            }
            if (user_trace)
            {
                fprintf(stderr,
                        "\nThe total number of documents in file2 is %u\n", k2);
            }

            if (!(k1 > 0 && k2 > 0))
            {
                if (user_trace)
                    fprintf(stderr,
                            "There hasn't enough documents to calculate a svm hyperplane!\n");
            }
            else
            {
                //initialize the svm_prob.x, svm_prob.y
                int *y = NULL;
                double b;
                double *deci_array = NULL;
                double AB[2];
                HYPERSPACE_FEATUREBUCKET_STRUCT **x = NULL;

                svm_prob.l = k1 + k2;
                x = calloc(svm_prob.l, sizeof(x[0]));
                y = calloc(svm_prob.l, sizeof(y[0]));
                for (i = 0; i < k1; i++)
                {
                    y[i] = 1;
                }
                for (i = k1; i < svm_prob.l; i++)
                {
                    y[i] = -1;
                }
                svm_prob.y = y;
                x[0] = &(file1_hashes[0]);
                k = 1;
                for (i = 1; i < file1_hashlens - 1; i++)
                {
                    if (file1_hashes[i].hash == 0)
                    {
                        CRM_ASSERT(k < svm_prob.l);
                        x[k++] = &(file1_hashes[i + 1]);
                        CRM_ASSERT(x[k - 1] != NULL);
                    }
                }
                CRM_ASSERT(k < svm_prob.l);
                x[k++] = &(file2_hashes[0]);
                CRM_ASSERT(x[k - 1] != NULL);
                for (i = 1; i < file2_hashlens - 1; i++)
                {
                    if ((file2_hashes[i].hash == 0)
                        && (file2_hashes[i + 1].hash != 0))   /* [i_a] shouldn't this be here too? */
                    {
                        CRM_ASSERT(k < svm_prob.l);
                        x[k++] = &(file2_hashes[i + 1]);
                        CRM_ASSERT(x[k - 1] != NULL);
                    }
                }
                svm_prob.x = x;
                if (internal_trace)
                {
                    for (i = 0; i < k; i++)
                    {
                        fprintf(stderr, "\nx[%d]=0x%08lX\n", i, (unsigned long int)x[i][1].hash);
                    }
                }
                Q_init();
                solve(); //result is in solver
                b = calc_b();

                //compute decision values for all training documents
                deci_array = (double *)calloc(svm_prob.l, sizeof(deci_array[0]));
                for (i = 0; i < svm_prob.l; i++)
                {
                    deci_array[i] = calc_decision(svm_prob.x[i], solver.alpha, b);
                }
                if (internal_trace)
                    fprintf(stderr, "done********\n");
                calc_AB(AB, deci_array, k1, k2);
                end_timer = time(NULL);
                run_time = difftime(end_timer, start_timer);
                if (user_trace)
                    fprintf(stderr, "run_time =  %f seconds\n", run_time);

                //IF MICROGROOMING IS ENABLED, WE'LL GET RID OF LESS THAN
                //10% DOCUMENTS THAT ARE FAR AWAY FROM THE HYPERPLANE
                //(WITH HIGH ABSOLUTE DECISION VALUE). HERE HAVE TWO
                //PARAMETERS YOU CAN CONTROL, ONE IS DELETE_FRACTION, THE
                //OTHER IS THE HOW FAR AWAY FROM THE HYPERPLANE. DEFAULT
                //SET DELETE_FRACTION=5% AND IF |DISTANCE| > 1.2 * AVERAGE
                //DISTANCE, THEN IT IS FAR AWAY FROM THE hyperplane.
                //
                if (microgroom && (run_time > param.max_run_time))
                {
                    double distance_fraction = 1.2;
                    int *id_desc;
                    //int *id_asc;
                    double average1 = 0.0, average2 = 0.0;
                    double delete_fraction;
                    int delete_num1 = 0, delete_num2 = 0;

                    id_desc = calloc(svm_prob.l, sizeof(id_desc[0]));
                    //id_asc = calloc(svm_prob.l, sizeof(id_asc[0]));
                    if (user_trace)
                        fprintf(stderr, "\nStart microgrooming......\n");
                    CRM_ASSERT(solver.deci_array == NULL);
                    solver.deci_array = deci_array;

                    // set an upper bound of delete fraction
                    delete_fraction = pow(param.max_run_time / run_time,
                            1.0 / 3.0);
                    for (i = 0; i < svm_prob.l; i++)
                    {
                        id_desc[i] = i;
                        if (i < k1)
                        {
                            average1 += solver.deci_array[i];
                        }
                        else
                            average2 += solver.deci_array[i];
                    }
                    average1 /= k1;
                    average2 /= k2;
                    QSORT(int, id_desc, svm_prob.l,
                            descend_int_decision_compare);
                    if (user_trace)
                        fprintf(stderr,
                                "average1=%f\taverage2=%f\n", average1, average2);

                    //  if decision[i] > 1.5 * average decision value, then
                    //  get rid of it.
                    i = 0;
                    j = svm_prob.l - 1;
                    while (((delete_num1 + delete_num2)
                            < floor(delete_fraction * svm_prob.l))
                           && (i < svm_prob.l) && (j >= 0))
                    {
                        if ((k1 - delete_num1) > (k2 - delete_num2))
                        {
                            //delete the farest document from the first class
                            CRM_ASSERT(id_desc[i] < svm_prob.l);
                            CRM_ASSERT(id_desc[i] >= 0);
                            if (svm_prob.y[id_desc[i]] == 1
                                && fabs(solver.deci_array[id_desc[i]])
                                > distance_fraction * fabs(average1))
                            {
                                if (user_trace)
                                    fprintf(stderr,
                                            "delete document %d! decision value=%f\n",
                                            id_desc[i], solver.deci_array[id_desc[i]]);
                                id_desc[i] = -1;
                                delete_num1++;
                            }
                            i++;
                        }
                        else if ((k1 - delete_num1) < (k2 - delete_num2))
                        {
                            //delete the farest document from the second class
                            CRM_ASSERT(id_desc[j] < svm_prob.l);
                            CRM_ASSERT(id_desc[j] >= 0);
                            if (svm_prob.y[id_desc[j]] == -1
                                && fabs(solver.deci_array[id_desc[j]]) > distance_fraction * fabs(average2))
                            {
                                if (user_trace)
                                    fprintf(stderr,
                                            "delete document %d! decision value=%f\n",
                                            id_desc[j], solver.deci_array[id_desc[j]]);
                                id_desc[j] = -1;
                                delete_num2++;
                            }
                            j--;
                        }
                        else
                        {
                            //delete the farest document from both classes
                            CRM_ASSERT(id_desc[i] < svm_prob.l);
                            CRM_ASSERT(id_desc[i] >= 0);
                            if (svm_prob.y[id_desc[i]] == 1
                                && fabs(solver.deci_array[id_desc[i]]) > distance_fraction * fabs(average1))
                            {
                                if (user_trace)
                                    fprintf(stderr,
                                            "delete document %d! decision value=%f\n",
                                            id_desc[i], solver.deci_array[id_desc[i]]);
                                id_desc[i] = -1;
                                delete_num1++;
                            }
                            i++;
                            CRM_ASSERT(id_desc[j] < svm_prob.l);
                            CRM_ASSERT(id_desc[j] >= 0);
                            if (svm_prob.y[id_desc[j]] == -1
                                && fabs(solver.deci_array[id_desc[j]]) > distance_fraction * fabs(average2))
                            {
                                if (user_trace)
                                    fprintf(stderr,
                                            "delete document %d! decision value=%f\n",
                                            id_desc[j], solver.deci_array[id_desc[j]]);
                                id_desc[j] = -1;
                                delete_num2++;
                            }
                            j--;
                        }
                    }
                    if (user_trace)
                        fprintf(stderr,
                                "delete_num1 = %d\t delete_num2 = %d\n",
                                delete_num1, delete_num2);

                    if (delete_num1 != 0 || delete_num2 != 0)
                    {
                        HYPERSPACE_FEATUREBUCKET_STRUCT **new_x = calloc((k1 + k2 - delete_num1 - delete_num2), sizeof(new_x[0]));

                        QSORT(int, id_desc, svm_prob.l, int_compare);
                        //now start deleting documents and write the
                        //remain documents to file1, this is unrecoverable.
                        j = 0;
                        for (i = 0; i < svm_prob.l; i++)
                        {
                            if ((id_desc[i] != -1) && (id_desc[i] < k1))
                            {
                                int temp_i;
                                int temp_count = 0;
                                CRM_ASSERT(id_desc[i] < svm_prob.l);
                                CRM_ASSERT(id_desc[i] >= 0);
                                //newalpha[j] = solver.alpha[id_desc[i]];
                                svm_prob.y[j] = svm_prob.y[id_desc[i]];
                                while (svm_prob.x[id_desc[i]][temp_count].hash != 0)
                                    temp_count++;
                                CRM_ASSERT(j < (k1 + k2 - delete_num1 - delete_num2));
                                new_x[j] = (HYPERSPACE_FEATUREBUCKET_STRUCT *)calloc((temp_count + 1),
                                        sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT));
                                for (temp_i = 0; temp_i < temp_count; temp_i++)
                                    new_x[j][temp_i] = svm_prob.x[id_desc[i]][temp_i];
                                new_x[j][temp_count].hash = 0;
                                j++;
                            }
                            else if ((id_desc[i] != -1) && (id_desc[i] >= k1))
                            {
                                int temp_count = 0;
                                int temp_i;
                                CRM_ASSERT(id_desc[i] < svm_prob.l);
                                CRM_ASSERT(id_desc[i] >= 0);
                                //newalpha[j] = solver.alpha[id_desc[i]];
                                svm_prob.y[j] = svm_prob.y[id_desc[i]];
                                while (svm_prob.x[id_desc[i]][temp_count].hash != 0)
                                    temp_count++;
                                CRM_ASSERT(j < (k1 + k2 - delete_num1 - delete_num2));
                                new_x[j] = (HYPERSPACE_FEATUREBUCKET_STRUCT *)calloc((temp_count + 1),
                                        sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT));
                                for (temp_i = 0; temp_i < temp_count; temp_i++)
                                    new_x[j][temp_i] = svm_prob.x[id_desc[i]][temp_i];
                                new_x[j][temp_count].hash = 0;
                                j++;
                            }
                        }
                        svm_prob.l = k1 + k2 - delete_num1 - delete_num2;
                        svm_prob.x = new_x;
                        if (delete_num1 != 0)
                        {
                            crm_force_munmap_filename(file1);
                            if (user_trace)
                                fprintf(stderr,
                                        "Opening a svm file %s for rewriting.\n",
                                        file1);
                            hashf = fopen(file1, "wb+");
                            if (hashf == 0)
                            {
                                fatalerror("For some reason, I was unable to write-open the svm file named ",
                                        file1);
                            }
                            else
                            {
                                if (user_trace)
                                {
                                    fprintf(stderr,
                                            "Writing to a svm file %s\n",
                                            file1);
                                }

                                //     And make sure the file pointer is at EOF.
                                (void)fseek(hashf, 0, SEEK_END);

                                if (ftell(hashf) == 0)
                                {
                                    CRM_PORTA_HEADER_INFO classifier_info = { 0 };

                                    classifier_info.classifier_bits = CRM_SVM;
		classifier_info.hash_version_in_use = selected_hashfunction;

                                    if (0 != fwrite_crm_headerblock(hashf, &classifier_info, NULL))
                                    {
                                        fatalerror("For some reason, I was unable to write the header to the svm file named ",
                                                file1);
                                    }
                                }

                                for (i = 0; i < k1 - delete_num1; i++)
                                {
                                    int temp_count = 0;
                                    int ret;

                                    while (svm_prob.x[i][temp_count].hash != 0)
                                    {
                                        temp_count++;
                                    }
                                    ret = fwrite(svm_prob.x[i], sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
                                            (temp_count + 1),
                                            hashf);
                                    if (ret != temp_count + 1)
                                    {
                                        fatalerror("For some reason, I was unable to write a feature bucket to the svm file named ",
                                                file1);
                                    }
                                }
                                fclose(hashf);
                            }
                        }
                        if (delete_num2 != 0)
                        {
                            crm_force_munmap_filename(file2);
                            if (user_trace)
                                fprintf(stderr,
                                        "Opening a svm file %s for rewriting.\n",
                                        file2);
                            hashf = fopen(file2, "wb+");
                            if (hashf == 0)
                            {
                                fatalerror("For some reason, I was unable to append-open the svm file named ",
                                        file2);
                            }
                            else
                            {
                                if (user_trace)
                                {
                                    fprintf(stderr,
                                            "Writing to a svm file %s\n",
                                            file2);
                                }

                                //     And make sure the file pointer is at EOF.
                                (void)fseek(hashf, 0, SEEK_END);

                                if (ftell(hashf) == 0)
                                {
                                    CRM_PORTA_HEADER_INFO classifier_info = { 0 };

                                    classifier_info.classifier_bits = CRM_SVM;
		classifier_info.hash_version_in_use = selected_hashfunction;

                                    if (0 != fwrite_crm_headerblock(hashf, &classifier_info, NULL))
                                    {
                                        fatalerror("For some reason, I was unable to write the header to the svm file named ",
                                                file2);
                                    }
                                }

                                for (i = k1 - delete_num1; i < svm_prob.l; i++)
                                {
                                    int temp_count = 0;
                                    int ret;

                                    while (svm_prob.x[i][temp_count].hash != 0)
                                        temp_count++;
                                    ret = fwrite(svm_prob.x[i], sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
                                            (temp_count + 1),
                                            hashf);
                                    if (ret != temp_count + 1)
                                    {
                                        fatalerror("For some reason, I was unable to append a feature bucket to the svm file named ",
                                                file2);
                                    }
                                }
                                fclose(hashf);
                            }
                        }

                        k1 -= delete_num1;
                        k2 -= delete_num2;

                        //recalculate the hyperplane
                        //
                        //free cache
                        cache_free(&svmcache);
                        if (user_trace)
                            fprintf(stderr, "recalculate the hyperplane!\n");
                        //recalculate the hyperplane
                        Q_init();
                        solve(); //  result is in solver
                        b = calc_b();
                        if (internal_trace)
                            fprintf(stderr, "b=%f\n", b);
                        if (user_trace)
                            fprintf(stderr,
                                    "Finishing calculate the hyperplane\n");

                        //compute A,B for sigmoid prediction
                        deci_array = (double *)realloc(deci_array, svm_prob.l
                                * sizeof(double));
                        for (i = 0; i < svm_prob.l; i++)
                        {
                            deci_array[i] = calc_decision(svm_prob.x[i],
                                    solver.alpha, b);
                        }
                        calc_AB(AB, deci_array, k1, k2);
                        if (internal_trace)
                            fprintf(stderr, "Finished calculating probability parameter\n");
                        for (i = 0; i < svm_prob.l; i++)
                        {
                            CRM_ASSERT(new_x[i] != NULL);
                            free(new_x[i]);
                        }
                    }
                    free(id_desc);
                }
                free(deci_array);
                solver.deci_array = NULL;

                //  write solver to file3
                if (user_trace)
                {
                    fprintf(stderr,
                            "Opening a solution file %s for writing alpha and b.\n",
                            file3);
                }
                hashf = fopen(file3, "wb+"); /* [i_a] on MSwin/DOS, fopen() opens in CRLF text mode by default; this will corrupt those binary values! */
                if (hashf == NULL)
                {
                    nonfatalerror("Couldn't write to .hypsvm file",
                            file3);
                    return 0;
                }
                else
                {
                    int ret;

                    if (user_trace)
                        fprintf(stderr, "Writing to a svm solution file %s\n", file3);

                    //     And make sure the file pointer is at EOF.
                    (void)fseek(hashf, 0, SEEK_END);

                    ret = 0;
                    if (ftell(hashf) == 0)
                    {
                        CRM_PORTA_HEADER_INFO classifier_info = { 0 };

                        classifier_info.classifier_bits = CRM_SVM;
		classifier_info.hash_version_in_use = selected_hashfunction;

                        if (0 != fwrite_crm_headerblock(hashf, &classifier_info, NULL))
                        {
                            nonfatalerror("Couldn't write the header to the .hypsvm file named ",
                                    file3);
                        }
                    }

                    ret = fwrite(&k1, sizeof(k1), 1, hashf);
                    ret += fwrite(&k2, sizeof(k2), 1, hashf);
                    for (i = 0; i < svm_prob.l; i++)
                        ret += fwrite(&(solver.alpha[i]), sizeof(solver.alpha[i]), 1, hashf);
                    ret += fwrite(&b, sizeof(b), 1, hashf);
                    ret += fwrite(&AB[0], sizeof(AB[0]), 1, hashf);
                    ret += fwrite(&AB[1], sizeof(AB[1]), 1, hashf);
                    if (ret != 2 + 3 + svm_prob.l)
                    {
                        fatalerror("Couldn't write the solution to the .hypsvm file named ",
                                file3);
                    }

                    fclose(hashf);
                }

                //free cache
                cache_free(&svmcache);
                free(solver.G);
                solver.G = NULL;
                free(DiagQ);
                DiagQ = NULL;
                free(solver.alpha);
                solver.alpha = NULL;
                free(x);
                x = NULL;
                free(y);
                y = NULL;
                if (user_trace)
                {
                    fprintf(stderr,
                            "Finish calculating SVM hyperplane, store the solution to %s!\n",
                            file3);
                }
            } //end if two svm files are not empty

            crm_force_munmap_filename(file1);
            crm_force_munmap_filename(file2);
            crm_force_munmap_filename(file3);
        }
    } //end if(file2[0])
regcomp_failed:
    return 0;
}


int crm_expr_svm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    int i, j, k, h;
    char ftext[MAX_PATTERN];
    int flen;
    char ptext[MAX_PATTERN]; //the regrex pattern
    int plen;
    char file1[MAX_PATTERN];
    char file2[MAX_PATTERN];
    char file3[MAX_PATTERN];
    regex_t regcb;
    regmatch_t match[5];
    int textoffset;
    int textmaxoffset;
    crmhash_t hashpipe[OSB_BAYES_WINDOW_LEN + 1];
    HYPERSPACE_FEATUREBUCKET_STRUCT *hashes; //  the hashes we'll sort
    int hashcounts;
    int cflags, eflags;
    int sense;
    int microgroom;
    int unique;
    int use_unigram_features;
    struct stat statbuf1;    //  for statting the hash file1
    struct stat statbuf2;    //  for statting the hash file2
    struct stat statbuf3;    //  for statting the hash file3
    double *alpha = NULL;
    double b;
    double AB[2];
    int slen;
    char svrbl[MAX_PATTERN]; //  the match statistics text buffer
    int svlen;
    //  the match statistics variable
    char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
    int stext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);
    double decision = 0;

    int totalfeatures = 0; //  total features
    int bestseen;
    double ptc[MAX_CLASSIFIERS]; // current running probability of this class
    int hashlens[MAX_CLASSIFIERS];
    char *hashname[MAX_CLASSIFIERS];
    int doc_num[MAX_CLASSIFIERS];

    //            extract the optional "match statistics" variable
    //
    crm_get_pgm_arg(svrbl, MAX_PATTERN, apb->p2start, apb->p2len);
    svlen = apb->p2len;
    svlen = crm_nexpandvar(svrbl, svlen, MAX_PATTERN);
    {
        int vstart, vlen;
        crm_nextword(svrbl, svlen, 0, &vstart, &vlen);
        memmove(svrbl, &svrbl[vstart], vlen);
        svlen = vlen;
        svrbl[vlen] = 0;
    }

    //     status variable's text (used for output stats)
    //
    stext[0] = 0;
    slen = 0;

    //            set our cflags, if needed.  The defaults are
    //            "case" and "affirm", (both zero valued).
    //            and "microgroom" disabled.
    cflags = REG_EXTENDED;
    eflags = 0;
    sense = +1;
    if (apb->sflags & CRM_NOCASE)
    {
        cflags = cflags | REG_ICASE;
        eflags = 1;
        if (user_trace)
            fprintf(stderr, "turning oncase-insensitive match\n");
    }
    ;
    if (apb->sflags & CRM_REFUTE)
    {
        sense = -sense;
        /////////////////////////////////////
        //    Take this out when we finally support refutation
        ////////////////////////////////////
        //      fprintf(stderr, "Hyperspace Refute is NOT SUPPORTED YET\n");
        //return (0);
        if (user_trace)
            fprintf(stderr, " refuting learning\n");
    }
    microgroom = 0;
    if (apb->sflags & CRM_MICROGROOM)
    {
        microgroom = 1;
        if (user_trace)
            fprintf(stderr, " enabling microgrooming.\n");
    }
    unique = 0;
    if (apb->sflags & CRM_UNIQUE)
    {
        unique = 1;
        if (user_trace)
            fprintf(stderr, " enabling uniqueifying features.\n");
    }

    use_unigram_features = 0;
    if (apb->sflags & CRM_UNIGRAM)
    {
        use_unigram_features = 1;
        if (user_trace)
            fprintf(stderr, " using only unigram features.\n");
    }

    //   Note that during a LEARN in hyperspace, we do NOT use the mmap of
    //    pre-existing memory.  We just write to the end of the file instead.
    //    malloc up the unsorted hashbucket space
    hashes = calloc(HYPERSPACE_MAX_FEATURE_COUNT,
            sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT));
    hashcounts = 0;

#if 01
    //           extract parameters for svm
    crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s2start, apb->s2len);
    plen = apb->s2len;
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);
    if (plen)
    {
        //set default parameters for SVM
        param.svm_type = C_SVC;
        param.kernel_type = LINEAR;
        param.cache_size = 100; //MB
        param.eps = 1e-3;
        param.C = 1;
        param.nu = 0.5;
        param.max_run_time = 1;     //second
        param.shrinking = 1;        //not available right now

        if (8 != sscanf(ptext,
                    "%d %d %lf %lf %lf %lf %lf %d",
                    &(param.svm_type),
                    &(param.kernel_type),
                    &(param.cache_size),
                    &(param.eps),
                    &(param.C),
                    &(param.nu),
                    &(param.max_run_time),
                    &(param.shrinking)))
        {
            nonfatalerror("Failed to decode the 8 SVM setup parameters [classify]: ", ptext);
        }
    }
    else
    {
        //set default parameters for SVM
        param.svm_type = C_SVC;
        param.kernel_type = LINEAR;
        param.cache_size = 100; //MB
        param.eps = 1e-3;
        param.C = 1;
        param.nu = 0.5;
        param.max_run_time = 1;               //second
        param.shrinking = 1;                  //not available right now
    }
#endif

    //     get the "this is a word" regex
    crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    plen = apb->s1len;
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);

    //   compile the word regex
    //
    if (internal_trace)
        fprintf(stderr, "\nWordmatch pattern is %s", ptext);

    i = crm_regcomp(&regcb, ptext, plen, cflags);
    if (i != 0)
    {
        crm_regerror(i, &regcb, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto regcomp_failed;
    }

    //   Now tokenize the input text
    //   We got txtptr, txtstart, and txtlen from the caller.
    //
    textoffset = txtstart;
    textmaxoffset = txtstart + txtlen;

    //   init the hashpipe with 0xDEADBEEF
    for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
    {
        hashpipe[h] = 0xDEADBEEF;
    }

    i = 0;
    j = 0;
    k = 0;
    //generate the sorted hashes of input text
    if (txtlen > 0)
    {
        CRM_ASSERT(hashcounts == 0);
        while (k == 0 && textoffset <= textmaxoffset
               && hashcounts < HYPERSPACE_MAX_FEATURE_COUNT)
        {
            int wlen;
            int slen = textmaxoffset - textoffset;
            // if pattern is empty, extract non graph delimited tokens
            // directly ([[graph]]+) instead of calling regexec  (8% faster)
            if (ptext[0] != 0)
            {
                k = crm_regexec(&regcb, &(txtptr[textoffset]),
                        slen, WIDTHOF(match), match, 0, NULL);
            }
            else
            {
                k = 0;
                //         skip non-graphical characters
                match[0].rm_so = 0;
                while (!crm_isgraph(txtptr[textoffset + match[0].rm_so])
                       && textoffset + match[0].rm_so < textmaxoffset)
                    match[0].rm_so++;
                match[0].rm_eo = match[0].rm_so;
                while (crm_isgraph(txtptr[textoffset + match[0].rm_eo])
                       && textoffset + match[0].rm_eo < textmaxoffset)
                    match[0].rm_eo++;
                if (match[0].rm_so == match[0].rm_eo)
                    k = 1;
            }
            if (!(k != 0 || textoffset > textmaxoffset))
            {
                wlen = match[0].rm_eo - match[0].rm_so;

                memmove(tempbuf,
                        &(txtptr[textoffset + match[0].rm_so]),
                        wlen);
                tempbuf[wlen] = 0;
                if (internal_trace)
                {
                    fprintf(stderr,
                            "Learn #%d t.o. %d strt %d end %d len %d is -%s-\n",
                            i,
                            textoffset,
                            (int)match[0].rm_so,
                            (int)match[0].rm_eo,
                            wlen,
                            tempbuf);
                }
                if (match[0].rm_eo == 0)
                {
                    nonfatalerror("The LEARN pattern matched zero length! ",
                            "\n Forcing an increment to avoid an infinite loop.");
                    match[0].rm_eo = 1;
                }
                //  slide previous hashes up 1
                for (h = OSB_BAYES_WINDOW_LEN - 1; h > 0; h--)
                {
                    hashpipe[h] = hashpipe[h - 1];
                }

                //  and put new hash into pipeline
                hashpipe[0] = strnhash(tempbuf, wlen);

                //  and account for the text used up.
                textoffset = textoffset + match[0].rm_eo;
                i++;
                //        is the pipe full enough to do the hashing?
                //  we init with 0xDEADBEEF, so the pipe is always full (i >=5)
                if (1)
                {
                    int j;
                    unsigned th = 0; //  a counter used only in TSS hashing
                    unsigned int hindex;
                    crmhash_t h1; //, h2;
                    //
                    th = 0;
                    //
                    if (use_unigram_features == 1)
                    {
                        h1 = hashpipe[0];
                        if (h1 == 0)
                            h1 = 0xdeadbeef;
                        if  (internal_trace)
                            fprintf(stderr, "Singleton feature : 0x%08lX\n", (unsigned long int)h1);
                        hashes[hashcounts].hash = h1;
                        hashcounts++;
                    }
                    else
                    {
                        for (j = 1;
                             j < OSB_BAYES_WINDOW_LEN; //OSB_BAYES_WINDOW_LEN;
                             j++)
                        {
                            h1 = hashpipe[0] * hctable[0]
                                 + hashpipe[j] * hctable[j << 1];
                            if (h1 == 0)
                                h1 = 0xdeadbeef;
                            hindex = h1;

                            if (internal_trace)
                                fprintf(stderr, "Polynomial %d has h1:0x%08lX\n",
                                        j, (unsigned long int)h1);

                            hashes[hashcounts].hash = h1;
                            hashcounts++;
                        }
                    }
                }
            }
            else
            {
                if (ptext[0] != 0)
                    crm_regfree(&regcb);
                k = 1;
            }
        }   //   end the while k==0
        //   Now sort the hashes array.
        //
        CRM_ASSERT(hashcounts >= 0);
        CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
        //mark the end of a feature vector
        hashes[hashcounts].hash = 0;

        QSORT(HYPERSPACE_FEATUREBUCKET_STRUCT, hashes, hashcounts,
                hash_compare);
        if (user_trace)
        {
            fprintf(stderr, "sorted hashes:\n");
            for (i = 0; i < hashcounts; i++)
            {
                fprintf(stderr, "hashes[%d]=0x%08lX\n", i, (unsigned long int)hashes[i].hash);
            }
            fprintf(stderr, "Total hashes generated: %d\n", hashcounts);
        }

        //   And uniqueify the hashes array
        //
        if (unique)
        {
            i = 0;
            j = 0;

            CRM_ASSERT(hashcounts >= 0);
            CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
            CRM_ASSERT(hashes[hashcounts].hash == 0);
            while (i < hashcounts)
            {
                if (hashes[i].hash != hashes[i + 1].hash)
                {
                    hashes[j] = hashes[i];
                    j++;
                }
                i++;
            }
            hashcounts = j;

            //mark the end of a feature vector
            hashes[hashcounts].hash = 0;
        }

        CRM_ASSERT(hashcounts >= 0);
        CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
        CRM_ASSERT(hashes[hashcounts].hash == 0);

        totalfeatures = hashcounts;
    }
    else
    {
        nonfatalerror("Sorry, but I can't classify the null string.", "");
        return 0;
    }

    if (user_trace)
    {
      fprintf(stderr,"sorted hashes:\n");
      for (i=0;i<hashcounts;i++)
	{
	  fprintf(stderr, "hashes[%d]=%lud\n",i,(unsigned long int)hashes[i].hash);
	}
        fprintf(stderr, "Total unique hashes generated: %d\n", hashcounts);
    }

    // extract the file names.( file1.svm | file2.svm | 1vs2_solver.svm )
    crm_get_pgm_arg(ftext, MAX_PATTERN, apb->p1start, apb->p1len);
    flen = apb->p1len;
    flen = crm_nexpandvar(ftext, flen, MAX_PATTERN);

    strcpy(
            ptext,
            "[[:space:]]*([[:graph:]]+)[[:space:]]*\\|[[:space:]]*([[:graph:]]+)[[:space:]]*\\|[[:space:]]*([[:graph:]]+)[[:space:]]*");
    plen = strlen(ptext);
    i = crm_regcomp(&regcb, ptext, plen, cflags);
    if (i != 0)
    {
        crm_regerror(i, &regcb, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto regcomp_failed;
    }
    k = crm_regexec(&regcb, ftext,
            flen, WIDTHOF(match), match, 0, NULL);
    if (k == 0)
    {
        int file1_hashlens;
        int file2_hashlens;
        int k1, k2, k3;
        HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
        HYPERSPACE_FEATUREBUCKET_STRUCT *file2_hashes;

        //get three input files.
        memmove(file1, &ftext[match[1].rm_so], (match[1].rm_eo - match[1].rm_so));
        file1[match[1].rm_eo - match[1].rm_so] = 0;
        memmove(file2, &ftext[match[2].rm_so], (match[2].rm_eo - match[2].rm_so));
        file2[match[2].rm_eo - match[2].rm_so] = 0;
        memmove(file3, &ftext[match[3].rm_so], (match[3].rm_eo - match[3].rm_so));
        file3[match[3].rm_eo - match[3].rm_so] = 0;
        if (internal_trace)
            fprintf(stderr, "file1=%s\tfile2=%s\tfile3=%s\n", file1, file2, file3);

        // open all files,
        // first check whether file3 is the current version solution.

        k1 = stat(file1, &statbuf1);
        k2 = stat(file2, &statbuf2);
        k3 = stat(file3, &statbuf3);

        if (k1 != 0)
        {
	  nonfatalerror ("Refuting from nonexistent data cannot be done!"
			 " More specifically, this data file doesn't exist: ",
			 file1);
            return 0;
        }
        else if (k2 != 0)
        {
      nonfatalerror ("Refuting from nonexistent data cannot be done!"
		     " More specifically, this data file doesn't exist: ",
		     file2);
            return 0;
        }
        else
        {
            FILE *hashf;

            k1 = 0;
            k2 = 0;

            file1_hashlens = statbuf1.st_size;
            crm_force_munmap_filename(file1);
            crm_force_munmap_filename(file2);

            file1_hashes = crm_mmap_file(file1,
                    0,
                    file1_hashlens,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    CRM_MADV_RANDOM,
                    &file1_hashlens);
            file1_hashlens = file1_hashlens
                             / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);

            hashlens[0] = file1_hashlens;
            hashname[0] = (char *)calloc((strlen(file1) + 10), sizeof(hashname[0][0]));
            if (!hashname[0])
                untrappableerror("Couldn't alloc hashname[0]\n",
                        "We need that part later, so we're stuck.  Sorry.");
            strcpy(hashname[0], file1);


            file2_hashlens = statbuf2.st_size;
            file2_hashes = crm_mmap_file(file2,
                    0,
                    file2_hashlens,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    CRM_MADV_RANDOM,
                    &file2_hashlens);
            file2_hashlens = file2_hashlens
                             / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
            hashlens[1] = file2_hashlens;
            hashname[1] = (char *)calloc(strlen(file2) + 10, sizeof(hashname[1][0]));
            if (!hashname[1])
                untrappableerror("Couldn't alloc hashname[1]\n",
                        "We need that part later, so we're stuck.  Sorry.");
            strcpy(hashname[1], file2);

            //find out how many documents in file1 and file2 separately
            for (i = 0; i < file1_hashlens; i++)
            {
                if (internal_trace)
                    fprintf(stderr,
                            "\nThe %dth hash value in file1 is 0x%08lX",
                            i, (unsigned long int)file1_hashes[i].hash);
                if (file1_hashes[i].hash == 0)
                {
                    k1++;
                }
            }
            if (user_trace)
                fprintf(stderr,
                        "\nThe total number of documents in file1 is %u\n", k1);

            for (i = 0; i < file2_hashlens; i++)
            {
                if (internal_trace)
                    fprintf(stderr,
                            "\nThe %dth hash value in file2 is 0x%08lX",
                            i, (unsigned long int)file2_hashes[i].hash);
                if (file2_hashes[i].hash == 0)
                {
                    k2++;
                }
            }

            if (user_trace)
                fprintf(stderr,
                        "\nThe total number of documents in file2 is %d\n", k2);

            hashf = fopen(file3, "rb+"); /* [i_a] on MSwin/DOS, fopen() opens in CRLF text mode by default; this will corrupt those binary values! */
            if (hashf == NULL)
            {
                nonfatalerror_ex(SRC_LOC(), "For some reason, I was unable to read-open the SVM 1vs2 solution file file named '%s': error = %d(%s)",
                        file3,
                    errno,
                    errno_descr(errno));
            }
            // else
            {
                int temp_k1 = 0, temp_k2 = 0;
                int *y = NULL;
                HYPERSPACE_FEATUREBUCKET_STRUCT **x = NULL;

                if (k3 == 0)
                {
                    if (hashf != NULL)
                    {
                        int ret;

                        if (is_crm_headered_file(hashf))
                        {
                            if (fseek(hashf, CRM114_HEADERBLOCK_SIZE, SEEK_SET))
                            {
                                fatalerror("For some reason, I was unable to skip the CRM header for the file named ",
                                        file3);
                            }
                        }

                        ret = fread(&temp_k1, sizeof(temp_k1), 1, hashf);
                        ret += fread(&temp_k2, sizeof(temp_k2), 1, hashf);
                        if (ret != 2)
                        {
                            fatalerror("Cannot read k1/k2 from SVM solution file: ", file3);
                        }
                    }
                    if (user_trace)
                        fprintf(stderr, "temp_k1=%d\ttemp_k2=%d\n", temp_k1, temp_k2);
                }
                doc_num[0] = k1;
                doc_num[1] = k2;
                //assign svm_prob.x, svm_prob.y
                svm_prob.l = k1 + k2;
                x = calloc(svm_prob.l, sizeof(x[0]));
                y = calloc(svm_prob.l, sizeof(y[0]));
                for (i = 0; i < k1; i++)
                {
                    y[i] = 1;
                }
                for (i = k1; i < svm_prob.l; i++)
                {
                    y[i] = -1;
                }
                svm_prob.y = y;
                x[0] = &(file1_hashes[0]);
                k = 1;
                for (i = 1; i < file1_hashlens - 1; i++)
                {
                    if (file1_hashes[i].hash == 0)
                    {
                        CRM_ASSERT(k < svm_prob.l);
                        x[k++] = &(file1_hashes[i + 1]);
                        CRM_ASSERT(x[k - 1] != NULL);
                    }
                }
                x[k++] = &(file2_hashes[0]);
                for (i = 1; i < file2_hashlens - 1; i++)
                {
                    if (file2_hashes[i].hash == 0)
                    {
                        CRM_ASSERT(k < svm_prob.l);
                        x[k++] = &(file2_hashes[i + 1]);
                        CRM_ASSERT(x[k - 1] != NULL);
                    }
                }
                svm_prob.x = x;

                alpha = (double *)calloc(svm_prob.l, sizeof(alpha[0]));

                if ((k3 != 0) || (temp_k1 != k1) || (temp_k2 != k2))
                {
                    if (user_trace)
                        fprintf(stderr,
                                "temp_k1=%d\ttemp_k2=%d\tSVM solution file is not up-to-date! we'll recalculate it!\n",
                                temp_k1, temp_k2);
                    //recalculate the svm solution
                    if ((k1 > 0) && (k2 > 0))
                    {
                        double *deci_array = NULL;
#if 0
                        //           extract parameters for svm
                        crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s2start, apb->s2len);
                        plen = apb->s2len;
                        plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);
                        if (plen)
                        {
                            //set default parameters for SVM
                            param.svm_type = C_SVC;
                            param.kernel_type = LINEAR;
                            param.cache_size = 100; //MB
                            param.eps = 1e-3;
                            param.C = 1;
                            param.nu = 0.5;
                            param.max_run_time = 1; //second
                            param.shrinking = 1;    //not available right now

                            if (8 != sscanf(ptext,
                                        "%d %d %lf %lf %lf %lf %lf %d",
                                        &(param.svm_type),
                                        &(param.kernel_type),
                                        &(param.cache_size),
                                        &(param.eps),
                                        &(param.C),
                                        &(param.nu),
                                        &(param.max_run_time),
                                        &(param.shrinking)))
                            {
                                nonfatalerror("Failed to decode the 8 SVM setup parameters [classify]: ", ptext);
                            }
                        }
                        else
                        {
                            //set default parameters for SVM
                            param.svm_type = C_SVC;
                            param.kernel_type = LINEAR;
                            param.cache_size = 100; //MB
                            param.eps = 1e-3;
                            param.C = 1;
                            param.nu = 0.5;
                            param.max_run_time = 1; //second
                            param.shrinking = 1;    //not available right now
                        }
#endif
                        if (internal_trace)
                        {
                            for (i = 0; i < k; i++)
                            {
                                fprintf(stderr,
                                        "\nx[%d]=0x%08lX\n", i, (unsigned long int)svm_prob.x[i][1].hash);
                            }
                        }
                        Q_init();
                        solve(); //result is in solver
                        b = calc_b();
                        if (internal_trace)
                        {
                            fprintf(stderr, "b=%f\n", b);
                        }
                        CRM_ASSERT(alpha != NULL);
                        CRM_ASSERT(svm_prob.l <= (k1 + k2));
                        for (i = 0; i < svm_prob.l; i++)
                            alpha[i] = solver.alpha[i];

                        //compute A,B for sigmoid prediction
                        deci_array = (double *)calloc(svm_prob.l, sizeof(deci_array[0]));
                        for (i = 0; i < svm_prob.l; i++)
                        {
                            deci_array[i] = calc_decision(svm_prob.x[i], alpha, b);
                        }
                        calc_AB(AB, deci_array, k1, k2);
                        //free cache
                        cache_free(&svmcache);
                        free(deci_array);
                        free(solver.G);
                        free(solver.alpha);
                        free(DiagQ);
                        deci_array = NULL;
                        solver.G = NULL;
                        solver.alpha = NULL;
                        DiagQ = NULL;
//                        free(x);
//                        free(y);
                        if (user_trace)
                            fprintf(stderr,
                                    "Recalculation of svm hyperplane is finished!\n");
                    }
                    else
                    {
                        if (user_trace)
                            fprintf(stderr,
                                    "There hasn't enough documents to recalculate a svm hyperplane!\n");
                        return 0;
                    }
                }
                else
                {
                    if (internal_trace)
                    {
                        for (i = 0; i < svm_prob.l; i++)
                        {
                            j = 0;
                            do
                            {
                                fprintf(stderr,
                                        "x[%d][%d]=0x%08lX\n",
                                        i, j, (unsigned long int)svm_prob.x[i][j].hash);
                            } while (svm_prob.x[i][j++].hash != 0);
                        }
                    }
                    b = 0.0;
                    AB[0] = 0.0;
                    AB[1] = 0.0;
                    if (hashf != NULL)
                    {
                        int ret = 0;

                        for (i = 0; i < svm_prob.l; i++)
                        {
                            alpha[i] = 0.0;
                            ret += fread(&alpha[i], sizeof(alpha[i]), 1, hashf);
                        }
                        ret += fread(&b, sizeof(b), 1, hashf);
                        ret += fread(&AB[0], sizeof(AB[0]), 1, hashf);
                        ret += fread(&AB[1], sizeof(AB[1]), 1, hashf);

                        if (ret != 3 + svm_prob.l)
                        {
                            fatalerror("Cannot read data from SVM solution file (is it corrupt?): ", file3);
                        }
                    }
                    else
                    {
                        // in case we can't load the data from file: initialize with zeroes.
                        for (i = 0; i < svm_prob.l; i++)
                        {
                            alpha[i] = 0.0;
                        }
                    }
                }

                decision = calc_decision(hashes, alpha, b);
                decision = sigmoid_predict(decision, AB[0], AB[1]);
                free(alpha);
                alpha = NULL;
                free(x);
                x = NULL;
                free(y);
                y = NULL;
            }
            if (hashf != NULL)
            {
                fclose(hashf);
            }
            crm_force_munmap_filename(file1);
            crm_force_munmap_filename(file2);
            crm_force_munmap_filename(file3);
        }   //end (k1==0 && k2 ==0)
    }       //end (k==0)
    else
    {
        nonfatalerror(
                "You need to input (file1.svm | file2.svm | f1vsf2.svmhyp)\n", "");
        return 0;
    }
    free(hashes);
    hashes = NULL;


    if (svlen > 0)
    {
        char buf[4096];
        double pr;
        char fname[MAX_FILE_NAME_LEN];
        buf[0] = 0;

        //   put in standard CRM114 result standard header:
        ptc[0] = decision;
        ptc[1] = 1 - decision;
        if (decision >= 0.5)
        {
            pr = 10 * (log10(decision + 1e-300) - log10(1.0 - decision + 1e-300));
            sprintf(buf,
                    "CLASSIFY succeeds; success probability: %6.4f  pR: %6.4f\n",
                    decision, pr);
            bestseen = 0;
        }
        else
        {
            pr = 10 * (log10(decision + 1e-300) - log10(1.0 - decision + 1e-300));
            sprintf(buf,
                    "CLASSIFY fails; success probability: %6.4f  pR: %6.4f\n",
                    decision, pr);
            bestseen = 1;
        }
        if (strlen(stext) + strlen(buf) <= stext_maxlen)
            strcat(stext, buf);

        //   Second line of the status report is the "best match" line:
        //
        if (bestseen)
            strncpy(fname, file2, MAX_FILE_NAME_LEN);
        else
            strncpy(fname, file1, MAX_FILE_NAME_LEN);
        fname[MAX_FILE_NAME_LEN - 1] = 0;
        snprintf(buf, WIDTHOF(buf), "Best match to file #%d (%s) "
                                    "prob: %6.4f  pR: %6.4f\n",
                bestseen,
                fname,
                ptc[bestseen],
                10 * (log10(ptc[bestseen] + 1e-300) - log10(1.0 - ptc[bestseen] + 1e-300)));
        buf[WIDTHOF(buf) - 1] = 0;
        if (strlen(stext) + strlen(buf) <= stext_maxlen)
            strcat(stext, buf);

        sprintf(buf, "Total features in input file: %d\n", totalfeatures);
        if (strlen(stext) + strlen(buf) <= stext_maxlen)
            strcat(stext, buf);
        for (k = 0; k < 2; k++)
        {
            snprintf(buf, WIDTHOF(buf),
                    "#%d (%s):"
                    " documents: %d, features: %d,  prob: %3.2e, pR: %6.2f\n",
                    k,
                    hashname[k],
                    doc_num[k],
                    hashlens[k],
                    ptc[k],
                    10 * (log10(ptc[k] + 1e-300) - log10(1.0 - ptc[k] + 1e-300)));
            buf[WIDTHOF(buf) - 1] = 0;

            if (strlen(stext) + strlen(buf) <= stext_maxlen)
                strcat(stext, buf);
        }

        for (k = 0; k < 2; k++)
        {
            free(hashname[k]);
        }

        //   finally, save the status output
        //
        crm_destructive_alter_nvariable(svrbl, svlen,
                stext, strlen(stext));
    }

    //    Return with the correct status, so an actual FAIL or not can occur.
    if (decision >= 0.5)
    {
        //   all done... if we got here, we should just continue execution
        if (user_trace)
            fprintf(stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
    }
    else
    {
        //  Classify was a fail.  Do the skip.
        if (user_trace)
            fprintf(stderr, "CLASSIFY was a FAIL, skipping forward.\n");
        //    and do what we do for a FAIL here
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        return 0;
    }

regcomp_failed:
    return 0;
}

#else /* CRM_WITHOUT_SVM */

int crm_expr_svm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}

#endif /* CRM_WITHOUT_SVM */




int crm_expr_svm_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}


int crm_expr_svm_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "SVM");
}

