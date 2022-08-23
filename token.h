/*
    token.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _TOKEN_H_
#define _TOKEN_H_ 1

#ifdef lint
#define void int
#endif
#include "token_defs.h"		/* get standard #define's */
#include <stdio.h>		/* get standard I/O definitions */
#include <ctype.h>		/* get standard string type macros */
#include <string.h>		/* get standard string type functions */
#include "structs.h"		/* define structures */
#include "add_defs.h"
#include "header.h"		/* get our extern declarations */
#include "ct.h"			/* character types */

#define QTBL(\
	noval,			/* t/f if no value is allowed */\
	optional,		/* t/f if value is optional */\
	number,			/* t/f is value must be a number */\
	output,			/* t/f if param is an output file */\
        string,			/* t/f if param is string (incl ws) */\
	negate,			/* t/f if param is negatible */\
	qual,			/* parameter mask */\
	name,			/* name of parameter */\
	index			/* output file index */\
) qual

enum quals {
#include "qual_tbl.h"
,QUAL_MAX};
#undef QTBL
extern char options[QUAL_MAX];

#if !defined(ALIGNMENT)
#define ALIGNMENT 0
#endif

#if ALIGNMENT
#if ALIGNMENT == 1
#define ADJ_ALIGN(ptr) if ((long)ptr&ALIGNMENT)) ++ptr;
#else
#define ADJ_ALIGN(ptr) (ptr += ((1<<ALIGNMENT) - ((int)ptr&((1<<ALIGNMENT)-1))) & ((1<<ALIGNMENT)-1));
#endif
#else
#define ADJ_ALIGN(ptr);
#endif

#if !defined(SUN)
extern void exit();
#endif

#ifndef VMS
#define EXIT_FALSE exit (0)
#define EXIT_TRUE  exit (1)		/* exit code for success */
#else
#define EXIT_FALSE exit_false();
#define EXIT_TRUE  exit (0x10000001)	/* exit code for quiet success */
#endif

#ifdef _toupper
#undef _toupper
#undef _tolower
#endif

extern char char_toupper[];

#define _toupper(c)	(char_toupper[(unsigned char)c])
#define _tolower(c)	((c) >= 'A' && (c) <= 'Z' ? (c) | 0x20:(c))

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#endif      /* _TOKEN_H_ */

