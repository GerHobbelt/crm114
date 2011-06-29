#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
using namespace std;

char fname[200], judge[200], clas[200], tfile[200], buf[1000], res[1000];
int i, j, k, nf;
FILE *in, *out;
double score;

char argv_rest[2048];

map<string, string> tmap;
map<string, string> cmap;


/*
 * case $# in
 * 3) cpath=$1 ; runid=$2 ;output=$3 ;;
 * 2) cpath=$1 ; runid=$2 ;output=results ;;
 * 1) cpath=$1 ; runid=none ;output=results ;;
 * 0) cpath=`pwd`;  output=results ;;
 */
int main(int argc, char **argv)
{
    int i;

    argv_rest[0] = 0;
    for (i = 4; i < argc; i++)
    {
        sprintf(argv_rest + strlen(argv_rest), "%s ", argv[i]);
    }
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
        printf("usage: %s corpusdir runid [resultfile [classify-args]]\n", argv[0]);
        exit(1);
    }
    if (!(out = fopen(res, "w")))
    {
        perror("results file");
        exit(1);
    }
    system("rm -f filter_out");
    sprintf(buf, "sh ./initialize %s < /dev/null", argv_rest);
    system(buf);
    printf("done init\n");
    while (gets(buf))
    {
        printf("buf %s\n", buf);
        if (2 != sscanf(buf, "%s%s", judge, fname))
        {
            printf("bad line in index file: %s\n", buf);
        }
        printf("judge %s fname %s end\n", judge, fname);
        if (!strcmp(judge, "ham") || !strcmp(judge, "spam")
           || !strcmp(judge, "Ham") || !strcmp(judge, "Spam"))
        {
            sprintf(buf, "./classify %s%s %s </dev/null > filter_out", argv[1], fname, argv_rest);
            printf("doing %s\n", buf);
            system(buf);
            in = fopen("filter_out", "r");
            if (!in)
            {
                perror("filter_out");
                exit(1);
            }
            strcpy(tfile, "");
            nf = fscanf(in, " class=%s score=%lf tfile=%s", clas, &score, tfile);
            tmap[fname] = tfile;
            cmap[fname] = clas;
            if (nf < 2)
                score = 0;
            if (nf < 1)
                strcpy(clas, "ham");
            fprintf(out, "%s judge=%s class=%s score=%0.8f\n", fname,
                    judge[1] == 'p' ? "spam" : "ham", clas, score);
            fflush(out);
            fclose(in);
        }
        if (!strcmp(judge, "ham") || !strcmp(judge, "HAM"))
        {
            sprintf(buf, "./train %s %s%s %s %s %s < /dev/null", "ham", argv[1], fname, cmap[fname].c_str(), tmap[fname].c_str(), argv_rest);
            printf("doing %s\n", buf);
            system(buf);
        }
        if (!strcmp(judge, "spam") || !strcmp(judge, "SPAM"))
        {
            sprintf(buf, "./train %s %s%s %s %s %s < /dev/null", "spam", argv[1], fname, cmap[fname].c_str(), tmap[fname].c_str(), argv_rest);
            printf("doing %s\n", buf);
            system(buf);
        }
    }
    sprintf(buf, "./finalize %s", argv_rest);
    system(buf);
    return 0;
}

