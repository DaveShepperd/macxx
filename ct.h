/*
    ct.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _CT_H_
#define _CT_H_ 1
/* Define the character classes */

/* WARNING: Don't alter the order of the following items */

#define CC_ALP  0		/* alphabetic (A-Z,a-z) */
#define CC_NUM	1		/* numeric (0-9) */
#define CC_OPER	2		/* expression operator character */
#define CC_PCX  3		/* any other printing character */
#define CC_WS   4		/* white space */
#define CC_EOL  5		/* end of line */

/* define the character types */

#define CT_EOL 	 0x001		/*  EOL */
#define CT_WS 	 0x002		/*  White space (space or tab) */
#define CT_COM 	 0x004		/*  COMMA */
#define CT_PCX 	 0x008		/*  PRINTING CHARACTER */
#define CT_NUM 	 0x010		/*  NUMERIC */
#define CT_ALP 	 0x020		/*  ALPHA, DOT, DOLLAR, UNDERSCORE */
#define CT_LC 	(0x040|CT_ALP)	/*  LOWER CASE ALPHA */
#define CT_SMC 	 0x080		/*  SEMI-COLON */
#define CT_BOP   0x100		/*  binary operator (legal between terms) */
#define CT_UOP   0x200		/*  unary operator (legal starting a term) */
#define CT_OPER (CT_BOP|CT_UOP)	/*  An expression operator */
#define CT_HEX  (0x400|CT_ALP)	/*  hex digit */
#define CT_LHEX (0x400|CT_LC)	/*  lower case hex digit */

#ifndef NO_EXTTBL
extern unsigned short cttbl[];
extern unsigned char  cctbl[];
#endif

#endif  /* _CT_H_ */
