/*
    opcas_dmp.h - Part of macxx, a cross assembler family for various micro-processors
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
#ifndef _OPCAS_DMP_H_
#define _OPCAS_DMP_H_

#ifndef STACK_DEBUG
#define STACK_DEBUG 0
#endif

extern void dump_stack(FILE *outf,EXP_stk *eps);
#if STACK_DEBUG
#define DUMP_STACK(outf,msg,stk) do { if (msg) fprintf(outf,"%s\n",msg); dump_stack(outf,stk); } while (0)
#else
#define DUMP_STACK(outf,msg,stk) do { ; } while (0)
#endif

#endif	/* _OPCAS_DMP_H_ */


