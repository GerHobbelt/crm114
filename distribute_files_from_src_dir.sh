#! /bin/sh

#
# copy files from the src/ directory to their intended locations.
#
# this script assumes you're running this script as part of the cleanup/autoconf migration process
# based on the CRM114 bleeding edge code from paolo et al (which should be located in src/  )
# 

check_file()
{
	echo "copying           : src/$2 -- $1/$2$3"
	if [ -f src/$2 ]
	then
		cp src/$2 $1/$2$3
	fi
}

process_file()
{
	echo "copying[processed]: src/$2 -- $1/$2$3"
	if [ -f src/$2 ]
	then
		cat src/$2 | sed -e 's,#! \+\/usr\/bin\/crm,#! @CRM@,g' > $1/$2$3
	fi
}

process_file	examples	pad.crm				.in
process_file	examples	shroud.crm			.in
process_file	mailfilter	classifymail.crm		.in
process_file	mailfilter	mailfilter.crm			.in
process_file	mailfilter	rewriteutil.crm			.in
process_file	tests		unionintersecttest.crm		.in
process_file	tests		aliustest.crm			.in
process_file	tests		approxtest.crm			.in
process_file	tests		argtest.crm			.in
process_file	tests		backwardstest.crm		.in
process_file	tests		beeptest.crm			.in
process_file	tests		bracktest.crm			.in
process_file	tests		classifytest.crm		.in
process_file	tests		escapetest.crm			.in
process_file	tests		eval_infiniteloop.crm		.in
process_file	tests		exectest.crm			.in
process_file	tests		fataltraptest.crm		.in
process_file	tests		inserttest_a.crm		.in
process_file	tests		inserttest_b.crm		.in
process_file	tests		inserttest_c.crm		.in
process_file	tests		learntest.crm			.in
process_file	tests		match_isolate_test.crm		.in
process_file	tests		matchtest.crm			.in
process_file	tests		mathalgtest.crm			.in
process_file	tests		mathrpntest.crm			.in
process_file	tests		nestaliustest.crm		.in
process_file	tests		overalterisolatedtest.crm	.in
process_file	tests		paolo_overvars.crm		.in
process_file	tests		randomiotest.crm		.in
process_file	tests		rewritetest.crm			.in
process_file	tests		skudtest.crm			.in
process_file	tests		statustest.crm			.in
process_file	tests		traptest.crm			.in
process_file	tests		uncaughttraptest.crm		.in
process_file	tests		unionintersecttest.crm		.in
process_file	tests		userdirtest.crm			.in
process_file	tests		windowtest.crm			.in
process_file	tests		windowtest_fromvar.crm		.in

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

check_file	tests		megatest_knowngood.log
check_file	tests		megatest.sh			.BillY
check_file	examples	pad.dat

process_file	examples	pad.crm				.in
process_file	examples	shroud.crm			.in

process_file	mailfilter	classifymail.crm		.in
check_file	mailfilter	mailfilter.cf
process_file	mailfilter	mailfilter.crm			.in
process_file	mailfilter	rewriteutil.crm			.in

