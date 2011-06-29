#include <stdio.h>

FILE *f[100];
int n[100], z[100];
int i, j, k, m;
char buf[2000];

int main(int argc, char **argv)
{
    for (i = 1; i < argc; i++)
    {
        f[i] = fopen(argv[i], "r");
        if (!f[i])
        {
            perror(argv[i]);
            exit(1);
        }
        for (n[i] = 0; fgets(buf, 1000, f[i]); n[i]++)
            ;
        fclose(f[i]);
    }
    for (i = 1; i < argc; i++)
    {
        f[i] = fopen(argv[i], "r");
        if (!f[i])
        {
            perror(argv[i]);
            exit(1);
        }
    }
    for (i = 1; i <= 1000; i++)
    {
        for (j = 1; j < argc; j++)
        {
            for (; z[j] *1000 < i * n[j]; z[j]++)
            {
                fgets(buf, 1000, f[j]);
                printf("%s", buf);
            }
        }
    }
    return 0;
}

