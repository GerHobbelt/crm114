#! /bin/sh

#
# copy files from the src/ directory to their intended locations.
#
# this script assumes you're running this script as part of the cleanup/autoconf migration process
# based on the CRM114 bleeding edge code from paolo et al (which should be located in src/  )
# 

check_file()
{
	echo "testing: src/$2 -- $1/$2"
	if [ -f src/$2 -a ! -e $1/$2$3 -a ! -e $1/$2 ]
	then
		cp src/$2 $1/$2$3
	fi;
}

check_file	examples	pad.crm				.in
check_file	examples	shroud.crm			.in
check_file	mailfilter	classifymail.crm		.in
check_file	mailfilter	mailfilter.crm			.in
check_file	mailfilter	rewriteutil.crm			.in
check_file	mailfilter	unionintersecttest.crm		.in
check_file	man		include.zmm			.in
check_file	tests		aliustest.crm			.in
check_file	tests		approxtest.crm			.in
check_file	tests		argtest.crm			.in
check_file	tests		backwardstest.crm		.in
check_file	tests		beeptest.crm			.in
check_file	tests		bracktest.crm			.in
check_file	tests		classifytest.crm		.in
check_file	tests		escapetest.crm			.in
check_file	tests		eval_infiniteloop.crm		.in
check_file	tests		exectest.crm			.in
check_file	tests		fataltraptest.crm		.in
check_file	tests		inserttest_a.crm		.in
check_file	tests		inserttest_b.crm		.in
check_file	tests		inserttest_c.crm		.in
check_file	tests		learntest.crm			.in
check_file	tests		match_isolate_test.crm		.in
check_file	tests		matchtest.crm			.in
check_file	tests		mathalgtest.crm			.in
check_file	tests		mathrpntest.crm			.in
check_file	tests		nestaliustest.crm		.in
check_file	tests		overalterisolatedtest.crm	.in
check_file	tests		paolo_overvars.crm		.in
check_file	tests		randomiotest.crm		.in
check_file	tests		rewritetest.crm			.in
check_file	tests		skudtest.crm			.in
check_file	tests		statustest.crm			.in
check_file	tests		traptest.crm			.in
check_file	tests		uncaughttraptest.crm		.in
check_file	tests		unionintersecttest.crm		.in
check_file	tests		userdirtest.crm			.in
check_file	tests		windowtest.crm			.in
check_file	tests		windowtest_fromvar.crm		.in

check_file	docs		classify_details.txt
check_file	docs		COLOPHON.txt
check_file	docs		QUICKREF.txt
check_file	docs		INTRO.txt
check_file	docs		knownbugs.txt
check_file	docs		FAQ.txt
check_file	docs		things_to_do.txt
check_file	docs		README 
check_file	docs		CRM114_Mailfilter_HOWTO.txt 
check_file	docs		inoc_passwd.txt 
check_file	docs		procmailrc.recipe 
check_file	docs		reto_procmail_recipe.recipe 

check_file	examples	blacklist.mfp
check_file	examples	priolist.mfp
check_file	examples	rewrites.mfp
check_file	examples	test_rewrites.mfp
check_file	examples	whitelist.mfp
check_file	examples	whitelist.mfp.example

check_file	examples	megatest_knowngood.log
check_file	examples	megatest.sh
check_file	examples	pad.dat

check_file	examples	pad.crm
check_file	examples	shroud.crm

check_file	mailfilter	classifymail.crm
check_file	mailfilter	mailfilter.cf
check_file	mailfilter	mailfilter.crm
check_file	mailfilter	rewriteutil.crm

