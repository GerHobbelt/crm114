#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int i, j, k, m, n, x;
double cpr, wpr, pr, c, p, like, sum;
double z, logomp, logwpr, logpr, logc, logp;

double binom(int x, int n, double p)
{
    int i;
    /* int j,k,m; */
    double cpr = 0, wpr, pr, c;
    /*
     * double like;
     * double z;
     */
    double logomp, logwpr, logpr, logc, logp, scale, scpr;

    if (x + x > n)
        return 1 - binom(n - x - 1, n, 1 - p);

    /*
     * printf("binom %d %d %lg\n",x,n,p);
     * p = exp(logp);
     */
    logp = log(p);
    c = 1;
    logc = 0;
    pr = pow(1 - p, n);
    logomp = log(1 - p);
    logpr = logomp * n;
    scale = logpr;
    scpr = 0;

    for (i = 0; i <= n; i++)
    {
        if (i)
        {
            c *= (n - i + 1);
            logc += log(n - i + 1);
            c /= i;
            logc -= log(i);
        }
        wpr = c * pr;
        logwpr = logc + logpr;
        cpr += wpr;
        scpr += exp(logwpr - scale);
        if (scpr > 1)
        {
            /* printf("rescale\n"); */
            scale += 10;
            scpr *= exp(-10);
        }
        /*
         * printf("%d %lg %lg %lg %lg\n",i,c,pr,wpr,cpr);
         * printf("%d %lg %lg %lg %lg\n",i,logc,logpr,logwpr,
         * scpr*exp(scale));
         */
        if (i == x)
        {
            /* printf("ret scpr %lg scale %lg %lg\n", scpr,scale,scpr*exp(scale)); */
            return scpr * exp(scale);
        }
        pr *= p;
        logpr += logp;
        pr /= (1 - p);
        logpr -= logomp;
    }
    /* printf("oops \n");  happens due to recursion with x = -1 */
    return 0;
}

double lci(int x, int n, double alpha)
{
    /* int i,j,k; */
    double hi = 1, lo = 0, mid;

    if (x == n)
        return 1;

    if (x)
        alpha /= 2;
    while (hi - lo > .0000001)
    {
        double z;

        mid = (hi + lo) / 2;
        z = binom(x, n, mid);
        if (z < alpha)
            hi = mid;
        else
            lo = mid;
    }
    return mid;
}

void printit(char *label, int x, int n, double p)
{
    printf("%s", label);
    printf("%0.2f (%0.2f-%0.2f)\n", 100.0 * x / n, 100.0 * (1 - lci(n - x, n, p)), 100.0 * lci(x, n, p));
    /* printf("%d %d %lg\n",x,n,p); */
}

int q;
char buf[100];

int main()
{
    int a, b, c, d;

    while (gets(buf) && strcmp(buf, "Overall"))
    {}
    if (strcmp("Overall", buf) || 4 != scanf("%d%d%d%d", &a, &b, &c, &d))
    {
        printf("error - this isn't a listing file\n");
        exit(1);
    }
    printf("              Gold Standard\n");
    printf("       +-------------------+\n");
    printf("       |       ham    spam |\n");
    printf("       |                   |\n");
    printf("Filter | ham%7d%7d |\n", a, b);
    printf("Result |spam%7d%7d |\n", c, d);
    printf("       +-------------------+\n");
    printf("Total       %7d%7d\n\n", a + c, b + d);
    printit("ham%:    ", c, a + c, .05);
    printit("spam%:   ", b, b + d, .05);
    printit("miss%:   ", c + b, a + b + c + d, .05);
    return 0;
}

