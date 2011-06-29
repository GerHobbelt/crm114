#!/bin/dash
# $Id:
# (C) >ten.fs.sresu@alpoo< oloap - GPLv2 http://www.gnu.org/licenses/gpl.txt
#classificatore crm114 SSTT base

C=; COPIA=0; TC=osbf; Lc=50000; Ll=; S=0; BT=5; ST=-10; TMPDIR=/dev/shm; R=
#non usate
#
escape () {
  sed -e 's,/,\\/,g' -e 's,\[,\\[,g' -e 's,(,\\(,g' \
    -e 's,:,\\:,g' -e 's,\],\\],g' -e 's,),\\),g'
}
escape2 () {
  sed -e 's,/,\\\\/,g' -e 's,\[,\\\\[,g' -e 's,(,\\\\(,g' \
      -e 's,:,\\\\:,g' -e 's,\],\\\\],g' -e 's,),\\\\),g'
}

uso () {
  crm \
  "-{match (:: :help:) /.*\n( .*# aiuto\n.*# opzioni crm114 [^\n]*\n).*/
    output /${0##*/} [opt] < msg\n\n:*:help:\n/
    output /predefiniti: TC=$TC, S=$S, Lc=$Lc, Ll=$Ll, COPIA=$S R=$R\n/
    output /    CS=:*:_env_PWD:\/spam.cfc, CB=:*:_env_PWD:\/boni.cfc\n/ 
    output /    TMPDIR=:*:_env_TMPDIR: (o :*:_env_PWD:\/tmp o \/dev\/shm)\n/
    output /    se classifica, e COPIA=1, scrive \n/
    output /    :*:_env_PWD:\/{__spam.mbx,__boni.mbx,__boh-[sb].mbx}\n\n/
    }" < $0
}

while test "$1";do
  case "$1" in
    -h|--h*|-\?) uso; exit ;; 		# aiuto
    -tc) shift; TC=$1 ;;		# classificatore

    -Lc) shift; Lc=$1 ;;		# processa max Lc car
    -Ll) shift; Ll=$1 ;;		# processa max Ll linee
    -s) shift; S=$1 ;;			# soglia spam/boni (offset)

    -cs) shift; CS=$1 ;;		# file .css classe spam
    -cb) shift; CB=$1 ;;		# file .css classe boni
					
					# se classifica:
    -copia) COPIA=1 ;;			#  copia i msg classificati ...
    -mbs) shift; MBS=$1 ;;		#   come spam qui (MBOX)
    -mbb) shift; MBB=$1 ;;		#   come boni qui (MBOX)
    -mbos) shift; MBOS=$1 ;;		#   come forse spam qui (MBOX)
    -mbob) shift; MBOB=$1 ;;		#   come forse boni qui (MBOX)
    -tmp) shift; TMPDIR=$1 ;;		# usa TMPDIR

    -c) C=; ST=-10; BT=5 ;;		# classifica SSTT
    -is|-ls) C=spam ST=-20; BT=10 ;;	# impara spam SSTTT
    -ib|-lb) C=boni ST=-20; BT=10 ;;	# impara boni SSTTT
    -r) R=1 ;;				# impara x SSTTTR

    -crmopt) shift; CRMOPT=$@; break ;;	# opzioni crm114 - deve essere alla fine
    *) uso; exit ;;
  esac
  shift
done
[ "$R" ] && {
  [ "$C" ] || {
    echo "Rinforza (-r) implica l'uso di -ib o -is." >&2
    exit 1
  }
}
if test "$Ll"; then
  L="-$Ll"
  CRMW=`expr 1000 \* $Ll \* 2`
else
  Lc=${Lc:-50000}
  L="-c $Lc"
  CRMW=`expr 2 \* $Lc`
fi
TC=${TC:-osbf}
case $TC in
  osb*|mark*|winn*) TC="$TC microgroom" ;;
  *) ;;
esac
S=${S:-0}
CS=${CS:-spam.cfc}
CB=${CB:-boni.cfc}
MBS=${MBS:-__spam.mbx}
MBB=${MBB:-__boni.mbx}
MBOS=${MBOS:-__boh-s.mbx}
MBOB=${MBOB:-__boh-b.mbx}
[ -s $CS ] && [ -s $CB ] || {
  echo "Manca $CS o $CB" >&2
  exit 3
}
TMPDIR=${TMPDIR:-/dev/shm}
if [ ! -d $TMPDIR ] || [ ! -w $TMPDIR ]; then
 TMPDIR=$PWD/tmp
 mkdir -p $TMPDIR || {
   echo "Fallito 'mkdir -p $TMPDIR'" >&2
   exit 5
 }
fi
msg=$TMPDIR/msg.$LOGNAME.$$
export msg S CS CB MBS MBB MBOS MBOB TMPDIR L C COPIA TC Lc Ll BT ST
trap "rm -f $msg" 0 1 2 3 4 6 8 9 11 13 15

umask 077
# copia
cat > $msg || {
  echo "Fallita scrittura copia del msg '$msg'" >&2
  exit 7
}

#troppo lento ...
#for v in TMPDIR CS CB MBS MBB MBOS MBOB PWD;do
#  eval e$v=`echo "${!v}"|escape2`
#done

# classifica e mette la copia in __spam.mbx, __boni.mbx, __boh-[sb].mbx
eval head $L < $msg |\
crm -w $CRMW $CRMOPT "-{ \
  isolate (:s:) //
  isolate (:C:) /$C/
  isolate (:R:) /$R/
  isolate (:st:) /$ST/
  isolate (:bt:) /$BT/
  isolate (:i:) /0/
  isolate (:maxI:) /5/
:classifica:
  classify <$TC> (:*:_env_CB: :*:_env_CS:) (:s:)
  #output /:*:s:\n/
  match <nomultiline> [:s:] (:: :p:) /^#0.*pR:[ ]*([-.0-9]+).*/
  #output /:*:p:\n/
  { eval /:@: :*:p: < $S :/
    { match [:C:] /.+/ #impara ...
      #class. spam, ma xe bon ...
      match [:C:] /boni/
      learn <$TC> (:*:_env_CB:)
      eval (:i:) /:@: :*:i: + 1 :/
      { eval /:@: :*:i: < :*:maxI: :/
	#output [stderr] /  corr.:*:i:\/:*:maxI: spam->boni\n/
	output [stdout] /!:*:i:/
	goto /:classifica:/
      } alius {
	output [stdout] /!:*:i:=:*:maxI:/
	output [stderr] \
	  /\nEcceduto max iterazioni :*:maxI: -SSTTT (pR: :*:p:)\n/
	goto /:fine:/
      }
    }
    { eval /:@: :*:p: < $ST :/
      #output /:*:p: -> spam\n/
      output [stdout] /-/
      { match <absent> [:C:] /.+/
        eval /:@: $COPIA = 1:/
	syscall /formail < :*:_env_msg: >> :*:_env_MBS:/
      }
    } alius { #forse spam ...
      { match <absent> [:R:] /.+/
        output [stdout] /xS/
      }
      #output [stderr] /\n:*:p: -> boh -\n/
      { match <absent> [:C:] /.+/
        eval /:@: $COPIA = 1:/
	syscall /formail < :*:_env_msg: >> :*:_env_MBOS:/
      } alius {
	match [:C:] /spam/
	learn <$TC> (:*:_env_CS:)
	{ match [:R:] /.+/ # rinforza
          eval (:i:) /:@: :*:i: + 1 :/
          { eval /:@: :*:i: < :*:maxI: :/
	    output [stdout] /?:*:i:/
	    goto /:classifica:/
          } alius {
	    output [stdout] /?:*:i:=:*:maxI:/
	    output [stderr] \
	      /\nEcceduto max iterazioni :*:maxI: -SSTTTR (pR: :*:p:)\n/
	    goto /:fine:/
	  }
	}
      }
    }
  } alius {
    { match [:C:] /.+/ #impara ...
      #class. bon, ma xe spam ...
      match [:C:] /spam/
      learn <$TC> (:*:_env_CS:)
      eval (:i:) /:@: :*:i: + 1 :/
      { eval /:@: :*:i: < :*:maxI: :/
	#output [stderr] /  corr.:*:i:\/:*:maxI: bin->spam\n/
	output [stdout] /!:*:i:/
	goto /:classifica:/
      } alius {
	output [stdout] /!:*:i:=:*:maxI:/
	output [stderr] \
	  /\nEcceduto max iterazioni :*:maxI: +SSTTT (pR: :*:p:)\n/
	goto /:fine:/
      }
    }
    { eval /:@: :*:p: > $BT :/
      #output /:*:p: -> bon\n/
      output [stdout] /+/
      { match <absent> [:C:] /.+/
        eval /:@: $COPIA = 1:/
	syscall /formail < :*:_env_msg: >> :*:_env_MBB:/
      }
    } alius { #forse bon ...
      { match <absent> [:R:] /.+/
        output [stdout] /xB/
      }
      #output [stderr] /\n:*:p: -> boh +\n/
      { match <absent> [:C:] /.+/
        eval /:@: $COPIA = 1:/
	syscall /formail < :*:_env_msg: >> :*:_env_MBOB:/
      } alius {
	match [:C:] /boni/
	learn <$TC> (:*:_env_CB:)
	{ match [:R:] /.+/ # rinforza
          eval (:i:) /:@: :*:i: + 1 :/
          { eval /:@: :*:i: < :*:maxI: :/
	    output [stdout] /?:*:i:/
	    goto /:classifica:/
          } alius {
	    output [stdout] /?:*:i:=:*:maxI:/
	    output [stderr] \
	      /\nEcceduto max iterazioni :*:maxI: +SSTTTR (pR: :*:p:)\n/
	    goto /:fine:/
	  }
	}
      }
    }
  }
  exit /0/
:fine:
  exit /11/
}"
rc=$?
rm -f $msg
exit $rc

