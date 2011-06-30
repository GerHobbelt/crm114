/*
 * *******************************************************************
 *    program : normal.cpp
 *    author  : Uwe Wystup, http://www.mathfinance.de
 *
 *    created : January 2000
 *
 *    usage   : auxiliary functions around the normal distribution
 * *******************************************************************
 */

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

double x;
int main()
{
    while (1 == scanf("%lf", &x))
    {
        printf("x %g foo %g\n", x, nc(x));
    }
    return 0;
}

