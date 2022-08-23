/*
    opcpp.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "lstcnsts.h"
#include "pst_tokens.h"

/* The following are variables specific to the particular assembler */

char macxx_name[] = "macpp";
char *macxx_target = "unknown";
char *macxx_descrip = "Macro pre-processor.";

unsigned short macxx_rel_salign = 0;    /* default alignment for .REL. segment */
unsigned short macxx_rel_dalign = 0;    /* default alignment for data in .REL. segment */
unsigned short macxx_abs_salign = 0;    /* default alignments for .ABS. segment */
unsigned short macxx_abs_dalign = 0;
char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = 0;    /* default edmask */
/* default list mask */
unsigned long macxx_lm_default = ~(LIST_ME|LIST_MEB|LIST_MES|LIST_LD);
int current_radix = 16;         /* default the radix to hex */

char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
char expr_escape = '^';     /* char that escapes an expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */
char open_operand = '(';    /* char that opens an operand indirection */
char close_operand = ')';   /* char that closes an operand indirection */
int max_opcode_length = 32; /* significant length of opcodes and macros */
int max_symbol_length = 32; /* significant length of symbols */

/* End of processor specific stuff */

int ust_init()
{
    return 0;                /* would fill a user symbol table */
}

#define MAC_PP
#include "opcommon.h"
