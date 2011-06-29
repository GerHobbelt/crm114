#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <float.h>

char judge[100];
double score;
char buf[1000];
int tfn, tfp, fn, fp, i;
int prevfp, prevfn;
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
    for (i = 0; 2 == scanf(" %*s %s %*s score=%lf", judge, &score); i++)
    {
        tfp += !strcmp(judge, "judge=ham");
        tfn += !strcmp(judge, "judge=spam");
    }
    fseek(stdin, 0, SEEK_SET);
    fn = tfn;
    /* printf("tfp %d tfn %d\n ",tfp,tfn); */
    prevfp = 1000000000;
    prevfn = -1;
    while (2 == scanf(" %*s %s %*s score=%lf", judge, &score))
    {
        fp += !strcmp(judge, "judge=ham");
        fn -= !strcmp(judge, "judge=spam");
        newfp = logit((double)fp / tfp);
        newfn = -logit((double)fn / tfn);
        if (newfp > bestfp)
            bestfp = newfp;
        if (newfn > bestfn)
            bestfn = newfn;
        if (bestfp > outfp && bestfn > outfn)
        {
            printf("%0.6f %0.6f\n", bestfp, bestfn);
            outfp = bestfp;
            outfn = bestfn;
        }
    }
    printf("%0.6f %0.6f\n", bestfp, bestfn);
    return 0;
}

