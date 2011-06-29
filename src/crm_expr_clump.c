//  crm_expr_clump.c

//  by Joe Langeway derived from crm_bit_entropy.c and produced for the crm114 so:
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
//     crm_expr_clump.c - automatically cluster unlabelled documents.
//
//     Original spec by Bill Yerazunis, original code by Joe Langeway,
//     recode for CRM114 use by Bill Yerazunis.
//
//     This code section (crm_expr_clump and subsidiary routines) is
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
 * Here's what's going on:
 *
 * Documents are fed in with calls to "clump" and the distance between each
 * document is recorded in a matrix. We then find clusters for automatic
 * classification without the need for a gold standard judgement ahead of time.
 *
 * Cluster assignments start at index 1 and negative numbers indicate perma
 * assignments made by crm.
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



#if !defined (CRM_WITHOUT_CLUMP)



#define MAX_CLUSTERS 4096
#define CLUSTER_LABEL_LEN 32
#define DOCUMENT_TAG_LEN 32

typedef struct mythical_clumper_header
{
    int32_t max_documents;
    int32_t n_documents;
    int32_t document_offsets_offset; //list of offsets to documents
    int32_t clusters_offset;         //cluster assignments of documents
    int32_t distance_matrix_offset;
    int32_t cluster_labels_offset;
    int32_t document_tags_offset;
    int32_t n_perma_clusters;
    int32_t file_length; //is the offset of new files when learning
    int32_t last_action; //0 = made clumps, non-zero means make clumps if you've got
    // a chance and we're told not to
    int32_t n_clusters;
} CLUMPER_HEADER_STRUCT;

typedef struct mythical_clumper_state
{
    char                  *file_origin;
    CLUMPER_HEADER_STRUCT *header;
    int32_t               *document_offsets;
    int32_t               *cluster_assignments;
    float                 *distance_matrix;
    char (*cluster_labels)[CLUSTER_LABEL_LEN];
    char (*document_tags)[DOCUMENT_TAG_LEN];
} CLUMPER_STATE_STRUCT;

// int max_documents = 1000;

static int make_new_clumper_backing_file(char *filename, int max_docs)
{
    CLUMPER_HEADER_STRUCT h = { 0 };
    FILE *f;
    int i;
    CRM_PORTA_HEADER_INFO classifier_info = { 0 };

    h.max_documents = max_docs;
    h.n_documents = 0;
    h.document_offsets_offset = sizeof(h);
    h.clusters_offset = h.document_offsets_offset +
                        sizeof(int32_t) * h.max_documents;
    h.distance_matrix_offset = h.clusters_offset +
                               sizeof(int32_t) * h.max_documents;
    h.cluster_labels_offset = h.distance_matrix_offset +
                              (sizeof(float) * h.max_documents * (h.max_documents + 1) / 2);
    h.document_tags_offset = h.cluster_labels_offset +
                             (sizeof(char) * h.max_documents * CLUSTER_LABEL_LEN);
    h.file_length = h.document_tags_offset +
                    (sizeof(char) * h.max_documents * DOCUMENT_TAG_LEN);
    h.n_perma_clusters = 0;
    h.n_clusters = 0;
    crm_force_munmap_filename(filename);
    f = fopen(filename, "wb");
    if (!f)
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't open your clumper backing file %s for writing; errno=%d(%s)\n",
                filename,
                errno,
                errno_descr(errno));
        return 0;
    }

    classifier_info.classifier_bits = CRM_CLUMP;
		classifier_info.hash_version_in_use = selected_hashfunction;

    if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't write header to file %s; errno=%d(%s)\n",
                filename, errno, errno_descr(errno));
        fclose(f);
        return 0;
    }

    if (1 != fwrite(&h, sizeof(h), 1, f))
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't write header to file %s; errno=%d(%s)\n",
                filename, errno, errno_descr(errno));
        fclose(f);
        return 0;
    }

    i = h.file_length - sizeof(h);
    if (internal_trace)
    {
        fprintf(stderr, "about to write %d zeros to backing file\n", i);
    }
    if (file_memset(f, 0, i))
    {
        fatalerror_ex(SRC_LOC(),
                "\n Couldn't write filler to file %s; errno=%d(%s)\n",
                filename, errno, errno_descr(errno));
        fclose(f);
        return 0;
    }

    fclose(f);
    if (internal_trace)
    {
        fprintf(stderr, "Just wrote backing file for clumper size %d\n", (int)h.file_length);
    }
    return 1;
}

static int map_file(CLUMPER_STATE_STRUCT *s, char *filename)
{
    struct stat statbuf;

    memset(s, 0, sizeof(s));

    if (stat(filename, &statbuf))
    {
        nonfatalerror("Couldn't stat file!", filename);
        return -1;
    }

    s->file_origin = crm_mmap_file(filename,
            0,
            statbuf.st_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            CRM_MADV_RANDOM,
            NULL);
    if (s->file_origin == MAP_FAILED)
    {
        nonfatalerror("Couldn't mmap file!", filename);
        return -1;
    }
    if (internal_trace)
        fprintf(stderr, "Definitely think I've mapped a file.\n");

    s->header = (CLUMPER_HEADER_STRUCT *)(s->file_origin);
    s->document_offsets =
        (void *)(s->file_origin + s->header->document_offsets_offset);
    s->cluster_assignments =
        (void *)(s->file_origin + s->header->clusters_offset);
    s->distance_matrix =
        (void *)(s->file_origin + s->header->distance_matrix_offset);
    s->cluster_labels =
        (void *)(s->file_origin + s->header->cluster_labels_offset);
    s->document_tags =
        (void *)(s->file_origin + s->header->document_tags_offset);
    return 0;
}

static void unmap_file(CLUMPER_STATE_STRUCT *s)
{
    crm_munmap_file((void *)s->file_origin);
}

static float *aref_dist_mat(float *m, int j, int i)
{
    if (i < j)
    {
        int t = i;
        i = j;
        j = t;
    }
    return m + i * (i - 1) / 2 + j;
}

static float get_document_affinity(crmhash_t *doc1, crmhash_t *doc2)
{
    int u = 0, l1 = 0, l2 = 0;

    for ( ; ;)
    {
        if (doc1[l1] == 0)
        {
            while (doc2[l2] != 0)
                l2++;
            break;
        }
        else if (doc2[l2] == 0)
        {
            while (doc1[l1] != 0)
                l1++;
            break;
        }
        else if (doc1[l1] == doc2[l2])
        {
            u++;
            l1++;
            l2++;
        }
        else if (doc1[l1] < doc2[l2])
        {
            l1++;
        }
        else if (doc1[l1] > doc2[l2])
        {
            l2++;
        }
        else
        {
            fprintf(stderr, "panic in the disco!\n");
            break;
        }
    }
    if (internal_trace)
    {
        fprintf(stderr, "Just compared two documents u=%d l1=%d l2=%d\n", u, l1, l2);
    }
    return pow((1.0 + u * u) / (1.0 + l1 * l2), 0.2);
}

#if defined (CRM_WITHOUT_MJT_INLINED_QSORT)

static int compare_features(const void *a, const void *b)
{
    if (*(crmhash_t *)a < *(crmhash_t *)b)
        return -1;

    if (*(crmhash_t *)a > *(crmhash_t *)b)
        return 1;

    return 0;
}

#else

#define compare_features(a, b) \
    (*(crmhash_t *)(a) < *(crmhash_t *)(b))

#endif


static int eat_document(ARGPARSE_BLOCK *apb,
        char *text, int  text_len, int  *ate,
        regex_t *regee,
        crmhash_t *feature_space, int max_features,
        uint64_t flags)
{
    int n_features = 0;
    int i, j;
    crmhash_t hash_pipe[OSB_BAYES_WINDOW_LEN];
    crmhash_t hash_coefs[] = { 1, 3, 5, 11, 23, 47 };
    regmatch_t match[1];
    char *t_start;
    int t_len;
    int unigram, unique, string;

    unique = !!(apb->sflags & CRM_UNIQUE);
    unigram = !!(apb->sflags & CRM_UNIGRAM);
    string = !!(apb->sflags & CRM_STRING);

    if (string)
        unique = 1;

    *ate = 0;

    for (i = 0; i < OSB_BAYES_WINDOW_LEN; i++)
        hash_pipe[i] = 0xdeadbeef;
    while (text_len > 0 && n_features < max_features - 1)
    {
        if (crm_regexec(regee, text, text_len, 1, match, 0, NULL))
        {
            // no match or regex error, we're done
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
        if (unigram)
        {
            feature_space[n_features++] = hash_pipe[0];
        }
        else
        {
            for (i = 1; i < OSB_BAYES_WINDOW_LEN
                 && hash_pipe[i] != 0xdeadbeef
                 && n_features < max_features - 1; i++)
            {
                feature_space[n_features++] =
                    hash_pipe[0] + hash_pipe[i] * hash_coefs[i];
                CRM_ASSERT(n_features < max_features - 1);
            }
        }
    }
    CRM_ASSERT(n_features < max_features);
    CRM_ASSERT(sizeof(feature_space[0]) == sizeof(crmhash_t));
    QSORT(crmhash_t, feature_space, n_features, compare_features);

    if (unique)
    {
        i = 0;
        j = 0;
        for (j = 0; j < n_features; j++)
        {
            if (feature_space[i] != feature_space[j])
                feature_space[++i] = feature_space[j];
        }
        feature_space[++i] = 0;
        n_features = i + 1; //the zero counts
    }
    else
    {
        feature_space[n_features++] = 0;
    }
    CRM_ASSERT(n_features <= max_features);
    return n_features;
}


static int find_closest_document(ARGPARSE_BLOCK *apb,
        CLUMPER_STATE_STRUCT *s,
        char *text, int text_len,
        regex_t *regee,
        uint64_t flags)
{
    crmhash_t feature_space[32768];
    int n, i;
    int b = -1;
    float b_s = 0.0;
    float n_s;

    n = eat_document(apb,
            text, text_len, &i,
            regee, feature_space, WIDTHOF(feature_space),
            flags);
    for (i = 0; i < s->header->n_documents; i++)
    {
        n_s = get_document_affinity(feature_space, (crmhash_t *)(s->file_origin + s->document_offsets[i]));
        if (n_s > b_s)
        {
            b = i;
            b_s = n_s;
        }
    }
    return b;
}

typedef struct mythical_cluster_head
{
    struct mythical_cluster_head *head;
    struct mythical_cluster_head *next_head;
    struct mythical_cluster_head *prev_head;
    struct mythical_cluster_head *next;
    int32_t count;
} CLUSTER_HEAD_STRUCT;

static void join_clusters(CLUSTER_HEAD_STRUCT *a, CLUSTER_HEAD_STRUCT *b)
{
    if (internal_trace)
    {
        fprintf(stderr, "Joining clusters of sizes %d and %d\n,",
                (int)a->head->count, (int)b->head->count);
    }

    while (a->next)
        a = a->next;
    b = b->head;
    a->next = b;
    a->head->count += b->count;
    b->count = 0; // though we wont actually touch this value anymore
    if (b->prev_head)
        b->prev_head->next_head = b->next_head;
    if (b->next_head)
        b->next_head->prev_head = b->prev_head;
    b->next_head = NULL;
    b->prev_head = NULL;
    do
    {
        b->head = a->head;
    } while ((b = b->next));
}

static void agglomerative_averaging_cluster(CLUMPER_STATE_STRUCT *s, int goal)
{
    int i, j, k, l;
    int n = s->header->n_documents;
    CLUSTER_HEAD_STRUCT *clusters = calloc(n, sizeof(clusters[0]));
    CLUSTER_HEAD_STRUCT *a, *b, *c;
    CLUSTER_HEAD_STRUCT first_head_ptr = { 0 };
    int m_size = (n * (n + 1) / 2 - 1);
    float *M = calloc(m_size, sizeof(M[0]));
    float d, e;
    int ck, cl, ckl;

    if (internal_trace)
        fprintf(stderr, "agglomerative averaging clustering...\n");

    if (!clusters)
    {
        untrappableerror("Cannot allocate cluster block (A)", "Stick a fork in us; we're _done_.");
    }
    if (!M)
    {
        untrappableerror("Cannot allocate distance matrix M[] (A)", "Stick a fork in us; we're _done_.");
    }

    for (i = 1; i < n - 1; i++)
    {
        clusters[i].head = &clusters[i];
        clusters[i].prev_head = &clusters[i - 1];
        clusters[i].next_head = &clusters[i + 1];
        clusters[i].next = NULL;
        clusters[i].count = 1;
    }
    clusters[0].head = &clusters[0];
    clusters[0].prev_head = &first_head_ptr;
    clusters[0].next_head = &clusters[1];
    clusters[0].next = NULL;
    clusters[0].count = 1;
    if (n > 1) //don't muck the first one!
    {
        clusters[n - 1].head = &clusters[n - 1];
        clusters[n - 1].prev_head = &clusters[n - 2];
        clusters[n - 1].next = NULL;
        clusters[n - 1].count = 1;
    }
    //always make sure the chain ends
    clusters[n - 1].next_head = NULL;

    first_head_ptr.next_head = &clusters[0];

    for (i = 0; i < m_size; i++)
        M[i] = s->distance_matrix[i];

    for (a = first_head_ptr.next_head; a; a = a->next_head)
    {
        if (s->cluster_assignments[a - clusters] < 0)
        {
            for (b = a->next_head; b; b = b->next_head)
            {
                if (s->cluster_assignments[a - clusters] == s->cluster_assignments[b - clusters])
                {
                    k = a - clusters;
                    l = b - clusters;
                    ck = clusters[k].count;
                    cl = clusters[l].count;
                    ckl = ck + cl;

                    for (c = &clusters[0]; c; c = c->next_head)
                    {
                        float *m_k_i;
                        float *m_l_i;

                        i = c - clusters;
                        if (i == k || i == l)
                            continue;
                        m_k_i = aref_dist_mat(M, k, i);
                        m_l_i = aref_dist_mat(M, l, i);
                        CRM_ASSERT(!(ckl <= FLT_EPSILON && ckl >= -FLT_EPSILON));
                        m_k_i[0] = (ck * m_k_i[0] + cl * m_l_i[0]) / ckl;
                        m_l_i[0] = 0.0;
                    }
                    join_clusters(&clusters[k], &clusters[l]);
                    n--;
                }
            }
        }
    }

    while (n > goal)
    {
        l = 0;
        k = 0;
        d = -1.0;
        for (a = first_head_ptr.next_head; a; a = a->next_head)
        {
            for (b = a->next_head; b; b = b->next_head)
            {
                i = a - clusters;
                j = b - clusters;
                if (s->cluster_assignments[i] < 0 && s->cluster_assignments[j] < 0)
                    e = *aref_dist_mat(M, i, j) = -1000000000.0;
                else
                    e = *aref_dist_mat(M, i, j);
                if (e > d)
                {
                    if (s->cluster_assignments[j] < 0)
                    {
                        k = j;
                        l = i;
                    }
                    else
                    {
                        k = i;
                        l = j;
                    }
                    d = e;
                }
            }
        }
        if (l == 0 && k == 0)
        {
            fprintf(stderr, "CLUMP FAILED TO JOIN ENOUGH CLUMPS!\n");
            break;
        }
        ck = clusters[k].count;
        cl = clusters[l].count;
        ckl = ck + cl;

        for (a = &clusters[0]; a; a = a->next_head)
        {
            float *m_k_i;
            float *m_l_i;

            i = a - clusters;
            if (i == k || i == l)
                continue;
            m_k_i = aref_dist_mat(M, k, i);
            m_l_i = aref_dist_mat(M, l, i);
            CRM_ASSERT(!(ckl <= FLT_EPSILON && ckl >= -FLT_EPSILON));
            m_k_i[0] = (ck * m_k_i[0] + cl * m_l_i[0]) / ckl;
            m_l_i[0] = 0.0;
        }
        join_clusters(&clusters[k], &clusters[l]);
        n--;
    }

    i = s->header->n_perma_clusters + 1;
    for (a = &clusters[0]; a; a = a->next_head)
    {
        if (s->cluster_assignments[a - clusters] < 0)
            j = -s->cluster_assignments[a - clusters];
        else
            j = i++;
        for (b = a; b; b = b->next)
            s->cluster_assignments[b - clusters] = j;
    }

    s->header->n_clusters = n;
    free(M);
    free(clusters);
}

static void index_to_pair(int t, int *i, int *j)
{
    int p = 2 * t;

    *i = (int)sqrt(p);
    if (*i * *i + *i > p)
        (*i)--;
    *j = t - (*i * (*i + 1) / 2);
}

#if defined (CRM_WITHOUT_MJT_INLINED_QSORT)

static int compare_float_ptrs(const void *a, const void *b)
{
    if (**(float **)a > **(float **)b)
        return 1;

    if (**(float **)a < **(float **)b)
        return -1;

    return 0;
}

#else

#define compare_float_ptrs(a, b) \
    (**(float **)(a) > **(float **)(b))

#endif

static void agglomerative_nearest_cluster(CLUMPER_STATE_STRUCT *s, int goal)
{
    int i, j, k, l, m;
    int n = s->header->n_documents;
    CLUSTER_HEAD_STRUCT *clusters = calloc(n, sizeof(clusters[0]));
    CLUSTER_HEAD_STRUCT *a, *b;
    CLUSTER_HEAD_STRUCT first_head_ptr = { 0 };
    int m_size = (n * (n + 1) / 2 - 1);
    float **M = calloc(m_size, sizeof(M[0]));

    if (internal_trace)
        fprintf(stderr, "agglomerative nearest clustering...\n");


    if (!clusters)
    {
        untrappableerror("Cannot allocate cluster block (B)", "Stick a fork in us; we're _done_.");
    }
    if (!M)
    {
        untrappableerror("Cannot allocate distance matrix M[] (B)", "Stick a fork in us; we're _done_.");
    }

    for (i = 1; i < n - 1; i++)
    {
        clusters[i].head = &clusters[i];
        clusters[i].prev_head = &clusters[i - 1];
        clusters[i].next_head = &clusters[i + 1];
        clusters[i].next = NULL;
        clusters[i].count = 1;
    }
    clusters[0].head = &clusters[0];
    clusters[0].prev_head = &first_head_ptr;
    clusters[0].next_head = &clusters[1];
    clusters[0].next = NULL;
    clusters[0].count = 1;
    if (n > 1) //don't muck the first one!
    {
        clusters[n - 1].head = &clusters[n - 1];
        clusters[n - 1].prev_head = &clusters[n - 2];
        clusters[n - 1].next = NULL;
        clusters[n - 1].count = 1;
    }
    //always make sure the chain ends
    clusters[n - 1].next_head = NULL;

    first_head_ptr.next_head = &clusters[0];

    for (i = 0; i < m_size; i++)
        M[i] = &s->distance_matrix[i];
    CRM_ASSERT(sizeof(M[0]) == sizeof(float *));
    CRM_ASSERT(sizeof(M[0][0]) == sizeof(float));
    QSORT(float *, M, m_size, compare_float_ptrs);

    for (a = first_head_ptr.next_head; a; a = a->next_head)
    {
        if (s->cluster_assignments[a - clusters] < 0)
        {
            for (b = a->next_head; b; b = b->next_head)
            {
                if (s->cluster_assignments[a - clusters] == s->cluster_assignments[b - clusters])
                {
                    k = a - clusters;
                    l = b - clusters;
                    join_clusters(&clusters[k], &clusters[l]);
                    n--;
                }
            }
        }
    }
    i = m_size;
    while (n > goal)
    {
        do
        {
            k = M[--i] - s->distance_matrix;
            index_to_pair(k, &l, &m);
        } while (clusters[m].head == clusters[l].head);

        join_clusters(&clusters[m], &clusters[l]);
        n--;
    }

    i = s->header->n_perma_clusters + 1;
    for (a = &clusters[0]; a; a = a->next_head)
    {
        if (s->cluster_assignments[a - clusters] < 0)
        {
            j = -s->cluster_assignments[a - clusters];
        }
        else
        {
            j = i++;
        }
        for (b = a; b; b = b->next)
        {
            s->cluster_assignments[b - clusters] = j;
        }
    }
    s->header->n_clusters = n;
    free(M);
    free(clusters);
}

double square(double a)
{
    return a * a;
}

#define H_BUCKETS 50

static void thresholding_average_cluster(CLUMPER_STATE_STRUCT *s)
{
    int i, j, k, l, ck, cl, ckl;
    int n = s->header->n_documents;
    CLUSTER_HEAD_STRUCT *clusters = calloc(n, sizeof(clusters[0]));
    CLUSTER_HEAD_STRUCT *a, *b, *c;
    CLUSTER_HEAD_STRUCT first_head_ptr = { 0 };
    int H[H_BUCKETS];
    int C[H_BUCKETS];
    float A[H_BUCKETS];
    float t_A, t, t_score, scoro, gM;
    float min, max, scale;
    int m_size = (n * (n + 1) / 2 - 1);
    float *M = calloc(m_size, sizeof(M[0]));
    float d, e;

    if (internal_trace)
        fprintf(stderr, "threshold average clustering...\n");


    if (!clusters)
    {
        untrappableerror("Cannot allocate cluster block (C)", "Stick a fork in us; we're _done_.");
    }
    if (!M)
    {
        untrappableerror("Cannot allocate distance matrix M[] (C)", "Stick a fork in us; we're _done_.");
    }

    for (i = 1; i < n - 1; i++)
    {
        clusters[i].head = &clusters[i];
        clusters[i].prev_head = &clusters[i - 1];
        clusters[i].next_head = &clusters[i + 1];
        clusters[i].next = NULL;
        clusters[i].count = 1;
    }
    clusters[0].head = &clusters[0];
    clusters[0].prev_head = &first_head_ptr;
    clusters[0].next_head = &clusters[1];
    clusters[0].next = NULL;
    clusters[0].count = 1;
    if (n > 1) //don't muck the first one!
    {
        clusters[n - 1].head = &clusters[n - 1];
        clusters[n - 1].prev_head = &clusters[n - 2];
        clusters[n - 1].next = NULL;
        clusters[n - 1].count = 1;
    }
    // always make sure the chain ends
    clusters[n - 1].next_head = NULL;

    first_head_ptr.next_head = &clusters[0];

    for (i = 0; i < m_size; i++)
        M[i] = s->distance_matrix[i];

    for (i = 0; i < H_BUCKETS; i++)
        H[i] = 0;

#if 01
    CRM_ASSERT(n >= 2);
    j = n * (n - 1) / 2;
    CRM_ASSERT(j > 0);
#else
    j = m_size; // [i_a] shouldn't that be j = (n * (n + 1) / 2 - 1) in previous line (a.k.a. m_size) ???
    // NOPE! Discussed this on the ML and Joe told me the other one is the correct count [i_a]
    CRM_ASSERT(j > 0);
#endif
    CRM_ASSERT(j <= m_size);
    min = 1000000000.0;
    max = -1000000000.0;

    for (i = 0; i < j; i++)
    {
        if (M[i] < min)
            min = s->distance_matrix[i];
        if (M[i] > max)
            max = s->distance_matrix[i];
    }
    scale = (max - min) / (H_BUCKETS - 0.1);
#if 0 /* [i_a] quick and dirty hack to prevent crash below, but this doesn't help, as now things further down below go bump in the night :-( */
    if (scale <= FLT_EPSILON && scale >= -FLT_EPSILON)
        scale = 1;
#endif
    CRM_ASSERT(!(scale <= FLT_EPSILON && scale >= -FLT_EPSILON));
    t = -1.0;
    for (i = 0; i < j; i++)
    {
        int index = (int)((M[i] - min) / scale);
        CRM_ASSERT(index >= 0);
        CRM_ASSERT(index < WIDTHOF(H));
        H[index]++;
    }
    if (internal_trace)
    {
        fprintf(stderr, "Histogram of document distances:\n");
        for (i = 0; i < H_BUCKETS; i++)
        {
            for (k = 0; k < H[i]; k += 100)
                fputc('*', stderr);
            fputc('\n', stderr);
        }
    }

    k = 0;
    t_A = 0.0;
    for (i = 0; i < H_BUCKETS; i++)
    {
        k = C[i] = H[i] + k;
        t_A = A[i] = H[i] * (min + (i + 0.5) * scale) + t_A;
    }
    CRM_ASSERT(j != 0);
    gM = t_A / (float)j;
    t_score = 0.0;
    for (i = 2; i < H_BUCKETS - 2; i++)
    {
        CRM_ASSERT(!((k - C[i]) <= FLT_EPSILON && (k - C[i]) >= -FLT_EPSILON));
        CRM_ASSERT(!(C[i] <= FLT_EPSILON && C[i] >= -FLT_EPSILON));
        scoro = square(gM - (t_A - A[i]) / (k - C[i])) * (k - C[i])
                + square(gM - A[i] / C[i]) * C[i];
        if (scoro > t_score)
        {
            t_score = scoro;
            t = min + scale * (float)i;
        }
    }
    if (internal_trace)
    {
        fprintf(stderr, "min = %f, max = %f, t = %f\n", min, max, t);
    }
    for (a = first_head_ptr.next_head; a; a = a->next_head)
    {
        if (s->cluster_assignments[a - clusters] < 0)
        {
            for (b = a->next_head; b; b = b->next_head)
            {
                if (s->cluster_assignments[a - clusters] == s->cluster_assignments[b - clusters])
                {
                    k = a - clusters;
                    l = b - clusters;
                    ck = clusters[k].count;
                    cl = clusters[l].count;
                    ckl = ck + cl;

                    for (c = &clusters[0]; c; c = c->next_head)
                    {
                        float *m_k_i;
                        float *m_l_i;

                        i = c - clusters;
                        if (i == k || i == l)
                            continue;
                        m_k_i = aref_dist_mat(M, k, i);
                        m_l_i = aref_dist_mat(M, l, i);
                        // m_k_i[0] = (ck * m_k_i[0] + cl * m_l_i[0]) / ckl;

                        // m_k_i[0] = CRM_MIN(m_k_i[0], m_l_i[0]);  -->
                        if (m_k_i[0] > m_l_i[0])
                        {
                            m_k_i[0] = m_l_i[0];
                        }
                        m_l_i[0] = 0.0;
                    }
                    join_clusters(&clusters[k], &clusters[l]);
                    n--;
                }
            }
        }
    }

    for ( ; ;)
    {
        l = 0;
        k = 0;
        d = -1.0;
        for (a = first_head_ptr.next_head; a; a = a->next_head)
        {
            for (b = a->next_head; b; b = b->next_head)
            {
                i = a - clusters;
                j = b - clusters;
                if (s->cluster_assignments[i] < 0 && s->cluster_assignments[j] < 0)
                {
                    e = *aref_dist_mat(M, i, j) = -1000000000.0;
                    if (internal_trace)
                        fprintf(stderr, " wonk!\n");
                }
                else
                {
                    e = *aref_dist_mat(M, i, j);
                }
                if (e > d)
                {
                    if (s->cluster_assignments[j] < 0)
                    {
                        k = j;
                        l = i;
                    }
                    else
                    {
                        k = i;
                        l = j;
                    }
                    d = e;
                }
            }
        }
        if (internal_trace)
        {
            fprintf(stderr, "l = %d, k = %d, d = %f\n", l, k, d);
        }
        if ((l == 0 && k == 0) || d < t)  //we're done
            break;

        ck = clusters[k].count;
        cl = clusters[l].count;
        ckl = ck + cl;

        for (a = &clusters[0]; a; a = a->next_head)
        {
            float *m_k_i;
            float *m_l_i;

            i = a - clusters;
            if (i == k || i == l)
                continue;
            m_k_i = aref_dist_mat(M, k, i);
            m_l_i = aref_dist_mat(M, l, i);
            CRM_ASSERT(!(ckl <= FLT_EPSILON && ckl >= -FLT_EPSILON));
            m_k_i[0] = (ck * m_k_i[0] + cl * m_l_i[0]) / ckl;
            m_l_i[0] = 0.0;
        }
        join_clusters(&clusters[k], &clusters[l]);
        n--;
    }

    i = s->header->n_perma_clusters + 1;
    for (a = &clusters[0]; a; a = a->next_head)
    {
        if (s->cluster_assignments[a - clusters] < 0)
            j = -s->cluster_assignments[a - clusters];
        else
            j = i++;
        for (b = a; b; b = b->next)
            s->cluster_assignments[b - clusters] = j;
    }

    s->header->n_clusters = n;
    free(M);
    free(clusters);
}

static void assign_perma_cluster(CLUMPER_STATE_STRUCT *s,
        int doc,
        char *lab)
{
    int i;

    for (i = 1; i <= s->header->n_perma_clusters; i++)
    {
        if (0 == strcmp(s->cluster_labels[i], lab))
            break;
    }
    if (i > s->header->n_perma_clusters)
    {
        i = ++(s->header->n_perma_clusters);
        strcpy(s->cluster_labels[i], lab);
    }
    s->cluster_assignments[doc] = -i;
}

int crm_expr_clump(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char htext[MAX_PATTERN];
    char filename[MAX_PATTERN];
    int htext_len;

    char regex_text[MAX_PATTERN]; //  the regex pattern
    int  regex_text_len;
    char param_text[MAX_PATTERN];
    int  param_text_len;
    int unique, unigram, bychunk;
    int n_clusters = 0;

    char tag[DOCUMENT_TAG_LEN];
    char classv[CLUSTER_LABEL_LEN];

    struct stat statbuf;

    CLUMPER_STATE_STRUCT s = { 0 };

    regex_t regee;
    regmatch_t matchee[2];

    int  i, j, k, l;

    char *txtptr;
    int txtstart;
    int txtlen;
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
	int len;

    int max_documents = 1000;

    len = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, len,
            NULL,
            &txtptr,
            &txtstart,
            &txtlen,
            errstr,
			WIDTHOF(errstr));
    if (i < 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;

		if (i == -1)
            fev = nonfatalerror(errstr, "");
        if (i == -2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }


    htext_len = crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    htext_len = crm_nexpandvar(htext, htext_len, MAX_PATTERN);
	CRM_ASSERT(htext_len < MAX_PATTERN);

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

	htext[j] = 0;
    strcpy(filename, &htext[i]);

    //use regex_text and regee to grab parameters
    param_text_len = crm_get_pgm_arg(param_text, MAX_PATTERN, apb->s2start, apb->s2len);
    param_text_len = crm_nexpandvar(param_text, param_text_len, MAX_PATTERN);
	CRM_ASSERT(param_text_len < MAX_PATTERN);
    param_text[param_text_len] = 0;
    if (internal_trace)
        fprintf(stderr, "param_text = %s\n", param_text);

    strcpy(regex_text, "n_clusters[[:space:]]*=[[:space:]]*([0-9]+)");
    if (crm_regcomp(&regee, regex_text, strlen(regex_text), REG_EXTENDED))
    {
        nonfatalerror("Problem compiling regex to grab params:", regex_text);
        return 0;
    }
    if (!crm_regexec(&regee, param_text, param_text_len, WIDTHOF(matchee), matchee, 0, NULL))
    {
		CRM_ASSERT(regee.re_nsub == 1);
        param_text[matchee[1].rm_eo + 1] = 0;
        if (internal_trace)
        {
            fprintf(stderr, "&param_text[matchee[1].rm_so] = %s\n",
                    &param_text[matchee[1].rm_so]);
        }
        n_clusters = atol(&param_text[matchee[1].rm_so]);
        if (internal_trace)
            fprintf(stderr, "n_clusters = %d\n", n_clusters);
    }
    strcpy(regex_text, "tag[[:space:]]*=[[:space:]]*([[:graph:]]+)");
    if (crm_regcomp(&regee, regex_text, strlen(regex_text), REG_EXTENDED))
    {
        nonfatalerror("Problem compiling regex to grab params:", regex_text);
        return 0;
    }
    if (!crm_regexec(&regee, param_text, param_text_len, WIDTHOF(matchee), matchee, 0, NULL))
    {
        param_text[matchee[1].rm_eo] = 0;
        strncpy(tag, &param_text[matchee[1].rm_so], sizeof(tag));
        tag[sizeof(tag) - 1] = 0;
    }
    else
    {
        tag[0] = 0;
    }
    strcpy(regex_text, "clump[[:space:]]*=[[:space:]]*([[:graph:]]+)");
    if (crm_regcomp(&regee, regex_text, strlen(regex_text), REG_EXTENDED))
    {
        return nonfatalerror("Problem compiling regex to grab params:", regex_text);
    }
    if (!crm_regexec(&regee, param_text, param_text_len, WIDTHOF(matchee), matchee, 0, NULL))
    {
        param_text[matchee[1].rm_eo] = 0;
        strncpy(classv, &param_text[matchee[1].rm_so], sizeof(classv));
        classv[sizeof(classv) - 1] = 0;
    }
    else
    {
        classv[0] = 0;
    }

    strcpy(regex_text, "max_documents[[:space:]]*=[[:space:]]*([[:graph:]]+)");
    if (crm_regcomp(&regee, regex_text, strlen(regex_text), REG_EXTENDED))
    {
        nonfatalerror("Problem compiling regex to grab params:", regex_text);
        return 0;
    }
    if (!crm_regexec(&regee, param_text, param_text_len, WIDTHOF(matchee), matchee, 0, NULL))
    {
        param_text[matchee[1].rm_eo] = 0;
        max_documents = atol(&param_text[matchee[1].rm_so]);
    }
    //we've already got a default max_documents

    regex_text_len = crm_get_pgm_arg(regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
    if (regex_text_len == 0)
    {
        strcpy(regex_text, "[[:graph:]]+");
        regex_text_len = strlen(regex_text);
    }
    //regex_text[regex_text_len] = 0;
    regex_text_len = crm_nexpandvar(regex_text, regex_text_len, MAX_PATTERN);
    if (crm_regcomp(&regee, regex_text, regex_text_len, REG_EXTENDED))
    {
        return nonfatalerror("Problem compiling this regex:", regex_text);
    }

    unique = !!(apb->sflags & CRM_UNIQUE);
    unigram = !!(apb->sflags & CRM_UNIGRAM);
    bychunk = !!(apb->sflags & CRM_BYCHUNK);

    if (apb->sflags & CRM_REFUTE)
    {
        if (map_file(&s, filename))
        {
            //we already nonfatalerrored
            return 0;
        }
        if (tag[0])
        {
            for (i = s.header->n_documents; i >= 0; i--)
            {
                if (0 == strcmp(tag, s.document_tags[i]))
                    break;
            }
        }
        else
        {
            i = find_closest_document(apb, &s, txtptr + txtstart, txtlen,
                    &regee, apb->sflags);
        }
        if (i < 0)
        {
            unmap_file(&s);
            return 0;
        }
        memmove(s.file_origin + s.document_offsets[i],
                s.file_origin + s.document_offsets[i + 1],
                (s.header->file_length - s.document_offsets[i + 1]));
        memmove(&s.document_tags[i],
                &s.document_tags[i + 1],
                sizeof(char) * DOCUMENT_TAG_LEN * (s.header->n_documents - i - 1));
        memmove(&s.cluster_labels[i],
                &s.cluster_labels[i + 1],
                sizeof(char) * CLUSTER_LABEL_LEN * (s.header->n_documents - i - 1));
        s.header->n_documents--;
        j = s.document_offsets[i + 1] - s.document_offsets[i];
        for (k = i; k < s.header->n_documents; k++)
        {
            s.document_offsets[k] = s.document_offsets[k + 1] - j;
            s.cluster_assignments[k] = s.cluster_assignments[k + 1];
        }
        s.header->file_length -= j;
        for (k = 0; k < s.header->n_documents; k++)
        {
            for (l = CRM_MAX(k + 1, i); l < s.header->n_documents; l++)
            {
                *aref_dist_mat(s.distance_matrix, k, l) = *aref_dist_mat(s.distance_matrix, k, l + 1);
            }
        }
        if (n_clusters > 0)
        {
            if (bychunk)
                agglomerative_averaging_cluster(&s, n_clusters);
            else
                agglomerative_nearest_cluster(&s, n_clusters);
        }
        else if (n_clusters == 0)
        {
            if (bychunk)
                thresholding_average_cluster(&s);
            else
                thresholding_average_cluster(&s);
        }
        l = s.header->file_length;

        unmap_file(&s);
        crm_force_munmap_filename(filename);
        truncate(filename, l);
        return 0;
    }
    else
    {
        //LEARNIN'!
        int n;
        crmhash_t feature_space[32768];
        FILE *f;
		ssize_t old_fileoffset;

        if (stat(filename, &statbuf))
        {
            if (!make_new_clumper_backing_file(filename, max_documents))
            {
                return 0;
            }
        }
        if (txtlen == 0)
        {
            if (tag[0] && classv[0]) //is not null
            {
                if (map_file(&s, filename))
                {
                    //we already nonfatalerrored
                    return 0;
                }
                // GROT GROT GROT
                for (i = s.header->n_documents - 1; i >= 0; i++ /* [i_a] should this REALLY be 'i++' instead of 'i--' ??? */)
                {
                    if (0 == strcmp(tag, s.document_tags[i]))
                        break;
                }
                if (i >= 0)
                    assign_perma_cluster(&s, i, classv);
                unmap_file(&s);
            }
            return 0;
        }

        n = eat_document(apb,
                txtptr + txtstart, txtlen, &i,
                &regee,
                feature_space, WIDTHOF(feature_space),
                apb->sflags);

        crm_force_munmap_filename(filename);

        f = fopen(filename, "ab");
        if (!f)
        {
            int fev = nonfatalerror_ex(SRC_LOC(),
                    "\n Couldn't open your new clumper file %s for writing; errno=%d(%s)\n",
                    filename,
                    errno,
                    errno_descr(errno));
            return fev;
        }

        //     And make sure the file pointer is at EOF.
        (void)fseek(f, 0, SEEK_END);

        if (ftell(f) == 0)
        {
            CRM_PORTA_HEADER_INFO classifier_info = { 0 };

            classifier_info.classifier_bits = CRM_CLUMP;
		classifier_info.hash_version_in_use = selected_hashfunction;

            if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
            {
                int fev;
				                fclose(f);
fev = nonfatalerror_ex(SRC_LOC(),
                        "\n Couldn't write header to file %s; errno=%d(%s)\n",
                        filename, errno, errno_descr(errno));
                return fev;
            }
        }

		old_fileoffset = ftell(f);
        if (n != fwrite(feature_space, sizeof(feature_space[0]), n, f))
        {
            int fev;
			int err = errno;

            CRM_ASSERT(f != NULL);
            fclose(f);
			// try to correct the failure by ditching the new, partially(?) written(?) data
			truncate(filename, old_fileoffset);
			fev = nonfatalerror_ex(SRC_LOC(), "Cannot write/append feature space to the clump backing file '%s': error = %d(%s)", 
				filename,
				err,
				errno_descr(err));
            return fev;
        }
        fclose(f);

        if (map_file(&s, filename))
        {
            //we already nonfatalerrored
            return 0;
        }
        if (s.header->n_documents >= s.header->max_documents)
        {
			int fev;
            unmap_file(&s);
            fev = nonfatalerror("This clump backing file is full and cannot"
                          " assimilate new documents!", filename);
            return fev;
        }
        i = s.header->n_documents++;

        s.document_offsets[i] = s.header->file_length;
        s.header->file_length += sizeof(feature_space[0]) * n;
        for (j = 0; j < i; j++)
        {
            *aref_dist_mat(s.distance_matrix, j, i) = get_document_affinity(
                    (void *)(s.file_origin + s.document_offsets[i]),
                    (void *)(s.file_origin + s.document_offsets[j]));
        }
        strcpy(s.document_tags[i], tag);
        if (classv[0])
            assign_perma_cluster(&s, i, classv);
        else
            s.cluster_assignments[i] = 0;

        if (n_clusters > 0)
        {
            if (bychunk)
                agglomerative_averaging_cluster(&s, n_clusters);
            else
                agglomerative_nearest_cluster(&s, n_clusters);
        }
        else if (n_clusters == 0)
        {
            if (bychunk)
                thresholding_average_cluster(&s);
            else
                thresholding_average_cluster(&s);
        }
        unmap_file(&s);
        return 0;
    }
}

int sprint_lab(CLUMPER_STATE_STRUCT *s, char *b, int l)
{
    if (l == 0)
    {
        strcpy(b, "unassigned");
        return strlen("unassigned");
    }
    if (s->cluster_labels[l][0] != 0)
        return sprintf(b, "%s", s->cluster_labels[l]);
    else
        return sprintf(b, "clump_#%d", l);
}

int sprint_tag(CLUMPER_STATE_STRUCT *s, char *b, int d)
{
    if (s->document_tags[d][0] != 0)
        return sprintf(b, "%s", s->document_tags[d]);
    else
        return sprintf(b, "document_#%d", d);
}

int crm_expr_pmulc(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char htext[MAX_PATTERN];
    char filename[MAX_PATTERN];
    int htext_len;

    char regex_text[MAX_PATTERN]; //  the regex pattern
    int regex_text_len;
    int unique, unigram, bychunk;

    char out_var[MAX_PATTERN];
    int out_var_len;

    float A[MAX_CLUSTERS];
    double T;
    int N[MAX_CLUSTERS];
    double p[MAX_CLUSTERS];
    double pR[MAX_CLUSTERS];

    crmhash_t feature_space[32768];
    int n;

    int closest_doc = -1;
    float closest_doc_affinity;

    int out_len = 0;

    CLUMPER_STATE_STRUCT s = { 0 };

    regex_t regee;

    int  i, j;

    char *txtptr;
    int txtstart;
    int txtlen;
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
	int boxtxtlen;

    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
            NULL,
            &txtptr,
            &txtstart,
            &txtlen,
            errstr,
			WIDTHOF(errstr));
    if (i < 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == -1)
            fev = nonfatalerror(errstr, "");
        if (i == -2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }


    htext_len = crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    htext_len = crm_nexpandvar(htext, htext_len, MAX_PATTERN);
	CRM_ASSERT(htext_len < MAX_PATTERN);

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

	htext[j] = 0;
    strcpy(filename, &htext[i]);

    //grab output variable name
    out_var_len = crm_get_pgm_arg(out_var, MAX_PATTERN, apb->p2start, apb->p2len);
    out_var_len = crm_nexpandvar(out_var, out_var_len, MAX_PATTERN);

    regex_text_len = crm_get_pgm_arg(regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
    if (regex_text_len == 0)
    {
        strcpy(regex_text, "[[:graph:]]+");
        regex_text_len = strlen(regex_text);
    }
    //regex_text[regex_text_len] = 0;
    regex_text_len = crm_nexpandvar(regex_text, regex_text_len, MAX_PATTERN);
    if (crm_regcomp(&regee, regex_text, regex_text_len, REG_EXTENDED))
    {
        nonfatalerror("Problem compiling this regex:", regex_text);
        return 0;
    }

    unique = !!(apb->sflags & CRM_UNIQUE);
    unigram = !!(apb->sflags & CRM_UNIGRAM);
    bychunk = !!(apb->sflags & CRM_BYCHUNK);

    if (map_file(&s, filename))
    {
        //we already nonfatalerrored
        return 0;
    }

    if (txtlen == 0)
    {
        for (i = 0; i < s.header->n_documents; i++)
        {
            A[0] = 0.0;
            N[0] = 1;
            for (j = 0; j < s.header->n_documents; j++)
            {
                if (i != j && s.cluster_assignments[i] == s.cluster_assignments[j])
                {
                    float *m_i_j;

                    m_i_j = aref_dist_mat(s.distance_matrix, i, j);

                    N[0]++;
                    if (bychunk)
                    {
                        A[0] += *m_i_j;
                    }
                    else if (*m_i_j > A[0])
                    {
                        A[0] = *m_i_j;
                    }
                }
            }
            if (bychunk)
            {
                A[0] /= N[0];
            }
            out_len += sprintf(outbuf + out_len, "%d (", i);
            out_len += sprint_tag(&s, outbuf + out_len, i);
            out_len += sprintf(outbuf + out_len, ") clump: %d (", (int)s.cluster_assignments[i]);
            out_len += sprint_lab(&s, outbuf + out_len, s.cluster_assignments[i]);
            out_len += sprintf(outbuf + out_len, ") affinity: %0.4f\n", A[0]);
        }
        outbuf[out_len] = 0;
        if (out_var_len)
            crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_len);
        unmap_file(&s);
    }
    else
    {
        if (internal_trace)
            fprintf(stderr, "pmulcing!\n");
        n = eat_document(apb,
                txtptr + txtstart, txtlen, &i,
                &regee,
                feature_space, WIDTHOF(feature_space),
                apb->sflags);
        closest_doc_affinity = -1.0;
        for (i = 0; i <= s.header->n_clusters; i++)
        {
            A[i] = 0.0;
            N[i] = 0;
        }
        if (bychunk)
        {
            for (i = 0; i < s.header->n_documents; i++)
            {
                j = s.cluster_assignments[i];
                if (j == 0)
                    continue;
                T = get_document_affinity(feature_space, (void *)(s.file_origin +
                                                                  s.document_offsets[i]));
                A[j] += T;
                if (T > closest_doc_affinity)
                {
                    closest_doc = i;
                    closest_doc_affinity = T;
                }
                N[j]++;
            }
	        T = 0.0; /* [i_a] */
            for (i = 1; i <= s.header->n_clusters; i++)
                T += A[i] /= N[i];
        }
        else
        {
            for (i = 0; i < s.header->n_documents; i++)
            {
                j = s.cluster_assignments[i];
                T = get_document_affinity(feature_space, (void *)(s.file_origin +
                                                                  s.document_offsets[i]));
                if (T > A[j])
                    A[j] = T;
                if (T > closest_doc_affinity)
                {
                    closest_doc = i;
                    closest_doc_affinity = T;
                }
                N[j]++;
            }
        }
        T = 0.0000001;
        j = 1;
        for (i = 1; i <= s.header->n_clusters; i++)
        {
            if (A[i] > A[j])
                j = i;
            if (A[i] == 0.0)
            {
                p[i] = 0.0;
            }
            else
            {
                CRM_ASSERT(!(A[i] <= FLT_EPSILON && A[i] >= -FLT_EPSILON));
                p[i] = normalized_gauss(1.0 / A[i] - 1.0, 0.5);
                //p[i] = A[i];
            }
            T += p[i];
        }
        if (s.header->n_clusters < 2)
            p[j = 1] = 0.0;
        for (i = 1; i <= s.header->n_clusters; i++)
        {
            p[i] /= T;
            pR[i] = 10 * (log10(0.0000001 + p[i]) - log10(1.0000001 - p[i]));
        }

        if (internal_trace)
            fprintf(stderr, "generating output...\n");

        if (p[j] > 0.5)
        {
            out_len += sprintf(outbuf + out_len, "PMULC succeeds; success probability: %0.4f pR: %0.4f\n", p[j], pR[j]);
        }
        else
        {
            out_len += sprintf(outbuf + out_len, "PMULC fails; success probability: %0.4f pR: %0.4f\n", p[j], pR[j]);
        }
        out_len += sprintf(outbuf + out_len, "Best match to clump #%d (", j);
        out_len += sprint_lab(&s, outbuf + out_len, j);
        out_len += sprintf(outbuf + out_len, ") prob: %0.4f  pR: %0.4f\n", p[j], pR[j]);
        out_len += sprintf(outbuf + out_len, "Closest document: #%d (", closest_doc);
        out_len += sprint_tag(&s, outbuf + out_len, closest_doc);
        out_len += sprintf(outbuf + out_len, ") affinity: %0.4f\n", closest_doc_affinity);
        out_len += sprintf(outbuf + out_len, "Total features in input file: %d\n", n);
        for (i = 1; i <= s.header->n_clusters; i++)
        {
            out_len += sprintf(outbuf + out_len, "%d: (", i);
            out_len += sprint_lab(&s, outbuf + out_len, i);
            out_len += sprintf(outbuf + out_len, "): documents: %d  affinity: %0.4f  prob: %0.4f  pR: %0.4f\n",
                    N[i], A[i], p[i], pR[i]);
        }

        if (p[j] > 0.5)
        {
            if (user_trace)
                fprintf(stderr, "CLUMP was a SUCCESS, continuing execution.\n");
        }
        else
        {
            if (user_trace)
                fprintf(stderr, "CLUMP was a FAIL, skipping forward.\n");
            //    and do what we do for a FAIL here
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        outbuf[out_len] = 0;
        if (internal_trace)
            fprintf(stderr, "JOE_TRACE:\n%s", outbuf);
        if (out_var_len)
            crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_len);
        unmap_file(&s);
    }
    return 0;
}


#else

int crm_expr_clump(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_pmulc(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}

#endif /* !CRM_WITHOUT_CLUMP */



int crm_expr_clump_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}


int crm_expr_clump_css_migrate(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "CLUMP");
}





