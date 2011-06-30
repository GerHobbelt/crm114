#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

/*
 *  seq seq10k run qrel err fneg fpos prev next x score
 *  1 0.000100 1 2 1 1 0 0 2 1.00000 0.520000
 */

int eq(double d1, double d2)
{
    return d1 + FLT_EPSILON >= d2 && d1 - FLT_EPSILON <= d2;
}

double logit(double x)
{
    if (eq(x, 1.0))
        x = 1.0 - FLT_EPSILON;
    if (eq(x, 0.0))
        x = FLT_EPSILON;

    x = x / (1.0 - x);
    return log10(x);
}

double unlogit(double x)
{
    double e = exp(x);

    if (eq(e, 1.0))
        e = 1.0 - FLT_EPSILON;
    if (eq(e, 0.0))
        e = FLT_EPSILON;

    return e / (1 + e);
}

int quant(double x)
{
    if (x <= .0001)
        return 0;

    if (x <= .0002)
        return 1;

    if (x <= .0005)
        return 2;

    if (x <= .001)
        return 3;

    if (x <= .002)
        return 4;

    if (x <= .005)
        return 5;

    if (x <= .01)
        return 6;

    if (x <= .02)
        return 7;

    if (x <= .05)
        return 8;

    if (x <= .1)
        return 9;

    if (x <= .2)
        return 10;

    if (x <= .5)
        return 11;

    return 12;
}

double rquant[] = { .0001, .0002, .0005, .001, .002, .005, .01, .02, .05, .1, .2, .5, 1 };

double Xfpyfn[13], Xfnyfp[13];
double sfpyfn[13], sfnyfp[13];
double vfpyfn[13], vfnyfp[13];

struct ss
{
    int qrel;
    double score;
    int hams, spams;
    struct ss *left, *right;
} *head, s[400000], t[400000];

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

int i, j, k, m, n, cnt[51];
double roca, x[1000], maxx = -9999, minx = 9999, meanx, varx, stdevx;
int thams, tspams;
double fp, fn;
double hams, spams, inversions;


#define isspam(x) (t[x].qrel == 2)

void insert(struct ss **r, int v)
{
    if (!(*r))
    {
        *r = &t[v];
        t[v].hams = !isspam(v);
        t[v].spams = isspam(v);
        return;
    }
    if (t[v].score < (*r)->score)
        insert(&(*r)->left, v);
    else
        insert(&(*r)->right, v);
    (*r)->hams += (!isspam(v));
    (*r)->spams += isspam(v);
}

void dump(struct ss *r, int d)
{
    int i;

    if (!r)
        return;

    dump(r->left, d + 1);
    for (i = 0; i < d; i++)
        printf(" ");
    printf("%g %d %d %d\n", r->score, r->qrel, r->hams, r->spams);
    dump(r->right, d + 1);
}

double hamgt(struct ss *r, double score)
{
    if (!r)
        return 0;

    if (score < r->score)
        return hamgt(r->left, score) + (r->qrel == 1) + (r->right ? r->right->hams : 0);

    return hamgt(r->right, score);
}

double spamlt(struct ss *r, double score)
{
    if (!r)
        return 0;

    if (score < r->score)
        return spamlt(r->left, score);

    return (r->left ? r->left->spams : 0) + (r->qrel == 2) + spamlt(r->right, score);
}

int main()
{
    int CLUMP;

    scanf("%*[^\n]");
    thams = tspams = 0;
    for (n = 0; 2 == scanf("%*d%*f%*d%d%*d%*d%*d%*d%*d%*f%lf",
                           &t[n].qrel, &t[n].score); n++)
    {
        t[n].score += random() * 1e-20;
        if (t[n].qrel == 1)
            thams++;
        else
            tspams++;
    }
    CLUMP = (n + 49) / 50;
    for (i = 0; i < n; i++)
    {
        if (isspam(i))
        {
            spams++;
            inversions += hamgt(head, t[i].score);
        }
        else
        {
            hams++;
            inversions += spamlt(head, t[i].score);
        }
        insert(&head, i);
        roca = inversions / hams / spams;
        if ((i + 1) % CLUMP == n % CLUMP)
            printf("%d %0.6f\n", i + 1, logit(roca));
#if 0
        printf("hams %lg spams %lg inversions %lg roca %lg\n", hams, spams, inversions, 100 * inversions / hams / spams);
        dump(head, 0);
        printf("-------------------\n");
#endif
    }
    return 0;
}

