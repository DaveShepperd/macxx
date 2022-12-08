/*
    dumdir.c - Part of macxx, a cross assembler family for various micro-processors
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

#include "token.h"

#define DIRDEF(name,func,flags) int func() { /* printf("Found %s\n","name"); */ no_such_opcode(); return 0; }

static int no_such_opcode(void)
{
	if ( !options[QUAL_IGNORE] )
		bad_token(tkn_ptr, "Sorry, directive not implimented in this version.");
	f1_eatit();         /* eat the line */
	return 0;
}

DIRDEF(.COPY,   op_copy,    0)
DIRDEF(.CROSS,  op_cross,   0)
DIRDEF(.DCREF,  op_dcref,   0)
DIRDEF(.ECREF,  op_ecref,   0)
DIRDEF(.IDENT,  op_ident,   0)
DIRDEF(.LIMIT,  op_limit,   DFLGEV)
DIRDEF(.MACLI,  op_maclib,  0)
DIRDEF(.MCALL,  op_mcall,   DFLSMC)
DIRDEF(.MPURG,  op_mpurge,  0)
DIRDEF(.NOCRO,  op_nocross, 0)
DIRDEF(.PRINT,  op_print,   0)
DIRDEF(.SYMBO,  op_symbol,  0)
DIRDEF(.DEFST,  op_defstack,   0)
DIRDEF(.GETPO,  op_getpointer,   0)
DIRDEF(.POP,    op_pop,     0)
DIRDEF(.PUSH,   op_push,    0)
DIRDEF(.PUTPO,  op_putpointer,  0)

#ifdef ATARIST
extern void printf();
void *memcpy(dst,src,siz)
char *src,*dst;
int siz;
{
    register char *s = src;
    register char *d = dst;
    register int siz;
    for (;siz > 0; --siz) *d++ = *s++;
    return;
}

void *memset(src,value,size)
char *src;      /* pointer to array to set */
int value;      /* value to set in array */
int size;       /* number of bytes to set */
{
    char *dst=src;
    int val=value,len=size;

    for (;len>0;--len) *dst++ = val;
    return;
}

int getpid()
{
    return 1;
}
#endif
