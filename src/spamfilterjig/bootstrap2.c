#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/*
 *  seq seq10k run qrel err fneg fpos prev next x score
 *  1 0.000100 1 2 1 1 0 0 2 1.00000 0.520000
 */

struct ss
{
    int qrel;
    double score;
} s[400000], t[400000];

double logit(double x)
{
    if (x == 1)
        x = 1 - FLT_MIN;
    if (x == 0)
        x = FLT_MIN;

    return log10(x / (1 - x));
}

int comp(struct ss *a, struct ss *b)
{
    if (a->score > b->score)
        return 1;

    if (a->score < b->score)
        return -1;

    return 0;

    if (a->qrel == 2)
        return -1;

    return 1;

    return 0;
}

int i, j, k, m, n, hams, spams, fn, inversions, cnt[51];
double roca, x[1000], maxx = -9999, minx = 9999, meanx, varx, stdevx;

#define SIZE 100

double unlogit(double x)
{
    double e = exp(x);

    return e / (1 + e);
}

int main()
{
    scanf("%*[^\n]");
    for (n = 0; 2 == scanf("%*d%*f%*d%d%*d%*d%*d%*d%*d%*f%lf",
                           &t[n].qrel, &t[n].score); n++)
    {
        t[n].score += random() * 1e-15;
    }
    hams = spams = fn = inversions = 0;
    memcpy(s, t, sizeof(t));
    qsort(s, n, sizeof(struct ss), (int(*) (const void *, const void *))comp);
    for (i = 0; i < n; i++)
    {
        if (s[i].qrel == 1)
            hams++;
        else
            spams++;
        if (s[i].qrel == 2)
            fn++;
        if (s[i].qrel == 1)
            inversions += fn;
    }
    roca = (double)inversions / ((double)hams * spams);
    /* printf("roca %lg\n",roca); */
    roca = logit(roca);
    printf("%g\n", roca);

    for (k = 0; k < SIZE; k++)
    {
        for (i = 0; i < n; i++)
        {
            s[i] = t[random() % n];
        }
        qsort(s, n, sizeof(struct ss), (int(*) (const void *, const void *))comp);
        hams = spams = fn = inversions = 0;
        for (i = 0; i < n; i++)
        {
            if (s[i].qrel == 1)
                hams++;
            else
                spams++;
            if (s[i].qrel == 2)
                fn++;
            if (s[i].qrel == 1)
                inversions += fn;
        }
        x[k] = (double)inversions / ((double)hams * spams);
        x[k] = logit(x[k]);
        if (x[k] > maxx)
            maxx = x[k];
        if (x[k] < minx)
            minx = x[k];
        meanx += x[k];
        /* printf("k %d %g %g\n",k,x[k],unlogit(x[k])); */
        printf("%g\n", x[k]);
    }
    return 0;

    meanx /= SIZE;
    for (k = 0; k < SIZE; k++)
        varx += (x[k] - meanx) * (x[k] - meanx);
    stdevx = sqrt(varx / (SIZE - 1));
    printf("%%1-ROCA%%: %g (%g - %g)\n", 100 * unlogit(roca),
           100 * unlogit(roca - 1.96 * stdevx), 100 * unlogit(roca + 1.96 * stdevx));
    printf("logit stderr: %g\n", stdevx);
    printf("(approx) linear stderr: %g\n", 100 * unlogit(roca + .5 * stdevx)
           - 100 * unlogit(roca - .5 * stdevx));
    for (i = 0; i < 20; i++)
        cnt[i] = 0;
    for (i = 0; i < SIZE; i++)
    {
        j = (x[i] - minx) / (maxx - minx) * 20;
        if (j == 20)
            j--;
        cnt[j]++;
    }
    for (i = 0; i < 20; i++)
    {
        printf("%3d", cnt[i]);
        for (j = 0; j < cnt[i]; j++)
            printf("*");
        printf("\n");
    }
    return 0;
}

