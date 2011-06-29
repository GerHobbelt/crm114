#include <math.h>
#include <stdio.h>

double fix(double x)
{
    return 100 * exp(x) / (1 + exp(x));
}

void slurp(double *x, double *err, double *p)
{
    char buf[1000];

    gets(buf);
    sscanf(buf, "%*s%lf%lf%*f%*[< ]%lf", x, err, p);
}

int main()
{
    double h, s, he, se, l, le, l1, l1e, l0, l0e;
    double h0, h1, h0e, h1e, hs, hse, s0, s1, s0e, s1e, ss, sse, hp, sp, x;

    slurp(&h, &he, &x);
    slurp(&s, &se, &x);
    slurp(&h0, &h0e, &x);
    slurp(&hs, &hse, &hp);
    slurp(&h1, &h1e, &x);
    slurp(&s0, &s0e, &x);
    slurp(&ss, &sse, &sp);
    slurp(&s1, &s1e, &x);
    l = (h + s) / 2;
    le = sqrt(he * he / 4 + se * se / 4);
    l0 = (h0 + s0) / 2;
    l0e = sqrt(h0e * h0e / 4 + s0e * s0e / 4);
    l1 = (h1 + s1) / 2;
    l1e = sqrt(h1e * h1e / 4 + s1e * s1e / 4);
    printf("\nLogistic Averages\n");
    printf("HAM%%:  %0.2f (%0.2f - %0.2f)\n", fix(h), fix(h - 1.96 * he), fix(h + 1.96 * he));
    printf("SPAM%%: %0.2f (%0.2f - %0.2f)\n", fix(s), fix(s - 1.96 * se), fix(s + 1.96 * se));
    printf("LAM%%:  %0.2f (%0.2f - %0.2f)\n", fix(l), fix(l - 1.96 * le), fix(l + 1.96 * le));
    printf("\nLEARNING CURVE\n");
    printf("HAM%%  Initial:  %0.2f (%0.2f - %0.2f)", fix(h0), fix(h0 - 1.96 * h0e), fix(h0 + 1.96 * h0e));
    printf("  Final: %0.2g (%0.2f - %0.2f)\n", fix(h1), fix(h1 - 1.96 * h1e), fix(h1 + 1.96 * h1e));
    printf("SPAM%% Initial:  %0.2f (%0.2f - %0.2f)", fix(s0), fix(s0 - 1.96 * s0e), fix(s0 + 1.96 * s0e));
    printf("  Final: %0.2g (%0.2f - %0.2f)\n", fix(s1), fix(s1 - 1.96 * s1e), fix(s1 + 1.96 * s1e));
    printf("LAM%%  Initial:  %0.2f (%0.2f - %0.2f)", fix(l0), fix(l0 - 1.96 * l0e), fix(l0 + 1.96 * l0e));
    printf("  Final: %0.2g (%0.2f - %0.2f)\n", fix(l1), fix(l1 - 1.96 * l1e), fix(l1 + 1.96 * l1e));
    return 0;
}

