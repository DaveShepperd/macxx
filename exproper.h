/*
    exproper.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _EXPROPER_H_
#define _EXPROPER_H_ 1

#define EXPROPER_ADD	'+'	/* add */
#define EXPROPER_SUB	'-'	/* subtract */
#define EXPROPER_MUL	'*'	/* multiply */
#define EXPROPER_DIV	'/'	/* divide */
#define EXPROPER_AND	'&'	/* and */
#define EXPROPER_OR	'|'	/* inclusive or */
#define EXPROPER_XOR	'^'	/* exclusive or */
#define EXPROPER_NEG	'_'	/* negate (2's compliment) */
#define EXPROPER_COM	'~'	/* compliment (1's compliment) */
#define EXPROPER_USD	'?'	/* unsigned divide */
#define EXPROPER_SWAP	'='	/* swap bytes */
#define EXPROPER_MOD	'\\'	/* modulo */
#define EXPROPER_SHL	'<'	/* shift left */
#define EXPROPER_SHR	'>'	/* shift right */
#define EXPROPER_TST	'!'	/* test for condition (following char is case) */
#define EXPROPER_TST_NOT '!'	/* ...not */
#define EXPROPER_TST_AND '&'	/* ...logical and */
#define EXPROPER_TST_OR '|'	/* ...logical or */
#define EXPROPER_TST_LT	'<'	/* ...less than */
#define EXPROPER_TST_GT	'>'	/* ...greater than */
#define EXPROPER_TST_EQ	'='	/* ...equal */
#define EXPROPER_TST_NE '#'	/* ...not equal */
#define EXPROPER_TST_LE '['	/* ...less than or equal */
#define EXPROPER_TST_GE ']'	/* ...greater than or equal */
#define EXPROPER_TSTNM	'@'	/* test for condition without error message */
#define EXPROPER_PICK	'$'	/* dup n'th item on stack */
#define EXPROPER_PURGE	'#'	/* purge top of stack */
#define EXPROPER_XCHG	'`'	/* exchange top 2 items */
#define EXPROPER_IF	'('	/* if condit true, do until endif */
#define EXPROPER_ELSE	'"'	/* if condit false, do until endif */

extern void init_exprs( void );
/* extern int do_exprs( int flag, EXP_stk *eps ); */
extern int compress_expr_psuedo( EXP_stk *ep );
extern int compress_expr( EXP_stk *exptr );
extern int exprs( int relative, EXP_stk *eps );
extern void dump_expr(EXP_stk *eps);
extern void dumpSymbolTable(int flag);

#endif  /* _EXPROPER_H_ */



