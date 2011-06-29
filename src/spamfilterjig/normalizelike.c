#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

/*
 *  seq seq10k run qrel err fneg fpos prev next x score
 *  1 0.000100 1 2 1 1 0 0 2 1.00000 0.520000
 */

#define SIZE 100

double logit(double x)
{
    if (x == 1)
        x = 1 - FLT_MIN;
    if (x == 0)
        x = FLT_MIN;

    return log10(x / (1 - x));
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
double xfpyfn[SIZE][13], xfnyfp[SIZE][13];
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
int hams, spams;


double unlogit(double x)
{
    double e = exp(x);

    return e / (1 + e);
}

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
    printf("%lg %d %d %d\n", r->score, r->qrel, r->hams, r->spams);
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
    head = NULL;
    hams = spams = 0;
    for (i = 0; i < n; i++)
    {
        double hamabove = hamgt(head, t[i].score);
        double spambelow = spamlt(head, t[i].score);
        double eps = .3;
        /*
         * double oddsratio = (hams-hamabove+eps)/(hamabove+eps) * (spambelow+eps)/(spams-spambelow+eps);
         * double oddsratio = 1/(hamabove+eps) * (spambelow+eps);
         * double logoddsratio = log(oddsratio);
         */
        double hamNtail = (hamabove + eps) / (hams + eps);
        double spamNtail = (spambelow + eps) / (spams + eps);
        double tail = (hamabove + spambelow + eps + eps) / (hams + spams + eps + eps);
        double spamCtail = spamNtail / tail;
        double hamCtail = hamNtail / tail;
        /* double logoddsratio = log(spamNtail/hamNtail); */
        double logoddsratio = log((spambelow * hams + eps) / (hamabove * spams + eps));
        printf("%d judge=%s class=%s score=%f hamprob=%f spamprob=%f sum %f hamNtail %f spamNtail %f tail %f\n", i + 1, isspam(i) ? "spam" : "ham", logoddsratio <= 0 ? "ham" : "spam",
               logoddsratio, hamCtail, spamCtail, hamCtail + spamCtail, hamNtail, spamNtail, tail);
        if (isspam(i))
        {
            spams++;
        }
        else
        {
            hams++;
        }
        insert(&head, i);
    }
    return 0;
}

