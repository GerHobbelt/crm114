//  crm_cuckoo_markov.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//
//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"




/////////////////////////////////////////////////////////////////////////////
//
// Test hash function properties
//
// - avalanche
//


typedef crmhash_t strnhash_f(const unsigned char *str, int len, crmhash_t seed);

#define INPUT_BYTE_WIDTH	128


#undef MIN
#undef MAX
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define MAX(a, b)  ((a) > (b) ? (a) : (b))



#include "crm_test_hashes.h"




int state[sizeof(crmhash_t) * 8];
int counts[MAX(MAX(512000, sizeof(teststr_derived)/sizeof(teststr_derived[0])), MAX(sizeof(teststr), INPUT_BYTE_WIDTH * 8))];
int spread16[65536];


/* Chi2 code ripped from http://www.fourmilab.ch/rpkp/experiments/analysis/chiCalc.html */


 /*  The following JavaScript functions for calculating normal and
        chi-square probabilities and critical values were adapted by
        John Walker from C implementations
        written by Gary Perlman of Wang Institute, Tyngsboro, MA
        01879.  Both the original C code and this JavaScript edition
        are in the public domain.  */

    /*  POZ  --  probability of normal z value

        Adapted from a polynomial approximation in:
                Ibbetson D, Algorithm 209
                Collected Algorithms of the CACM 1963 p. 616
        Note:
                This routine has six digit accuracy, so it is only useful for absolute
                z values < 6.  For z values >= to 6.0, poz() returns 0.0.
    */

    double poz(double z) 
	{
        double y, x, w;
        double Z_MAX = 6.0;              /* Maximum meaningful z value */
        
        if (z == 0.0) 
		{
            x = 0.0;
        } 
		else 
		{
            y = 0.5 * fabs(z);
            if (y >= (Z_MAX * 0.5)) {
                x = 1.0;
            } else if (y < 1.0) {
                w = y * y;
                x = ((((((((0.000124818987 * w
                         - 0.001075204047) * w + 0.005198775019) * w
                         - 0.019198292004) * w + 0.059054035642) * w
                         - 0.151968751364) * w + 0.319152932694) * w
                         - 0.531923007300) * w + 0.797884560593) * y * 2.0;
            } else {
                y -= 2.0;
                x = (((((((((((((-0.000045255659 * y
                               + 0.000152529290) * y - 0.000019538132) * y
                               - 0.000676904986) * y + 0.001390604284) * y
                               - 0.000794620820) * y - 0.002034254874) * y
                               + 0.006549791214) * y - 0.010557625006) * y
                               + 0.011630447319) * y - 0.009279453341) * y
                               + 0.005353579108) * y - 0.002141268741) * y
                               + 0.000535310849) * y + 0.999936657524;
            }
        }
        return z > 0.0 ? ((x + 1.0) * 0.5) : ((1.0 - x) * 0.5);
    }

 
    static double BIGX = 20.0;                  /* max value to represent exp(x) */

    double ex(double x) {
        return (x < -BIGX) ? 0.0 : exp(x);
    }   

    /*  POCHISQ  --  probability of chi-square value

              Adapted from:
                      Hill, I. D. and Pike, M. C.  Algorithm 299
                      Collected Algorithms for the CACM 1967 p. 243
              Updated for rounding errors based on remark in
                      ACM TOMS June 1985, page 185
    */

    double pochisq(double x, int df) {
        double a, y, s;
        double e, c, z;
        int even;                     /* True if df is an even number */

        double LOG_SQRT_PI = 0.5723649429247000870717135; /* log(sqrt(pi)) */
        double I_SQRT_PI = 0.5641895835477562869480795;   /* 1 / sqrt(pi) */
        
        if (x <= 0.0 || df < 1) {
            return 1.0;
        }
        
        a = 0.5 * x;
        even = !(df & 1);
		y = 0;
        if (df > 1) {
            y = ex(-a);
        }
        s = (even ? y : (2.0 * poz(-sqrt(x))));
        if (df > 2) {
            x = 0.5 * (df - 1.0);
            z = (even ? 1.0 : 0.5);
            if (a > BIGX) {
                e = (even ? 0.0 : LOG_SQRT_PI);
                c = log(a);
                while (z <= x) {
                    e = log(z) + e;
                    s += ex(c * z - a - e);
                    z += 1.0;
                }
                return s;
            } else {
                e = (even ? 1.0 : (I_SQRT_PI / sqrt(a)));
                c = 0.0;
                while (z <= x) {
                    e = e * (a / z);
                    c = c + e;
                    z += 1.0;
                }
                return c * y + s;
            }
        } else {
            return s;
        }
    }

    /*  CRITCHI  --  Compute critical chi-square value to
                     produce given p.  We just do a bisection
                     search for a value within CHI_EPSILON,
                     relying on the monotonicity of pochisq().  */

    double critchi(double p, int df) {
        double CHI_EPSILON = 0.000001;   /* Accuracy of critchi approximation */
        double CHI_MAX = 99999.0;        /* Maximum chi-square value */
        double minchisq = 0.0;
        double maxchisq = CHI_MAX;
        double chisqval;
        
        if (p <= 0.0) {
            return maxchisq;
        } else {
            if (p >= 1.0) {
                return 0.0;
            }
        }
        
        chisqval = df / sqrt(p);    /* fair first value */
        while ((maxchisq - minchisq) > CHI_EPSILON) {
            if (pochisq(chisqval, df) < p) {
                maxchisq = chisqval;
            } else {
                minchisq = chisqval;
            }
            chisqval = (maxchisq + minchisq) * 0.5;
        }
        return chisqval;
    }
    

    //  CALC_X_DF  --  Button action to calculate Q from X and DF

    double calc_x_df(double chi2, int df)
    {
        return pochisq(chi2, df);
    }

    //  CALC_Q_DF  --  Button action to calculate X from Q and DF

    double calc_q_df(double q, int df) {
        return critchi(q, df);
    }


/* end of rip - Chi2 code ripped from http://www.fourmilab.ch/rpkp/experiments/analysis/chiCalc.html */



void show_stats(int input_width)
{
	int bp;
        double expected_bitcount_p = 0.5; // half the bits should see change for every single bit of change in input
        double expected_stateval_p = 0.5; // half the bits should see change for every single bit of change in input
        double chi2;
        double chi2_sum;
	int min_c, max_c;
	int sum;
	double expected_value;
	double expected_value_orig;
	double q;
	int df;
	int count_dist[1 + sizeof(crmhash_t) * 8] = {0};
	int spread_cnt[65536];
	
	printf("CHI2 avalanche bitcount per single input bit: (input count k = %8d)\n", input_width);

	sum = 0;
	min_c = max_c = counts[0];

	for (bp = 0; bp < input_width; bp++)
	{
		int c = counts[bp];
		
		if (c >= 0 && c < sizeof(crmhash_t) * 8)
			count_dist[c]++;
		else
			count_dist[sizeof(crmhash_t) * 8]++;

		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);
		sum += c;
	}

	printf("bit counts: error: %7d\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", count_dist[sizeof(crmhash_t) * 8], min_c, max_c, sum, max_c - min_c);

	df = sizeof(crmhash_t) * 8 - 1; // number of possible values - 1
	expected_value_orig = (sizeof(crmhash_t) * 8) * expected_bitcount_p;
	expected_value = ((double)sum) / input_width;

	sum = 0;
	chi2_sum = 0.0;

	min_c = max_c = counts[0];

	for (bp = 0; bp < sizeof(crmhash_t) * 8; bp++)
	{
		int c = count_dist[bp];
		
		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);
		sum += c;

		chi2 = c - expected_value;
		chi2 *= chi2;
		// chi2 *= fabs(chi2);
		chi2 /= expected_value;
		chi2_sum += chi2;
//		printf("bit %2d: chi2 = %7.4lf, overflow factor: %6.0f, count: %5d%s", bp, chi2, chi2 / min_allowable_chi2, counts[bp], (bp & 1 ? "\n": "         "));
	}
    q = calc_x_df(chi2_sum, df);

	printf("bit count freq:\n"
		"chi2 sum: %10.4lf, Q: %10.4lf, df: %5d, expected: %10.4lf/%10.4lf,\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", chi2_sum, q, df, expected_value, expected_value_orig, min_c, max_c, sum, max_c - min_c);

	printf("CHI2 for probability of change in a bit in hash for any single bit in input:\n");

	sum = 0;
	chi2_sum = 0.0;

	min_c = max_c = state[0];

	//state[0] = INPUT_BYTE_WIDTH * 8;
	//state[1] = 0;
	//state[2] = INPUT_BYTE_WIDTH * 8 / 2 + 1;
	for (bp = 0; bp < WIDTHOF(state); bp++)
	{
		int c = state[bp];
		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);
		sum += c;
	}

	df = WIDTHOF(state) - 1; // number of possible values - 1
	expected_value = input_width * expected_stateval_p;
	expected_value_orig = input_width * ((double)sum) / (WIDTHOF(state) * input_width);

	for (bp = 0; bp < WIDTHOF(state); bp++)
	{
		int c = state[bp];

		chi2 = c - expected_value;
		chi2 *= chi2;
		// chi2 *= fabs(chi2);
		chi2 /= expected_value;
		chi2_sum += chi2;
//		printf("hashbit %2d: chi2 = %7.4lf, overflow factor: %6.0f, count: %5d%s", bp, chi2, chi2 / min_allowable_chi2, state[bp], (bp & 1 ? "\n" : "     "));
	}
    q = calc_x_df(chi2_sum, df);

	printf("bit change counts:\n"
		"chi2 sum: %10.4lf, Q: %10.4lf, df: %5d, expected: %10.4lf/%10.4lf,\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", chi2_sum, q, df, expected_value, expected_value_orig, min_c, max_c, sum, max_c - min_c);



	printf("CHI2 for spread 4/8/16:\n");



	sum = 0;
	chi2_sum = 0.0;
	memset(spread_cnt, 0, sizeof(spread_cnt));

	min_c = INT_MAX;
	max_c = 0;

	for (bp = 0; bp < WIDTHOF(spread16); bp++)
	{
		int c = spread16[bp];

		spread_cnt[bp & 0xF] += c;
		sum += c;
	}

	df = 16 - 1; // number of possible values - 1
	expected_value = ((double)sum) / 16;

	for (bp = 0; bp < 16; bp++)
	{
		int c = spread_cnt[bp];

		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);

		chi2 = c - expected_value;
		chi2 *= chi2;
		// chi2 *= fabs(chi2);
		chi2 /= expected_value;
		chi2_sum += chi2;
//		printf("hashbit %2d: chi2 = %7.4lf, overflow factor: %6.0f, count: %5d%s", bp, chi2, chi2 / min_allowable_chi2, state[bp], (bp & 1 ? "\n" : "     "));
	}
    q = calc_x_df(chi2_sum, df);

	printf("4-bit value spread:\n"
		"chi2 sum: %10.4lf, Q: %10.4lf, df: %5d, expected: %10.4lf/%10.4lf,\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", chi2_sum, q, df, expected_value, expected_value_orig, min_c, max_c, sum, max_c - min_c);



	sum = 0;
	chi2_sum = 0.0;
	memset(spread_cnt, 0, sizeof(spread_cnt));

	min_c = INT_MAX;
	max_c = 0;

	for (bp = 0; bp < WIDTHOF(spread16); bp++)
	{
		int c = spread16[bp];

		spread_cnt[bp & 0xFF] += c;
		sum += c;
	}

	df = 256 - 1; // number of possible values - 1
	expected_value = ((double)sum) / 256;

	for (bp = 0; bp < 256; bp++)
	{
		int c = spread_cnt[bp];

		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);

		chi2 = c - expected_value;
		chi2 *= chi2;
		// chi2 *= fabs(chi2);
		chi2 /= expected_value;
		chi2_sum += chi2;
//		printf("hashbit %2d: chi2 = %7.4lf, overflow factor: %6.0f, count: %5d%s", bp, chi2, chi2 / min_allowable_chi2, state[bp], (bp & 1 ? "\n" : "     "));
	}
    q = calc_x_df(chi2_sum, df);

	printf("8-bit value spread:\n"
		"chi2 sum: %10.4lf, Q: %10.4lf, df: %5d, expected: %10.4lf/%10.4lf,\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", chi2_sum, q, df, expected_value, expected_value_orig, min_c, max_c, sum, max_c - min_c);



	sum = 0;
	chi2_sum = 0.0;
	memset(spread_cnt, 0, sizeof(spread_cnt));

	min_c = INT_MAX;
	max_c = 0;

	for (bp = 0; bp < WIDTHOF(spread16); bp++)
	{
		int c = spread16[bp];

		spread_cnt[bp & 0xFFF] += c;
		sum += c;
	}

	df = 0x1000 - 1; // number of possible values - 1
	expected_value = ((double)sum) / 0x1000;

	for (bp = 0; bp < 0x1000; bp++)
	{
		int c = spread_cnt[bp];

		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);

		chi2 = c - expected_value;
		chi2 *= chi2;
		// chi2 *= fabs(chi2);
		chi2 /= expected_value;
		chi2_sum += chi2;
//		printf("hashbit %2d: chi2 = %7.4lf, overflow factor: %6.0f, count: %5d%s", bp, chi2, chi2 / min_allowable_chi2, state[bp], (bp & 1 ? "\n" : "     "));
	}
    q = calc_x_df(chi2_sum, df);

	printf("12-bit value spread:\n"
		"chi2 sum: %10.4lf, Q: %10.4lf, df: %5d, expected: %10.4lf/%10.4lf,\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", chi2_sum, q, df, expected_value, expected_value_orig, min_c, max_c, sum, max_c - min_c);



	sum = 0;
	chi2_sum = 0.0;

	min_c = INT_MAX;
	max_c = 0;

	for (bp = 0; bp < WIDTHOF(spread16); bp++)
	{
		int c = spread16[bp];

		sum += c;
	}

	df = WIDTHOF(spread16) - 1; // number of possible values - 1
	expected_value = ((double)sum) / WIDTHOF(spread16);

	for (bp = 0; bp < WIDTHOF(spread16); bp++)
	{
		int c = spread16[bp];

		min_c = MIN(min_c, c);
		max_c = MAX(max_c, c);

		chi2 = c - expected_value;
		chi2 *= chi2;
		// chi2 *= fabs(chi2);
		chi2 /= expected_value;
		chi2_sum += chi2;
//		printf("hashbit %2d: chi2 = %7.4lf, overflow factor: %6.0f, count: %5d%s", bp, chi2, chi2 / min_allowable_chi2, state[bp], (bp & 1 ? "\n" : "     "));
	}
    q = calc_x_df(chi2_sum, df);

	printf("16-bit value spread:\n"
		"chi2 sum: %10.4lf, Q: %10.4lf, df: %5d, expected: %10.4lf/%10.4lf,\n    min count: %7d, max count: %7d, sum: %7d, delta: %7d\n", chi2_sum, q, df, expected_value, expected_value_orig, min_c, max_c, sum, max_c - min_c);
} 








/* check that every input bit changes every output bit half the time */
#define HASHSTATE 1
#define HASHLEN   1
#define MAXPAIR 60
#define MAXLEN  70

void driver2(strnhash_f hashfunc)
{
    uint8_t qa[MAXLEN + 1], qb[MAXLEN + 2], *a = &qa[0], *b = &qb[1];
    uint32_t c[HASHSTATE], d[HASHSTATE], i = 0, j = 0, k, l, m = 0, z;
    uint32_t e[HASHSTATE], f[HASHSTATE], g[HASHSTATE], h[HASHSTATE];
    uint32_t x[HASHSTATE], y[HASHSTATE];
    uint32_t hlen;

    printf("No more than %d trials should ever be needed \n", MAXPAIR / 2);
    for (hlen = 0; hlen < MAXLEN; ++hlen)
    {
        z = 0;
        for (i = 0; i < hlen; ++i) /*----------------------- for each input byte, */
        {
            for (j = 0; j < 8; ++j) /*------------------------ for each input bit, */
            {
                for (m = 1; m < 8; ++m) /*------------ for serveral possible initvals, */
                {
                    for (l = 0; l < HASHSTATE; ++l)
                        e[l] = f[l] = g[l] = h[l] = x[l] = y[l] = ~((uint32_t)0);

                    /*---- check that every output bit is affected by that input bit */
                    for (k = 0; k < MAXPAIR; k += 2)
                    {
                        uint32_t finished = 1;
                        /* keys have one bit different */
                        for (l = 0; l < hlen + 1; ++l)
                        {
                            a[l] = b[l] = (uint8_t)0;
                        }
                        /* have a and b be two keys differing in only one bit */
                        a[i] ^= (k << j);
                        a[i] ^= (k >> (8 - j));
                        c[0] = hashfunc(a, hlen, m);
                        b[i] ^= ((k + 1) << j);
                        b[i] ^= ((k + 1) >> (8 - j));
                        d[0] = hashfunc(b, hlen, m);
                        /* check every bit is 1, 0, set, and not set at least once */
                        for (l = 0; l < HASHSTATE; ++l)
                        {
                            e[l] &= (c[l] ^ d[l]);
                            f[l] &= ~(c[l] ^ d[l]);
                            g[l] &= c[l];
                            h[l] &= ~c[l];
                            x[l] &= d[l];
                            y[l] &= ~d[l];
                            if (e[l] | f[l] | g[l] | h[l] | x[l] | y[l])
                                finished = 0;
                        }
                        if (finished)
                            break;
                    }
                    if (k > z)
                        z = k;
                    if (k == MAXPAIR)
                    {
                        printf("Some bit didn't change: ");
                        printf("%.8x %.8x %.8x %.8x %.8x %.8x  ",
                                e[0], f[0], g[0], h[0], x[0], y[0]);
                        printf("i %d j %d m %d len %d\n", i, j, m, hlen);
                    }
                    if (z == MAXPAIR)
                        goto done;
                }
            }
        }
done:
        if (z < MAXPAIR)
        {
            printf("Mix success  %2d bytes  %2d initvals  ", i, m);
            printf("required  %d  trials\n", z / 2);
        }
    }
    printf("\n");
}

/* Check for reading beyond the end of the buffer and alignment problems */
void driver3(strnhash_f hashfunc)
{
    uint8_t buf[MAXLEN + 20], *b;
    uint32_t len;
    uint8_t q[] = "This is the time for all good men to come to the aid of their country...";
    uint32_t h;
    uint8_t qq[] = "xThis is the time for all good men to come to the aid of their country...";
    uint32_t i;
    uint8_t qqq[] = "xxThis is the time for all good men to come to the aid of their country...";
    uint32_t j;
    uint8_t qqqq[] = "xxxThis is the time for all good men to come to the aid of their country...";
    uint32_t ref, x, y;
    uint8_t *p;

    printf("Endianness.  These lines should all be the same (for values filled in):\n");
    printf("%.8x                            %.8x                            %.8x\n",
            hashfunc(q, (sizeof(q) - 1), 13),
            hashfunc(q, (sizeof(q) - 5), 13),
            hashfunc(q, (sizeof(q) - 9), 13));
    p = q;
    printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
            hashfunc(p, sizeof(q) - 1, 13), hashfunc(p, sizeof(q) - 2, 13),
            hashfunc(p, sizeof(q) - 3, 13), hashfunc(p, sizeof(q) - 4, 13),
            hashfunc(p, sizeof(q) - 5, 13), hashfunc(p, sizeof(q) - 6, 13),
            hashfunc(p, sizeof(q) - 7, 13), hashfunc(p, sizeof(q) - 8, 13),
            hashfunc(p, sizeof(q) - 9, 13), hashfunc(p, sizeof(q) - 10, 13),
            hashfunc(p, sizeof(q) - 11, 13), hashfunc(p, sizeof(q) - 12, 13));
    p = &qq[1];
    printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
            hashfunc(p, sizeof(q) - 1, 13), hashfunc(p, sizeof(q) - 2, 13),
            hashfunc(p, sizeof(q) - 3, 13), hashfunc(p, sizeof(q) - 4, 13),
            hashfunc(p, sizeof(q) - 5, 13), hashfunc(p, sizeof(q) - 6, 13),
            hashfunc(p, sizeof(q) - 7, 13), hashfunc(p, sizeof(q) - 8, 13),
            hashfunc(p, sizeof(q) - 9, 13), hashfunc(p, sizeof(q) - 10, 13),
            hashfunc(p, sizeof(q) - 11, 13), hashfunc(p, sizeof(q) - 12, 13));
    p = &qqq[2];
    printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
            hashfunc(p, sizeof(q) - 1, 13), hashfunc(p, sizeof(q) - 2, 13),
            hashfunc(p, sizeof(q) - 3, 13), hashfunc(p, sizeof(q) - 4, 13),
            hashfunc(p, sizeof(q) - 5, 13), hashfunc(p, sizeof(q) - 6, 13),
            hashfunc(p, sizeof(q) - 7, 13), hashfunc(p, sizeof(q) - 8, 13),
            hashfunc(p, sizeof(q) - 9, 13), hashfunc(p, sizeof(q) - 10, 13),
            hashfunc(p, sizeof(q) - 11, 13), hashfunc(p, sizeof(q) - 12, 13));
    p = &qqqq[3];
    printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
            hashfunc(p, sizeof(q) - 1, 13), hashfunc(p, sizeof(q) - 2, 13),
            hashfunc(p, sizeof(q) - 3, 13), hashfunc(p, sizeof(q) - 4, 13),
            hashfunc(p, sizeof(q) - 5, 13), hashfunc(p, sizeof(q) - 6, 13),
            hashfunc(p, sizeof(q) - 7, 13), hashfunc(p, sizeof(q) - 8, 13),
            hashfunc(p, sizeof(q) - 9, 13), hashfunc(p, sizeof(q) - 10, 13),
            hashfunc(p, sizeof(q) - 11, 13), hashfunc(p, sizeof(q) - 12, 13));
    printf("\n");

    /* check hashfunc doesn't read before or after the ends of the string */
    for (h = 0, b = buf + 1; h < 8; ++h, ++b)
    {
        for (i = 0; i < MAXLEN; ++i)
        {
            len = i;
            for (j = 0; j < i; ++j)
                *(b + j) = 0;

            /* these should all be equal */
            ref = hashfunc(b, len, (uint32_t)1);
            *(b + i) = (uint8_t) ~0;
            *(b - 1) = (uint8_t) ~0;
            x = hashfunc(b, len, (uint32_t)1);
            y = hashfunc(b, len, (uint32_t)1);
            if ((ref != x) || (ref != y))
            {
                printf("alignment error: %.8x %.8x %.8x %d %d\n", ref, x, y,
                        h, i);
            }
        }
    }
}

/* check for problems with nulls */
void driver4(strnhash_f hashfunc)
{
    uint8_t buf[1];
    uint32_t h, i, state2[HASHSTATE];


    buf[0] = ~0;
    for (i = 0; i < HASHSTATE; ++i)
        state2[i] = 1;
    printf("These should all be different\n");
    for (i = 0, h = 0; i < 8; ++i)
    {
        h = hashfunc(buf, 0, h);
        printf("%2ld  0-byte strings, hash is  %.8lx\n", (long)i, (long)h);
    }
}


void hash_selftest(strnhash_f hashfunc)
{
    driver2(hashfunc); /* test that whole key is hashed thoroughly */
    driver3(hashfunc); /* test that nothing but the key is hashed */
    driver4(hashfunc); /* test hashing multiple buffers (all buffers are null) */
}






int test_avalanche1(strnhash_f hashfunc)
{
	unsigned char feed[INPUT_BYTE_WIDTH];
	int bitpos;
	crmhash_t v;
	int bc;
	int bp;
	int countlen = 0;
	
	for (bitpos = 0; bitpos < INPUT_BYTE_WIDTH * 8; bitpos++)
	{
		memset(feed, 0, sizeof(feed));
		feed[bitpos >> 3 /* div 8 */] = (1 << (bitpos & 0x7 /* mod 8 */));

		v = hashfunc((const unsigned char *)feed, sizeof(feed), 0);
#if 0
		v = (v & 0xFFFF) ^ ((v >> 16) & 0xFFFF);
		v = (v & 0xFF) ^ ((v >> 8) & 0xFF);
		v = v ^ (v << 8);
		v = v ^ (v << 16);
#endif

		// printf("hash = %08X, bitpos = %d, a[0]=%02X, a[1]=%02X, a[2]=%02X, a[3]=%02X, a[4]=%02X, a[5]=%02X, a[6]=%02X, a[7]=%02X, a[8]=%02X, a[9]=%02X, a[10]=%02X, a[11]=%02X\n", v, bitpos, feed[0], feed[1], feed[2], feed[3], feed[4], feed[5], feed[6], feed[7], feed[8], feed[9], feed[10], feed[11]);
		spread16[v & (WIDTHOF(spread16) - 1)]++;

		bc = 0;  // bit count
		bp = 0;  // bit position in hash
		while (v)
		{
			if (v & 1)
			{
				bc++;
				state[bp]++;
			}
			v >>= 1;
			bp++;
		}
		counts[countlen++] = bc;
	}
	return countlen;
} 




int test_avalanche2(strnhash_f hashfunc)
{
	const char *feed;
	int strpos;
	crmhash_t v;
	int bc;
	int bp;
	int len;
	int countlen = 0;
	int feedlen;
	
	for (strpos = 0; strpos < strlen(teststr); strpos++)
	{
		feed = teststr + strpos;
		len = strlen(teststr) - strpos;

#if 0
		for (feedlen = 0; feedlen < 16; feedlen++)
		{
			if (len < feedlen)
				break;

			if (feedlen != 0)
				printf("TEST: %.*s\n", feedlen, feed);
		}
#endif
			v = hashfunc((const unsigned char *)feed, len, 0);

#if 0
			v = (v & 0xFFFF) ^ ((v >> 16) & 0xFFFF);
			v = (v & 0xFF) ^ ((v >> 8) & 0xFF);
			v = v ^ (v << 8);
			v = v ^ (v << 16);
#endif

			// printf("hash = %08X, strpos = %d, a[0]=%02X, a[1]=%02X, a[2]=%02X, a[3]=%02X, a[4]=%02X, a[5]=%02X, a[6]=%02X, a[7]=%02X, a[8]=%02X, a[9]=%02X, a[10]=%02X, a[11]=%02X\n", v, strpos, feed[0], feed[1], feed[2], feed[3], feed[4], feed[5], feed[6], feed[7], feed[8], feed[9], feed[10], feed[11]);
		spread16[v & (WIDTHOF(spread16) - 1)]++;


			bc = 0;  // bit count
			bp = 0;  // bit position in hash
			while (v)
			{
				if (v & 1)
				{
					bc++;
					state[bp]++;
				}
				v >>= 1;
				bp++;
			}
			counts[countlen++] = bc;
	}
	return countlen;
} 










int test_avalanche3(strnhash_f hashfunc)
{
	const char *feed;
	int strpos;
	crmhash_t v;
	int bc;
	int bp;
	int len;
	int countlen = 0;
	int feedlen;
	
	for (strpos = 0; strpos < sizeof(teststr_derived)/sizeof(teststr_derived[0]); strpos++)
	{
		//printf("X: %d", (int)(sizeof(counts)/sizeof(counts[0])));
		feed = teststr_derived[strpos];
		len = strlen(feed);
		//printf("len: %d / %d / %d: %s\n", len, strpos, (int)(sizeof(teststr_derived)/sizeof(teststr_derived[0])), feed);


			v = hashfunc(feed, len, 0);

#if 0
			v = (v & 0xFFFF) ^ ((v >> 16) & 0xFFFF);
			v = (v & 0xFF) ^ ((v >> 8) & 0xFF);
			v = v ^ (v << 8);
			v = v ^ (v << 16);
#endif

			// printf("hash = %08X, strpos = %d, a[0]=%02X, a[1]=%02X, a[2]=%02X, a[3]=%02X, a[4]=%02X, a[5]=%02X, a[6]=%02X, a[7]=%02X, a[8]=%02X, a[9]=%02X, a[10]=%02X, a[11]=%02X\n", v, strpos, feed[0], feed[1], feed[2], feed[3], feed[4], feed[5], feed[6], feed[7], feed[8], feed[9], feed[10], feed[11]);
		spread16[v & (WIDTHOF(spread16) - 1)]++;


			bc = 0;  // bit count
			bp = 0;  // bit position in hash
			while (v)
			{
				if (v & 1)
				{
					bc++;
					state[bp]++;
				}
				v >>= 1;
				bp++;
			}
			counts[countlen++] = bc;

		//printf("Y: ");
	}
	return countlen;
} 






int test_avalanche4(strnhash_f hashfunc, int mode)
{
	uint32_t feed;
	int strpos;
	crmhash_t v;
	int bc;
	int bp;
	int len;
	int countlen = 0;
	int feedlen;
	int m;
	int s;

	s = 128000;
	m = 0xFFFFFFFFU / s;
	
	for (strpos = 0; strpos < s; strpos++)
	{
		feed = strpos * (mode == 0 ? 1 : m);
		len = sizeof(feed);

			v = hashfunc((void *)&feed, len, 0);

#if 0
			v = (v & 0xFFFF) ^ ((v >> 16) & 0xFFFF);
			v = (v & 0xFF) ^ ((v >> 8) & 0xFF);
			v = v ^ (v << 8);
			v = v ^ (v << 16);
#endif

			// printf("hash = %08X, strpos = %d, a[0]=%02X, a[1]=%02X, a[2]=%02X, a[3]=%02X, a[4]=%02X, a[5]=%02X, a[6]=%02X, a[7]=%02X, a[8]=%02X, a[9]=%02X, a[10]=%02X, a[11]=%02X\n", v, strpos, feed[0], feed[1], feed[2], feed[3], feed[4], feed[5], feed[6], feed[7], feed[8], feed[9], feed[10], feed[11]);
		spread16[v & (WIDTHOF(spread16) - 1)]++;


			bc = 0;  // bit count
			bp = 0;  // bit position in hash
			while (v)
			{
				if (v & 1)
				{
					bc++;
					state[bp]++;
				}
				v >>= 1;
				bp++;
			}
			counts[countlen++] = bc;

		//printf("Y: ");
	}
	return countlen;
} 




















void test_avalanche(strnhash_f hashfunc)
{
	int countlen;

	hash_selftest(hashfunc);

	memset(state, 0, sizeof(state));
	memset(counts, 0, sizeof(counts));
	memset(spread16, 0, sizeof(spread16));

	countlen = test_avalanche1(hashfunc);
	printf("\nSingle bit:\n");
	show_stats(countlen);

	memset(state, 0, sizeof(state));
	memset(counts, 0, sizeof(counts));
	memset(spread16, 0, sizeof(spread16));

	countlen = test_avalanche2(hashfunc);
	printf("\nSample text (shifted):\n");
	show_stats(countlen);

	memset(state, 0, sizeof(state));
	memset(counts, 0, sizeof(counts));
	memset(spread16, 0, sizeof(spread16));

	countlen = test_avalanche3(hashfunc);
	printf("\nSample text token set:\n");
	show_stats(countlen);

	memset(state, 0, sizeof(state));
	memset(counts, 0, sizeof(counts));
	memset(spread16, 0, sizeof(spread16));

	countlen = test_avalanche4(hashfunc, 0);
	printf("\nTest number series:\n");
	show_stats(countlen);

	memset(state, 0, sizeof(state));
	memset(counts, 0, sizeof(counts));
	memset(spread16, 0, sizeof(spread16));

	countlen = test_avalanche4(hashfunc, 1);
	printf("\nTest number series (spread):\n");
	show_stats(countlen);

	printf("\n\n=================================================================\n\n");
} 




///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// (CRM_WITH_OLD_HASH_FUNCTION)

crmhash_t old_crm114_hash(const unsigned char *str, int len, crmhash_t seed)
{
    long i;
    // unsigned long hval;
    int32_t hval;
    crmhash_t tmp;

    // initialize hval
    hval = len ^ seed;

    //  for each character in the incoming text:
    for (i = 0; i < len; i++)
    {
        //    xor in the current byte against each byte of hval
        //    (which alone gaurantees that every bit of input will have
        //    an effect on the output)

        tmp = str[i] & 0xFF;
        tmp = tmp | (tmp << 8) | (tmp << 16) | (tmp << 24);
        hval ^= tmp;

        //    add some bits out of the middle as low order bits.
        hval = hval + ((hval >> 12) & 0x0000ffff);

        //     swap most and min significative bytes
        tmp = (hval << 24) | ((hval >> 24) & 0xff);
        hval &= 0x00ffff00;         // zero most and min significative bytes of hval
        hval |= tmp;                // OR with swapped bytes

        //    rotate hval 3 bits to the left (thereby making the
        //    3rd msb of the above mess the hsb of the output hash)
        hval = (hval << 3) + (hval >> 29);
    }
    return hval;
}





crmhash_t crm114_ger_hash(const unsigned char *str, int len, crmhash_t seed)
{
    size_t i;
    crmhash_t hval;
    crmhash_t tmp;

    // initialize hval
    hval = len ^ seed;

    //  for each character in the incoming text:
    for (i = 0; i < len; i++)
    {
        //    xor in the current byte against each byte of hval
        //    (which alone gaurantees that every bit of input will have
        //    an effect on the output)

        tmp = ((unsigned char *)str)[i];
        tmp = tmp | (tmp << 8);
        tmp = tmp | (tmp << 16);
        hval ^= tmp;

        //    add some bits out of the middle as low order bits.
        hval = hval + ((hval >> 12) & 0x0000ffff);

        //     swap most and min significant bytes
        tmp = (hval << 24) | ((hval >> 24) & 0xff);
        hval &= 0x00ffff00;         // zero most and min significant bytes of hval
        hval |= tmp;                // OR with swapped bytes

        //    rotate hval 3 bits to the left (thereby making the
        //    3rd msb of the above mess the hsb of the output hash)
        hval = (hval << 3) | ((hval >> 29) & 0x7);
    }
    return hval;
}





/*
 * -------------------------------------------------------------------------------
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 *
 * These are functions for producing 32-bit hashes for hash table lookup.
 * hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final()
 * are externally useful functions.  Routines to test the hash are included
 * if SELF_TEST is defined.  You can use this free for any purpose.  It's in
 * the public domain.  It has no warranty.
 *
 * You probably want to use hashlittle().  hashlittle() and hashbig()
 * hash byte arrays.  hashlittle() is is faster than hashbig() on
 * little-endian machines.  Intel and AMD are little-endian machines.
 * On second thought, you probably want hashlittle2(), which is identical to
 * hashlittle() except it returns two 32-bit hashes for the price of one.
 * You could implement hashbig2() if you wanted but I haven't bothered here.
 *
 * If you want to find a hash of, say, exactly 7 integers, do
 * a = i1;  b = i2;  c = i3;
 * mix(a,b,c);
 * a += i4; b += i5; c += i6;
 * mix(a,b,c);
 * a += i7;
 * final(a,b,c);
 * then use c as the hash value.  If you have a variable length array of
 * 4-byte integers to hash, use hashword().  If you have a byte array (like
 * a character string), use hashlittle().  If you have several byte arrays, or
 * a mix of things, see the comments above hashlittle().
 *
 * Why is this so big?  I read 12 bytes at a time into 3 4-byte integers,
 * then mix those integers.  This is fast (you can do a lot more thorough
 * mixing with 12*3 instructions on 3 integers than you can with 3 instructions
 * on 1 byte), but shoehorning those bytes into integers efficiently is messy.
 * -------------------------------------------------------------------------------
 */
#if defined (MACHINE_IS_LITTLE_ENDIAN)
#define HASH_LITTLE_ENDIAN 1
#define HASH_BIG_ENDIAN 0
#elif defined (MACHINE_IS_BIG_ENDIAN)
#define HASH_LITTLE_ENDIAN 0
#define HASH_BIG_ENDIAN 1
#else
#define HASH_LITTLE_ENDIAN 0
#define HASH_BIG_ENDIAN 0
#endif



#define hashsize(n) ((uint32_t)1 << (n))
#define hashmask(n) (hashsize(n) - 1)
#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))


/*
 * -------------------------------------------------------------------------------
 * mix -- mix 3 32-bit values reversibly.
 *
 * This is reversible, so any information in (a,b,c) before mix() is
 * still in (a,b,c) after mix().
 *
 * If four pairs of (a,b,c) inputs are run through mix(), or through
 * mix() in reverse, there are at least 32 bits of the output that
 * are sometimes the same for one pair and different for another pair.
 * This was tested for:
 * pairs that differed by one bit, by two bits, in any combination
 * of top bits of (a,b,c), or in any combination of bottom bits of
 * (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 * the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 * is commonly produced by subtraction) look like a single 1-bit
 * difference.
 * the base values were pseudorandom, all zero but one bit set, or
 * all zero plus a counter that starts at zero.
 *
 * Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
 * satisfy this are
 *  4  6  8 16 19  4
 *  9 15  3 18 27 15
 * 14  9  3  7 17  3
 * Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
 * for "differ" defined as + with a one-bit base and a two-bit delta.  I
 * used http://burtleburtle.net/bob/hash/avalanche.html to choose
 * the operations, constants, and arrangements of the variables.
 *
 * This does not achieve avalanche.  There are input bits of (a,b,c)
 * that fail to affect some output bits of (a,b,c), especially of a.  The
 * most thoroughly mixed value is c, but it doesn't really even achieve
 * avalanche in c.
 *
 * This allows some parallelism.  Read-after-writes are good at doubling
 * the number of bits affected, so the goal of mixing pulls in the opposite
 * direction as the goal of parallelism.  I did what I could.  Rotates
 * seem to cost as much as shifts on every machine I could lay my hands
 * on, and rotates are much kinder to the top and bottom bits, so I used
 * rotates.
 * -------------------------------------------------------------------------------
 */
#define mix(a, b, c)                       \
    {                                      \
        a -= c;  a ^= rot(c, 4);  c += b;  \
        b -= a;  b ^= rot(a, 6);  a += c;  \
        c -= b;  c ^= rot(b, 8);  b += a;  \
        a -= c;  a ^= rot(c, 16);  c += b; \
        b -= a;  b ^= rot(a, 19);  a += c; \
        c -= b;  c ^= rot(b, 4);  b += a;  \
    }

/*
 * -------------------------------------------------------------------------------
 * final -- final mixing of 3 32-bit values (a,b,c) into c
 *
 * Pairs of (a,b,c) values differing in only a few bits will usually
 * produce values of c that look totally different.  This was tested for
 * pairs that differed by one bit, by two bits, in any combination
 * of top bits of (a,b,c), or in any combination of bottom bits of
 * (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 * the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 * is commonly produced by subtraction) look like a single 1-bit
 * difference.
 * the base values were pseudorandom, all zero but one bit set, or
 * all zero plus a counter that starts at zero.
 *
 * These constants passed:
 * 14 11 25 16 4 14 24
 * 12 14 25 16 4 14 24
 * and these came close:
 * 4  8 15 26 3 22 24
 * 10  8 15 26 3 22 24
 * 11  8 15 26 3 22 24
 * -------------------------------------------------------------------------------
 */
#define final(a, b, c)           \
    {                            \
        c ^= b; c -= rot(b, 14); \
        a ^= c; a -= rot(c, 11); \
        b ^= a; b -= rot(a, 25); \
        c ^= b; c -= rot(b, 16); \
        a ^= c; a -= rot(c, 4);  \
        b ^= a; b -= rot(a, 14); \
        c ^= b; c -= rot(b, 24); \
    }

/*
 * --------------------------------------------------------------------
 * This works on all machines.  To be useful, it requires
 * -- that the key be an array of uint32_t's, and
 * -- that the length be the number of uint32_t's in the key
 *
 * The function hashword() is identical to hashlittle() on little-endian
 * machines, and identical to hashbig() on big-endian machines,
 * except that the length has to be measured in uint32_ts rather than in
 * bytes.  hashlittle() is more complicated than hashword() only because
 * hashlittle() has to dance around fitting the key bytes into registers.
 * --------------------------------------------------------------------
 */
uint32_t hashword(
        const uint32_t *k,            /* the key, an array of uint32_t values */
        size_t          length,       /* the length of the key, in uint32_ts */
        uint32_t        initval)      /* the previous hash, or an arbitrary value */
{
    uint32_t a, b, c;

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + (((uint32_t)length) << 2) + initval;

    /*------------------------------------------------- handle most of the key */
    while (length > 3)
    {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a, b, c);
        length -= 3;
        k += 3;
    }

    /*------------------------------------------- handle the last 3 uint32_t's */
    switch (length)                  /* all the case statements fall through */
    {
    case 3:
        c += k[2];

    case 2:
        b += k[1];

    case 1:
        a += k[0];
        final(a, b, c);

    case 0:   /* case 0: nothing left to add */
        break;
    }
    /*------------------------------------------------------ report the result */
    return c;
}


/*
 * --------------------------------------------------------------------
 * hashword2() -- same as hashword(), but take two seeds and return two
 * 32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
 * both be initialized with seeds.  If you pass in (*pb)==0, the output
 * (*pc) will be the same as the return value from hashword().
 * --------------------------------------------------------------------
 */
void hashword2(
        const uint32_t *k,               /* the key, an array of uint32_t values */
        size_t          length,          /* the length of the key, in uint32_ts */
        uint32_t       *pc,              /* IN: seed OUT: primary hash value */
        uint32_t       *pb)              /* IN: more seed OUT: secondary hash value */
{
    uint32_t a, b, c;

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + ((uint32_t)(length << 2)) + *pc;
    c += *pb;

    /*------------------------------------------------- handle most of the key */
    while (length > 3)
    {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a, b, c);
        length -= 3;
        k += 3;
    }

    /*------------------------------------------- handle the last 3 uint32_t's */
    switch (length)                  /* all the case statements fall through */
    {
    case 3:
        c += k[2];

    case 2:
        b += k[1];

    case 1:
        a += k[0];
        final(a, b, c);

    case 0:   /* case 0: nothing left to add */
        break;
    }
    /*------------------------------------------------------ report the result */
    *pc = c;
    *pb = b;
}


/*
 * -------------------------------------------------------------------------------
 * hashlittle() -- hash a variable-length key into a 32-bit value
 * k       : the key (the unaligned variable-length array of bytes)
 * length  : the length of the key, counting by bytes
 * initval : can be any 4-byte value
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Two keys differing by one or two bits will have
 * totally different hash values.
 *
 * The best hash table sizes are powers of 2.  There is no need to do
 * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 * use a bitmask.  For example, if you need only 10 bits, do
 * h = (h & hashmask(10));
 * In which case, the hash table should have hashsize(10) elements.
 *
 * If you are hashing n strings (uint8_t **)k, do it like this:
 * for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);
 *
 * By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
 * code any way you wish, private, educational, or commercial.  It's free.
 *
 * Use for hash table lookup, or anything where one collision in 2^^32 is
 * acceptable.  Do NOT use for cryptographic purposes.
 * -------------------------------------------------------------------------------
 */

uint32_t hashlittle(const void *key, size_t length, uint32_t initval)
{
    uint32_t a, b, c;                                      /* internal state */

    union
    {
        const void *ptr;
        size_t      i;
    } u;                                      /* needed for Mac Powerbook G4 */

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

    u.ptr = key;
    if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0))
    {
        const uint32_t *k = (const uint32_t *)key;     /* read 32-bit chunks */
#ifdef VALGRIND
        const uint8_t  *k8;
#endif

        /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
        while (length > 12)
        {
            a += k[0];
            b += k[1];
            c += k[2];
            mix(a, b, c);
            length -= 12;
            k += 3;
        }

        /*----------------------------- handle the last (probably partial) block */
        /*
         * "k[2]&0xffffff" actually reads beyond the end of the string, but
         * then masks off the part it's not allowed to read.  Because the
         * string is aligned, the masked-off tail is in the same word as the
         * rest of the string.  Every machine with memory protection I've seen
         * does it on word boundaries, so is OK with this.  But VALGRIND will
         * still catch it and complain.  The masking trick does make the hash
         * noticably faster for short strings (like English words).
         */
#ifndef VALGRIND

        switch (length)
        {
        case 12:
            c += k[2];
            b += k[1];
            a += k[0];
            break;

        case 11:
            c += k[2] & 0xffffff;
            b += k[1];
            a += k[0];
            break;

        case 10:
            c += k[2] & 0xffff;
            b += k[1];
            a += k[0];
            break;

        case 9:
            c += k[2] & 0xff;
            b += k[1];
            a += k[0];
            break;

        case 8:
            b += k[1];
            a += k[0];
            break;

        case 7:
            b += k[1] & 0xffffff;
            a += k[0];
            break;

        case 6:
            b += k[1] & 0xffff;
            a += k[0];
            break;

        case 5:
            b += k[1] & 0xff;
            a += k[0];
            break;

        case 4:
            a += k[0];
            break;

        case 3:
            a += k[0] & 0xffffff;
            break;

        case 2:
            a += k[0] & 0xffff;
            break;

        case 1:
            a += k[0] & 0xff;
            break;

        case 0:
            return c;               /* zero length strings require no mixing */
        }

#else /* make valgrind happy */

        k8 = (const uint8_t *)k;
        switch (length)
        {
        case 12:
            c += k[2];
            b += k[1];
            a += k[0];
            break;

        case 11:
            c += ((uint32_t)k8[10]) << 16; /* fall through */

        case 10:
            c += ((uint32_t)k8[9]) << 8; /* fall through */

        case 9:
            c += k8[8];                  /* fall through */

        case 8:
            b += k[1];
            a += k[0];
            break;

        case 7:
            b += ((uint32_t)k8[6]) << 16; /* fall through */

        case 6:
            b += ((uint32_t)k8[5]) << 8; /* fall through */

        case 5:
            b += k8[4];                  /* fall through */

        case 4:
            a += k[0];
            break;

        case 3:
            a += ((uint32_t)k8[2]) << 16; /* fall through */

        case 2:
            a += ((uint32_t)k8[1]) << 8; /* fall through */

        case 1:
            a += k8[0];
            break;

        case 0:
            return c;
        }

#endif /* !valgrind */
    }
    else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0))
    {
        const uint16_t *k = (const uint16_t *)key;     /* read 16-bit chunks */
        const uint8_t  *k8;

        /*--------------- all but last block: aligned reads and different mixing */
        while (length > 12)
        {
            a += k[0] + (((uint32_t)k[1]) << 16);
            b += k[2] + (((uint32_t)k[3]) << 16);
            c += k[4] + (((uint32_t)k[5]) << 16);
            mix(a, b, c);
            length -= 12;
            k += 6;
        }

        /*----------------------------- handle the last (probably partial) block */
        k8 = (const uint8_t *)k;
        switch (length)
        {
        case 12:
            c += k[4] + (((uint32_t)k[5]) << 16);
            b += k[2] + (((uint32_t)k[3]) << 16);
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 11:
            c += ((uint32_t)k8[10]) << 16;  /* fall through */

        case 10:
            c += k[4];
            b += k[2] + (((uint32_t)k[3]) << 16);
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 9:
            c += k8[8];                     /* fall through */

        case 8:
            b += k[2] + (((uint32_t)k[3]) << 16);
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 7:
            b += ((uint32_t)k8[6]) << 16;   /* fall through */

        case 6:
            b += k[2];
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 5:
            b += k8[4];                     /* fall through */

        case 4:
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 3:
            a += ((uint32_t)k8[2]) << 16;   /* fall through */

        case 2:
            a += k[0];
            break;

        case 1:
            a += k8[0];
            break;

        case 0:
            return c;                      /* zero length requires no mixing */
        }
    }
    else
    {
        /* need to read the key one byte at a time */
        const uint8_t *k = (const uint8_t *)key;

        /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
        while (length > 12)
        {
            a += k[0];
            a += ((uint32_t)k[1]) << 8;
            a += ((uint32_t)k[2]) << 16;
            a += ((uint32_t)k[3]) << 24;
            b += k[4];
            b += ((uint32_t)k[5]) << 8;
            b += ((uint32_t)k[6]) << 16;
            b += ((uint32_t)k[7]) << 24;
            c += k[8];
            c += ((uint32_t)k[9]) << 8;
            c += ((uint32_t)k[10]) << 16;
            c += ((uint32_t)k[11]) << 24;
            mix(a, b, c);
            length -= 12;
            k += 12;
        }

        /*-------------------------------- last block: affect all 32 bits of (c) */
        switch (length)              /* all the case statements fall through */
        {
        case 12:
            c += ((uint32_t)k[11]) << 24;

        case 11:
            c += ((uint32_t)k[10]) << 16;

        case 10:
            c += ((uint32_t)k[9]) << 8;

        case 9:
            c += k[8];

        case 8:
            b += ((uint32_t)k[7]) << 24;

        case 7:
            b += ((uint32_t)k[6]) << 16;

        case 6:
            b += ((uint32_t)k[5]) << 8;

        case 5:
            b += k[4];

        case 4:
            a += ((uint32_t)k[3]) << 24;

        case 3:
            a += ((uint32_t)k[2]) << 16;

        case 2:
            a += ((uint32_t)k[1]) << 8;

        case 1:
            a += k[0];
            break;

        case 0:
            return c;
        }
    }

    final(a, b, c);
    return c;
}


/*
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
void hashlittle2(
        const void *key,          /* the key to hash */
        size_t      length,       /* length of the key */
        uint32_t   *pc,           /* IN: primary initval, OUT: primary hash */
        uint32_t   *pb)           /* IN: secondary initval, OUT: secondary hash */
{
    uint32_t a, b, c;                                      /* internal state */

    union
    {
        const void *ptr;
        size_t      i;
    } u;                                      /* needed for Mac Powerbook G4 */

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
    c += *pb;

    u.ptr = key;
    if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0))
    {
        const uint32_t *k = (const uint32_t *)key;     /* read 32-bit chunks */
#ifdef VALGRIND
        const uint8_t  *k8;
#endif

        /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
        while (length > 12)
        {
            a += k[0];
            b += k[1];
            c += k[2];
            mix(a, b, c);
            length -= 12;
            k += 3;
        }

        /*----------------------------- handle the last (probably partial) block */
        /*
         * "k[2]&0xffffff" actually reads beyond the end of the string, but
         * then masks off the part it's not allowed to read.  Because the
         * string is aligned, the masked-off tail is in the same word as the
         * rest of the string.  Every machine with memory protection I've seen
         * does it on word boundaries, so is OK with this.  But VALGRIND will
         * still catch it and complain.  The masking trick does make the hash
         * noticably faster for short strings (like English words).
         */
#ifndef VALGRIND

        switch (length)
        {
        case 12:
            c += k[2];
            b += k[1];
            a += k[0];
            break;

        case 11:
            c += k[2] & 0xffffff;
            b += k[1];
            a += k[0];
            break;

        case 10:
            c += k[2] & 0xffff;
            b += k[1];
            a += k[0];
            break;

        case 9:
            c += k[2] & 0xff;
            b += k[1];
            a += k[0];
            break;

        case 8:
            b += k[1];
            a += k[0];
            break;

        case 7:
            b += k[1] & 0xffffff;
            a += k[0];
            break;

        case 6:
            b += k[1] & 0xffff;
            a += k[0];
            break;

        case 5:
            b += k[1] & 0xff;
            a += k[0];
            break;

        case 4:
            a += k[0];
            break;

        case 3:
            a += k[0] & 0xffffff;
            break;

        case 2:
            a += k[0] & 0xffff;
            break;

        case 1:
            a += k[0] & 0xff;
            break;

        case 0:
            *pc = c;
            *pb = b;
            return;                  /* zero length strings require no mixing */
        }

#else /* make valgrind happy */

        k8 = (const uint8_t *)k;
        switch (length)
        {
        case 12:
            c += k[2];
            b += k[1];
            a += k[0];
            break;

        case 11:
            c += ((uint32_t)k8[10]) << 16; /* fall through */

        case 10:
            c += ((uint32_t)k8[9]) << 8; /* fall through */

        case 9:
            c += k8[8];                  /* fall through */

        case 8:
            b += k[1];
            a += k[0];
            break;

        case 7:
            b += ((uint32_t)k8[6]) << 16; /* fall through */

        case 6:
            b += ((uint32_t)k8[5]) << 8; /* fall through */

        case 5:
            b += k8[4];                  /* fall through */

        case 4:
            a += k[0];
            break;

        case 3:
            a += ((uint32_t)k8[2]) << 16; /* fall through */

        case 2:
            a += ((uint32_t)k8[1]) << 8; /* fall through */

        case 1:
            a += k8[0];
            break;

        case 0:
            *pc = c;
            *pb = b;
            return;                  /* zero length strings require no mixing */
        }

#endif /* !valgrind */
    }
    else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0))
    {
        const uint16_t *k = (const uint16_t *)key;     /* read 16-bit chunks */
        const uint8_t  *k8;

        /*--------------- all but last block: aligned reads and different mixing */
        while (length > 12)
        {
            a += k[0] + (((uint32_t)k[1]) << 16);
            b += k[2] + (((uint32_t)k[3]) << 16);
            c += k[4] + (((uint32_t)k[5]) << 16);
            mix(a, b, c);
            length -= 12;
            k += 6;
        }

        /*----------------------------- handle the last (probably partial) block */
        k8 = (const uint8_t *)k;
        switch (length)
        {
        case 12:
            c += k[4] + (((uint32_t)k[5]) << 16);
            b += k[2] + (((uint32_t)k[3]) << 16);
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 11:
            c += ((uint32_t)k8[10]) << 16;  /* fall through */

        case 10:
            c += k[4];
            b += k[2] + (((uint32_t)k[3]) << 16);
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 9:
            c += k8[8];                     /* fall through */

        case 8:
            b += k[2] + (((uint32_t)k[3]) << 16);
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 7:
            b += ((uint32_t)k8[6]) << 16;   /* fall through */

        case 6:
            b += k[2];
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 5:
            b += k8[4];                     /* fall through */

        case 4:
            a += k[0] + (((uint32_t)k[1]) << 16);
            break;

        case 3:
            a += ((uint32_t)k8[2]) << 16;   /* fall through */

        case 2:
            a += k[0];
            break;

        case 1:
            a += k8[0];
            break;

        case 0:
            *pc = c;
            *pb = b;
            return;                  /* zero length strings require no mixing */
        }
    }
    else
    {
        /* need to read the key one byte at a time */
        const uint8_t *k = (const uint8_t *)key;

        /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
        while (length > 12)
        {
            a += k[0];
            a += ((uint32_t)k[1]) << 8;
            a += ((uint32_t)k[2]) << 16;
            a += ((uint32_t)k[3]) << 24;
            b += k[4];
            b += ((uint32_t)k[5]) << 8;
            b += ((uint32_t)k[6]) << 16;
            b += ((uint32_t)k[7]) << 24;
            c += k[8];
            c += ((uint32_t)k[9]) << 8;
            c += ((uint32_t)k[10]) << 16;
            c += ((uint32_t)k[11]) << 24;
            mix(a, b, c);
            length -= 12;
            k += 12;
        }

        /*-------------------------------- last block: affect all 32 bits of (c) */
        switch (length)              /* all the case statements fall through */
        {
        case 12:
            c += ((uint32_t)k[11]) << 24;

        case 11:
            c += ((uint32_t)k[10]) << 16;

        case 10:
            c += ((uint32_t)k[9]) << 8;

        case 9:
            c += k[8];

        case 8:
            b += ((uint32_t)k[7]) << 24;

        case 7:
            b += ((uint32_t)k[6]) << 16;

        case 6:
            b += ((uint32_t)k[5]) << 8;

        case 5:
            b += k[4];

        case 4:
            a += ((uint32_t)k[3]) << 24;

        case 3:
            a += ((uint32_t)k[2]) << 16;

        case 2:
            a += ((uint32_t)k[1]) << 8;

        case 1:
            a += k[0];
            break;

        case 0:
            *pc = c;
            *pb = b;
            return;                  /* zero length strings require no mixing */
        }
    }

    final(a, b, c);
    *pc = c;
    *pb = b;
}




crmhash_t hashlittle2_hash(const unsigned char *str, int len, crmhash_t seed)
{
    uint32_t a = seed;
    uint32_t b = 0;

    hashlittle2(str, len, &a, &b);
    return a;
}



crmhash_t fake_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= v ^ 0x10 ^ (v >> 32);
	}
	return ret;
}


crmhash_t mult_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= (v ^ 0x10 ^ (v >> 32)) + (v >> 17);
	}
	return ret;
}

crmhash_t mult2_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= (v ^ 0x10000 ^ (v >> 32)) + (v >> 17);
	}
	return ret;
}

crmhash_t mult3_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435759;
		ret ^= (v ^ 0x10000 ^ (v >> 32)) + (v >> 19);
	}
	return ret;
}

crmhash_t mult4_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435759;
		ret ^= (v ^ 0x10000 ^ (v >> 32)) - (7 * (v >> 19));
	}
	return ret;
}

crmhash_t mult5_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}
{
		uint64_t v = ret - 0;
		v *= 2654435761;
		ret ^= v ^ 0x10000 ^ (v >> 32) ^ (v >> 19);
}
	return ret;
}





/*
 *  RFC 1321 compliant MD5 implementation
 *
 *  Copyright (C) 2006-2007  Christophe Devine
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License, version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */
/*
 *  The MD5 algorithm was designed by Ron Rivest in 1991.
 *
 *  http://www.ietf.org/rfc/rfc1321.txt
 */



/**
 * \brief          MD5 context structure
 */
typedef struct
{
    unsigned long total[2];     /*!< number of bytes processed  */
    unsigned long state[4];     /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */

    unsigned char ipad[64];     /*!< HMAC: inner padding        */
    unsigned char opad[64];     /*!< HMAC: outer padding        */
}
md5_context;


/*
 * 32-bit integer manipulation macros (little endian)
 */
#ifndef GET_ULONG_LE
#define GET_ULONG_LE(n,b,i)                             \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)    ]       )        \
        | ( (unsigned long) (b)[(i) + 1] <<  8 )        \
        | ( (unsigned long) (b)[(i) + 2] << 16 )        \
        | ( (unsigned long) (b)[(i) + 3] << 24 );       \
}
#endif

#ifndef PUT_ULONG_LE
#define PUT_ULONG_LE(n,b,i)                             \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n)       );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
}
#endif

/*
 * MD5 context setup
 */
void md5_starts( md5_context *ctx )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

static void md5_process( md5_context *ctx, const unsigned char data[64] )
{
    unsigned long X[16], A, B, C, D;

    GET_ULONG_LE( X[ 0], data,  0 );
    GET_ULONG_LE( X[ 1], data,  4 );
    GET_ULONG_LE( X[ 2], data,  8 );
    GET_ULONG_LE( X[ 3], data, 12 );
    GET_ULONG_LE( X[ 4], data, 16 );
    GET_ULONG_LE( X[ 5], data, 20 );
    GET_ULONG_LE( X[ 6], data, 24 );
    GET_ULONG_LE( X[ 7], data, 28 );
    GET_ULONG_LE( X[ 8], data, 32 );
    GET_ULONG_LE( X[ 9], data, 36 );
    GET_ULONG_LE( X[10], data, 40 );
    GET_ULONG_LE( X[11], data, 44 );
    GET_ULONG_LE( X[12], data, 48 );
    GET_ULONG_LE( X[13], data, 52 );
    GET_ULONG_LE( X[14], data, 56 );
    GET_ULONG_LE( X[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
{                                                       \
    a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
}

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))

    P( A, B, C, D,  0,  7, 0xD76AA478 );
    P( D, A, B, C,  1, 12, 0xE8C7B756 );
    P( C, D, A, B,  2, 17, 0x242070DB );
    P( B, C, D, A,  3, 22, 0xC1BDCEEE );
    P( A, B, C, D,  4,  7, 0xF57C0FAF );
    P( D, A, B, C,  5, 12, 0x4787C62A );
    P( C, D, A, B,  6, 17, 0xA8304613 );
    P( B, C, D, A,  7, 22, 0xFD469501 );
    P( A, B, C, D,  8,  7, 0x698098D8 );
    P( D, A, B, C,  9, 12, 0x8B44F7AF );
    P( C, D, A, B, 10, 17, 0xFFFF5BB1 );
    P( B, C, D, A, 11, 22, 0x895CD7BE );
    P( A, B, C, D, 12,  7, 0x6B901122 );
    P( D, A, B, C, 13, 12, 0xFD987193 );
    P( C, D, A, B, 14, 17, 0xA679438E );
    P( B, C, D, A, 15, 22, 0x49B40821 );

#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))

    P( A, B, C, D,  1,  5, 0xF61E2562 );
    P( D, A, B, C,  6,  9, 0xC040B340 );
    P( C, D, A, B, 11, 14, 0x265E5A51 );
    P( B, C, D, A,  0, 20, 0xE9B6C7AA );
    P( A, B, C, D,  5,  5, 0xD62F105D );
    P( D, A, B, C, 10,  9, 0x02441453 );
    P( C, D, A, B, 15, 14, 0xD8A1E681 );
    P( B, C, D, A,  4, 20, 0xE7D3FBC8 );
    P( A, B, C, D,  9,  5, 0x21E1CDE6 );
    P( D, A, B, C, 14,  9, 0xC33707D6 );
    P( C, D, A, B,  3, 14, 0xF4D50D87 );
    P( B, C, D, A,  8, 20, 0x455A14ED );
    P( A, B, C, D, 13,  5, 0xA9E3E905 );
    P( D, A, B, C,  2,  9, 0xFCEFA3F8 );
    P( C, D, A, B,  7, 14, 0x676F02D9 );
    P( B, C, D, A, 12, 20, 0x8D2A4C8A );

#undef F
    
#define F(x,y,z) (x ^ y ^ z)

    P( A, B, C, D,  5,  4, 0xFFFA3942 );
    P( D, A, B, C,  8, 11, 0x8771F681 );
    P( C, D, A, B, 11, 16, 0x6D9D6122 );
    P( B, C, D, A, 14, 23, 0xFDE5380C );
    P( A, B, C, D,  1,  4, 0xA4BEEA44 );
    P( D, A, B, C,  4, 11, 0x4BDECFA9 );
    P( C, D, A, B,  7, 16, 0xF6BB4B60 );
    P( B, C, D, A, 10, 23, 0xBEBFBC70 );
    P( A, B, C, D, 13,  4, 0x289B7EC6 );
    P( D, A, B, C,  0, 11, 0xEAA127FA );
    P( C, D, A, B,  3, 16, 0xD4EF3085 );
    P( B, C, D, A,  6, 23, 0x04881D05 );
    P( A, B, C, D,  9,  4, 0xD9D4D039 );
    P( D, A, B, C, 12, 11, 0xE6DB99E5 );
    P( C, D, A, B, 15, 16, 0x1FA27CF8 );
    P( B, C, D, A,  2, 23, 0xC4AC5665 );

#undef F

#define F(x,y,z) (y ^ (x | ~z))

    P( A, B, C, D,  0,  6, 0xF4292244 );
    P( D, A, B, C,  7, 10, 0x432AFF97 );
    P( C, D, A, B, 14, 15, 0xAB9423A7 );
    P( B, C, D, A,  5, 21, 0xFC93A039 );
    P( A, B, C, D, 12,  6, 0x655B59C3 );
    P( D, A, B, C,  3, 10, 0x8F0CCC92 );
    P( C, D, A, B, 10, 15, 0xFFEFF47D );
    P( B, C, D, A,  1, 21, 0x85845DD1 );
    P( A, B, C, D,  8,  6, 0x6FA87E4F );
    P( D, A, B, C, 15, 10, 0xFE2CE6E0 );
    P( C, D, A, B,  6, 15, 0xA3014314 );
    P( B, C, D, A, 13, 21, 0x4E0811A1 );
    P( A, B, C, D,  4,  6, 0xF7537E82 );
    P( D, A, B, C, 11, 10, 0xBD3AF235 );
    P( C, D, A, B,  2, 15, 0x2AD7D2BB );
    P( B, C, D, A,  9, 21, 0xEB86D391 );

#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
}

/*
 * MD5 process buffer
 */
void md5_update( md5_context *ctx, const unsigned char *input, int ilen )
{
    int fill;
    unsigned long left;

    if( ilen <= 0 )
        return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (unsigned long) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, fill );
        md5_process( ctx, ctx->buffer );
        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        md5_process( ctx, input );
        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, ilen );
    }
}

static const unsigned char md5_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * MD5 final digest
 */
void md5_finish( md5_context *ctx, unsigned char output[16] )
{
    unsigned long last, padn;
    unsigned long high, low;
    unsigned char msglen[8];

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_ULONG_LE( low,  msglen, 0 );
    PUT_ULONG_LE( high, msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    md5_update( ctx, md5_padding, padn );
    md5_update( ctx, msglen, 8 );

    PUT_ULONG_LE( ctx->state[0], output,  0 );
    PUT_ULONG_LE( ctx->state[1], output,  4 );
    PUT_ULONG_LE( ctx->state[2], output,  8 );
    PUT_ULONG_LE( ctx->state[3], output, 12 );
}

/*
 * output = MD5( input buffer )
 */
void md5( const unsigned char *input, int ilen, unsigned char output[16] )
{
    md5_context ctx;

    md5_starts( &ctx );
    md5_update( &ctx, input, ilen );
    md5_finish( &ctx, output );

    memset( &ctx, 0, sizeof( md5_context ) );
}

/*
 * output = MD5( file contents )
 */
int md5_file( char *path, unsigned char output[16] )
{
    FILE *f;
    size_t n;
    md5_context ctx;
    unsigned char buf[1024];

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( 1 );

    md5_starts( &ctx );

    while( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        md5_update( &ctx, buf, (int) n );

    md5_finish( &ctx, output );

    memset( &ctx, 0, sizeof( md5_context ) );

    if( ferror( f ) != 0 )
    {
        fclose( f );
        return( 2 );
    }

    fclose( f );
    return( 0 );
}

/*
 * MD5 HMAC context setup
 */
void md5_hmac_starts( md5_context *ctx, const unsigned char *key, int keylen )
{
    int i;
    unsigned char sum[16];

    if( keylen > 64 )
    {
        md5( key, keylen, sum );
        keylen = 16;
        key = sum;
    }

    memset( ctx->ipad, 0x36, 64 );
    memset( ctx->opad, 0x5C, 64 );

    for( i = 0; i < keylen; i++ )
    {
        ctx->ipad[i] = (unsigned char)( ctx->ipad[i] ^ key[i] );
        ctx->opad[i] = (unsigned char)( ctx->opad[i] ^ key[i] );
    }

    md5_starts( ctx );
    md5_update( ctx, ctx->ipad, 64 );

    memset( sum, 0, sizeof( sum ) );
}

/*
 * MD5 HMAC process buffer
 */
void md5_hmac_update( md5_context *ctx, const unsigned char *input, int ilen )
{
    md5_update( ctx, input, ilen );
}

/*
 * MD5 HMAC final digest
 */
void md5_hmac_finish( md5_context *ctx, unsigned char output[16] )
{
    unsigned char tmpbuf[16];

    md5_finish( ctx, tmpbuf );
    md5_starts( ctx );
    md5_update( ctx, ctx->opad, 64 );
    md5_update( ctx, tmpbuf, 16 );
    md5_finish( ctx, output );

    memset( tmpbuf, 0, sizeof( tmpbuf ) );
}

/*
 * output = HMAC-MD5( hmac key, input buffer )
 */
void md5_hmac( unsigned char *key, int keylen, unsigned char *input, int ilen,
               unsigned char output[16] )
{
    md5_context ctx;

    md5_hmac_starts( &ctx, key, keylen );
    md5_hmac_update( &ctx, input, ilen );
    md5_hmac_finish( &ctx, output );

    memset( &ctx, 0, sizeof( md5_context ) );
}

#if defined(XYSSL_SELF_TEST)

/*
 * RFC 1321 test vectors
 */
static const char md5_test_str[7][81] =
{
    { "" }, 
    { "a" },
    { "abc" },
    { "message digest" },
    { "abcdefghijklmnopqrstuvwxyz" },
    { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" },
    { "12345678901234567890123456789012345678901234567890123456789012" \
      "345678901234567890" }
};

static const unsigned char md5_test_sum[7][16] =
{
    { 0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04,
      0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E },
    { 0x0C, 0xC1, 0x75, 0xB9, 0xC0, 0xF1, 0xB6, 0xA8,
      0x31, 0xC3, 0x99, 0xE2, 0x69, 0x77, 0x26, 0x61 },
    { 0x90, 0x01, 0x50, 0x98, 0x3C, 0xD2, 0x4F, 0xB0,
      0xD6, 0x96, 0x3F, 0x7D, 0x28, 0xE1, 0x7F, 0x72 },
    { 0xF9, 0x6B, 0x69, 0x7D, 0x7C, 0xB7, 0x93, 0x8D,
      0x52, 0x5A, 0x2F, 0x31, 0xAA, 0xF1, 0x61, 0xD0 },
    { 0xC3, 0xFC, 0xD3, 0xD7, 0x61, 0x92, 0xE4, 0x00,
      0x7D, 0xFB, 0x49, 0x6C, 0xCA, 0x67, 0xE1, 0x3B },
    { 0xD1, 0x74, 0xAB, 0x98, 0xD2, 0x77, 0xD9, 0xF5,
      0xA5, 0x61, 0x1C, 0x2C, 0x9F, 0x41, 0x9D, 0x9F },
    { 0x57, 0xED, 0xF4, 0xA2, 0x2B, 0xE3, 0xC9, 0x55,
      0xAC, 0x49, 0xDA, 0x2E, 0x21, 0x07, 0xB6, 0x7A }
};

/*
 * Checkup routine
 */
int md5_self_test( int verbose )
{
    int i;
    unsigned char md5sum[16];

    for( i = 0; i < 7; i++ )
    {
        if( verbose != 0 )
            printf( "  MD5 test #%d: ", i + 1 );

        md5( (unsigned char *) md5_test_str[i],
             strlen( md5_test_str[i] ), md5sum );

        if( memcmp( md5sum, md5_test_sum[i], 16 ) != 0 )
        {
            if( verbose != 0 )
                printf( "failed\n" );

            return( 1 );
        }

        if( verbose != 0 )
            printf( "passed\n" );
    }

    if( verbose != 0 )
        printf( "\n" );

    return( 0 );
}

#endif


crmhash_t mult6_hash(const unsigned char *str, int len, crmhash_t seed)
{
	crmhash_t ret;
    unsigned char md5sum[16];
	crmhash_t *p = (crmhash_t *)md5sum;

    md5_context ctx;

    md5_starts( &ctx );
    md5_update( &ctx, (const unsigned char *)&seed, sizeof(seed) );
    md5_update( &ctx, str, len );
    md5_finish( &ctx, md5sum );

    memset( &ctx, 0, sizeof( md5_context ) );

	ret = p[0] ^ p[1] ^ p[2] ^ p[4];

	return ret;
}



crmhash_t mult7_hash(const unsigned char *str, int len, crmhash_t seed)
{
	crmhash_t ret;
    unsigned char md5sum[16];
	crmhash_t *p = (crmhash_t *)md5sum;

    md5_context ctx;

    md5_starts( &ctx );
    md5_update( &ctx, (const char *)&seed, sizeof(seed) );
    md5_update( &ctx, str, len );
    md5_finish( &ctx, md5sum );

    memset( &ctx, 0, sizeof( md5_context ) );

	ret = p[0];

	return ret;
}



crmhash_t fake2_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = 0x55555555UL ^ seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= v ^ (v >> 32);
	}
	return ret;
}



crmhash_t fake3_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = 0x55555555UL ^ seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= v ^ ((v >> 48) & 0xFFFFU) ^ ((v >> 16) & 0xFFFF0000U);
	}
	return ret;
}



crmhash_t fake4_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = 0x55555555UL ^ seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761;
		ret ^= v ^ ((v >> 48) & 0xFFFFU) ^ ((v >> 16) & 0xFFFF0000U) ^ 0x10001;
	}
	return ret;
}



crmhash_t fake5_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761; // 7
		ret ^= v ^ ((v >> 48) & 0xFFFFU) ^ ((v >> 16) & 0xFFFF0000U) ^ 0x10001;
	}
	return ret;
}



crmhash_t fake6_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;

	for (i = 0; i < len; i++)
	{
		uint64_t v = ret - str[i];
		v *= 2654435761; // 7
		ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
	}
	return ret;
}



crmhash_t fake7_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;
	uint64_t m;

	for (i = 0; i < len; i++)
	{
		uint32_t n = str[i];
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
//		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
//		v += 0x09050301; // given that special ret and n, v may be zero. Make sure this step has an effect anyhow.

		//ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
		m = (v >> 43) ^ (v >> 17);
		m &= 0xFFFFFFFFU; // m * 65537 in next line:
		ret ^= v ^ (m << 32) ^ m ^ (v << 37);
		// ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}

	{
		uint32_t n = len;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
//		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
//		v += 0x09050301; // given that special ret and n, v may be zero. Make sure this step has an effect anyhow.

		//ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
		m = (v >> 43) ^ (v >> 17);
		m &= 0xFFFFFFFFU; // m * 65537 in next line:
		ret ^= v ^ (m << 32) ^ m ^ (v << 37);
		// ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}
	return ret;
}



crmhash_t fake8_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;
	uint64_t m;

	for (i = 0; i < len; i++)
	{
		uint32_t n = str[i];
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
//		v += 0x09050301; // given that special ret and n, v may be zero. Make sure this step has an effect anyhow.

		//ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
		m = (v >> 43) ^ (v >> 17);
		m &= 0xFFFFFFFFU; // m * 65537 in next line:
		ret ^= v ^ (m << 32) ^ m ^ (v << 37);
		// ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}

	{
		uint32_t n = len;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
//		v += 0x09050301; // given that special ret and n, v may be zero. Make sure this step has an effect anyhow.

		//ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
		m = (v >> 43) ^ (v >> 17);
		m &= 0xFFFFFFFFU; // m * 65537 in next line:
		ret ^= v ^ (m << 32) ^ m ^ (v << 37);
		// ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}
	return ret;
}



crmhash_t fake9_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;
	uint64_t m;

	for (i = 0; i < len; i++)
	{
		uint32_t n = str[i];
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
		v += 0x09050301; // given that special ret and n, v may be zero. Make sure this step has an effect anyhow.

		//ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
		m = (v >> 43) ^ (v >> 17);
		m &= 0xFFFFFFFFU; // m * 65537 in next line:
		ret ^= v ^ (m << 32) ^ m ^ (v << 37);
		// ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}

	{
		uint32_t n = len;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
		v += 0x09050301; // given that special ret and n, v may be zero. Make sure this step has an effect anyhow.

		//ret ^= v ^ (v >> 43) ^ (v >> 17) ^ (v << 39) ^ 0x10001;
		m = (v >> 43) ^ (v >> 17);
		m &= 0xFFFFFFFFU; // m * 65537 in next line:
		ret ^= v ^ (m << 32) ^ m ^ (v << 37);
		// ret ^= v ^ 0x10000 ^ (v >> 32) ^ (7 * (v >> 19));
	}
	return ret;
}





/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

//static unsigned long mt[N]; /* the array for the state vector  */
//static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void init_genrand(unsigned long *mt, unsigned long s)
{
	int mti;

    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned long *mt, const unsigned char *init_key, int key_length)
{
    int i, j, k;
    init_genrand(mt, 19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
		int v;
        if (j>=key_length) 
		{
			// this way, we can cope with zero-length array inits too
			v = 7;
			j=0;
		}
		else
		{
			v = init_key[j++];
		}
		v <<= 8;
        if (j>=key_length) 
		{
			// this way, we can cope with zero-length array inits too
			v ^= 7;
			j=0;
		}
		else
		{
			v ^= init_key[j++];
		}
		v <<= 8;
        if (j>=key_length) 
		{
			// this way, we can cope with zero-length array inits too
			v ^= 7;
			j=0;
		}
		else
		{
			v ^= init_key[j++];
		}
		v <<= 8;
        if (j>=key_length) 
		{
			// this way, we can cope with zero-length array inits too
			v ^= 7;
			j=0;
		}
		else
		{
			v ^= init_key[j++];
		}
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + v + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] ^= 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(unsigned long *mt, int *mti)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (*mti >= N) { /* generate N words at one time */
        int kk;

//        if (mti == N+1)   /* if init_genrand() has not been called, */
//            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        *mti = 0;
    }

    y = mt[(*mti)++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}



crmhash_t fake10_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret;
 unsigned long mt[N]; /* the array for the state vector  */
int mti=N+1; /* run MT once more; act as if uninitialized... */

init_by_array(mt, str, len);
ret = genrand_int32(mt, &mti);

return ret;
}



crmhash_t fake11_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;
	uint64_t m;

	for (i = 0; i < len; i++)
	{
		uint32_t n = str[i] + i;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
		v++;

		ret = v - (v >> 32);
		
    /* Tempering */
    ret ^= (ret >> 11);
    ret ^= (ret << 7) & 0x9d2c5680UL;
    ret ^= (ret << 15) & 0xefc60000UL;
    ret ^= (ret >> 18);
	}

	{
		uint32_t n = len;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 2654435761; // 7
		v++;

		ret = v - (v >> 32);
		
    /* Tempering */
    ret ^= (ret >> 11);
    ret ^= (ret << 7) & 0x9d2c5680UL;
    ret ^= (ret << 15) & 0xefc60000UL;
    ret ^= (ret >> 18);
	}
	return ret;
}



crmhash_t fake12_hash(const unsigned char *str, int len, crmhash_t seed)
{
	int i;
	crmhash_t ret = seed;
	uint64_t m;

	for (i = 0; i < len; i++)
	{
		uint32_t n = str[i] + i;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 1812433253UL; // 7
		v++;

		ret = v - (v >> 32);
		
    /* Tempering */
    ret ^= (ret >> 11);
    ret ^= (ret << 7) & 0x9d2c5680UL;
    ret ^= (ret << 15) & 0xefc60000UL;
    ret ^= (ret >> 18);
	}

	{
		uint32_t n = len;
		uint64_t v;

		n ^= n << 8;
		n ^= n << 16;
		n -= 0x09050301; // make sure every byte in the 32-bit value has a different bit pattern
		v = ret - n;

		v *= 1812433253UL; // 7
		v++;

		ret = v - (v >> 32);
		
    /* Tempering */
    ret ^= (ret >> 11);
    ret ^= (ret << 7) & 0x9d2c5680UL;
    ret ^= (ret << 15) & 0xefc60000UL;
    ret ^= (ret >> 18);
	}
	return ret;
}















int main(void)
{
	int i = 0;

#if 0
#if 10
	printf("\nTesting: old_crm114_hash\n\n");
	test_avalanche(old_crm114_hash);

	printf("\nTesting: crm114_ger_hash\n\n");
	test_avalanche(crm114_ger_hash);

	printf("\nTesting: hashlittle2_hash\n\n");
	test_avalanche(hashlittle2_hash);
#endif

	printf("\nTesting: fake_hash\n\n");
	test_avalanche(fake_hash);

	printf("\nTesting: mult_hash\n\n");
	test_avalanche(mult_hash);

#if 0
	printf("\nTesting: mult2_hash\n\n");
	test_avalanche(mult2_hash);

	printf("\nTesting: mult3_hash\n\n");
	test_avalanche(mult3_hash);

	printf("\nTesting: mult4_hash\n\n");
	test_avalanche(mult4_hash);            // <-- looks like this has the best distribution; how about funnels, etc.??
#endif

	printf("\nTesting: mult5_hash\n\n");
	test_avalanche(mult5_hash);

	printf("\nTesting: mult6_hash (wrapped MD5)\n\n");
	test_avalanche(mult6_hash);

	printf("\nTesting: mult7_hash (NONwrapped MD5)\n\n");
	test_avalanche(mult7_hash);

#if 0
	printf("\nTesting: fake2_hash\n\n");
	test_avalanche(fake2_hash);

	printf("\nTesting: fake3_hash\n\n");
	test_avalanche(fake3_hash);

	printf("\nTesting: fake4_hash\n\n");
	test_avalanche(fake4_hash);

	printf("\nTesting: fake5_hash\n\n");
	test_avalanche(fake5_hash);

	printf("\nTesting: fake6_hash\n\n");
	test_avalanche(fake6_hash);
#endif
#else
	printf("\nTesting: mult7_hash (NONwrapped MD5)\n\n");
	test_avalanche(mult7_hash);
#endif

	printf("\nTesting: fake7_hash (%.8x) (%d)\n\n", fake7_hash((void *)&i, sizeof(i), 0), (int)sizeof(i));
	test_avalanche(fake7_hash);

	printf("\nTesting: fake8_hash (%.8x) (%d)\n\n", fake8_hash((void *)&i, sizeof(i), 0), (int)sizeof(i));
	test_avalanche(fake8_hash);

	printf("\nTesting: fake9_hash (%.8x) (%d)\n\n", fake9_hash((void *)&i, sizeof(i), 0), (int)sizeof(i));
	test_avalanche(fake9_hash);

#if 0
	printf("\nTesting: fake10_hash [Mersenne] (%.8x) (%d)\n\n", fake10_hash((void *)&i, sizeof(i), 0), (int)sizeof(i));
	test_avalanche(fake10_hash);
#endif

	printf("\nTesting: fake11_hash (%.8x) (%d)\n\n", fake11_hash((void *)&i, sizeof(i), 0), (int)sizeof(i));
	test_avalanche(fake11_hash);

	printf("\nTesting: fake12_hash (%.8x) (%d)\n\n", fake12_hash((void *)&i, sizeof(i), 0), (int)sizeof(i));
	test_avalanche(fake12_hash);

	return 0;
}

