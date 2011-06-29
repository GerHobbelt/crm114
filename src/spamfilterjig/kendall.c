#include <stdio.h>
#include <string.h>

int i, j, k;
char n[200][2][100], c[200][2][100];
int z, r[200][2], inv;
double s[200][2];

int main()
{
    for (i = 0; 4 == scanf("%d%s%s%lf", &r[i][0], n[i][0], c[i][0], &s[i][0]); i++)
    {
        while (4 != (z = scanf("%d%s%s%lf", &r[i][1], n[i][1], c[i][1], &s[i][1])) || strcmp(n[i][0], n[i][1]))
        {
            printf("oops! %s %s\n", n[i][0], n[i][1]);
            if (z != 4)
                break;
            strcpy(n[i][0], n[i][1]);
            r[i][0] = r[i][1];
        }
    }
    for (j = 0; j < i; j++)
    {
        for (k = j + 1; k < i; k++)
        {
            inv += (r[j][0] > r[k][0]) != (r[j][1] > r[k][1]);
        }
    }
    printf("inv %d tot %d kendall %0.3f\n", inv, i * (i - 1) / 2, (double)(i * (i - 1) / 2 - inv - inv) / (i * (i - 1) / 2));
    return 0;
}

