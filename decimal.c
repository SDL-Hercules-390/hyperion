/* DECIMAL.C    (C) Copyright Roger Bowler, 1991-2012                */
/*              (C) and others 2013-2022                             */
/*              ESA/390 Packed Decimal Routines                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains packed decimal subroutines for ESA/390.      */
/*                                                                   */
/* Acknowledgements:                                                 */
/*      The lowest level string-math functions are modelled on       */
/*      algorithms in D.E.Knuth's 'The Art of Computer Programming   */
/*      Vol.2', and on C.E.Burton's algorithms in DDJ #89.           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Complete rework for reworked instruction decode/execution code    */
/*                                               Jan Jaeger 01/07/00 */
/* Add trialrun to ED and EDMK                   Jan Jaeger 19/07/00 */
/* Fix random MP bug - Mario Bezzi                                   */
/* Clear DXC on data exception - Peter Kuschnerus                V209*/
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */
/* TP instruction - Roger Bowler                            08/02/01 */
/* packed_to_binary subroutine - Roger Bowler               29/06/03 */
/* binary_to_packed subroutine - Roger Bowler              02Jul2003 */
/* PER 1 GRA - Fish                                        31Jan2022 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DECIMAL_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if !defined(_DECIMAL_C)

#define _DECIMAL_C

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define MAX_DECIMAL_LENGTH      16
#define MAX_DECIMAL_DIGITS      (((MAX_DECIMAL_LENGTH)*2)-1)

/*-------------------------------------------------------------------*/
/* Convert packed decimal number to binary                           */
/*                                                                   */
/* This subroutine is called by the CVB/CVBY/CVBG instructions.      */
/* It performs the conversion of a 8-byte or 16-byte packed          */
/* decimal number into a 64-bit SIGNED binary result.                */
/* This routine is not architecture-dependent; all of its operands   */
/* are contained in work areas passed by the architecture-dependent  */
/* instruction routines which handle all main-storage accesses and   */
/* possible program checks.                                          */
/*                                                                   */
/* Input:                                                            */
/*      dec     An 8 or 16 byte area containing a copy of the        */
/*              packed decimal storage operand.                      */
/*      len     Length-1 (in bytes) of the packed decimal input      */
/*              (7 for CVB/CVBY or 15 for CVBG).                     */
/* Output:                                                           */
/*      result  Points to an U64 field which will receive the        */
/*              result as a 64-bit SIGNED binary number.             */
/*      ovf     Points to an int field which will be set to 1 if     */
/*              the result overflows 63 bits plus sign, else 0.      */
/*              If overflow occurs, the result field will contain    */
/*              the rightmost 64 bits of the result.                 */
/*      dxf     Points to an int field which will be set to 1 if     */
/*              invalid digits or sign were detected, else 0.        */
/*              The result field is not set if the dxf is set to 1.  */
/*-------------------------------------------------------------------*/
void packed_to_binary (BYTE *dec, int len, U64 *result,
                        int *ovf, int *dxf)
{
U64     dreg;                           /* 64-bit result accumulator */
int     i;                              /* Loop counter              */
int     h, d=0;                         /* Decimal digits            */
U64     inter_u64max_div10;
int     inter_u64max_rem10;
U64     pos_u64max = 9223372036854775807ULL;  // (LLONG_MAX)
U64     neg_u64max = 9223372036854775808ULL;  // (LLONG_MIN)

    /* Initialize result flags */
    *ovf = 0;
    *dxf = 0;

    /* Initialize 64-bit result accumulator */
    dreg = 0;

    /* Initialize max unsigned intermediate value for overflow check */
    if ((dec[len] & 0x0F) == 0x0B ||
        (dec[len] & 0x0F) == 0x0D)
    {
        inter_u64max_div10 =       (neg_u64max / 10);
        inter_u64max_rem10 = (int) (neg_u64max % 10);
    }
    else if ((dec[len] & 0x0F) < 0x0A)
    {
        *dxf = 1;
        return;
    }
    else
    {
        inter_u64max_div10 =       (pos_u64max / 10);
        inter_u64max_rem10 = (int) (pos_u64max % 10);
    }

    /* Convert decimal digits to binary */
    for (i = 0; i <= len; i++)
    {
        /* Isolate high-order and low-order digits */
        h = (dec[i] & 0xF0) >> 4;
        d = dec[i] & 0x0F;

        /* Data exception if high-order digit is invalid */
        if (h > 9)
        {
            *dxf = 1;
            return;
        }

        /* Check for overflow before accumulating */
        if ( dreg >  inter_u64max_div10 ||
            (dreg == inter_u64max_div10 &&
                h >  inter_u64max_rem10)) // (NOTE: 'h', not 'd')
        {
            *ovf = 1;
        }

        /* Accumulate high-order digit into result */
        dreg *= 10;
        dreg += h;

        /* Check for valid low-order digit or sign */
        if (i < len)
        {
            /* Data exception if low-order digit is invalid */
            if (d > 9)
            {
                *dxf = 1;
                return;
            }

            /* Check for overflow before accumulating */
            if ( dreg >  inter_u64max_div10 ||
                (dreg == inter_u64max_div10 &&
                    d >  inter_u64max_rem10)) // (NOTE: 'd', not 'h')
            {
                *ovf = 1;
            }

            /* Accumulate low-order digit into result */
            dreg *= 10;
            dreg += d;
        }
        else
        {
            /* Data exception if sign is invalid */
            if (d < 10)
            {
                *dxf = 1;
                return;
            }
        }

    } /* end for(i) */

    /* Result is negative if sign is X'B' or X'D' */
    if (d == 0x0B || d == 0x0D)
    {
        /* Check for UNDERflow (less than min negative) */
        if ( dreg > neg_u64max )
            *ovf = 1;
        else
        dreg = -((S64)dreg);
    }
    else
    {
        /* Check for OVERflow (greater than max positive) */
        if ( dreg > pos_u64max )
            *ovf = 1;
    }

    /* Set result field and return */
    *result = dreg;

} /* end function packed_to_binary */

/*-------------------------------------------------------------------*/
/* Convert binary number to packed decimal                           */
/*                                                                   */
/* This subroutine is called by the CVD/CVDY/CVDG instructions.      */
/* It performs the conversion of a 64-bit signed binary number       */
/* to a 16-byte packed decimal result. Since the maximum 63 bit      */
/* number is less than 31 decimal digits, overflow cannot occur.     */
/* Similarly, the maximum 31 bit number is less than 15 decimal      */
/* digits, therefore CVD/CVDY can safely use the rightmost eight     */
/* bytes of the packed decimal result without risk of overflow.      */
/*                                                                   */
/* This routine is not architecture-dependent; all of its operands   */
/* are contained in work areas passed by the architecture-dependent  */
/* instruction routines which handle all main-storage accesses and   */
/* possible program checks.                                          */
/*                                                                   */
/* Input:                                                            */
/*      bin     Binary number (63 bits plus sign)                    */
/* Output:                                                           */
/*      result  Points to a 16-byte field which will receive the     */
/*              result as a packed decimal number (31 digits + sign) */
/*-------------------------------------------------------------------*/
void binary_to_packed (S64 bin, BYTE *result)
{
int     i;                              /* Array subscript           */
int     d;                              /* Decimal digit or sign     */

    /* Special case when input is maximum negative value */
    if ((U64)bin == 0x8000000000000000ULL)
    {
        memcpy (result,
                "\x00\x00\x00\x00\x00\x00\x92\x23"
                "\x37\x20\x36\x85\x47\x75\x80\x8D",
                16);
    }
    else
    {
        /* Load absolute value and generate sign */
        if ((U64)bin < 0x8000000000000000ULL)
        {
            /* Value is positive */
            d = 0x0C;
        }
        else
        {
            /* Value is negative */
            bin = -bin;
            d = 0x0D;
        }

        /* Store sign and decimal digits from right to left */
        memset (result, 0, 16);
        for (i = 16 - 1; d != 0 || bin != 0; i--)
        {
            result[i] = d;
            d = bin % 10;
            bin /= 10;
            result[i] |= (d << 4);
            d = bin % 10;
            bin /= 10;
        }
    }

} /* end function(binary_to_packed) */

/*-------------------------------------------------------------------*/
/* Add two decimal byte strings as unsigned decimal numbers          */
/*                                                                   */
/* Input:                                                            */
/*      dec1    A 31-byte area containing the decimal digits of      */
/*              the first operand.  Each byte contains one decimal   */
/*              digit in the low-order 4 bits of the byte.           */
/*      dec2    A 31-byte area containing the decimal digits of      */
/*              the second operand.  Each byte contains one decimal  */
/*              digit in the low-order 4 bits of the byte.           */
/* Output:                                                           */
/*      result  Points to a 31-byte area to contain the result       */
/*              digits. One decimal digit is placed in the low-order */
/*              4 bits of each byte.                                 */
/*      count   Points to an integer to receive the number of        */
/*              digits in the result excluding leading zeroes.       */
/*              This field is set to zero if the result is all zero, */
/*              or to MAX_DECIMAL_DIGITS+1 if overflow occurred.     */
/*-------------------------------------------------------------------*/
void add_decimal (BYTE *dec1, BYTE *dec2,
                        BYTE *result, int *count)
{
int     d;                              /* Decimal digit             */
int     i;                              /* Array subscript           */
int     n = 0;                          /* Significant digit counter */
int     carry = 0;                      /* Carry indicator           */

    /* Add digits from right to left */
    for (i = MAX_DECIMAL_DIGITS - 1; i >= 0; i--)
    {
        /* Add digits from first and second operands */
        d = dec1[i] + dec2[i] + carry;

        /* Check for carry into next digit */
        if (d > 9) {
            d -= 10;
            carry = 1;
        } else {
            carry = 0;
        }

        /* Check for significant digit */
        if (d != 0)
            n = MAX_DECIMAL_DIGITS - i;

        /* Store digit in result */
        result[i] = d;

    } /* end for */

    /* Check for carry out of leftmost digit */
    if (carry)
        n = MAX_DECIMAL_DIGITS + 1;

    /* Return significant digit counter */
    *count = n;

} /* end function add_decimal */

/*-------------------------------------------------------------------*/
/* Subtract two decimal byte strings as unsigned decimal numbers     */
/*                                                                   */
/* Input:                                                            */
/*      dec1    A 31-byte area containing the decimal digits of      */
/*              the first operand.  Each byte contains one decimal   */
/*              digit in the low-order 4 bits of the byte.           */
/*      dec2    A 31-byte area containing the decimal digits of      */
/*              the second operand.  Each byte contains one decimal  */
/*              digit in the low-order 4 bits of the byte.           */
/* Output:                                                           */
/*      result  Points to a 31-byte area to contain the result       */
/*              digits. One decimal digit is placed in the low-order */
/*              4 bits of each byte.                                 */
/*      count   Points to an integer to receive the number of        */
/*              digits in the result excluding leading zeroes.       */
/*              This field is set to zero if the result is all zero. */
/*      sign    -1 if the result is negative (operand2 > operand1)   */
/*              +1 if the result is positive (operand2 <= operand1)  */
/*-------------------------------------------------------------------*/
void subtract_decimal (BYTE *dec1, BYTE *dec2,
                        BYTE *result, int *count, int *sign)
{
int     d;                              /* Decimal digit             */
int     i;                              /* Array subscript           */
int     n = 0;                          /* Significant digit counter */
int     borrow = 0;                     /* Borrow indicator          */
int     rc;                             /* Return code               */
BYTE   *higher;                         /* -> Higher value operand   */
BYTE   *lower;                          /* -> Lower value operand    */

    /* Compare digits to find which operand has higher numeric value */
    rc = memcmp (dec1, dec2, MAX_DECIMAL_DIGITS);

    /* Return positive zero result if both operands are equal */
    if (rc == 0) {
        memset (result, 0, MAX_DECIMAL_DIGITS);
        *count = 0;
        *sign = +1;
        return;
    }

    /* Point to higher and lower value operands and set sign */
    if (rc > 0) {
        higher = dec1;
        lower = dec2;
       *sign = +1;
    } else {
        lower = dec1;
        higher = dec2;
       *sign = -1;
    }

    /* Subtract digits from right to left */
    for (i = MAX_DECIMAL_DIGITS - 1; i >= 0; i--)
    {
        /* Subtract lower operand digit from higher operand digit */
        d = higher[i] - lower[i] - borrow;

        /* Check for borrow from next digit */
        if (d < 0) {
            d += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }

        /* Check for significant digit */
        if (d != 0)
            n = MAX_DECIMAL_DIGITS - i;

        /* Store digit in result */
        result[i] = d;

    } /* end for */

    /* Return significant digit counter */
    *count = n;

} /* end function subtract_decimal */

/*-------------------------------------------------------------------*/
/* Divide two decimal byte strings as unsigned decimal numbers       */
/*                                                                   */
/* Input:                                                            */
/*      dec1    A 31-byte area containing the decimal digits of      */
/*              the dividend.  Each byte contains one decimal        */
/*              digit in the low-order 4 bits of the byte.           */
/*      count1  The number of significant digits in the dividend.    */
/*      dec2    A 31-byte area containing the decimal digits of      */
/*              the divisor.  Each byte contains one decimal         */
/*              digit in the low-order 4 bits of the byte.           */
/*      count2  The number of significant digits in the divisor.     */
/* Output:                                                           */
/*      quot    Points to a 31-byte area to contain the quotient     */
/*              digits. One decimal digit is placed in the low-order */
/*              4 bits of each byte.                                 */
/*      rem     Points to a 31-byte area to contain the remainder    */
/*              digits. One decimal digit is placed in the low-order */
/*              4 bits of each byte.                                 */
/* Restrictions:                                                     */
/*      It is assumed that the caller has already verified that      */
/*      divide overflow cannot occur, that the divisor is not zero,  */
/*      and that the dividend has at least one high order zero.      */
/*-------------------------------------------------------------------*/
void divide_decimal (BYTE *dec1, int count1, BYTE *dec2,
                        int count2, BYTE *quot, BYTE *rem)
{
BYTE   *num1;                           /* -> dividend digits        */
BYTE   *num2;                           /* -> divisor digits         */
int     div, flag, scale;               /* Work areas for algorithm  */
int     index, index1, index2;          /* Work areas for algorithm  */
int     indexq, indexr, temp1, temp2;   /* Work areas for algorithm  */
int     temp3, temp4, temp5, qtest;     /* Work areas for algorithm  */

    /* Clear the result fields */
    memset (quot, 0, MAX_DECIMAL_DIGITS);
    memset (rem, 0, MAX_DECIMAL_DIGITS);

    /* If dividend is zero then return zero quotient and remainder */
    if (count1 == 0)
        return;

    /* If dividend is less than divisor then return zero quotient
       and set remainder equal to dividend */
    if (memcmp (dec1, dec2, MAX_DECIMAL_DIGITS) < 0)
    {
        memcpy (rem, dec1, MAX_DECIMAL_DIGITS);
        return;
    }

    /* Adjust dividend digit count to give one leading zero */
    count1++;

    /* Point to significant digits of dividend with leading zero */
    num1 = dec1 + MAX_DECIMAL_DIGITS - count1;

    /* Point to significant digits of divisor */
    num2 = dec2 + MAX_DECIMAL_DIGITS - count2;

    scale = 10 / (num2[0] + 1);
    if (scale > 1)
    {
        for (index1 = count1-1, flag = 0; index1 >= 0; index1--)
        {
            div = flag + scale*num1[index1];
            num1[index1] = div % 10;
            flag = div / 10;
        } /* end for(index1) */

        for (index2 = count2-1, flag = 0; index2 >= 0; index2--)
        {
            div = flag + scale*num2[index2];
            num2[index2] = div % 10;
            flag = div / 10;
        } /* end for(index2) */

    } /* end if(scale>1) */

    for (index1 = 0; index1 < count1-count2; index1++)
    {
        if (num2[0] == num1[index1])
            qtest = 9;
        else
        {
            temp2 = (index1+1 < count1) ? num1[index1+1] : 0;
            qtest = (10*num1[index1] + temp2) / num2[0];
        }
        temp2 = num1[index1];
        temp4 = num2[0];
        temp1 = (count2 >= 2) ? num2[1] : 0;
        if (index1+1 < count1)
        {
            temp3 = num1[index1+1];
            temp5 = (index1+2 < count1) ? num1[index1+2] : 0;
        }
        else
        {
            temp3 = 0;
            temp5 = 0;
        }
        while (qtest*temp1 > (10*(10*temp2 + temp3
                            - qtest*temp4) + temp5))
            --qtest;

        for (index = index1+count2, index2 = count2-1, flag = 0;
                index >= index1; index--, index2--)
        {
            if (index2 >= 0)
                flag -= qtest*num2[index2];
            div = flag + num1[index];
            if (div < 0)
            {
                flag = div / 10;
                div %= 10;
                if (div < 0)
                {
                    div += 10;
                    --flag;
                }
            }
            else
                flag = 0;
            num1[index] = div;
        } /* end for(index) */

        indexq = MAX_DECIMAL_DIGITS - (count1-count2) + index1;
        if (flag != 0)
        {
            quot[indexq] = qtest - 1;
            for (index = index1+count2, index2 = count2-1, flag = 0;
                    index >= index1; index--, index2--)
            {
                if (index2 >= 0)
                    flag += num2[index2];
                div = flag + num1[index];
                if (div > 9)
                {
                    div -= 10;
                    flag = 1;
                }
                else
                    flag = 0;
                num1[index] = div;
            } /* end for(index) */
        }
        else
            quot[indexq] = qtest;
    } /* end for(index1) */

    for (index1 = count1-count2,
            indexr = MAX_DECIMAL_DIGITS-count2, flag = 0;
            index1 < count1; index1++, indexr++)
    {
        div = num1[index1] + 10*flag;
        rem[indexr] = div / scale;
        flag = div % scale;
    } /* end for(index1) */

    for (index2 = 0, flag = 0; index2 < count2; index2++)
    {
        div = num2[index2] + 10*flag;
        num2[index2] = div / scale;
        flag = div % scale;
    } /* end for(index2) */

} /* end function divide_decimal */

#endif /*!defined(_DECIMAL_C)*/

/*-------------------------------------------------------------------*/
/* Load a packed decimal storage operand into a decimal byte string  */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of packed decimal storage operand    */
/*      len     Length minus one of storage operand (range 0-15)     */
/*      arn     Access register number associated with operand       */
/*      regs    CPU register context                                 */
/* Output:                                                           */
/*      result  Points to a 31-byte area into which the decimal      */
/*              digits are loaded.  One decimal digit is loaded      */
/*              into the low-order 4 bits of each byte, and the      */
/*              result is padded to the left with high-order zeroes  */
/*              if the storage operand contains less than 31 digits. */
/*      count   Points to an integer to receive the number of        */
/*              digits in the result excluding leading zeroes.       */
/*              This field is set to zero if the result is all zero. */
/*      sign    Points to an integer which will be set to -1 if a    */
/*              negative sign was loaded from the operand, or +1 if  */
/*              a positive sign was loaded from the operand.         */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, or if the operand causes a data exception         */
/*      because of invalid decimal digits or sign.                   */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(load_decimal) (VADR addr, int len, int arn, REGS *regs,
                        BYTE *result, int *count, int *sign)
{
int     h;                              /* Hexadecimal digit         */
int     i, j;                           /* Array subscripts          */
int     n;                              /* Significant digit counter */
BYTE    pack[MAX_DECIMAL_LENGTH];       /* Packed decimal work area  */

    /* Fetch the packed decimal operand into work area */
    memset( pack, 0, sizeof(pack) );
    ARCH_DEP(vfetchc) (pack+sizeof(pack)-len-1, len, addr, arn, regs);

    /* Unpack digits into result */
    for (i=0, j=0, n=0; i < MAX_DECIMAL_DIGITS; i++)
    {
        /* Load source digit */
        if (i & 1)
            h = pack[j++] & 0x0F;
        else
            h = pack[j] >> 4;

        /* Check for valid numeric */
        if (h > 9)
        {
            regs->dxc = DXC_DECIMAL;
            ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
            return;
        }

        /* Count significant digits */
        if (n > 0 || h != 0)
            n++;

        /* Store decimal digit in result */
        result[i] = h;

    } /* end for */

    /* Check for valid sign */
    h = pack[MAX_DECIMAL_LENGTH-1] & 0x0F;
    if (h < 0x0A)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
        return;
    }

    /* Set number of significant digits */
    *count = n;

    /* Set sign of operand */
    *sign = (h == 0x0B || h == 0x0D) ? -1 : 1;

} /* end function ARCH_DEP(load_decimal) */

/*-------------------------------------------------------------------*/
/* Store decimal byte string into packed decimal storage operand     */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of packed decimal storage operand    */
/*      len     Length minus one of storage operand (range 0-15)     */
/*      arn     Access register number associated with operand       */
/*      regs    CPU register context                                 */
/*      dec     A 31-byte area containing the decimal digits to be   */
/*              stored.  Each byte contains one decimal digit in     */
/*              the low-order 4 bits of the byte.                    */
/*      sign    -1 if a negative sign is to be stored, or +1 if a    */
/*              positive sign is to be stored.                       */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or protection exception.  */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(store_decimal) (VADR addr, int len, int arn, REGS *regs,
                        BYTE *dec, int sign)
{
int     i, j;                           /* Array subscripts          */
BYTE    pack[MAX_DECIMAL_LENGTH];       /* Packed decimal work area  */

    /* if operand crosses page, make sure both pages are accessible */
    if((addr & PAGEFRAME_PAGEMASK) !=
        ((addr + len) & PAGEFRAME_PAGEMASK))
        ARCH_DEP(validate_operand) (addr, arn, len, ACCTYPE_WRITE_SKP, regs);

    /* Pack digits into packed decimal work area */
    for (i=0, j=0; i < MAX_DECIMAL_DIGITS; i++)
    {
        if (i & 1)
            pack[j++] |= dec[i];
        else
            pack[j] = dec[i] << 4;
    } /* end for */

    /* Pack the sign into low-order digit of work area */
    pack[MAX_DECIMAL_LENGTH-1] |= (sign < 0 ? 0x0D : 0x0C);

    /* Store the result at the operand location */
    ARCH_DEP(vstorec) (pack+sizeof(pack)-len-1, len, addr, arn, regs);

} /* end function ARCH_DEP(store_decimal) */


/*-------------------------------------------------------------------*/
/* FA   AP    - Add Decimal                                   [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(add_decimal)
{
int     l1, l2;                         /* Length values             */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
BYTE    dec1[MAX_DECIMAL_DIGITS];       /* Work area for operand 1   */
BYTE    dec2[MAX_DECIMAL_DIGITS];       /* Work area for operand 2   */
BYTE    dec3[MAX_DECIMAL_DIGITS];       /* Work area for result      */
int     count1, count2, count3;         /* Significant digit counters*/
int     sign1, sign2, sign3;            /* Sign of operands & result */

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Load operands into work areas */
    ARCH_DEP(load_decimal) (effective_addr1, l1, b1, regs, dec1, &count1, &sign1);
    ARCH_DEP(load_decimal) (effective_addr2, l2, b2, regs, dec2, &count2, &sign2);

    /* Add or subtract operand values */
    if (count2 == 0)
    {
        /* If second operand is zero then result is first operand */
        memcpy (dec3, dec1, MAX_DECIMAL_DIGITS);
        count3 = count1;
        sign3 = sign1;
    }
    else if (count1 == 0)
    {
        /* If first operand is zero then result is second operand */
        memcpy (dec3, dec2, MAX_DECIMAL_DIGITS);
        count3 = count2;
        sign3 = sign2;
    }
    else if (sign1 == sign2)
    {
        /* If signs are equal then add operands */
        add_decimal (dec1, dec2, dec3, &count3);
        sign3 = sign1;
    }
    else
    {
        /* If signs are opposite then subtract operands */
        subtract_decimal (dec1, dec2, dec3, &count3, &sign3);
        if (sign1 < 0) sign3 = -sign3;
    }

    /* Set condition code */
    cc = (count3 == 0) ? 0 : (sign3 < 1) ? 1 : 2;

    /* Overflow if result exceeds first operand length */
    if (count3 > (l1+1) * 2 - 1)
        cc = 3;

    /* Set positive sign if result is zero */
    if (count3 == 0)
        sign3 = 1;

    /* Store result into first operand location */
    ARCH_DEP(store_decimal) (effective_addr1, l1, b1, regs, dec3, sign3);

    /* Set condition code */
    regs->psw.cc = cc;

    /* Program check if overflow and PSW program mask is set */
    if (cc == 3 && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_decimal) */


/*-------------------------------------------------------------------*/
/* F9   CP    - Compare Decimal                               [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_decimal)
{
int     l1, l2;                         /* Length values             */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
BYTE    dec1[MAX_DECIMAL_DIGITS];       /* Work area for operand 1   */
BYTE    dec2[MAX_DECIMAL_DIGITS];       /* Work area for operand 2   */
int     count1, count2;                 /* Significant digit counters*/
int     sign1, sign2;                   /* Sign of each operand      */
int     rc;                             /* Return code               */

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Load operands into work areas */
    ARCH_DEP(load_decimal) (effective_addr1, l1, b1, regs, dec1, &count1, &sign1);
    ARCH_DEP(load_decimal) (effective_addr2, l2, b2, regs, dec2, &count2, &sign2);

    /* Result is equal if both operands are zero */
    if (count1 == 0 && count2 == 0)
    {
        regs->psw.cc = 0;
        return;
    }

    /* Result is low if operand 1 is -ve and operand 2 is +ve */
    if (sign1 < 0 && sign2 > 0)
    {
        regs->psw.cc = 1;
        return;
    }

    /* Result is high if operand 1 is +ve and operand 2 is -ve */
    if (sign1 > 0 && sign2 < 0)
    {
        regs->psw.cc = 2;
        return;
    }

    /* If signs are equal then compare the digits */
    rc = memcmp (dec1, dec2, MAX_DECIMAL_DIGITS);

    /* Return low or high (depending on sign) if digits are unequal */
    if (rc < 0)
        regs->psw.cc = (sign1 > 0) ? 1 : 2;
    else
        if (rc > 0)
            regs->psw.cc = (sign1 > 0) ? 2 : 1;
        else
            regs->psw.cc = 0;

} /* end DEF_INST(compare_decimal) */


/*-------------------------------------------------------------------*/
/* FD   DP    - Divide Decimal                                [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_decimal)
{
int     l1, l2;                         /* Length values             */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
BYTE    dec1[MAX_DECIMAL_DIGITS];       /* Operand 1 (dividend)      */
BYTE    dec2[MAX_DECIMAL_DIGITS];       /* Operand 2 (divisor)       */
BYTE    quot[MAX_DECIMAL_DIGITS];       /* Quotient                  */
BYTE    rem[MAX_DECIMAL_DIGITS];        /* Remainder                 */
int     count1, count2;                 /* Significant digit counters*/
int     sign1, sign2;                   /* Sign of operands          */
int     signq, signr;                   /* Sign of quotient/remainder*/

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Program check if the second operand length exceeds 15 digits
       or is equal to or greater than the first operand length */
    if (l2 > 7 || l2 >= l1)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Load operands into work areas */
    ARCH_DEP(load_decimal) (effective_addr1, l1, b1, regs, dec1, &count1, &sign1);
    ARCH_DEP(load_decimal) (effective_addr2, l2, b2, regs, dec2, &count2, &sign2);

    /* Program check if second operand value is zero */
    if (count2 == 0)
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* Perform trial comparison to determine potential overflow.
       The leftmost digit of the divisor is aligned one digit to
       the right of the leftmost dividend digit.  When the divisor,
       so aligned, is less than or equal to the dividend, ignoring
       signs, a divide exception is indicated.  As a result of this
       comparison, it is also certain that the leftmost digit of the
       dividend must be zero, and that the divisor cannot be zero */
    if (memcmp(dec2 + (MAX_DECIMAL_DIGITS - l2*2 - 2),
                dec1 + (MAX_DECIMAL_DIGITS - l1*2 - 1),
                l2*2 + 2) <= 0)
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_DIVIDE_EXCEPTION);

    /* Perform decimal division */
    divide_decimal (dec1, count1, dec2, count2, quot, rem);

    /* Quotient is positive if operand signs are equal, and negative
       if operand signs are opposite, even if quotient is zero */
    signq = (sign1 == sign2) ? 1 : -1;

    /* Remainder sign is same as dividend, even if remainder is zero */
    signr = sign1;

    /* Store remainder into entire first operand location.  The entire
       field will be filled in order to check for store protection.
       Subsequently the quotient will be stored in the leftmost bytes
       of the first operand location, overwriting high order zeroes */
    ARCH_DEP(store_decimal) (effective_addr1, l1, b1, regs, rem, signr);

    /* Store quotient in leftmost bytes of first operand location */
    ARCH_DEP(store_decimal) (effective_addr1, l1-l2-1, b1, regs, quot, signq);

} /* end DEF_INST(divide_decimal) */


/*-------------------------------------------------------------------*/
/* DE   ED    - Edit                                          [SS-a] */
/* DF   EDMK  - Edit and Mark                                 [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(edit_x_edit_and_mark)
{
int     l;                              /* Length value              */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2,                /* Effective addresses       */
        addr1,
        addr2;
int     cc = 0;                         /* Condition code            */
int     sig = 0;                        /* Significance indicator    */
int     trial_run;                      /* 1=trial run               */
int     i;                              /* Loop counter              */
int     d;                              /* 1=Use right source digit  */
int     h;                              /* Hexadecimal digit         */
BYTE    sbyte;                          /* Source operand byte       */
BYTE    fbyte;                          /* Fill byte                 */
BYTE    pbyte;                          /* Pattern byte              */
BYTE    rbyte;                          /* Result byte               */

    SS_L(inst, regs, l, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* If addr1 crosses page, make sure both pages are accessible */
    if((effective_addr1 & PAGEFRAME_PAGEMASK) !=
        ((effective_addr1 + l) & PAGEFRAME_PAGEMASK))
        ARCH_DEP(validate_operand) (effective_addr1, b1, l, ACCTYPE_WRITE_SKP, regs);

    /* If addr2 might cross page, do a trial run to catch possible access rupts */
    if((effective_addr2 & PAGEFRAME_PAGEMASK) !=
        ((effective_addr2 + l) & PAGEFRAME_PAGEMASK))
        trial_run = 1;
    else
        trial_run = 0;

    for(;trial_run >= 0; trial_run--)
    {
        /* Initialize variables */
        addr1 = effective_addr1;
        addr2 = effective_addr2;
        cc = 0;
        sig = 0;
        sbyte = 0;
        fbyte = 0;

        /* Process first operand from left to right */
        for (i = 0, d = 0; i < l+1; i++)
        {
            /* Fetch pattern byte from first operand */
            pbyte = ARCH_DEP(vfetchb) ( addr1, b1, regs );

            /* The first pattern byte is also the fill byte */
            if (i == 0) fbyte = pbyte;

            /* If pattern byte is digit selector (X'20') or
               significance starter (X'21') then fetch next
               hexadecimal digit from the second operand */
            if (pbyte == 0x20 || pbyte == 0x21)
            {
                if (d == 0)
                {
                    /* Fetch source byte and extract left digit */
                    sbyte = ARCH_DEP(vfetchb) ( addr2, b2, regs );
                    h = sbyte >> 4;
                    sbyte &= 0x0F;
                    d = 1;

                    /* Increment second operand address */
                    addr2++;
                    addr2 &= ADDRESS_MAXWRAP(regs);

                    /* Program check if left digit is not numeric */
                    if (h > 9)
                    {
                        regs->dxc = DXC_DECIMAL;
                        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
                    }

                }
                else
                {
                    /* Use right digit of source byte */
                    h = sbyte;
                    d = 0;
                }

                /* For the EDMK instruction only, insert address of
                   result byte into general register 1 if the digit
                   is non-zero and significance indicator was off */
                if (!trial_run && (inst[0] == 0xDF) && h > 0 && sig == 0)
                {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                    if (regs->psw.amode64)
                        regs->GR_G(1) = addr1;
                    else
#endif
                    if ( regs->psw.amode )
                        regs->GR_L(1) = addr1;
                    else
                        regs->GR_LA24(1) = addr1;
                }

                /* Replace the pattern byte by the fill character
                   or by a zoned decimal digit */
                rbyte = (sig == 0 && h == 0) ? fbyte : (0xF0 | h);
                if(!trial_run)
                    ARCH_DEP(vstoreb) ( rbyte, addr1, b1, regs );
                else
                    ARCH_DEP(validate_operand) (addr1, b1, 0, ACCTYPE_WRITE_SKP, regs);

                /* Set condition code 2 if digit is non-zero */
                if (h > 0) cc = 2;

                /* Turn on significance indicator if pattern
                   byte is significance starter or if source
                   digit is non-zero */
                if (pbyte == 0x21 || h > 0)
                    sig = 1;

                /* Examine right digit for sign code */
                if (d == 1 && sbyte > 9)
                {
                    /* Turn off the significance indicator if
                       the right digit is a plus sign code */
                    if (sbyte != 0x0B && sbyte != 0x0D)
                        sig = 0;

                    /* Take next digit from next source byte */
                    d = 0;
                }
            }

            /* If pattern byte is field separator (X'22') then
               replace it by the fill character, turn off the
               significance indicator, and zeroize conditon code  */
            else if (pbyte == 0x22)
            {
                if(!trial_run)
                    ARCH_DEP(vstoreb) ( fbyte, addr1, b1, regs );
                else
                    ARCH_DEP(validate_operand) (addr1, b1, 0, ACCTYPE_WRITE_SKP, regs);
                sig = 0;
                cc = 0;
            }

            /* If pattern byte is a message byte (anything other
               than X'20', X'21', or X'22') then replace it by
               the fill byte if the significance indicator is off */
            else
            {
                if (sig == 0)
                {
                    if (!trial_run)
                        ARCH_DEP(vstoreb) ( fbyte, addr1, b1, regs );
                    else
                        ARCH_DEP(validate_operand) (addr1, b1, 0, ACCTYPE_WRITE_SKP, regs);
                }
                else /* store message byte */
                {
                    if (!trial_run)
                        ARCH_DEP(vstoreb) ( pbyte, addr1, b1, regs );
                    else
                        ARCH_DEP(validate_operand) (addr1, b1, 0, ACCTYPE_WRITE_SKP, regs);
                }
            }

            /* Increment first operand address */
            addr1++;
            addr1 &= ADDRESS_MAXWRAP(regs);

        } /* end for(i) */

    } /* end for(trial_run) */

    /* Replace condition code 2 by condition code 1 if the
       significance indicator is on at the end of editing */
    if (sig && cc == 2) cc = 1;

    /* Set condition code */
    regs->psw.cc = cc;

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    if (inst[0] == 0xDF) // EDMK?
        PER_GRA_CHECK( regs, PER_GRA_MASK( 1 ));
#endif

} /* end DEF_INST(edit_x_edit_and_mark) */


/*-------------------------------------------------------------------*/
/* FC   MP    - Multiply Decimal                              [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_decimal)
{
int     l1, l2;                         /* Length values             */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
BYTE    dec1[MAX_DECIMAL_DIGITS];       /* Work area for operand 1   */
BYTE    dec2[MAX_DECIMAL_DIGITS];       /* Work area for operand 2   */
BYTE    dec3[MAX_DECIMAL_DIGITS];       /* Work area for result      */
int     count1, count2;                 /* Significant digit counters*/
int     sign1, sign2, sign3;            /* Sign of operands & result */
int     d;                              /* Decimal digit             */
int     i1, i2, i3;                     /* Array subscripts          */
int     carry;                          /* Carry indicator           */

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Program check if the second operand length exceeds 15 digits
       or is equal to or greater than the first operand length */
    if (l2 > 7 || l2 >= l1)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Load operands into work areas */
    ARCH_DEP(load_decimal) (effective_addr1, l1, b1, regs, dec1, &count1, &sign1);
    ARCH_DEP(load_decimal) (effective_addr2, l2, b2, regs, dec2, &count2, &sign2);

    /* Program check if the number of bytes in the second operand
       is less than the number of bytes of high-order zeroes in the
       first operand; this ensures that overflow cannot occur */
    if (l2 > l1 - (count1/2 + 1))
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Clear the result field */
    memset( dec3, 0, MAX_DECIMAL_DIGITS );

    /* Perform decimal multiplication */
    for (i2 = MAX_DECIMAL_DIGITS-1; i2 >= 0; i2--)
    {
        if (dec2[i2] != 0)
        {
            for (i1 = MAX_DECIMAL_DIGITS - 1, i3 = i2, carry = 0;
                        i3 >= 0; i1--, i3--)
            {
                d = carry + dec1[i1]*dec2[i2] + dec3[i3];
                dec3[i3] = d % 10;
                carry = d / 10;
            }
        }
    } /* end for(i2) */

    /* Result is positive if operand signs are equal, and negative
       if operand signs are opposite, even if result is zero */
    sign3 = (sign1 == sign2) ? 1 : -1;

    /* Store result into first operand location */
    ARCH_DEP(store_decimal) (effective_addr1, l1, b1, regs, dec3, sign3);

} /* end DEF_INST(multiply_decimal) */


#if defined(_MSVC_) && (_MSC_VER >= VS2010)
#pragma optimize( "", off )
#endif

/*-------------------------------------------------------------------*/
/* F0   SRP   - Shift and Round Decimal                       [SS-c] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_and_round_decimal)
{
int     l1, i3;                         /* Length and rounding       */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
BYTE    dec[MAX_DECIMAL_DIGITS];        /* Work area for operand     */
int     count;                          /* Significant digit counter */
int     sign;                           /* Sign of operand/result    */
int     i, j;                           /* Array subscripts          */
int     d;                              /* Decimal digit             */
int     carry;                          /* Carry indicator           */

    SS(inst, regs, l1, i3, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b1 );
    TXFC_INSTR_CHECK( regs );

    /* Load operand into work area */
    ARCH_DEP(load_decimal) (effective_addr1, l1, b1, regs, dec, &count, &sign);

    /* Program check if rounding digit is invalid */
    if (i3 > 9)
    {
        regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

    /* Isolate low-order six bits of shift count */
    effective_addr2 &= 0x3F;

    /* Shift count 0-31 means shift left, 32-63 means shift right */
    if (effective_addr2 < 32)
    {
        /* Set condition code according to operand sign */
        cc = (count == 0) ? 0 : (sign < 0) ? 1 : 2;

        /* Set cc=3 if non-zero digits will be lost on left shift */
        if (count > 0 && effective_addr2 > (VADR)((l1+1)*2 - 1 - count))
            cc = 3;

        /* Shift operand left */
        for (i=0, j=effective_addr2; i < MAX_DECIMAL_DIGITS; i++, j++)
            dec[i] = (j < MAX_DECIMAL_DIGITS) ? dec[j] : 0;
    }
    else
    {
        /* Calculate number of digits (1-32) to shift right */
        effective_addr2 = 64 - effective_addr2;

        /* Add the rounding digit to the leftmost of the digits
           to be shifted out and propagate the carry to the left */
        carry = (effective_addr2 > MAX_DECIMAL_DIGITS) ? 0 :
                (dec[MAX_DECIMAL_DIGITS - effective_addr2] + i3) / 10;
        count = 0;

        /* Shift operand right */
        for (i=MAX_DECIMAL_DIGITS-1, j=MAX_DECIMAL_DIGITS-1-effective_addr2;
                i >= 0; i--, j--)
        {
            d = (j >= 0) ? dec[j] : 0;
            d += carry;
            carry = d / 10;
            d %= 10;
            dec[i] = d;
            if (d != 0)
                count = MAX_DECIMAL_DIGITS - i;
        }

        /* Set condition code according to operand sign */
        cc = (count == 0) ? 0 : (sign < 0) ? 1 : 2;
    }

    /* Make sign positive if result is zero */
    if (cc == 0)
        sign = +1;

    /* Store result into operand location */
    ARCH_DEP(store_decimal) (effective_addr1, l1, b1, regs, dec, sign);

    /* Set condition code */
    regs->psw.cc = cc;

    /* Program check if overflow and PSW program mask is set */
    if (cc == 3 && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_OVERFLOW_EXCEPTION);

} /* end DEF_INST(shift_and_round_decimal) */

#if defined(_MSVC_) && (_MSC_VER >= VS2010)
#pragma optimize( "", on )
#endif


/*-------------------------------------------------------------------*/
/* FB   SP    - Subtract Decimal                              [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_decimal)
{
int     l1, l2;                         /* Length values             */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
BYTE    dec1[MAX_DECIMAL_DIGITS];       /* Work area for operand 1   */
BYTE    dec2[MAX_DECIMAL_DIGITS];       /* Work area for operand 2   */
BYTE    dec3[MAX_DECIMAL_DIGITS];       /* Work area for result      */
int     count1, count2, count3;         /* Significant digit counters*/
int     sign1, sign2, sign3;            /* Sign of operands & result */

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Load operands into work areas */
    ARCH_DEP(load_decimal) (effective_addr1, l1, b1, regs, dec1, &count1, &sign1);
    ARCH_DEP(load_decimal) (effective_addr2, l2, b2, regs, dec2, &count2, &sign2);

    /* Add or subtract operand values */
    if (count2 == 0)
    {
        /* If second operand is zero then result is first operand */
        memcpy (dec3, dec1, MAX_DECIMAL_DIGITS);
        count3 = count1;
        sign3 = sign1;
    }
    else if (count1 == 0)
    {
        /* If first operand is zero then result is -second operand */
        memcpy (dec3, dec2, MAX_DECIMAL_DIGITS);
        count3 = count2;
        sign3 = -sign2;
    }
    else if (sign1 != sign2)
    {
        /* If signs are opposite then add operands */
        add_decimal (dec1, dec2, dec3, &count3);
        sign3 = sign1;
    }
    else
    {
        /* If signs are equal then subtract operands */
        subtract_decimal (dec1, dec2, dec3, &count3, &sign3);
        if (sign1 < 0) sign3 = -sign3;
    }

    /* Set condition code */
    cc = (count3 == 0) ? 0 : (sign3 < 1) ? 1 : 2;

    /* Overflow if result exceeds first operand length */
    if (count3 > (l1+1) * 2 - 1)
        cc = 3;

    /* Set positive sign if result is zero */
    if (count3 == 0)
        sign3 = 1;

    /* Store result into first operand location */
    ARCH_DEP(store_decimal) (effective_addr1, l1, b1, regs, dec3, sign3);

    /* Return condition code */
    regs->psw.cc = cc;

    /* Program check if overflow and PSW program mask is set */
    if (cc == 3 && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_decimal) */


/*-------------------------------------------------------------------*/
/* F8   ZAP   - Zero and Add                                  [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(zero_and_add)
{
int     l1, l2;                         /* Length values             */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     cc;                             /* Condition code            */
BYTE    dec[MAX_DECIMAL_DIGITS];        /* Work area for operand     */
int     count;                          /* Significant digit counter */
int     sign;                           /* Sign                      */

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Load second operand into work area */
    ARCH_DEP(load_decimal) (effective_addr2, l2, b2, regs, dec, &count, &sign);

    /* Set condition code */
    cc = (count == 0) ? 0 : (sign < 1) ? 1 : 2;

    /* Overflow if result exceeds first operand length */
    if (count > (l1+1) * 2 - 1)
        cc = 3;

    /* Set positive sign if result is zero */
    if (count == 0)
        sign = +1;

    /* Store result into first operand location */
    ARCH_DEP(store_decimal) (effective_addr1, l1, b1, regs, dec, sign);

    /* Return condition code */
    regs->psw.cc = cc;

    /* Program check if overflow and PSW program mask is set */
    if (cc == 3 && DOMASK(&regs->psw))
        ARCH_DEP(program_interrupt) (regs, PGM_DECIMAL_OVERFLOW_EXCEPTION);

} /* end DEF_INST(zero_and_add) */


#if defined(FEATURE_016_EXT_TRANSL_FACILITY_2)
/*-------------------------------------------------------------------*/
/* EBC0 TP    - Test Decimal                                 [RSL-a] */
/*-------------------------------------------------------------------*/
DEF_INST(test_decimal)
{
int     l1;                             /* Length value              */
int     b1;                             /* Base register number      */
VADR    effective_addr1;                /* Effective address         */
int     i;                              /* Array subscript           */
int     cc = 0;                         /* Condition code            */
BYTE    pack[MAX_DECIMAL_LENGTH];       /* Packed decimal work area  */

    RSL(inst, regs, l1, b1, effective_addr1);
    PER_ZEROADDR_XCHECK( regs, b1 );

    TXFC_INSTR_CHECK( regs );

    /* Fetch the packed decimal operand into the work area */
    ARCH_DEP(vfetchc) (pack, l1, effective_addr1, b1, regs);

    /* Test each byte of the operand */
    for (i=0; ; i++)
    {
        /* Test the high-order digit of the byte */
        if ((pack[i] & 0xF0) > 0x90)
            cc = 2;

        /* Exit if this is the last byte */
        if (i == l1) break;

        /* Test the low-order digit of the byte */
        if ((pack[i] & 0x0F) > 0x09)
            cc = 2;
    }

    /* Test the sign in the last byte */
    if ((pack[i] & 0x0F) < 0x0A)
        cc |= 1;

    /* Return condition code */
    regs->psw.cc = cc;

} /* end DEF_INST(test_decimal) */
#endif /*defined(FEATURE_016_EXT_TRANSL_FACILITY_2)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "decimal.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "decimal.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
