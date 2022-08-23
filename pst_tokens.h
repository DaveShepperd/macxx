/*
    pst_tokens.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _PST_TOKENS_H_
#define _PST_TOKENS_H_ 1

typedef enum {
      OPCL00=0,		/* 0 operands required */
      OPCL01,		/* bit mode instructions */
      OPCL02,		/* 0,1 or 2 operands */
      OPCL03,		/* branch instructions */
      OPCLPBR=0x0004,	/* use program bank reg */
      OPCLINDX=OPCLPBR*2, /* index instructions */

      OP_CLASS_OPC = 0x8000,
      OP_CLASS_MAC = 0x4000
} OPClass;

typedef enum {
      DFLPST  	=0x200,			/* .PST directive */
      DFLPARAM	=0x100,			/* pass the opcode struct * as param */
      DFLIIF 	=0x080,			/* immediate conditional */
      DFLMEND	=0x040,			/* macro end directive */
      DFLEND	=0x020,			/* .END DIRECTIVE */
      DFLGEV	=0x010,			/* DIRECTIVE REQUIRES EVEN LOCATION */
      DFLGBM	=0x008,			/* DIRECTIVE USES BYTE MODE */
      DFLCND	=0x004,			/* CONDITIONAL DIRECTIVE */
      DFLMAC	=0x002,			/* MACRO DIRECTIVE */
      DFLSMC	=0x001			/* MCALL */
} DIRFlags;

#include "pst_structs.h"

#endif /* _PST_TOKENS_H_ */

