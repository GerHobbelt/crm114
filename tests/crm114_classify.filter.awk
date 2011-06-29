#
# AWK script to process classify reports from CRM114 test runs
#

#
# script to see which fields carry what:
#
#    awk '{ printf("1=%s, 2=%s, 3=%s, 4=%s, 5=%s, 6=%s, 7=%s, 8=%s, 9=%s, 10=%s, 11=%s, 12=%s, 13=%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13); printf("val = %le : ", $8); }' < ref/SBPH_Markovian_test1.step3.refoutput

#
# input example:
#
#  type Q 
# CLASSIFY fails; success probability: 0.0000  pR: -10.6507
# Best match to file #1 (q_test.css) prob: 1.0000  pR: 10.6507  
# Total features in input file: 176
# #0 (i_test.css): features: 140946, hits: 693, prob: 2.24e-11, pR: -10.65 
# #1 (q_test.css): features: 133586, hits: 5022, prob: 1.00e+00, pR:  10.65 
#

function tol(val, tolerance, offset)
{
	#printf("TOL: val = %e, tol=%e\n", val, tolerance);
	if (tolerance > 0.0)
	{
		val = 1.0 * val - offset;
	#printf("TOL0: val = %e\n", val);
		val /= tolerance;
	#printf("TOL1: val = %e\n", val);
		val = sprintf("%.0f", val); # eqv. int(val) but also for large numbers
	#printf("TOL2: val = %e\n", val);
		val *= tolerance;
		val += offset;
	}
	#printf("TOLe: val = %e\n", val);
	return val;
}

BEGIN {
	#-v entropy=50 -v jumps=2 -v prob=2.0 pR=2.0
	printf("TOLERANCES: documents=%f, features=%f, file_features=%f, hits=%f, entropy=%f, jumps=%f prob=%e pR=%e chcs=%e\n", documents, features, file_features, hits, entropy, jumps, prob, pR, chcs);
	printf("OFFSETS: o_documents=%f, o_features=%f, o_file_features=%f, o_hits=%f, o_entropy=%f, o_jumps=%f o_prob=%e o_pR=%e o_chcs=%e\n", o_documents, o_features, o_file_features, o_hits, o_entropy, o_jumps, o_prob, o_pR, o_chcs);
}

/CLASSIFY.*; success probability: .* pR:/	{
	printf("CVT: CLASSIFY %s success probability: %f  pR: %f\n", $2, tol($5, prob, o_prob), tol($7, pR, o_pR)); 
	next;
}

/Best match to file.* prob: .* pR:/	{
	# make sure old and new filenames match by stripping off a lot:
	s = $6;
	gsub(/.*\//, "", s);
	sub(/\..*/, "", s);
	sub(/_mt_ng_.*/, "", s);
	sub(/\(/, "", s);
	printf("CVT: Best match to file %s %s prob: %e  pR: %f\n", $5, s, tol($8, prob, o_prob), tol($10, pR, o_pR)); 
	next;
}

/Total features in input file:/	{
	printf("CVT: Total features in input file: %d\n", tol($6, file_features, o_file_features)); 
	next;
}

/: features: [0-9]*, hits: [0-9]*, prob: .*, pR:/	{
	printf("CVT: %s %s features: %d, hits: %d, prob: %e, pR: %f\n", $1, $2, tol($4, features, o_features), tol($6, hits, o_hits), tol($8, prob, o_prob), tol($10, pR, o_pR)); 
	next;
}

/Best match to file.* prob: .* pR:/	{
	printf("CVT: Best match to file %s %s prob: %e  pR: %f\n", $5, $6, tol($8, prob, o_prob), tol($10, pR, o_pR)); 
	next;
}

/: features: .* entropy: .* jumps: .* prob: .* pR:/	{
	printf("CVT: %s %s features: %d, entropy: %e, jumps: %d, prob: %e, pR: %e\n", $1, $2, tol($4, features, o_features), tol($7, entropy, o_entropy), tol($9, jumps, o_jumps), tol($11, prob, o_prob), tol($13, pR, o_pR));
	next;
}


/: documents: .* features: .* prob: .* pR:/	{
	printf("CVT: %s %s documents: %d, features: %d, prob: %e, pR: %e\n", $1, $2, tol($4, documents, o_documents), tol($6, features, o_features), tol($8, prob, o_prob), tol($10, pR, o_pR));
	next;
}

# and then there's a whitespace 'oddity' in vanilla SVM/SKS so here's the above, now
# with the witespace fix so we get proper matches: note the ':documents:' start of the match RE.
/:documents: .* features: .* prob: .* pR:/	{
        s=$2;
	gsub(/:documents:/, ":", s);
	printf("CVT: %s %s documents: %d, features: %d, prob: %e, pR: %e\n", $1, s, tol($3, documents, o_documents), tol($5, features, o_features), tol($7, prob, o_prob), tol($9, pR, o_pR));
	next;
}

/: features: .* L1: .* L2: .* L3: .* [lL]4: .* prob: .* pR:/	{
	# make sure old and new filenames match by stripping off a lot:
	s = $2;
	gsub(/.*\//, "", s);
	sub(/\..*/, "", s);
	sub(/_mt_ng_.*/, "", s);
	sub(/\(/, "", s);
	printf("%s features: %d, L1: %d, L2: %d, L3: %d, L4: %d, prob: %e, pR: %e\n", s, tol($4, features, o_features), $6, $8, $10, $12, tol($14, prob, o_prob), tol($16, pR, o_pR));
	next;
}

# #0 (i_test.css): features: 35239, chcs: 108.39, prob: 1.00e-00, pR:  50.35
/#[0-9] \([^\)]*\): features: .* chcs: .* prob: .* pR: .*/	{
	# make sure old and new filenames match by stripping off a lot:
	s = $2;
	gsub(/.*\//, "", s);
	sub(/\..*/, "", s);
	sub(/\(/, "", s);
	printf("%s =%s=: features: %d chcs: %e prob: %e, pR: %e\n", $1, s, tol($4, features, o_features), tol($6, chcs, o_chcs), tol($8, prob, o_prob), tol($10, pR, o_pR));
	next;
}

# #0 (i_test.css): prob: 9.66e-01, pR: 265.70
/#[0-9] \([^\)]*\): prob: .* pR: .*/	{
	# make sure old and new filenames match by stripping off a lot:
	s = $2;
	gsub(/.*\//, "", s);
	sub(/\..*/, "", s);
	sub(/\(/, "", s);
	printf("%s =%s=: prob: %e, pR: %e\n", $1, s, tol($4, prob, o_prob), tol($6, pR, o_pR));
	next;
}

/-\{.*\( *\.*\/[-a-zA-Z0-9\/._]*/	{
	s = $0;
	gsub(/ \.\//, " ", s);
	gsub(/ \/[-a-zA-Z0-9\/._]*\/[A-Z]/, " *", s);
	printf("%s\n", s);
	next;
}
	
	{
	printf("%s\n", $0);
}
