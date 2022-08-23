/*
    outx.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _OUTX_H_
#define _OUTX_H_

#include "token.h"
#include "exproper.h"
#include "vlda_structs.h"

extern FILE *outxsym_fp;    /* global pointer for outx routines */
extern FILE *outxabs_fp;
extern char *eline;
extern TMP_struct *tmp_top,*tmp_ptr,*tmp_pool;

extern void trunc_err( long mask, long tv);
extern SEG_struct *get_absseg(void);
extern void rewind_tmp(void);
extern int dbg_output( void );
extern void termobj(long traddr);
extern int outbstr(char *from, int len );
extern char *outexp(EXP_stk *eps, char *s, char *wrt, FILE *fp );
extern void outorg(EXP_stk *eps);
extern char *outxfer(EXP_stk *eps, FILE *fp);
extern int outtstexp(char *asc, int alen, EXP_stk *eps);
extern int outboffexp(char *asc, int alen, EXP_stk *eps);
extern int outoorexp(char *asc, int alen, EXP_stk *eps);

#endif /* _OUTX_H_ */
