//  crm_math_exec.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2006  William S. Yerazunis, all rights reserved.
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

//    the command line argc, argv
extern int prog_argc;
extern char **prog_argv;

//    the auxilliary input buffer (for WINDOW input)
extern char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used 
//    wherever needed.  These are sized to the same size as the data window.
extern char *inbuf;
extern char *outbuf;
extern char *tempbuf;

static int math_formatter ( double value, char *format, char *buf);


//
//           strmath - evaluate a string for the mathematical result
//
long strmath (char *buf, long inlen, long maxlen, long *retstat)
{
  long status;
  
  if (inlen < 0)
    {
      fatalerror ("Bug in caller to strmath() - it makes no sense to",
		  " have a negative length string!  \n");
      return (0);
    };

  //   Check for first-character control of Algebraic v. RPN
  if (buf[0] == 'A')
    {
      status = stralmath (&buf[1], inlen-1, maxlen, retstat);
      memmove (buf, &buf[1], inlen-1);
      buf[inlen] = '\0';
      return (status);
    }
  if (buf[0] == 'R')
    {
      status = strpnmath (&buf[1], inlen-1, maxlen, retstat);
      memmove (buf, &buf[1], inlen-1);
      buf[inlen] = '\0';
      return (status);
    }

  //   No first-character control, so use q_expansion_mode to control.    
  if (q_expansion_mode == 0 || q_expansion_mode == 2)
    {
      return (stralmath (buf, inlen, maxlen, retstat));
    }
  else
    {
      return (strpnmath (buf, inlen, maxlen, retstat));
    }
}

//    strpnmath - do a basic math evaluation of very simple expressions.
//
//    This does math, in RPN, on a string, and returns a string value.
//
long strpnmath (char *buf, long inlen, long maxlen, long *retstat)
{
  double stack [DEFAULT_MATHSTK_LIMIT];     // the evaluation stack
  double sd;             //  how many 10^n's we've seen since a decimal
  long od;               //  output decimal flag               
  long ip, op;           //  in string pointer, out string pointer
  long sp;               //  stack pointer - points to next (vacant) space
  long sinc;             //  stack incrment enable - do we start a new number
  long errstat;          //  error status

  char outformat[64];    // output format
  long outstringlen;

  //    start off by initializing things
  ip = 0;    //  in pointer is zero
  op = 0;    // output pointer is zero
  sp = 0;    // still at the top of the stack
  od = 0;    // no decimals seen yet, so no flag to output in decimal
  sinc = 0;  // no autopush.
  outformat[0] = '\0'; 

  //     now our number-inputting hacks
  stack[sp] = 0.0 ; 
  sd = 1.0;

  //      all initialized... let's begin.
  
  if (internal_trace) 
    fprintf (stderr, "Math on '%s' len %ld retstat %lx \n", 
	     buf, inlen, (long) retstat);

  for (ip = 0; ip < inlen; ip++)
    {
      if (internal_trace)
	fprintf (stderr, "ip = %ld, sp = %ld, stack[sp] = %f, ch='%c'\n", 
	       ip, sp, stack[sp], buf[ip]);

      if (sp < 0) 
	{
	  errstat = nonfatalerror ("Stack Underflow in math evaluation",
			 "");
	  return (0);
	};

      if (sp >= DEFAULT_MATHSTK_LIMIT)
	{
	  errstat=nonfatalerror ("Stack Overflow in math evaluation.\n "
				 "CRM114 Barbie says 'This math is too hard'.",
				 buf);
	  return (0);
	};

      switch (buf[ip])
	{
	  //
	  //        a digit,or maybe a number - big change - we now use strtod
	  //
	case '.':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '-':
	  {
	    char *frejected;
	    //    handle the case of a minus sign that isn't a unary -.
	    if (buf[ip] == '-' && !( isdigit (buf[ip+1])))
	      {
		if (sp > 0)
		  {
		    sp--;
		    stack[sp] = stack[sp] - stack[sp+1];
		    sinc = 1;
		  }
		break;
	      };
	    
	    //   use atof to convert the string we're looking at.
	    sp++;
	    stack[sp] = strtod ( &buf[ip], &frejected);
	    if (user_trace)
	      fprintf (stderr, "got number: %e\n", stack[sp]);
	    //
	    //    Now, move [ip] over to accomodate characters used.
	    //    (the -1 is because there's an auto-increment in the big 
	    //    FOR-loop)
	    ip = ((unsigned long) frejected) - ((unsigned long) buf ) - 1;
	  }
	  break;
	  //
	  //         and some basic math ops...
	  //
	case '+':
	  {
	    if (sp > 0)
	      {
		sp--;
		stack[sp] = stack[sp] + stack[sp+1];
		sinc = 1;
	      }
	  };
	  break;
	case '*':
	  {
	    if (sp > 0)
	      {
		sp--;
		stack[sp] = stack[sp] * stack[sp+1];
		sinc = 1;
	      }
	  };
	  break;
	case '/':
	  {
	    if (sp > 0)
	      {
		sp--;
		if(stack[sp+1] != 0.0)
		  {
		    stack[sp] = stack[sp] / stack[sp+1];
		    sinc = 1;
		  }
		else
		  {
		    if (retstat) *retstat = -1;
		    nonfatalerror ("Attempt to divide by zero in this:",
				   buf);
		  }
	      }
	  };
	  break;
	case '%':
	  {
	    if (sp > 0)
	      {
		sp--;
		stack[sp] = ((long long) stack[sp]) % ((long long)stack[sp+1]);
		sinc = 1;
	      }
	  };
	  break;

	case '=':
	  {
	    if (sp > 0)
	      {
		sp--;
		if (stack[sp] == stack[sp+1])
		  {
		    if (retstat) *retstat = 0;
		    stack[sp] = 1;
		  }
	        else 
		  {
		    if (retstat) *retstat = 1;
		    stack[sp] = 0;
		  };
		sinc = 1;
	      }
	  };
	  break;

	case '!':
	  {
	    if (sp > 0 && buf[ip+1] == '=')
	      {
		ip++; // gobble up the equals sign
		sp--;
		if (stack[sp] != stack[sp+1])
		  {
		    if (retstat) *retstat = 0;
		    stack[sp] = 1;
		  }
	        else 
		  {
		    if (retstat) *retstat = 1;
		    stack[sp] = 0;
		  };
		sinc = 1;
	      }
	  };
	  break;

	case '>':
	  {
	    if (buf[ip+1] == '=')
	      {
		ip++;   // gobble up the equals sign too... 
		if (sp > 0)
		  {
		    sp--;
		    if (stack[sp] >= stack[sp+1])
		      {
			if (retstat) *retstat = 0;
			stack[sp] = 1;
		      }
		    else 
		      {
			if (retstat) *retstat = 1;
			stack[sp] = 0;
		      };
		    sinc = 1;
		  }
	      }
	    else
	      {
		if (sp > 0)
		  {
		    sp--;
		    if (stack[sp] > stack[sp+1])
		      {
			if (retstat) *retstat = 0;
			stack[sp] = 1;
		      }
		    else 
		      {
			if (retstat) *retstat = 1;
			stack[sp] = 0;
		      };
		    sinc = 1;
		  }
	      };
	  }
	  break;

	case '<':
	  {
	    if (buf[ip+1] == '=')
	      {
		ip++; // gobble up the equals sign
		if (sp > 0)
		  {
		    sp--;
		    if (stack[sp] <= stack[sp+1])
		      {
			if (retstat) *retstat = 0;
			stack[sp] = 1;
		      }
		    else 
		      {
			if (retstat) *retstat = 1;
			stack[sp] = 0;
		      };
		    sinc = 1;
		  }
	      }
	    else
	      {
		if (sp > 0)
		  {
		    sp--;
		    if (stack[sp] < stack[sp+1])
		      {
			if (retstat) *retstat = 0;
			stack[sp] = 1;
		      }
		   else 
		      {
			if (retstat) *retstat = 1;
			stack[sp] = 0;
		      };
		    sinc = 1;
		  }
	      };
	  };
	  break;
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
	case 'x':
	case 'X':
	  //             User-specified formatting; use the user's 
	  //             top-of-stack value as a format.
	  //
	  {
	    if (sp > 0)
	      {
		char tempstring [2048];
		//  Special case - if the format is an integer, add a ".0"
		//  to the format string so we get integer output.
		if ( ((long)stack[sp+1]) / 1 == stack[sp+1])
		  {
		    sprintf(outformat,"%%%g.0%c",stack[sp+1], buf[ip]);
		  }
		else
		  {
		    sprintf(outformat,"%%%g%c", stack[sp+1], buf[ip]);
		  };
		sp--;
		if (internal_trace)
		  fprintf (stderr, "Format string -->%s<-- \n", outformat);
		stack[sp+1] = 0;
		sprintf (tempstring, outformat, stack[sp]);
		if (internal_trace)
		  fprintf (stderr, "Intermediate result string -->%s<-- \n", 
			   tempstring);
		//   Note that X formatting (hexadecimal) does NOT do the 
		//   back conversion; the only effect is to store the 
		//   format string for later.
		if (buf[ip] != 'x' &&
		    buf[ip] != 'X')
		  stack[sp] = strtod (tempstring, NULL);
	      }
	  };
	  break;
	case ' ':
	case '\n':
	case '\t':
	  //
	  //        a space is just an end-of-number - push the number we're 
	  //        seeing.  
	  {
	    sinc = 1;
	  };
	  break;
	case '(':
	case ')':
	  //         why are you using parenthesis in RPN code??
	  {
	    nonfatalerror ("It's just silly to use parenthesis in RPN!",
			   " Perhaps you should check your setups?");
	    sinc = 1;
	  };
	  break;

	default:
	  {
	    char bogus[4];
	    bogus[0] = buf[ip];
	    bogus[1] = '\000';
	    nonfatalerror (" Sorry, but I can't do RPN math on the un-mathy "
			   "character found: ", bogus); 
	    sinc = 1;
	  };
	  break;
	};
    };

  if (internal_trace)
    {
      fprintf (stderr, 
	     "Final qexpand state:  ip = %ld, sp = %ld, stack[sp] = %f, ch='%c'\n", 
	       ip, sp, stack[sp], buf[ip]);
      if (retstat) 
	fprintf (stderr, "retstat = %ld\n", *retstat);
    };

  //      now the top of stack contains the result of the calculation.
  //      fprintf it into the output buffer, and we're done.
  outstringlen = math_formatter ( stack[sp], outformat, buf) ;
  return (outstringlen);
}
 


/////////////////////////////////////////////////////////////////
//
//     Here's where we format a floating point number so it's "purty".
//     Note that if "format" is NULL, or a null string, we do smart
//     formatting on the number itself.
//
//   
int math_formatter ( double value, char *format, char *buf)
{
  //  If the user supplied a format, use that.
  //  
  if (format && format[0] != '\0')
    {
      //
      //  special case - if the user supplied an x or X-format, that's
      //  preconversion to integer; use that strlen() does not count
      //  the null termination.
      if (format[strlen(format)-1] == 'x'
	  || format[strlen(format)-1] == 'X')
	{
	  long long equiv ;
	  //  fprintf (stderr, "BLARG\n"); 
	  equiv = value * 1;
	  //fprintf (stderr, "Equiv: %llx\n", equiv);
	  sprintf (buf, format, equiv);
	  return (strlen (buf));
	};
      //  	
      //    Nothing so special; use the user format as it is.
      sprintf (buf, format, value);
      return (strlen (buf));
    };
      
	


  //   Nope - we didn't get a preferred formatting, so here's the 
  //   adaptive smart code.
	       
  //
  //      print out 0 as 0
  //
  if (value == 0.0 )
    {
      sprintf (buf, "0");
      goto formatdone;
    }
  //
  //       use E notation if bigger than 1 trillion
  //
  if (value > 1000000000000.0 || value < -1000000000000.0 )
    {
      sprintf (buf, "%.5E", value);
      goto formatdone;
    }
  // 
  //       use E notation if smaller than .01
  //
  if ( value  < 0.01 && value > -0.01 )
    {
      sprintf (buf, "%.5E", value);
      goto formatdone;
    }
  //
  //       if an integer value, print with 0 precision
  //
  if (((long)(value*2.0) / 2) == value)
    {
      sprintf (buf, "%.0f", value);
      goto formatdone;
    }
  //
  //       if none of the above, print with five digits precision
  //
  sprintf (buf, "%.5f", value);
  //
  //
  //         one way or another, once we're here, we've sprinted it.
 formatdone:
  return (strlen (buf));
}

//
//      stralnmath - evaluate a mathematical expression in algebraic
//    (that is, infix parenthesized) notation.
//
//      The algorithm is this:
//    see an open parenthesis - push an empty level
//    see a close parethesis -  try to "reduce", then pop over the empty 
//    see an operator - push it onto opstack, sp++
//    see a number - push it, then try to "reduce" if there's a valid op.
//
//    reduce: 
//         while sp > 0 
//             if opstack[sp-1] valid
//                   sp--
//                   execute opstack[sp] on [sp], [sp+1]
//                   replace sp with result
//
//    Note that the empty levels (opstack[sp] == '\000') that are
//    produced by open and close parens are how we prevent runaway
//    reduce operations on the stack.
//

long stralmath (char *buf, long inlen, long maxlen, long *retstat)
{
  double valstack [DEFAULT_MATHSTK_LIMIT];     // the evaluation value stack
  long opstack [DEFAULT_MATHSTK_LIMIT];     // the evaluation operator stack
  double sd;             //  how many 10^n's we've seen since a decimal
  long od;               //  output decimal flag               
  long ip, op;           //  in string pointer, out string pointer
  long sp;               //  stack pointer - points to next (vacant) space
  long sinc;             //  stack incrmenter - do we push on next digit in?
  long errstat;          //  error status

  char outformat[64];   // output format (if needed)

  //    start off by initializing things
  ip = 0;    //  in pointer is zero
  op = 0;    // output pointer is zero
  sp = 0;    // still at the top of the stack
  od = 0;    // no decimals seen yet, so no flag to output in decimal
  sinc = 0;  // no autopush.

  outformat [0] = '\0';

  //     now our number-inputting hacks
  valstack[sp] = 0.0 ; 
  opstack[sp] = '\000';
  sd = 1.0;

  //      all initialized... let's begin.

  if (internal_trace) 
    fprintf (stderr, "Math on '%s' len %ld *retstat %lx \n", 
	     buf, inlen, (long) retstat);

  for (ip = 0; ip < inlen; ip++)
    {
      if (internal_trace)
	fprintf (stderr, "ip = %ld, sp = %ld, valstack[sp] = %f," 
		 "opstack = '%c', h='%c'\n",
		 ip, sp, valstack[sp], (short) opstack[sp], buf[ip]);

      if (sp < 0) 
	{
	  errstat = nonfatalerror ("Stack Underflow in math evaluation",
			 "");
	  return (0);
	};

      if (sp >= DEFAULT_MATHSTK_LIMIT)
	{
	  errstat = nonfatalerror ("Stack Overflow in math evaluation. ",
			 "CRM114 Barbie says 'This math is too hard'.");
	  return (0);
	};


      switch (buf[ip])
	{
	  //
	  //        a digit,or maybe a number - big change - we now use strtod
	  //
	case '.':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '-':
	  {
	    char *frejected;
	    if (user_trace)
	      fprintf (stderr, " found a poss number starting with %c, ",
		       buf[ip]);

	    //    handle the case of a minus sign that isn't a unary -.
	    if (buf[ip] == '-' 
		&& !( isdigit (buf[ip+1]))) 
	      {
		if (sp > 0 && opstack[sp-1] != '\000')
		  {
		    long state;
		    if (user_trace) 
		      fprintf (stderr, "nope, dyadic minus. \n");
		    if (sp > 0) sp--;
		    state = stralmath_reduce (valstack, opstack, 
					      &sp, outformat);
		    if (state != -1 && retstat) *retstat = state;
		  }
		opstack[sp] = buf[ip]; 
		if (opstack[sp] == ' ') opstack[sp] = '\000';
	      }
	    else
	      {
		
		//   use atof to convert the string we're looking at.
		sp++;
		opstack[sp] = '\000';
		valstack[sp] = strtod ( &buf[ip], &frejected);
		if (user_trace)
		  fprintf (stderr, " got number: %e\n", valstack[sp]);
		//
		//    Now, move [ip] over to accomodate characters used.
		//    (the -1 is because there's an auto-increment in the big 
		//    FOR-loop)
		ip = ((unsigned long) frejected) - ((unsigned long) buf ) - 1;
		sinc = 0;
	      }
	  }
	  break;
	  //
	  //         and now the parenthesis ops
	  //
	  //        open paren- we start a new stack level
	case '(':
	  {
	    if (user_trace)
	      fprintf (stderr, " got open parenthesis\n");
	    sd = 1.0;
	    sp++;
	    valstack[sp] = 0.0;
	    opstack[sp] = '\000';
	  };
	  break;
	  //
	  //        close paren- we finish evaluation down to the open,
	  //        then supply the result to the next lower level.
	case ')':
	  {
	    long state;
	    if (user_trace)
	      fprintf (stderr, " got close parenthesis\n");
	    if (sp > 0) sp--;
	    state = stralmath_reduce (valstack, opstack, &sp, outformat);
	    if (state != -1 && retstat) *retstat = state;
	    if (sp > 0) sp--;
	    //   and get rid of that extra level we inserted before.
	    valstack[sp] = valstack[sp+1];
	  };
	  break;
	  //   
	  //      The se dyadic operators just put themselves on the
	  //      stack uunless there's a prior dyadic operator, that
	  //      operator runs first.
	  //
	  //	case '-': (this is handled up above, as part of unary '-'
	case '+':
	case '*':
	case '/':
	case '%':
	case '=':
	case '>':
	case '<':
	case '!':
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
	case 'x':
	case 'X':
	  {
	    if (user_trace)
	      {
		if (buf[ip+1] == '=')
		  {
		    fprintf (stderr, " got 2-char dyadic operator %c%c\n", 
			     buf[ip], buf[ip+1]);
		  }
		else
		  fprintf (stderr, " got dyadic operator %c\n", buf[ip]);
	      };
	    //     If there's something on the stack, run it first.
	    if (sp > 0 && opstack[sp-1] != '\000')
	      {
		long state;
		if (sp > 0) sp--;
		state = stralmath_reduce (valstack, opstack, &sp, outformat);
		if (state != -1 && retstat) *retstat = state;
	      };
	    sd = 1.0;
	    sinc = 1;
	    //         now put ourselves onto the stack
	    if (buf[ip+1] == '=')
	      {
		opstack[sp] = (buf[ip] << 8) + buf[ip+1];
		ip++ ; // gobble up the extra characters
	      }
	    else
	      {
		opstack[sp] = buf[ip];
	      }
	    if (opstack[sp] == ' ') opstack[sp] = '\000';
	  };
	  break;
	case ' ':
	case '\n':
	case '\t':
	  break;
	default:
	  {
	    char bogus[4];
	    bogus[0] = buf[ip];
	    bogus[1] = '\000';
	    nonfatalerror (" Sorry, but I can't do math on the un-mathy "
			   "character found: ", bogus); 
	  };
	  break;
	};
    };

  //  Now do final executes....
  if (sp > 0) 
    {
      long state;
      if (sp > 0 && opstack [sp-1] != '\000') sp--;
      state = stralmath_reduce (valstack, opstack, &sp, outformat);
      if (state != -1 && retstat) *retstat = state; 
    };



  if (internal_trace)
    {
      fprintf (stderr, 
	     "Final qexpand state:  ip = %ld, sp = %ld, valstack[sp] = %f, ch='%c'\n", 
	       ip, sp, valstack[sp], buf[ip]);
      if (retstat) 
	fprintf (stderr, "retstat = %ld\n", *retstat);
    };

  //      now the top of stack contains the result of the calculation.
  //      fprintf it into the output buffer, and we're done.

  return (math_formatter (valstack[sp], outformat, buf));
}

//////////////////////////////////////////////////////////////////////
//
//            stralmath_reduce - actually do the math for algebraic arithmetic.
//            retval is 0 for "successful EQ's", and 1 for unsuccessful,
//            and -1 for "no change".
//
long stralmath_reduce (double *valstack, long *opstack, long *sp, char *outformat)
{
  long retval;
  retval = -1;
  if (internal_trace)
    fprintf (stderr, "  start: *sp = %3ld, "
	     "vs[*sp] = %6.3f, vs[*sp+1] = %6.3f, "
	     "op[*sp] = '%c'\n", 
	     *sp, valstack[*sp], valstack[*sp+1], (short) opstack[*sp]);
  
  while (*sp >= 0 && opstack[*sp] != '\000')
    {
      if (internal_trace)
	fprintf (stderr, "running: *sp = %3ld, "
		 "vs[*sp] = %6.3f, vs[*sp+1] = %6.3f, "
		 "op[*sp] = '0x%lX'\n", 
		 *sp, valstack[*sp], valstack[*sp+1], 
		 (unsigned long)opstack[*sp]);
      switch (opstack[*sp])
	{
	case '+':
	  valstack[*sp] = valstack[*sp] + valstack[*sp+1];
	  opstack[*sp] = '\000';
	  break;
	case '-':
	  valstack[*sp] = valstack[*sp] - valstack[*sp+1];
	  opstack[*sp] = '\000';
	  break;
	case '*':
	  valstack[*sp] = valstack[*sp] * valstack[*sp+1];
	  opstack[*sp] = '\000';
	  break;
	case '/':
	  if (valstack[*sp+1] == 0.0) 
	    {
	      opstack[*sp+1] = '\000';
	      nonfatalerror ("Attempt to divide by zero.","");
	      return (-1);		    
	    };
	  valstack[*sp] = valstack[*sp] / valstack[*sp+1];
	  opstack[*sp] = '\000';
	  break;
	case '%':
	  valstack[*sp] = ((long long)valstack[*sp]) % ((long long) valstack[*sp+1]);
	  opstack[*sp] = '\000';
	  break;
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
	case 'x':
	case 'X':
	  //             User-specified formatting; use the user's 
	  //             top-of-stack value as a format.
	  //
	  //             Note the funny business in the cases where a .0
	  //             format is needed, because .0 won't print directly
	  //             when we build the format with a %g .
	  {
	    char tempstring [2048];
	    if ( ((long)valstack[*sp+1]) / 1 == valstack[*sp+1])
	      {
		sprintf (outformat, "%%%g.0%c", 
			 valstack[*sp+1], (short) opstack[*sp]);
	      }
	    else
	      {
		sprintf (outformat, "%%%g%c", valstack[*sp+1], 
			 (short) opstack[*sp]);
	      };
	    if (internal_trace)
	      fprintf (stderr, "Format string -->%s<-- \n", outformat);

	    //      A little more funny business needed for hexadecimal print
	    //      out, because X format can't take IEEE floating point
	    //      as inputs.
	    if (opstack[*sp] != 'x' &&
		opstack[*sp] != 'X')
	      {
		// fprintf (stderr, "NORMAL ");
		sprintf (tempstring, outformat, valstack[*sp]);
		valstack[*sp] = strtod (tempstring, NULL);
	      }
	    else
	      {
		//    Note that we actually don't use the results of octal
		//    conversion; the only effect is to set the final format
		//    string.
		long long equiv;
		// fprintf (stderr, "BLORT ");
		equiv = valstack[*sp] + 0;
		// fprintf (stderr, "equiv -->%10lld<-- \n", equiv);
		sprintf (tempstring, outformat, equiv);
	      }
	    if (internal_trace)
	      fprintf (stderr, "Intermediate result string -->%s<-- \n", 
		       tempstring);
	    opstack [*sp] = '\000';
	  };
	  break;
	case (('!' << 8) + '='):
	  //fprintf (stderr, "BANG EQUALS \n");
	  if (valstack[*sp] != valstack[*sp+1])
	    {
	      valstack[*sp] = 1 ;
	    }
	  else
	    {
	      valstack[*sp] = 0;
	    }
	  opstack[*sp] = '\000';
	  retval = 1 - valstack[*sp]; 
	  break;
	case '=':
	  if (valstack[*sp] == valstack[*sp+1])
	    {
	      valstack[*sp] = 1 ;
	    }
	  else
	    {
	      valstack[*sp] = 0;
	    }
	  opstack[*sp] = '\000';
	  retval = 1 - valstack[*sp]; 
	  break;
	case '>':
	  if (valstack[*sp] > valstack[*sp+1])
	    {
	      valstack[*sp] = 1 ;
	    }
	  else
	    {
	      valstack[*sp] = 0;
	    }
	  opstack[*sp] = '\000';
	  retval = 1 - valstack[*sp]; 
	  break;
	case (('>' << 8) + '='):
	  // fprintf (stderr, "GREATER EQUALS\n");
	  if (valstack[*sp] >= valstack[*sp+1])
	    {
	      valstack[*sp] = 1 ;
	    }
	  else
	    {
	      valstack[*sp] = 0;
	    }
	  opstack[*sp] = '\000';
	  retval = 1 - valstack[*sp]; 
	  break;
	case (('<' << 8) + '='):
	  // fprintf (stderr, "LESSER EQUALS\n");
	  if (valstack[*sp] <= valstack[*sp+1])
	    {
	      valstack[*sp] = 1 ;
	    }
	  else
	    {
	      valstack[*sp] = 0;
	    }
	  opstack[*sp] = '\000';
	  retval = 1 - valstack[*sp]; 
	  break;
	case '<':
	  if (valstack[*sp] < valstack[*sp+1])
	    {
	      valstack[*sp] = 1 ;
	    }
	  else
	    {
	      valstack[*sp] = 0;
	    }
	  opstack[*sp] = '\000';
	  retval = 1 - valstack[*sp]; 
	  break;
	default:
	  break;
	};
      if (*sp > 0 && opstack[(*sp)-1] != '\000') *sp = *sp - 1;
    };
  if (internal_trace)
    fprintf (stderr, " finish: *sp = %3ld, "
	     "vs[*sp] = %6.3f, vs[*sp+1] = %6.3f, "
	     "op[*sp] = '0x%lX'\n", 
	     *sp, valstack[*sp], valstack[*sp+1], 
	     (unsigned long) opstack[*sp]);
  return (retval);
}
