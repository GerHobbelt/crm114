
all: 		bin_dir \
		boot boot2 boot2a \
		diff1 roca binom3 regress normalize plotroc plotroc_line lam \
		run_sh active_sh \
		scripts

scripts: script script2
script: bin/script
bin/script: src/script
	cp src/script bin

script2: bin/script2
bin/script2: src/script2
	cp src/script2 bin

bin_dir:
	mkdir bin || echo bin directory already present

boot: bin/boot 
bin/boot: src/bootstep.c 
	gcc -O2 -Wall -pedantic  -o bin/boot src/bootstep.c -lm

boot2: bin/boot2 
bin/boot2: src/bootstrap2.c
	gcc -O2 -Wall -pedantic  -o bin/boot2 src/bootstrap2.c -lm

boot2a: bin/boot2a 
bin/boot2a: src/bootstrap2a.c  
	gcc -O2 -Wall -pedantic  -o bin/boot2a src/bootstrap2a.c -lm

diff1: bin/diff1 
bin/diff1: src/diff1.c
	gcc -O2 -Wall -pedantic  -o bin/diff1 src/diff1.c -lm

roca: bin/roca 
bin/roca: src/roca.c
	gcc -O2 -Wall -pedantic  -o bin/roca src/roca.c -lm

binom3: bin/binom3 
bin/binom3: src/binom3.c 
	gcc -O2 -Wall -pedantic  -o bin/binom3 src/binom3.c -lm

regress: bin/regress 
bin/regress: src/regress.c 
	gcc -O2 -Wall -pedantic  -o bin/regress src/regress.c -lm

normalize: bin/normalize 
bin/normalize: src/normalize.c 
	gcc -O2 -Wall -pedantic  -o bin/normalize src/normalize.c -lm

plotroc: bin/plotroc 
bin/plotroc: src/plotroc.c  
	gcc -O2 -Wall -pedantic  -o bin/plotroc src/plotroc.c -lm

plotroc_line: bin/plotroc_line 
bin/plotroc_line: src/plotroc_line.c 
	gcc -O2 -Wall -pedantic  -o bin/plotroc_line src/plotroc_line.c -lm

lam: bin/lam 
bin/lam: src/lam.c 
	gcc -O2 -Wall -pedantic  -o bin/lam src/lam.c -lm

run_sh: ../run.sh 
../run.sh: src/run.cpp
	g++ -O2 -Wall -pedantic  -o ../run.sh src/run.cpp

active_sh: ../active.sh 
../active.sh: src/active.cpp
	g++ -O2 -Wall -pedantic  -o ../active.sh src/active.cpp



erase:
	rm -r tmp results

clean:
	rm -r bin tmp results ../run.sh ../active.sh

go: all
	./exec.sh


