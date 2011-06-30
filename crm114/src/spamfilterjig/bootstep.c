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

int FP, FN, H, S;
double lm, lam[SIZE], lama, lamsq;

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
    int run, qrel;
    double score;
} s[400000], t[400000];

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

int i, j, k, m, n, hams, spams, cnt[51];
double inversions;
double roca, x[1000], maxx = -9999, minx = 9999, meanx, varx, stdevx;
int thams, tspams;
double fp, fn;


double unlogit(double x)
{
    double e = exp(x);

    return e / (1 + e);
}

int main()
{
    scanf("%*[^\n]");
    thams = tspams = 0;
    for (n = 0; 3 == scanf("%*d%*f%d%d%*d%*d%*d%*d%*d%*f%lf",
                           &t[n].run, &t[n].qrel, &t[n].score); n++)
    {
        t[n].score += random() * 1e-20;
        if (t[n].qrel == 1)
            thams++;
        else
            tspams++;
    }
    hams = spams = fn = inversions = 0;
    for (i = 0; i < 13; i++)
        Xfpyfn[i] = Xfnyfp[i] = -1;
    qsort(t, n, sizeof(struct ss), (int(*) (const void *, const void *))comp);
    FP = FN = 0;
    for (i = 0; i < n; i++)
    {
        if (t[i].qrel == 1 && t[i].run != 1)
            FP++;
        if (t[i].qrel == 2 && t[i].run != 2)
            FN++;
        if (t[i].qrel == 1)
            hams++;
        else
            spams++;
        /* if (t[i].qrel == 2) fn++; */
        if (t[i].qrel == 1)
            inversions += spams;
        fp = thams - hams;
        if (fp == 0)
            fp = .5;
        fn = spams;
        if (fn == 0)
            fn = .5;
        /* printf("fnr %0.2f fpr %0.2f\n", 100.0*fn/tspams, 100.0*fp/thams); */
        Xfnyfp[quant((double)fn / tspams)] = (double)fp / thams;
        if (Xfpyfn[quant((double)fp / thams)] < 0)
            Xfpyfn[quant((double)fp / thams)] = (double)fn / tspams;
    }
    lm = (logit(FN / (double)(spams)) + logit(FP / (double)hams)) / 2;
/*   printf("HAM%%: %7.2f\nSPAM%%: %6.2f\n",100*(double)FP/hams, 100*(double)FN/spams); */
    roca = (double)inversions / ((double)hams * spams);
    if (roca == 0)
        roca = .5 / ((double)hams * spams);
#if 0
    printf("roca %lg\n", roca);
    roca = logit(roca);
#endif
    roca = logit(roca);
    lama = lamsq = 0;
    for (k = 0; k < SIZE; k++)
    {
        thams = tspams = 0;
        for (i = 0; i < 13; i++)
            xfpyfn[k][i] = xfnyfp[k][i] = -1;
        for (i = 0; i < n; i++)
        {
            s[i] = t[random() % n];
            s[i].score -= random() * 1e-20;
            if (s[i].qrel == 1)
                thams++;
            else
                tspams++;
        }
        qsort(s, n, sizeof(struct ss), (int(*) (const void *, const void *))comp);
        hams = spams = fn = inversions = 0;
        FP = FN = 0;
        for (i = 0; i < n; i++)
        {
            if (s[i].qrel == 1 && s[i].run != 1)
                FP++;
            if (s[i].qrel == 2 && s[i].run != 2)
                FN++;
            if (s[i].qrel == 1)
                hams++;
            else
                spams++;
            /* if (s[i].qrel == 2) fn++; */
            if (s[i].qrel == 1)
                inversions += spams;
            fp = thams - hams;
            if (fp == 0)
                fp = .5;
            fn = spams;
            if (fn == 0)
                fn = .5;
            xfnyfp[k][quant((double)fn / tspams)] = (double)fp / thams;
            if (xfpyfn[k][quant((double)fp / thams)] < 0)
                xfpyfn[k][quant((double)fp / thams)] = (double)fn / tspams;
        }
        lam[k] = (logit(FN / (double)(spams)) + logit(FP / (double)hams)) / 2;
        lama += lam[k];
        x[k] = (double)inversions / ((double)hams * spams);
        if (0 == x[k])
            x[k] = .5 / ((double)hams * spams);
#if 0
        printf("roca %lg\n", x[k]);
        x[k] = logit(x[k]);
#endif
        x[k] = logit(x[k]);
        if (x[k] > maxx)
            maxx = x[k];
        if (x[k] < minx)
            minx = x[k];
        meanx += x[k];
        /* printf("k %d %lg %lg\n",k,x[k],unlogit(x[k])); */
        for (i = 0; i < 13; i++)
        {
            sfpyfn[i] += logit(xfpyfn[k][i]);
            sfnyfp[i] += logit(xfnyfp[k][i]);
        }
    }
    lama /= SIZE;
    for (k = 0; k < SIZE; k++)
    {
        lamsq += (lam[k] - lama) * (lam[k] - lama);
    }
    lamsq = sqrt(lamsq / (SIZE - 1));
    printf("lam%% %8.2f (%0.2f - %0.2f)\n\n", 100 * unlogit(lm), 100 * unlogit(lm - 1.96 * lamsq), 100 * unlogit(lm + 1.96 * lamsq));
#define sq(x) ((x) * (x))

    meanx /= SIZE;
    for (k = 0; k < SIZE; k++)
        varx += (x[k] - meanx) * (x[k] - meanx);
    stdevx = sqrt(varx / (SIZE - 1));
    printf("1-ROCA%%: %0.4f (%0.4f - %0.4f)\n\n", 100 * unlogit(roca),
           100 * unlogit(roca - 1.96 * stdevx), 100 * unlogit(roca + 1.96 * stdevx));


    printf("HAM MISC%%  SPAM MISC%% (95%% C.L.)\n");
    for (i = 0; i < 12; i++)
    {
        sfpyfn[i] /= SIZE;
        for (k = 0; k < SIZE; k++)
        {
            vfpyfn[i] += sq(logit(xfpyfn[k][i]) - sfpyfn[i]);
        }
        stdevx = sqrt(vfpyfn[i] / (SIZE - 1));
        printf("%10.2f %10.2f (%0.2f - %0.2f)\n", 100 * rquant[i], fabs(100 * Xfpyfn[i]),
               100 * unlogit(logit(Xfpyfn[i]) - 1.96 * stdevx),
               100 * unlogit(logit(Xfpyfn[i]) + 1.96 * stdevx));
    }
    printf("\nSPAM MISC%%  HAM MISC%% (95%% C.L.)\n");
    for (i = 0; i < 12; i++)
    {
        sfnyfp[i] /= SIZE;
        for (k = 0; k < SIZE; k++)
        {
            vfnyfp[i] += sq(logit(xfnyfp[k][i]) - sfnyfp[i]);
        }
        stdevx = sqrt(vfnyfp[i] / (SIZE - 1));
        printf("%10.2f %10.2f (%0.2f - %0.2f)\n", 100 * rquant[i], fabs(100 * Xfnyfp[i]),
               100 * unlogit(logit(Xfnyfp[i]) - 1.96 * stdevx),
               100 * unlogit(logit(Xfnyfp[i]) + 1.96 * stdevx));
    }
    exit(0);
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

