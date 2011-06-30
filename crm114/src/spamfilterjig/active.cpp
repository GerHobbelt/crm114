#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
using namespace std;

char fname[200], judge[200], clas[200], tfile[200], buf[1000], res[1000];
int i, j, k, nf, limit;
FILE *in, *out;
double score;
int n, m, ndone;
string Name[400000];
char Class[400000];
char Done[400000];

int main(int argc, char **argv)
{
    if (argc < 4)
        strcpy(res, "results");
    else
        strcpy(res, argv[3]);
    if (argc >= 2)
    {
        sprintf(fname, "%s/index", argv[1]);
        if (!freopen(fname, "r", stdin))
        {
            perror(fname);
            exit(1);
        }
    }
    else
    {
        printf("usage: %s corpus\n", argv[0]);
        exit(1);
    }
    if (!(out = fopen(res, "w")))
    {
        perror("results file");
        exit(1);
    }
    system("rm -f filter_out");
    system("sh ./initialize < /dev/null");
    printf("done init\n");
    for (n = 0; gets(buf); n++)
    {
        if (2 != sscanf(buf, "%s%s", judge, fname))
        {
            printf("bad line in index file: %s\n", buf);
        }
        Name[n] = fname;
        Class[n] = judge[0];
    }
    m = (int)(n * 0.9); /* last 10% is test set */
    for (limit = 100; limit <= m; limit *= 2)
    {
        for (; ndone < limit; ndone++)
        {
            while (Done[k = random() % m])
            {}
            Done[k] = 1;
            sprintf(buf, "./train %s %s%s < /dev/null", Class[k] == 's' ? "spam" : "ham", argv[1], Name[k].c_str());
            printf("doing %s\n", buf);
            system(buf);
        }
        for (k = m; k < n; k++)
        {
            sprintf(buf, "./classify %s%s </dev/null > filter_out", argv[1], Name[k].c_str());
            printf("doing %s\n", buf);
            system(buf);
            in = fopen("filter_out", "r");
            if (!in)
            {
                perror("filter_out");
                exit(1);
            }
            nf = fscanf(in, "class=%s score=%lf tfile=%s", clas, &score, tfile);
            if (nf < 2)
                score = 0;
            if (nf < 1)
                strcpy(clas, "ham");
            fprintf(out, "%s judge=%s class=%s score=%0.8f teach=%d\n", fname,
                    Class[k] == 's' ? "spam" : "ham", clas, score, limit);
            fflush(out);
            fclose(in);
        }
    }
    system("./finalize");
    return 0;
}

