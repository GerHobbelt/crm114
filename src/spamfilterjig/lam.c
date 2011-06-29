#include <math.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    double a, b;

    sscanf(argv[1], "%lf", &a);
    sscanf(argv[2], "%lf", &b);
    printf("%0.5g", (a + b) / 2);
    return 0;
}

