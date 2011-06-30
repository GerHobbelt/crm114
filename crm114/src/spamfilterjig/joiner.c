#include <stdio.h>
#include <math.h>


/* standard normal density function */
double ndf(double t)
{
    return 0.398942280401433 * exp(-t * t / 2);
}

/* standard normal cumulative distribution function */
double nc(double x)
{
    double result;

    if (x < -7.)
        result = ndf(x) / sqrt(1. + x * x);
    else if (x > 7.)
        result = 1. - nc(-x);
    else
    {
        result = 0.2316419;
        static double a[5] = { 0.31938153, -0.356563782, 1.781477937, -1.821255978, 1.330274429 };
        result = 1. / (1 + result * fabs(x));
        result = 1 - ndf(x) * (result * (a[0] + result * (a[1] + result * (a[2] + result * (a[3] + result * a[4])))));
        if (x <= 0.)
            result = 1. - result;
    }
    return result;
}

int n, i, j, k;
double x[2000], y[2000], d[2000], var, meanx, meany, mean, sterr, varx, vary, sterrx, sterry, s1, s2, z;
FILE *f, *g;

double unlogit(double x)
{
    return exp(x) / (1 + exp(x));
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("need 2 filename args\n");
        exit(1);
    }

    f = fopen(argv[1], "r");
    g = fopen(argv[2], "r");

    fscanf(f, "%lf", &s1);
    fscanf(g, "%lf", &s2);

    for (n = 0; 1 == fscanf(f, "%lf", &x[n]); n++)
    {
        fscanf(g, "%lf", &y[n]);
        d[n] = x[n] - y[n];
        mean += d[n];
        meanx += x[n];
        meany += y[n];
    }

    mean /= n;
    meanx /= n;
    meany /= n;
    for (i = 0; i < n; i++)
    {
        var += (d[i] - mean) * (d[i] - mean);
        varx += (x[i] - meanx) * (x[i] - meanx);
        vary += (y[i] - meany) * (y[i] - meany);
    }

    sterr = sqrt(var / (n - 1));
    sterrx = sqrt(varx / (n - 1));
    sterry = sqrt(vary / (n - 1));

    printf("mean x %lg (%lg)  %lg\n", meanx, sterrx, unlogit(meanx));
    printf("mean y %lg (%lg)  %lg\n", meany, sterry, unlogit(meany));
    printf("mean d %lg (%lg)\n", mean, sterr);

    printf("%lg %lg [%lg %lg] difference %lg stdev %lg is %lg SDs p < %lg (1 tail)\n", s1, s2, unlogit(s1), unlogit(s2), s1 - s2, sterr, fabs(s1 - s2) / sterr, nc(-fabs(s1 - s2) / sterr),
           nc(-fabs(s1 - s2) / sterr / 2));

    return 0;
}

