#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <float.h>


char buf[1000];
int tfn, tfp, fn, fp;
double newfp, newfn, outfp = -99, outfn = -99, bestfp = -99, bestfn = -99;

double logit(double x)
{
    if (x == 1)
        x = 1 - FLT_MIN;
    if (x == 0)
        x = FLT_MIN;

    return log10(x / (1 - x));
}


int main()
{
    int i;
    double lastfp = -99, lastfn = -99;

    tfp = 100000;
    tfn = 100000;

    fn = tfn + 1;
    fp = 0;
    /* printf("tfp %d tfn %d\n ",tfp,tfn); */
    for (i = 0; i < tfn; i++)
    {
        fp = i;
        fn = tfn - i;
        if (fp == 0)
            fp = 1;
        if (fn == 0)
            fn = 1;
        newfp = logit((double)fp / tfp);
        newfn = -logit((double)fn / tfn);
        if (newfp > bestfp)
            bestfp = newfp;
        if (newfn > bestfn)
            bestfn = newfn;
        if (bestfp > outfp && bestfn > outfn)
        {
            if (bestfp >= lastfp + 0.1 || bestfn >= lastfn + 0.1)
            {
                printf("%0.6f %0.6f\n", bestfp, bestfn);
                lastfp = bestfp;
                lastfn = bestfn;
            }
            outfp = bestfp;
            outfn = bestfn;
        }
    }
    printf("%0.6f %0.6f\n", bestfp, bestfn);
    return 0;
}

