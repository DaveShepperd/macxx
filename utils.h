/*
    pass1.c - Part of macxx, a cross assembler family for various micro-processors
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
#ifndef _UTILS_H_
#define _UTILS_H_ (1)

/** detab() - de-tab a string.
 * At entry:
 * @param input - pointer to null terminated input string
 * @param tabWidth - number of characters a tab represents
 * @param numTabs - stop de-tabbing after this many tabs (0=all
 *  			  of them).
 * @param column - starting column (modulo tabWidth)
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
extern int deTab(const char *input, int tabWidth, int numTabs, int column, char *output, size_t outputSize);

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
extern int longToAscii(long data, char *dst, size_t maxDstSize, int numDigits, int leadingZeros, int radix);

/** strnrchr() Look backwards through a string for character
 *  At entry:
 *  @param ptr - pointer to end of string to search
 *  @param chr - character to search for
 *  @param len - maximum number of characters to look through
 *  At exit:
 *  @return pointer to found character or NULL
 *
 *  @note This function is similar to strrchr() except it
 *  	  provides a "stop looking" option.
 **/
extern const char *strnrchr(const char *ptr, int chr, size_t len);

#endif	/* _UTILS_H_*/
