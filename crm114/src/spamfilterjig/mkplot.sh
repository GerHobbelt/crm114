#! /bin/sh

cat $1 | awk -f templates/counted_graphs.awk | sed -e 's/postscript eps/postscript color/' | gnuplot | ps2pdf - > $1.pdf
