/*
    utils.c - Part of macxx, a cross assembler family for various micro-processors
    Copyright (C) 2008 David Shepperd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>

/* Support for longToAscii() due to ISO C17 does not support '%lb' sprintf construct */
static int longToAsciiRadix2(long data, char *dst, size_t maxDstSize, int numDigits, int leadingZeros)
{
	unsigned long udata, msb;
	char *dstSav = dst, *binDst = dst, lead;
	int sign = 0;
	
	if (data < 0)
	{
		/* The input is negative, so make it positive and record the fact */
		sign = 1;
		udata = -data;
	}
	else
	{
		udata = data;
	}
	/* Make sure the number of digits isn't too many (32 bit long)*/
	if ( numDigits > 31 )
		numDigits = 31;
	/* Get a bit to walk the bit stream */
	msb = 1<<(numDigits-1);
	/* Assume leading spaces */
	lead = ' ';
	if ( leadingZeros )
	{
		/* Nope, caller wants leading 0's */
		lead = '0';
		if ( sign )
		{
			/* So the sign has to come first */
			*binDst++ = '-';
			sign = 0;	/* signal sign already present */
		}
	}
	/* while there is room in the buffer and the number of digits desired */
	while ( msb && binDst < dstSav+(sign?numDigits-1:numDigits) )
	{
		if ( (udata&msb) )
		{
			/* need to put in a '1' */
			if ( sign )
			{
				/* if negative, the sign digit comes first */
				*binDst++ = '-';
				sign = 0;
			}
			/* from here on, digits become either '0' or '1' */
			lead = '0';
			*binDst++ = '1';
		}
		else 
			*binDst++ = lead;
		/* walk the test bit one to the right */
		msb >>= 1;
	}
	*binDst = 0;	/* terminate the string */
	return binDst-dstSav; /* return length of string */
}

/** longToAscii() - convert long to ascii string
 *  At entry:
 *  @param data - data to convert
 *  @param dst - pointer to destination string buffer
 *  @param maxDstSize - size of destination string buffer
 *  @param numDigits - number of digits to output leadingZeros -
 *  @param non-zero if to include leading zeroes
 *  			   in the converted string
 *  @param radix - radix to use for conversion
 *
 *  At exit:
 *  @return length of resulting string. dst buffer contains
 *  ASCII number, nul terminated.
 **/
int longToAscii(long data, char *dst, size_t maxDstSize, int numDigits, int leadingZeros, int radix)
{
	int dstLen=0;
	char fmt[32],*fmtPtr;
	
	if ( dst )
	{
		/* There is a place to deposit the conversion */
		if ( maxDstSize > 1  )
		{
			/* There is room for at least 1 digit besides the trailing nul */
			if ( numDigits > 0 )
			{
				/* The caller wants there to be digits */
				if ( numDigits > maxDstSize - 1 )
					numDigits = maxDstSize-1;	/* Cannot be more than the size of buffer */
				/* manufacture a sprintf format string */
				fmtPtr = fmt;
				*fmtPtr++ = '%';	/* Starts with a percent */
				if ( leadingZeros )
					*fmtPtr++ = '0';	/* If they want leading zeros */
				/* set the number of digits to convert */
				fmtPtr += snprintf(fmtPtr,sizeof(fmt)-(fmtPtr-fmt),"%d",numDigits);
				*fmtPtr++ = 'l';	/* data is a signed longword */
				switch (radix)
				{
				case 2:
					/* warning: ISO C17 does not support the ‘%lb’ gnu_printf format [-Wformat=] */
#if 0
					/* This would be the easy way, but for legacy and maximum portablity, do it the hard way */
					*fmtPtr++ = 'b';
					break;
#else
					return longToAsciiRadix2(data,dst,maxDstSize,numDigits,leadingZeros);
#endif
				case 8:
					*fmtPtr++ = 'o';	/*  octal format indicator */
					break;
				default:
				case 10:
					*fmtPtr++ = 'd';	/* signed decimal indicator */
					break;
				case 16:
					*fmtPtr++ = 'X';	/* hexidecimal indicator */
					break;
				}
				if ( fmtPtr )
				{
					*fmtPtr = 0;		/* terminate the format string */
					/* convert the data to ASCII as appropriate */
					dstLen = snprintf(dst, maxDstSize, fmt, data);
				}
			}
			dst[maxDstSize-1] = 0;	/* Just make sure last byte in the caller's buffer is nul */
		}
		else if ( maxDstSize == 1 )
		{
			/* There's room for at least the nul character so put one there */
			*dst = 0;
		}
	}
	return dstLen;
}

/** detab() - de-tab a string.
 * At entry:
 * @param input - pointer to null terminated input string
 * @param tabWidth - number of characters a tab represents
 * @param numTabs - stop de-tabbing after this many tabs (0=all
 *  			  of them).
 * @param column - starting column number (0 to tabWidth-1)
 * @param output - pointer to output buffer
 * @param outputSize - size of output buffer
 * At exit:
 * @return len of resulting string.
 * 
 * @note Input copied to output with tabs replaced with
 * appropriate number of spaces. If the input is longer than the
 * output with the added spaces, the string will be truncated.
 * The output buffer will always be terminated with a nul.
 * Returns
 */
int deTab(const char *input, int tabWidth, int numTabs, int column, char *output, size_t outputSize)
{
	int len = 0, cc, stop=numTabs, spaces;
	
	if ( output )
	{
		/* Provided an output */
		if ( outputSize )
		{
			/* and there's room enough for a nul, stick one in just in case */
			*output = 0;
		}
	}
	if ( input && output && outputSize )
	{
		/* The buffer pointers are usable */
		/* Make the tab width something reasonable */
		if ( tabWidth < 1 )
			tabWidth = 1;
		if ( tabWidth > 8 )
			tabWidth = 8;
		spaces = column % tabWidth;
		while ( (cc=*input++) && len < outputSize-1 )
		{
			if ( spaces <= 0 )
				spaces = tabWidth;
			if ( cc == '\t' )
			{
				while ( spaces > 0 && len < outputSize-1 )
				{
					output[len] = ' ';
					++len;
					--spaces;
				}
				if ( numTabs )
				{
					--stop;
					if ( stop <= 0 )
					{
						while ( (cc=*input++) && len < outputSize-1 )
						{
							output[len] = cc;
							++len;
						}
						break;
					}
				}
			}
			else
			{
				output[len] = cc;
				++len;
				--spaces;
			}
		}
		output[len] = 0;
	}
	return len;
}




