#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

#define MAX_TESTS 400000         /* maximum number of tests results this tool can process */
#define LEARN_GRAPH_STEPS  50    /* number of samples used to construct the learn graph */
#define THRESHOLD_GRAPH_STEPS 10 /* number of points in graph where threshold is displayed */

int CLUMP;
#define INCLUDE 1
int seq[MAX_TESTS], run[MAX_TESTS], qrel[MAX_TESTS], next[MAX_TESTS], prev[MAX_TESTS];
int maxseq = 0;
double sc, score[MAX_TESTS], x[MAX_TESTS];
int i, j, k, m, n, pr, ne, p;
int fp, fn, tp, tn, tfp, tfn, ttp, ttn;
double sumcnt, sumit, nfp, ntp, thistp, thisfp;
double nfn, ntn, thistn, thisfn;
double nn, np, thisn, thisp;
double nrecall, nprec, thisrecall, thisprec;
FILE *d, *s, *r, *t;
char tmp[1000];
int perm[MAX_TESTS];
int ofp, otp, ofn, otn;
double thres_range;
double thres_step;
double th;
double acc;

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

int comp(int *a, int *b)
{
    if (score[*a] > score[*b])
        return -1;

    if (score[*a] < score[*b])
        return +1;

    return 0;
}

int comp1(int *a, int *b)
{
    if (score[*a] > score[*b])
        return +1;

    if (score[*a] < score[*b])
        return -1;

    return 0;
}

int main(int argc, char **argv)
{
    sprintf(tmp, "%s.roc", argv[1]);
    r = fopen(tmp, "w");
    sprintf(tmp, "%s.roc2", argv[1]);
    t = fopen(tmp, "w");
    freopen(argv[1], "r", stdin);
    sprintf(tmp, "%s.txt", argv[1]);
    d = fopen(tmp, "w");
    sprintf(tmp, "%s.spss", argv[1]);
    s = fopen(tmp, "w");
    if (argc > 2)
        maxseq = atoi(argv[2]);
    fprintf(s, "seq seq10k run qrel err fneg fpos prev next x score\n");

    for (n = 0; 4 == scanf("%d%d%d%lf%*[^\n]", &seq[n], &run[n], &qrel[n], &score[n]); n++)
        ;
    printf("%d records read\n", n);
#if 0
    for (i = 0; i < n; i++)
        if (seq[i] > maxseq)
            maxseq = seq[i];
    maxseq = 49086;
#endif

    CLUMP = (n + LEARN_GRAPH_STEPS - 1) / LEARN_GRAPH_STEPS;
    for (i = 0; i < n; i++)
        if (run[i] != 2)
            run[i] = 1;
#if 0
    if (!strncmp(argv[1], "crm", 3))
    {
        for (i = 0; i < n; i++)
            score[i] = -score[i];
    }
#endif
    for (i = 0; i < n; i++)
    {
        if (qrel[i] == 1 && run[i] != 2)
            tn++;
        if (qrel[i] == 1 && run[i] == 2)
            fp++;
        if (qrel[i] == 2 && run[i] == 2)
            tp++;
        if (qrel[i] == 2 && run[i] != 2)
            fn++;
        if (i % CLUMP == CLUMP - 1 || i == n - 1)
        {
            printf("Sequence %d to %d\n", i - i % CLUMP, i);
            printf("N: %6d%6d | %6d\nY: %6d%6d | %6d\n   ------------   ------\n   %6d%6d | %6d\n", tn, fn, tn + fn, fp, tp, fp + tp, tn + fp, fn + tp, tn + fp + fn + tp);
            printf("err:     %0.2f%%\n", 100.0 * (fn + fp) / (double)(fn + fp + tn + tp));
            printf("fp rate: %0.2f%%\n", 100.0 * fp / (double)(tn + fp));
            printf("tp rate: %0.2f%%\n", 100.0 * tp / (double)(tp + fn));
            printf("fn rate: %0.2f%%\n", 100.0 * fn / (double)(tp + fn));
            printf("tn rate: %0.2f%%\n", 100.0 * tn / (double)(tn + fp));
            printf("acc:     %0.2f%%\n", 100.0 * (tn + tp) / (double)(fn + fp + tn + tp));
            printf("prec:    %0.2f%%\n", 100.0 * tp / (double)(tp + fp));
            printf("recall:  %0.2f%%\n", 100.0 * tp / (double)(tp + fn));
            printf("f-meas:  %0.2f%%\n", 100.0 * 2.0 * tp / (double)(tp + fp + tp + fn));

            thisfp = logit(fp / (double)(tn + fp));
            thistp = logit(tp / (double)(tp + fn));
            fprintf(t, "%0.6f %0.6f\n", thisfp, thistp);

            ttp += tp;
            ttn += tn;
            tfp += fp;
            tfn += fn;
            tp = tn = fp = fn = 0;
            printf("\n");
        }
    }
    printf("Overall\n");
    printf("N: %6d%6d | %6d\nY: %6d%6d | %6d\n   ------------   ------\n   %6d%6d | %6d\n", ttn, tfn, ttn + tfn, tfp, ttp, tfp + ttp, ttn + tfp, tfn + ttp, ttn + tfp + tfn + ttp);
    printf("err:     %0.2f%%\n", 100.0 * (tfn + tfp) / (double)(tfn + tfp + ttn + ttp));
    printf("fp rate: %0.2f%%\n", 100.0 * tfp / (double)(ttn + tfp));
    printf("tp rate: %0.2f%%\n", 100.0 * ttp / (double)(ttp + tfn));
    printf("fn rate: %0.2f%%\n", 100.0 * tfn / (double)(ttp + tfn));
    printf("tn rate: %0.2f%%\n", 100.0 * ttn / (double)(ttn + tfp));
    printf("acc:     %0.2f%%\n", 100.0 * (ttn + ttp) / (double)(tfn + tfp + ttn + ttp));
    printf("prec:    %0.2f%%\n", 100.0 * ttp / (double)(ttp + tfp));
    printf("recall:  %0.2f%%\n", 100.0 * ttp / (double)(ttp + tfn));
    printf("f-meas:  %0.2f%%\n", 100.0 * 2.0 * ttp / (double)(ttp + tfp + ttp + tfn));

    /*
     * ROC curve generation according to
     * [Fawcett, 2006, An introduction to ROC analysis: algorithm 1, pp. 866]
     *
     * ttn ~ TN
     * ttp ~ TP
     * tfn ~ FN
     * tfp ~ FP
     *
     * qrel[] ~ n/p columns
     * run[] ~ Y/N hypothesis rows
     *
     * However, this implementation has a little change to it: instead of
     * calculating FP/N and TP/P, it tracks TP & FP like it should,
     * then, instead of using N and P, it calculates TN & FN alongside,
     * so N and P are (as they were): N = TP + FN, P = FP + TN
     */

    /* Lsorted: */
    for (i = 0; i < n; i++)
        perm[i] = i;
    qsort(perm, n, sizeof(int), (int(*) (const void *, const void *))comp);

    printf("Sorted L: seq score hyp\n");
    for (i = 0; i < n; i++)
    {
        printf("%6d %7.3f %s\n", i, score[perm[i]], (qrel[perm[i]] == 1 ? "P" : "N"));
    }
    printf("\n");

    thres_range = score[perm[0]] - score[perm[n - 1]];
    thres_step = (thres_range + THRESHOLD_GRAPH_STEPS - 1) / THRESHOLD_GRAPH_STEPS;

    tp = fp = 0;
    ofp = otp = 0;            /* coordinate (0,0) */
    fn = ofn = ttp + tfn;     /* starts as P */
    tn = otn = tfp + ttn;     /* starts as N */
    ntp = -1000;              /* -Inf */
    nfp = -1000;
    ntn = -1000; /* -Inf */
    nfn = -1000;
    nrecall = -1000;
    nprec = -1000;
    nn = -1000;
    np = -1000;

    sc = score[perm[0]] - 1.0; /* fprev = -Inf --> start at (0,0) */
    th = sc;

    for (i = 0;; i++)
    {
        if (i == n || score[perm[i]] != sc)
        {
            printf("\n");
            printf("ROC curve construction point %d\n", i);
            printf("N: %6d%6d | %6d\nY: %6d%6d | %6d\n   ------------   ------\n   %6d%6d | %6d\n", otn, ofn, otn + ofn, ofp, otp, ofp + otp, otn + ofp, ofn + otp, otn + ofp + ofn + otp);
            printf("err:     %0.2f%%\n", 100.0 * (ofn + ofp) / (double)(ofn + ofp + otn + otp));
            printf("fp rate: %0.2f%% (logit: %0.2f)\n", 100.0 * ofp / (double)(otn + ofp), logit(ofp / (double)(otn + ofp)));
            printf("tp rate: %0.2f%% (logit: %0.2f)\n", 100.0 * otp / (double)(otp + ofn), logit(otp / (double)(otp + ofn)));
            printf("fn rate: %0.2f%%\n", 100.0 * ofn / (double)(otp + ofn));
            printf("tn rate: %0.2f%%\n", 100.0 * otn / (double)(otn + ofp));
            printf("acc:     %0.2f%%\n", 100.0 * (otn + otp) / (double)(ofn + ofp + otn + otp));
            printf("prec:    %0.2f%% (logit: %0.2f)\n", 100.0 * otp / (double)(otp + ofp), logit(otp / (double)(otp + ofp)));
            printf("recall:  %0.2f%% (logit: %0.2f)\n", 100.0 * otp / (double)(otp + ofn), logit(otp / (double)(otp + ofn)));
            printf("f-meas:  %0.2f%%\n", 100.0 * 2.0 * otp / (double)(otp + ofp + otp + ofn));

            /*
             *      before, this was essentially log(FP / (N - FP))
             *      which looked very alike logit(fpr) ~ log(fpr / (1 - fpr))
             *
             *      and it is:
             *
             *      FP / (N - FP) =?= (FP/N) / (1 - (FP/N)) =
             *                        (FP/N) / (N/N - FP/N) = FP / (N - FP)
             *
             *      Why the different code this time then?
             *
             *      Because then I can discard the logit() when I want/need.
             */
            if ((otn + ofp) == 0)
            {
                thisfp = -1000;
                thistp = -1000;
            }
            else
            {
                thisfp = logit(ofp / (double)(otn + ofp));
                thistp = logit(otp / (double)(otp + ofn));
            }
            printf("%0.6f %0.6f %0.6f %0.6f\n", thisfp, thistp, nfp, ntp);

            /* 3rd axis: p/N */
            thisp = (otp + ofn) / (double)(ofn + ofp + otn + otp);

            if ((otp + ofp) == 0)
            {
                thisrecall = -1000;
                thisprec = -1000;
            }
            else
            {
                thisrecall = logit(otp / (double)(otp + ofp));
                thisprec = logit(otp / (double)(otp + ofp));
            }

            if ((otp + ofn) == 0)
            {
                thisfn = -1000;
                thistn = -1000;
            }
            else
            {
                thisfn = logit(ofn / (double)(otp + ofn));
                thistn = logit(otn / (double)(otn + ofp));
            }

            /* 3rd axis: n/N */
            thisn = (otn + ofp) / (double)(ofn + ofp + otn + otp);

            if (!eq(nfp, -1000) && !eq(ntp, -1000)
               && !eq(nrecall, -1000) && !eq(nprec, -1000)
               && !eq(nfn, -1000) && !eq(ntn, -1000))
            {
                /* ROC curve: */
                if (!eq(thistp, ntp) && !eq(thisfp, nfp))
                {
                    if (!eq(nfp, -1000) && !eq(ntp, -1000))
                    {
                        fprintf(r, "%0.6f\t%0.6f\t%0.6f\t", nfp, ntp, np);
                    }
                    else
                    {
                        fprintf(r, "NA\tNA\tNA\t");
                    }
                    nfp = thisfp;
                    ntp = thistp;
                    np = thisp;
                }
                else
                {
                    if (!eq(nfp, -1000) && !eq(ntp, -1000))
                    {
                        fprintf(r, "%0.6f\t%0.6f\t%0.6f\t", nfp, ntp, np);
                    }
                    else
                    {
                        fprintf(r, "NA\tNA\tNA\t");
                    }
                }
                /* PN curve */
                if (!eq(thisprec, nprec) && !eq(thisrecall, nrecall))
                {
                    if (!eq(nrecall, -1000) && !eq(nprec, -1000))
                    {
                        fprintf(r, "%0.6f\t%0.6f\t", nrecall, nprec);
                    }
                    else
                    {
                        fprintf(r, "NA\tNA\t");
                    }
                    nprec = thisprec;
                    nrecall = thisrecall;
                }
                else
                {
                    if (!eq(nrecall, -1000) && !eq(nprec, -1000))
                    {
                        fprintf(r, "%0.6f\t%0.6f\t", nrecall, nprec);
                    }
                    else
                    {
                        fprintf(r, "NA\tNA\t");
                    }
                }
                /* ROC curve for ham threshold checking: */
                if (!eq(thistn, ntn) && !eq(thisfn, nfn))
                {
                    if (!eq(nfn, -1000) && !eq(ntn, -1000))
                    {
                        fprintf(r, "%0.6f\t%0.6f\t%0.6f\t", nfn, ntn, nn);
                    }
                    else
                    {
                        fprintf(r, "NA\tNA\tNA\t");
                    }
                    nfn = thisfn;
                    ntn = thistn;
                    nn = thisn;
                }
                else
                {
                    if (!eq(nfn, -1000) && !eq(ntn, -1000))
                    {
                        fprintf(r, "%0.6f\t%0.6f\t%0.6f\t", nfn, ntn, nn);
                    }
                    else
                    {
                        fprintf(r, "NA\tNA\tNA\t");
                    }
                }
            }
            else
            {
                nfp = thisfp;
                ntp = thistp;
                np = thisp;

                nprec = thisprec;
                nrecall = thisrecall;

                nfn = thisfn;
                ntn = thistn;
                nn = thisn;
            }
            fprintf(r, "\n");




            ofp = fp;
            otp = tp;
            ofn = fn;
            otn = tn;

            if (i == n)
                break;
            sc = score[perm[i]];
        }

        /*
         * also adjust TN and FN alongside. This way, the confusion matrix always
         * has the same P and N as before during this whole exercise.
         */
        if (qrel[perm[i]] == 1)
        {
            fp++;
            tn--;
        }
        if (qrel[perm[i]] == 2)
        {
            tp++;
            fn--;
        }
    }


    if (!eq(nfp, -1000) && !eq(ntp, -1000)
       && !eq(thistp, ntp) && !eq(thisfp, nfp))
    {
        fprintf(r, "%0.6f\t%0.6f\t%0.6f\t", nfp, ntp, np);
    }
    else
    {
        fprintf(r, "NA\tNA\tNA\t");
    }
    /* PN curve */
    if (!eq(nrecall, -1000) && !eq(nprec, -1000)
       && !eq(thisprec, nprec) && !eq(thisrecall, nrecall))
    {
        fprintf(r, "%0.6f\t%0.6f\t", nrecall, nprec);
    }
    else
    {
        fprintf(r, "NA\tNA\t");
    }
    /* ROC curve for ham threshold checking: */
    if (!eq(nfn, -1000) && !eq(ntn, -1000)
       && !eq(thistn, ntn) && !eq(thisfn, nfn))
    {
        fprintf(r, "%0.6f\t%0.6f\t%0.6f\t", nfn, ntn, nn);
    }
    else
    {
        fprintf(r, "NA\tNA\tNA\t");
    }

    fprintf(r, "\n");






    if (1)
    {
        int hams, spams, fn, inversions;
        double roca;
        qsort(perm, n, sizeof(int), (int(*) (const void *, const void *))comp1);
        hams = spams = fn = inversions = 0;
        for (i = 0; i < n; i++)
        {
            if (qrel[perm[i]] == 1)
                hams++;
            else
                spams++;
            if (qrel[perm[i]] == 2)
                fn++;
            if (qrel[perm[i]] == 1)
                inversions += fn;
        }
        roca = (double)inversions / ((double)hams * spams);
#if 0
        printf("roca %lg\n", roca);
        roca = logit(roca);
#endif

        printf("roca%%:   %0.3g\n", 100.0 * roca);
    }


#if 0
    printf("inversions: %d\n", inversions);
    printf("possible: %d\n", (ttn + tfp) * (ttp + tfn));
    printf("roca%%:   %0.2lg\n", 100 * (double)inversions / ((double)(ttn + tfp) * (ttp + tfn)));
    printf("\n");
#endif

    pr = 0;
    for (i = 0; i < n; i++)
    {
        prev[i] = pr;
        if (qrel[i] != run[i])
            pr = i + 1;               /*
                                       * seq[i];
                                       * if (qrel[i] == INCLUDE && qrel[i] != run[i]) pr = i+1;//seq[i];
                                       */
    }
    ne = n + 1;
    for (i = n - 1; i >= 0; i--)
    {
        next[i] = ne;
        if (qrel[i] != run[i])
            ne = i + 1;               /*
                                       * seq[i];
                                       * if (qrel[i] == INCLUDE && qrel[i] != run[i]) ne = i+1;//seq[i];
                                       */
    }
    for (i = 0; prev[i] == 0; i++)
        prev[i] = 1 - (next[0] - 1);
    for (i = n - 1; next[i] == n + 1; i--)
        next[i] = n + (n - prev[n - 1]);
    for (i = 0; i < n; i++)
    {
        x[i] = 1.0L / (next[i] - prev[i]);
        if (qrel[i] != run[i])
            x[i] *= 2;
        /* if (qrel[i] == INCLUDE && qrel[i] != run[i]) x[i] *= 2; */
    }

    tp = fp = tn = fn = 0;
    np = 0;
    nn = 0;

    for (i = 0; i < n; i++)
    {
        fprintf(s, "%d %0.6f %d %d %d %d %d %d %d %0.5f %0.6f\n",
                seq[i], (double)(seq[i] - maxseq) / 10000.0, run[i], qrel[i],
                qrel[i] != run[i], run[i] == 1 && (qrel[i] != run[i]),
                run[i] == 2 && (qrel[i] != run[i]), prev[i], next[i], x[i],
                score[i]);

        if (qrel[i] == 1 && run[i] != 2)
            tn++;
        if (qrel[i] == 1 && run[i] == 2)
            fp++;
        if (qrel[i] == 2 && run[i] == 2)
            tp++;
        if (qrel[i] == 2 && run[i] != 2)
            fn++;

        np = (tp + fn) / (double)(tn + tp + fn + fp);
        nn = (tn + fp) / (double)(tn + tp + fn + fp);

        /* sumit += x[i]; */
        sumit += run[i] != qrel[i];
        sumcnt++;
        if (i % CLUMP == CLUMP - 1 /* || i == n-1 */)
        {
            if (sumit > 0 || i == n - 1)
            {
#if 0
                fprintf(d, "%d %0.6f\n", seq[i - i % CLUMP / 2], log10(sumit / (1 + i % CLUMP - sumit)));
#endif
                fprintf(d, "%d\t%0.6f\t%0.6f\t%0.6f\n", seq[i - (int)sumcnt / 2], logit(sumit / sumcnt), logit(np), logit(nn));
                sumit = sumcnt = 0;
            }
        }
    }
    fclose(s);
    fclose(d);
    fclose(r);
    fclose(t);
#if 0
    sprintf(tmp, "sed -e 's/RUNNAME/%s/' adhoc1.plot | gnuplot > %s.ps", argv[1], argv[1]);
    system(tmp);
#endif
    return 0;
}

