//  crm_math_exec.c  - Controllable Regex Mutilator,  version v1.0
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



static int math_formatter(double value, char *format, char *buf, int buflen);


//
// strmath - evaluate a string for the mathematical result,
//           returning the length of the valid string.
//
int strmath(char *buf, int inlen, int maxlen, int *retstat)
{
    int status;

    if (inlen < 0)
    {
        fatalerror("Bug in caller to strmath() - it makes no sense to",
            " have a negative length string!\n");
        return 0;
    }

    //   Check for first-character control of Algebraic v. RPN
    if (buf[0] == 'A')
    {
        memmove(buf, &buf[1], inlen - 1);
        buf[inlen - 1] = 0;
        status = stralmath(buf, inlen - 1, maxlen, retstat);
        return status;
    }
    if (buf[0] == 'R')
    {
        //      Do we want to do selective tracing?
        memmove(buf, &buf[1], inlen - 1);
        buf[inlen - 1] = 0;
        status = strpnmath(buf, inlen - 1, maxlen, retstat);
        return status;
    }

    //   No first-character control, so use q_expansion_mode to control.
    if (q_expansion_mode == 0 || q_expansion_mode == 2)
    {
        return stralmath(buf, inlen, maxlen, retstat);
    }
    else
    {
        return strpnmath(buf, inlen, maxlen, retstat);
    }
}

//    strpnmath - do a basic math evaluation of very simple expressions.
//
//    This does math, in RPN, on a string, and returns a string value.
//
int strpnmath(char *buf, int inlen, int maxlen, int *retstat)
{
    double stack[DEFAULT_MATHSTK_LIMIT];    // the evaluation stack
    double sd;                              //  how many 10^n's we've seen since a decimal
    int od;                                 //  output decimal flag
    int ip, op;                             //  in string pointer, out string pointer
    int sp;                                 //  stack pointer - points to next (vacant) space
    int sinc;                               //  stack incrment enable - do we start a new number
    int errstat;                            //  error status

    char outformat[64];  // output format
    int outstringlen;

    //    start off by initializing things
    ip = 0;   //  in pointer is zero
    op = 0;   // output pointer is zero
    sp = 0;   // still at the top of the stack
    od = 0;   // no decimals seen yet, so no flag to output in decimal
    sinc = 0; // no autopush.
    outformat[0] = 0;

    //     now our number-inputting hacks
    stack[sp] = 0.0;
    sd = 1.0;

    //      all initialized... let's begin.

    if (internal_trace)
	{
        fprintf(stderr, "Math on '%s' len %d retstat %p\n",
            buf, inlen, (void *)retstat);
	}

    for (ip = 0; ip < inlen; ip++)
    {
        if (internal_trace)
            fprintf(stderr, "ip = %d, sp = %d, stack[sp] = %f, ch='%c'\n",
                ip, sp, stack[sp], (crm_isascii(buf[ip]) && crm_isprint(buf[ip]) ? buf[ip] : '.'));

        if (sp < 0)
        {
            errstat = nonfatalerror("Stack Underflow in math evaluation",
                "");
            return 0;
        }

        if (sp >= DEFAULT_MATHSTK_LIMIT)
        {
            errstat = nonfatalerror("Stack Overflow in math evaluation.\n "
                                    "CRM114 Barbie says 'This math is too hard'.",
                buf);
            return 0;
        }

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
        case '+':
            {
                char *frejected;
                //    handle the case of a minus sign that isn't a unary -.
                if (buf[ip] == '-' && !(crm_isdigit(buf[ip + 1])))
                {
                    if (sp > 0)
                    {
                        sp--;
                        stack[sp] = stack[sp] - stack[sp + 1];
                        sinc = 1;
                    }
                    break;
                }
                if (buf[ip] == '+' && !(crm_isdigit(buf[ip + 1])))
                {
                    if (sp > 0)
                    {
                        sp--;
                        stack[sp] = stack[sp] + stack[sp + 1];
                        sinc = 1;
                    }
                    break;
                }

                //   Neither unary +/-  so we use strtod to convert
                //   the string we're looking at to floating point.
                sp++;
                stack[sp] = strtod(&buf[ip], &frejected);
                if (user_trace)
                    fprintf(stderr, "got number: %e\n", stack[sp]);
                //
                //    Now, move [ip] over to accomodate characters used.
                //    (the -1 is because there's an auto-increment in the big
                //    FOR-loop)
                ip = (int)(frejected - buf) - 1;
            }
            break;

            //
            //   and now the standard math operators (except for - and + above)
            //
        case '*':
            {
                if (sp > 0)
                {
                    sp--;
                    stack[sp] = stack[sp] * stack[sp + 1];
                    sinc = 1;
                }
            }
            break;

        case '/':
            {
                if (sp > 0)
                {
                    sp--;
                    // don't worry about divide-by-zero, we get INF in IEEE.
                    stack[sp] = stack[sp] / stack[sp + 1];
                    sinc = 1;
                }
            }
            break;

        case '%':
            {
                if (sp > 0)
                {
                    sp--;
#ifdef CRM_SUPPORT_FMOD
                    stack[sp] = fmod(stack[sp], stack[sp + 1]);
#else
                    stack[sp] = ((int64_t)stack[sp]) % ((int64_t)stack[sp + 1]);
#endif
                    sinc = 1;
                }
            }
            break;

        case '^': // exponentiation - for positive bases, neg base + int exp.
            if (sp > 0)
            {
                sp--;
                if (stack[sp] < 0.0
                    /* use FLT_EPSILON to compensate for added inaccuracy due to previous calculations */
                    && ((int64_t)(stack[sp + 1])) >= stack[sp + 1] - FLT_EPSILON
                    && ((int64_t)(stack[sp + 1])) <= stack[sp + 1] + FLT_EPSILON)
                {
                    stack[sp] = stack[sp] / 0.0;
                }
                else
                {
                    stack[sp] = pow(stack[sp], stack[sp + 1]);
                }
                if (internal_trace)
                    fprintf(stderr, "exp out: %f\n", stack[sp]);
                sinc = 1;
            }
            break;

        case 'v': // logs as  BASE v ARG; (NaN on BASE <= 0)
            if (sp > 0)
            {
                sp--;
                if (stack[sp] <= 0.0)
                {
                    stack[sp] = stack[sp] / 0.0;
                }
                else
                {
                    stack[sp] = log(stack[sp + 1]) / log(stack[sp]);
                }
                sinc = 1;
            }
            break;

        case '=':
            {
                if (sp > 0)
                {
                    sp--;
                    if (stack[sp] == stack[sp + 1])
                    {
                        if (retstat)
                            *retstat = 0;
                        stack[sp] = 1.0;
                    }
                    else
                    {
                        if (retstat)
                            *retstat = 1;
                        stack[sp] = 0.0;
                    }
                    sinc = 1;
                }
            }
            break;

        case '!':
            {
                if (sp > 0 && buf[ip + 1] == '=')
                {
                    ip++; // gobble up the equals sign
                    sp--;
                    if (stack[sp] != stack[sp + 1])
                    {
                        if (retstat)
                            *retstat = 0;
                        stack[sp] = 1.0;
                    }
                    else
                    {
                        if (retstat)
                            *retstat = 1;
                        stack[sp] = 0.0;
                    }
                    sinc = 1;
                }
            }
            break;

        case '>':
            {
                if (buf[ip + 1] == '=')
                {
                    ip++; // gobble up the equals sign too...
                    if (sp > 0)
                    {
                        sp--;
                        if (stack[sp] >= stack[sp + 1])
                        {
                            if (retstat)
                                *retstat = 0;
                            stack[sp] = 1.0;
                        }
                        else
                        {
                            if (retstat)
                                *retstat = 1;
                            stack[sp] = 0.0;
                        }
                        sinc = 1;
                    }
                }
                else
                {
                    if (sp > 0)
                    {
                        sp--;
                        if (stack[sp] > stack[sp + 1])
                        {
                            if (retstat)
                                *retstat = 0;
                            stack[sp] = 1;
                        }
                        else
                        {
                            if (retstat)
                                *retstat = 1;
                            stack[sp] = 0;
                        }
                        sinc = 1;
                    }
                }
            }
            break;

        case '<':
            {
                if (buf[ip + 1] == '=')
                {
                    ip++; // gobble up the equals sign
                    if (sp > 0)
                    {
                        sp--;
                        if (stack[sp] <= stack[sp + 1])
                        {
                            if (retstat)
                                *retstat = 0;
                            stack[sp] = 1;
                        }
                        else
                        {
                            if (retstat)
                                *retstat = 1;
                            stack[sp] = 0;
                        }
                        sinc = 1;
                    }
                }
                else
                {
                    if (sp > 0)
                    {
                        sp--;
                        if (stack[sp] < stack[sp + 1])
                        {
                            if (retstat)
                                *retstat = 0;
                            stack[sp] = 1;
                        }
                        else
                        {
                            if (retstat)
                                *retstat = 1;
                            stack[sp] = 0;
                        }
                        sinc = 1;
                    }
                }
            }
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
                    char tempstring[2048];
                    tempstring[0] = 0;
                    sp--;
                    //  Special case - if the format is an integer, add a ".0"
                    //  to the format string so we get integer output.
                    if (buf[ip] == 'x' || buf[ip] == 'X')
                    {
                        if (((int)stack[sp + 1]) <= stack[sp + 1] + FLT_EPSILON
                            && ((int)stack[sp + 1]) >= stack[sp + 1] - FLT_EPSILON)
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%%.0gll%c",
                                stack[sp + 1], (short)buf[ip]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                        else
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%0%.0gll%c",
                                stack[sp + 1], (short)buf[ip]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                    }
                    else
                    {
                        if (((int)stack[sp + 1]) <= stack[sp + 1] + FLT_EPSILON
                            && ((int)stack[sp + 1]) >= stack[sp + 1] - FLT_EPSILON)
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%%.0g.0%c", stack[sp + 1], buf[ip]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                        else
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%%g%c", stack[sp + 1], buf[ip]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                    }
                    if (internal_trace)
                        fprintf(stderr, "Format string -->%s<-- \n", outformat);
                    stack[sp + 1] = 0;
                    if (buf[ip] != 'x' && buf[ip] != 'X')
                    {
                        snprintf(tempstring, WIDTHOF(tempstring), outformat, stack[sp]);
                        tempstring[WIDTHOF(tempstring) - 1] = 0;
                        if (internal_trace)
						{
                            fprintf(stderr,
                                "Intermediate result string -->%s<-- \n",
                                tempstring);
                    }
					}
                    else
                    {
                        int64_t intpart;
                        intpart = (int64_t)stack[sp];
                        snprintf(tempstring, WIDTHOF(tempstring), outformat, intpart);
                        tempstring[WIDTHOF(tempstring) - 1] = 0;
                        if (internal_trace)
						{
							fprintf(stderr,
                                "Intermediate hex result string -->%s<-- \n",
                                tempstring);
                    }
					}
                    //   And now do the back conversion of the result.
                    //   Note that X formatting (hexadecimal) does NOT do the
                    //   back conversion; the only effect is to store the
                    //   format string for later.
                    if (buf[ip] != 'x'
                        && buf[ip] != 'X')
                    {
                        stack[sp] = strtod(tempstring, NULL);
                    }
                }
            }
            break;

        case ' ':
        case '\r':
        case '\n':
        case '\t':
            //
            //        a space is just an end-of-number - push the number we're
            //        seeing.
            {
                sinc = 1;
            }
            break;

        case '(':
        case ')':
            //         why are you using parenthesis in RPN code??
            {
                nonfatalerror("It's just silly to use parenthesis in RPN!",
                    " Perhaps you should check your setups?");
                sinc = 1;
            }
            break;

        default:
            {
                char bogus[4];
                bogus[0] = buf[ip];
                bogus[1] = 0;
                nonfatalerror(" Sorry, but I can't do RPN math on the un-mathy "
                              "character found: ", bogus);
                sinc = 1;
            }
            break;
        }
    }

    if (internal_trace)
    {
        fprintf(stderr,
            "Final qexpand state:  ip = %d, sp = %d, stack[sp] = %f, ch='%c'\n",
            ip, sp, stack[sp], (crm_isascii(buf[ip]) && crm_isprint(buf[ip]) ? buf[ip] : '.'));
        if (retstat)
            fprintf(stderr, "retstat = %d\n", *retstat);
    }

    //      now the top of stack contains the result of the calculation.
    //      fprintf it into the output buffer, and we're done.
    outstringlen = math_formatter(stack[sp], outformat, buf, maxlen);
    CRM_ASSERT(outstringlen >= 0);
    CRM_ASSERT(outstringlen < maxlen);
    return outstringlen;
}



/////////////////////////////////////////////////////////////////
//
//     Here's where we format a floating point number so it's "purty".
//     Note that if "format" is NULL, or a null string, we do smart
//     formatting on the number itself.
//
//
int math_formatter(double value, char *format, char *buf, int buflen)
{
    int outlen;

    CRM_ASSERT(buf != NULL);
    CRM_ASSERT(buflen > 0);
    CRM_ASSERT(format != NULL);

    //  If the user supplied a format, use that.
    //
    if (format && format[0] != 0)
    {
        //
        //  special case - if the user supplied an x or X-format, that's
        //  preconversion to integer; use that strlen() does not count
        //  the null termination.
        if (format[strlen(format) - 1] == 'x'
            || format[strlen(format) - 1] == 'X')
        {
            int64_t equiv;
            if (internal_trace)
                fprintf(stderr, "Final hex format: %s\n", format);
            equiv = (int64_t)value;
            outlen = snprintf(buf, buflen, format, equiv);
            /*
             *    [i_a] taken from MSVC2005 documentation (and why it's dangerous to simply return outlen)
             *    - emphasis is mine -:
             *
             *    Return Value
             *
             *    Let len be the length of the formatted data string (not including the terminating null). len
             *    and count are in bytes for _snprintf, wide characters for _snwprintf.
             *
             *    If len < count, then len characters are stored in buffer, a null-terminator is appended,
             *    and len is returned.
             *
             *    If len = count, then len characters are stored in buffer, no null-terminator is appended,
             *    and len is returned.
             *
             *    If len > count, then count characters are stored in buffer, no null-terminator is appended,
             *    and a NEGATIVE value is returned.
             *
             *    If buffer is a null pointer and count is nonzero, or format is a null pointer, the invalid
             *    parameter handler is invoked, as described in Parameter Validation. If execution is
             *    allowed to continue, THESE FUNCTIONS RETURN -1 and set errno to EINVAL.
             */
            buf[buflen - 1] = 0;
            /* return (outlen); ** [i_a] */
            return (int)strlen(buf);
        }
        //
        //    Nothing so special; use the user format as it is.
        if (internal_trace)
            fprintf(stderr, "Final format: %s\n", format);
        outlen = snprintf(buf, buflen, format, value);
        buf[buflen - 1] = 0;
        /* return (outlen); ** [i_a] */
        return (int)strlen(buf);
    }

    //   Nope - we didn't get a preferred formatting, so here's the
    //   adaptive smart code.

    //
    //      print out 0 as 0
    //
    if (value == 0.0)
    {
        outlen = snprintf(buf, buflen, "0");
        buf[buflen - 1] = 0;
        goto formatdone;
    }
    //
    //       use E notation if bigger than 1 trillion
    //
    if (value > 1000000000000.0 || value < -1000000000000.0)
    {
        outlen = snprintf(buf, buflen, "%.5E", value);
        buf[buflen - 1] = 0;
        goto formatdone;
    }
    //
    //       use E notation if smaller than .01
    //
    if (value  < 0.01 && value > -0.01)
    {
        outlen = snprintf(buf, buflen, "%.5E", value);
        buf[buflen - 1] = 0;
        goto formatdone;
    }
    //
    //       if an integer value, print with 0 precision
    //
    if (((int)(value * 2.0) / 2) == value)
    {
        outlen = snprintf(buf, buflen, "%.0f", value);
        buf[buflen - 1] = 0;
        goto formatdone;
    }
    //
    //       if none of the above, print with five digits precision
    //
    outlen = snprintf(buf, buflen, "%.5f", value);
    buf[buflen - 1] = 0;
    //
    //
    //         one way or another, once we're here, we've sprinted it.
    formatdone:
    if (internal_trace)
        fprintf(stderr, "math_formatter outlen = %d / %d\n", outlen, (int)strlen(buf));
    /* return (outlen); ** [i_a] */
    return (int)strlen(buf);
}

////////////////////////////////////////////////////////////////////
//
//   Alternative implementation of the uglyness that is string math.
//
//   This version uses two stacks (left arg, op) and a single scalar
//   rightarg.  Partial computations are kept on the leftarg and op
//   stack.  The current stack status is held in validstack, and is
//   the OR of LEFTVALID, OPVALID, and RIGHTVALID.
//
#define LEFTVALID 0x1
#define OPVALID 0x2
#define RIGHTVALID 0x4

int stralmath(char *buf, int inlen, int maxlen, int *retstat)
{
    double leftarg[DEFAULT_MATHSTK_LIMIT];        // left float arg
    int opstack[DEFAULT_MATHSTK_LIMIT];           // operand
    double rightarg;                              // right float arg
    int validstack[DEFAULT_MATHSTK_LIMIT];        // validity markers
    int sp;                                       // stack pointer
    int ip, op;                                   // input and output pointer
    int errstat;                                  //  error status
    char *frejected;                              //  done loc. for a strtod.
    char outformat[256];                          //  how to format our result
    int state;                                    // Local copy of state, in case

    // retstat is NULL (not used)
    //   Start off by initializing things
    ip = 0;
    op = 0;
    sp = 0;
    outformat[0] = 0;
    state = 0;

    //     Set up the stacks
    //
    leftarg[0] = 0.0;
    rightarg = 0.0;
    opstack[0] = 0;
    validstack[0] = 0;

    //  initialization done... begin the work.
    if (internal_trace)
    {
        fprintf(stderr, "Starting Algebraic Math on '%s' (len %d)\n",
            buf, inlen);
    }

    for (ip = 0; ip < inlen; ip++)
    {
        //   Debugging trace
        if (internal_trace)
        {
            fprintf(stderr,
                "ip = %d, sp = %d, L=%f, Op=%c, R=%f, V=%x next='%c'\n",
                ip, sp,
                leftarg[sp], (short)opstack[sp],
                rightarg, (short)validstack[sp],
                (crm_isascii(buf[ip]) && crm_isprint(buf[ip]) ? buf[ip] : '.'));
        }

        //    Top of the loop- we're a state machine driven by the top of
        //    the stack's validity.

        if (sp >= DEFAULT_MATHSTK_LIMIT)
        {
            errstat = nonfatalerror("Stack Overflow in math evaluation. ",
                "CRM114 Barbie says 'This math is too hard'.");
            if (retstat)
                *retstat = 0;
            return 0;
        }

        switch (validstack[sp])
        {
        case (0):
            //  empty top of stack; can accept either number or monadic operator
            if (internal_trace)
                fprintf(stderr, "stacktop empty\n");

            switch (buf[ip])
            {
                //   Monadic operators and numbers
            case '-':
            case '+':
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
            case '.':
            case ',':   // for those locales that use , not . as decimal
                {
                    if (internal_trace)
                        fprintf(stderr, "found left numeric\n");

                    leftarg[sp] = strtod(&buf[ip], &frejected);
                    if (user_trace)
                        fprintf(stderr, " Got left arg %e\n", leftarg[sp]);
                    ip = (int)(frejected - buf) - 1;
                    validstack[sp] = LEFTVALID;
                }
                break;

            case '(':
                {
                    if (internal_trace)
                        fprintf(stderr,
                            "Open Paren - start new math stack level\n");
                    sp++;
                    leftarg[sp] = 0.0;
                    rightarg = 0.0;
                    opstack[sp] = 0;
                    validstack[sp] = 0;
                }
                break;

                //      deal with a possible rightarg strtod situation
            case ' ':
                break;

            default:
                errstat = nonfatalerror("Math expression makes no sense",
                    " (need to have a number here).");
                if (retstat)
                    *retstat = 0;
                return 0;
            }
            break;

            //  if left arg is valid; next thing must be an operator;
            //   however op then op is also valid and should form composite
            //    operators like '>=' and '!=' (see below).

        case (LEFTVALID):
            if (internal_trace)
                fprintf(stderr, "leftvalid\n");
            switch (buf[ip])
            {
            case '-':
            case '+':
            case '*':
            case '/':
            case '%':
            case '>':
            case '<':
            case '=':
            case '!':
            case '^':
            case 'v':
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'x':
            case 'X':
                {
                    if (internal_trace)
                        fprintf(stderr, "found op\n");
                    opstack[sp] = (buf[ip] & 0xFF);
                    validstack[sp] = LEFTVALID | OPVALID;
                    //   is the next char also an op?  If so, gobble it up?
                    switch ((opstack[sp] << 8) | buf[ip + 1])
                    {
                    case (('<' << 8) + '='): /* [i_a] */
                    case (('>' << 8) + '='):
                    case (('!' << 8) + '='):
                        if (internal_trace)
                            fprintf(stderr, "two-char operator\n");
                        opstack[sp] = ((opstack[sp] << 8) | buf[ip + 1]);
                        ip++;
                    }
                }
                break;

            case ')':
                //   close paren pops the stack, and returns the left arg
                //   to "whereever", which might be leftarg stack, or rightarg
                if (internal_trace)
                    fprintf(stderr, "close parenthesis, pop stack down\n");
                sp--;
                if (validstack[sp] == (LEFTVALID | OPVALID))
                {
                    rightarg = leftarg[sp + 1];
                    validstack[sp] = LEFTVALID | OPVALID | RIGHTVALID;
                }
                else
                {
                    leftarg[sp] = leftarg[sp + 1];
                    validstack[sp] = LEFTVALID;
                }
                break;

            case ' ':
                break;

            default:
                errstat = nonfatalerror("Math needs an operator in: ",
                    buf);
                if (retstat)
                    *retstat = 0;
                return 0;
            }
            break;

        case (LEFTVALID | OPVALID):
            //  left arg and op are both valid; right now we can have
            //   an enhanced operator (next char is also an op)
            if (internal_trace)
                fprintf(stderr, "left + opvalid\n");
            switch (buf[ip])
            {
            case '(':
                {
                    if (internal_trace)
                    {
                        fprintf(stderr,
                            "Open Paren - start new math stack level\n");
                    }
                    sp++;
                    leftarg[sp] = 0.0;
                    rightarg = 0.0;
                    opstack[sp] = 0;
                    validstack[sp] = 0;
                }
                break;

                //      deal with a possible rightarg strtod situation
            case '-':
            case '+':
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
            case '.':
            case ',':
                {
                    rightarg = strtod(&buf[ip], &frejected);
                    if (internal_trace)
                        fprintf(stderr, " Got right arg %e\n", rightarg);
                    ip = (int)(frejected - buf) - 1;
                    validstack[sp] = validstack[sp] | RIGHTVALID;
                }

            case ' ':
                break;

            default:
                errstat = nonfatalerror("Math is missing a number in: ",
                    buf);
                if (retstat)
                    *retstat = 0;
                return 0;
            }
        }

        //////////////////////////////////////////////////
        //
        //   Now if we have a left-op-right situation, and can
        //    execute the operator right here and now.
        //
        while (validstack[sp] == (LEFTVALID | OPVALID | RIGHTVALID))
        {
            if (internal_trace)
                fprintf(stderr, "Executing %c operator\n", (short)opstack[sp]);
            switch (opstack[sp])
            {
                //    Math operators
            case '+':
                leftarg[sp] += rightarg;
                break;

            case '-':
                leftarg[sp] -= rightarg;
                break;

            case '*':
                leftarg[sp] *= rightarg;
                break;

            case '/':
                leftarg[sp] /= rightarg;
                break;

            case '%':
#ifdef CRM_SUPPORT_FMOD
                leftarg[sp] = fmod(leftarg[sp], rightarg);
#else
                leftarg[sp] = (int64_t)leftarg[sp] % (int64_t)rightarg;
#endif
                break;

            case '^':
                //    since we don't do complex numbers (yet) handle as NaN
                if (leftarg[sp] < 0.0
                    /* use FLT_EPSILON to compensate for added inaccuracy due to previous calculations */
                    && ((int64_t)(leftarg[sp])) >= leftarg[sp] - FLT_EPSILON
                    && ((int64_t)(leftarg[sp])) <= leftarg[sp] + FLT_EPSILON)
                {
                    leftarg[sp] /= 0.0;
                }
                else
                {
                    leftarg[sp] = pow(leftarg[sp], rightarg);
                }
                if (internal_trace)
                    fprintf(stderr, "exp out: %f\n", leftarg[sp]);
                break;

            case 'v': //   Logarithm  BASE v ARG
                //      Negative bases on logarithms?  Not for us!  force NaN
                if (leftarg[sp] <= 0.0)
                {
                    leftarg[sp] /= 0.0;
                }
                else
                {
                    leftarg[sp] = log(rightarg) / log(leftarg[sp]);
                }
                break;

                //      Relational operators
            case '<':
                if (leftarg[sp] < rightarg)
                {
                    leftarg[sp] = 1.0;
                    state = 0;
                }
                else
                {
                    leftarg[sp] = 0;
                    state = 1;
                }
                break;

            case '>':
                if (leftarg[sp] > rightarg)
                {
                    leftarg[sp] = 1;
                    state = 0;
                }
                else
                {
                    leftarg[sp] = 0;
                    state = 1;
                }
                break;

            case '=':
                if (leftarg[sp] == rightarg)
                {
                    leftarg[sp] = 1;
                    state = 0;
                }
                else
                {
                    leftarg[sp] = 0;
                    state = 1;
                }
                break;

            case (('<' << 8) + '='):
                if (leftarg[sp] <= rightarg)
                {
                    leftarg[sp] = 1;
                    state = 0;
                }
                else
                {
                    leftarg[sp] = 0;
                    state = 1;
                }
                break;

            case (('>' << 8) + '='):
                if (leftarg[sp] >= rightarg)
                {
                    leftarg[sp] = 1;
                    state = 0;
                }
                else
                {
                    leftarg[sp] = 0;
                    state = 1;
                }
                break;

            case (('!' << 8) + '='):
                if (leftarg[sp] != rightarg)
                {
                    leftarg[sp] = 1;
                    state = 0;
                }
                else
                {
                    leftarg[sp] = 0;
                    state = 1;
                }
                break;

                //           Formatting operators
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'x':
            case 'X':
                {
                    char tempstring[2048];

                    if (internal_trace)
                    {
                        fprintf(stderr, "Formatting operator '%c'\n",
                            (short)opstack[sp]);
                    }
                    // char tempstring [2048];
                    //     Do we have a float or an int format?
                    if (opstack[sp] == 'x' || opstack[sp] == 'X')
                    {
                        if (((int)rightarg) <= rightarg + FLT_EPSILON
                            && ((int)rightarg) >= rightarg - FLT_EPSILON)
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%%.0gll%c",
                                rightarg, (short)opstack[sp]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                        else
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%0%.0gll%c",
                                rightarg, (short)opstack[sp]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                    }
                    else
                    {
                        if (((int)rightarg) <= rightarg + FLT_EPSILON
                            && ((int)rightarg) >= rightarg - FLT_EPSILON)
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%%.0g.0%c",
                                rightarg, (short)opstack[sp]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                        else
                        {
                            snprintf(outformat, WIDTHOF(outformat), "%%%g%c",
                                rightarg, (short)opstack[sp]);
                            outformat[WIDTHOF(outformat) - 1] = 0;
                        }
                    }
                    if (internal_trace)
                        fprintf(stderr, "Format string -->%s<-- \n", outformat);

                    //      A little more funny business needed for
                    //      hexadecimal print out, because X format
                    //      can't take IEEE floating point as inputs.

                    if (opstack[sp] != 'x'
                        && opstack[sp] != 'X')
                    {
                        if (internal_trace)
                            fprintf(stderr, "Normal convert ");
                        snprintf(tempstring, WIDTHOF(tempstring), outformat, leftarg[sp]);
                        tempstring[WIDTHOF(tempstring) - 1] = 0;
                        leftarg[sp] = strtod(tempstring, NULL);
                        validstack[sp] = LEFTVALID;
                    }
                    else
                    {
                        //    Note that we actually don't use the
                        //    results of octal conversion; the only
                        //    effect is to set the final format
                        //    string.
                        int64_t equiv;
                        if (internal_trace)
                            fprintf(stderr, "Oct/Hex Convert ");
                        equiv = (int64_t)leftarg[sp];
                        if (internal_trace)
                            fprintf(stderr, "equiv -->%10lld<-- \n", (long long int)equiv);
                        snprintf(tempstring, WIDTHOF(tempstring), outformat, equiv);
                        tempstring[WIDTHOF(tempstring) - 1] = 0;
                    }
                }
                break;

            default:
                errstat = nonfatalerror("Math operator makes no sense in: ",
                    buf);
                if (retstat)
                    *retstat = 0;
                return 0;

                break;
            }
            validstack[sp] = LEFTVALID;
        }

        //   Check to see that the stack is still valid.
        if (sp < 0)
        {
            errstat = nonfatalerror("Too many close parenthesis in this math: ",
                buf);
            if (retstat)
                *retstat = 0;
            return 0;
        }
    }
    //      We made it all the way through.  Now return the math formatter result
    if (internal_trace)
        fprintf(stderr, "Returning at sp= %d and value %f\n", sp, leftarg[sp]);
    if (retstat)
        *retstat = state;

    //      Check that we made it all the way down the stack
    if (sp != 0)
    {
        errstat = nonfatalerror("Not enough close parenthesis in this math: ",
            buf);
        if (retstat)
            *retstat = 0;
        return 0;
    }

    //    All's good, return with a value.
    {
        int return_length;
        return_length = math_formatter(leftarg[sp], outformat, buf, maxlen);
        CRM_ASSERT(return_length >= 0);
        CRM_ASSERT(return_length < maxlen);
        return return_length;
    }
}

