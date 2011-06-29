/* $Id:
 * (C) >ten.fs.sresu@alpoo< oloap - GPLv2 http://www.gnu.org/licenses/gpl.txt
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int main (int argc, char **argv) {
  int skip,lcase,cp,c,l,m,n,tp,debug,drop,subs,join,ns,as,ss,ps;
  char *p,s[1000],t[1000];
  char numeri[11]  ="1234567890";
  char snum[11]    ="izegsbtxqo";
  char accenti[56] ="àáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ";
  char saccen[56]  ="aaaaaaaceeeeiiiidnoooooiouuuuypyaaaaaaaceeeeiiiidnooooo";
  char speciali[46]="|!$@×ØÙÚÛÜİŞß¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿`";
  char sspec[46]   ="lisaxouuuuypsiclxylsxcaxxxrxopzexmpxxioxgzexx";
  char spazio[31]  ="\n\r\t\b\v<>[]()=?'\"{}%&\\-_;:,./|*+";
  char punt[11]    ="-_;:,./|*+";
  char *discard;

  l = 30;
  m = 3;
  lcase = 1;
  s[0] = t[0] = '\0';
  skip = debug = drop = join = subs = 0;
  ns = as = ss = ps = 1;
  discard=punt;
  for(n=1;n<argc;n++) {
    if(strncmp(argv[n],"-l",2)==0 && argc > n+1)
      sscanf (argv[1+n], "%d", &l);
    else if(strncmp(argv[n],"-m",2)==0 && argc > n+1)
      sscanf (argv[n+1], "%d", &m);
    else if(strncmp(argv[n],"-i",2)==0 && argc > n+1)
      discard=argv[n+1];
    else if(strncmp(argv[n],"-D",2)==0)
      debug=1;
    else if(strncmp(argv[n],"-d",2)==0)
      drop=1;
    else if(strncmp(argv[n],"-n",2)==0)
      ns=0;
    else if(strncmp(argv[n],"-a",2)==0)
      as=0;
    else if(strncmp(argv[n],"-s",2)==0)
      ss=0;
    else if(strncmp(argv[n],"-S",2)==0)
      subs=1;
    else if(strncmp(argv[n],"-c",2)==0)
      lcase=0;
    else if(strncmp(argv[n],"-p",2)==0) {
      ps=0; spazio[0]='\n'; spazio[1]='\0';
    } else if(strncmp(argv[n],"-j",2)==0)
      join=1;
    else if(strncmp(argv[n],"-h",2)==0 || strncmp(argv[n],"--h",3)==0)
      debug=-1;
  }
  if(debug==1) fprintf(stderr,
        "args: debug=%d, drop=%d, max=%d, min=%d, join=%d, discard=%s\n"
        "      mangle: numbers=%d, i18n=%d, specials=%d, spaces=%d\n",
        debug,drop,l,m,join,discard,ns,as,ss,ps);
  if(debug==-1 || m>998 || m>=l || m<1 || l<1) {
    printf(
    "strings2 - strings(1) on steroids, or a blend strings(1)+tr(1)+sed(1)"
    "\nto horribly mangle data stream on stdin.\n"
    "This mess, (C) >ten.fs.sresu@alpoo< oloap\n"
    "License is GNU/GPLv2. NO WARRANTY - Use @ your own risk.\n"
    "\n"
    "Usage: strings2 [-l max] [-m min] [-D[ebug]] [-d[rop]] [-j[oin]]\n"
    "         [-n] [-a] [-s] [-p] [-i \"dropchars\"]\n"
    "     with 998 >= max > min, min < 80, dropchars = chars to discard\n"
    "     [default: max=30 min=3 dropchars=\"%s\" and < 0x20]\n"
    "     -l max  split/discard strings longer than max\n"
    "     -c      keep case (default is to lowercase)\n"
    "     -m min  discard strings shorter than min\n"
    "     -drop   discard longer (than max) strings\n"
    "     -Subs   print 'LongerThan%%d' for strings longer (than max) strings\n"
    "     -Debug  print every subst done.\n"
    "     -join   join <1-char><space>*<1-char> sequences.\n"
    "     -n      don't do numbers subst.\n"
    "     -a      don't do international language subst.\n"
    "     -s      don't do specials subst.\n"
    "     -p      don't do spaces subst (except \\n ->' ' unless \\n is\n"
    "             in dropchars) .\n"
    " Description:\n"
    "   strings2 reads stdin and writes it on stdout after mangling it\n"
    "   as follows:\n"
    "   -numbers-\n"
    "   in : %s\n"
    "   out: %s\n"
    "   -i18n-\n"
    "   in : %s\n"
    "   out: %s\n"
    "   -specials-\n"
    "   in : %s\n"
    "   out: %s\n"
    "   -spaces-\n"
    "   in : ",
         discard,numeri,snum,accenti,saccen,speciali,sspec);
    for(n=0;n<strlen(spazio);n++) {
      if(spazio[n]=='\n')
        fputs("\\n",stdout);
      else if(spazio[n]=='\r')
        fputs("\\r",stdout);
      else if(spazio[n]=='\b')
        fputs("\\b",stdout);
      else if(spazio[n]=='\t')
        fputs("\\t",stdout);
      else if(spazio[n]=='\v')
        fputs("\\v",stdout);
      else 
        fputc(spazio[n],stdout);
    }
    fputc('\n',stdout);
    printf(
         "   out: ' ' - except chars in dropchars, which get discarded.\n"
         "   String after subst can be max chars at most: with -d it's \n"
         "   discarded at all, otherwise it's splitted.\n"
         "   Strings shorter than min chars are discarded.\n"
         "   With -j, non-blank chars in 1-char sequences like '. .  . .'\n"
         "   longer than min are joined and written on stdout.\n"
         "   Chars < 0x20 are always discarded.\n"); 
    exit(1);
  }
  n = tp = 0;
  cp=' ';
  while (1) {
    c=fgetc(stdin);
    if(feof(stdin)) break;
    if(lcase) c=tolower(c);
    if(discard!=NULL && strchr(discard,c)!=NULL) {
      if(debug==1) fprintf(stderr,"elimina: %x -> '', n=%d\n",c,n);
      continue;
    }else if(ns==1 && (p=strchr(numeri,c))!=NULL) {
      if(debug==1) 
        fprintf(stderr,"numeri: %x -> %x, n=%d\n",
                c,snum[p-numeri],n);
      c=snum[p-numeri];
    }else if(as==1 && (p=strchr(accenti,c))!=NULL) {
      if(debug==1) 
        fprintf(stderr,"accenti: %x -> %x, n=%d\n",
                c,saccen[p-accenti],n);
      c=saccen[p-accenti];
    }else if(ss==1 && (p=strchr(speciali,c))!=NULL) {
      if(debug==1) 
        fprintf(stderr,"speciali: %x -> %x, n=%d\n",
                c,sspec[p-speciali],n);
      c=sspec[p-speciali];
    }else if(strchr(spazio,c)!=NULL) {
      if(debug==1) fprintf(stderr,"spazi: %x -> ' ', n=%d\n",c,n);
      c=' ';
    }else if(c < 0x20) {
      if(debug==1) fprintf(stderr,"< 0x20: %x -> '', n=%d\n",c,n);
      continue;
    }
    if(c==' ') {
      if(cp==' ') continue;
      skip=0;
      if(n>=m) {
        if(drop==0 || n<=l) {
          s[n++]=' '; s[n]='\0';
          fputs(s,stdout);
        }
        t[0]='\0'; tp=0;
      }else if(n==1==join) {
        if(tp==l) {
          if(drop==0) {
            t[tp++]=' '; t[tp]='\0';
            fputs(t,stdout);
          }
          t[0]='\0'; tp=0;
        }
        t[tp++]=s[0]; t[tp]='\0';
      }
      s[0]='\0';
      n=0;
    } else {
      if(skip) {
        cp=c;
        continue;
      }
      s[n++]=c; s[n]='\0';
      if(join==1 && n>1) {
        if(tp >= m) {
          t[tp++]=' '; t[tp]='\0';
          fputs(t,stdout);
        }
        t[0]='\0'; tp=0;
      }
      if(n==l) {
        if (subs==1) {
          skip=1;
          printf("LongerThan%d ",l);
        }else if(drop==0) {
          s[n++]=' '; s[n]='\0';
          fputs(s,stdout);
        } else
          skip=1;
        s[0] = t[0] = '\0';
        n = tp = 0;
        cp=' ';
      } else
        cp=c;
    }
  }
  //if(join==1 && strlen(t) >= m && (drop==0 || strlen(t)<=l)) printf("%s ",t);
  if(join==1 && tp >= m && (drop==0 || tp <=l)) printf("%s ",t);
  if(n>=m && (drop==0 || n<=l)) fputs(s,stdout);
  exit (0);
}


