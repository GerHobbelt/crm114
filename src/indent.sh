#! /bin/bash

cp backup/* ./

for f in *.c *.h ; do 
#	indent -bad -bap -bl -bli0 -bls -cbi4 -cdw -cli0 -di8 -hnl -i4 -l80 -lp -nbc -nce -ncs -npcs -nprs -npsl -nsaf -nsai -nsaw -nsob -nut -pi4 -sc -ss -T index_t -T hitcount_t -T regex_t -T regmatch_t -T regamatch_t -T regaparams_t -T Qitem_t -T regoff_t -T tre_char_t -T COOCCURRENCE_SCORE_TYPE $f
#	indent -bad -bap -bbo -bl -bli0 -bls -cbi4 -cdw -cli0 -di8 -hnl -i4 -l78 -lp -nbc -nce -ncs -npcs -npro -nprs -npsl -nsaf -nsai -nsaw -nsob -nut -pi4 -sc -ss -T index_t -T hitcount_t -T regex_t -T regmatch_t -T regamatch_t -T regaparams_t -T Qitem_t -T regoff_t -T tre_char_t -T COOCCURRENCE_SCORE_TYPE $f
	uncrustify -c ../uncrustify.cfg --replace -F $f
done;
