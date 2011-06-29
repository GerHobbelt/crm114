#include <stdio.h>

int main()
{
    double r;

    scanf("%lf", &r);
    printf("1-ROCA%%: %0.3f\n", (1 - r) * 100);
    return 0;
}

