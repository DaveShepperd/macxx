/*
    m682k.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _M682K_H_
#define _M682K_H_ 1

#define TRUE 1
#define FALSE 0
#if defined(NULL)
#undef NULL
#endif
#define NULL 0

/*effective address structure */
typedef struct ea {
    struct exp_stk *exp[2];	/* array of ptrs to expression stacks */
    unsigned long base_reg;	/* base register */
    unsigned long index_reg;	/* index register */
    unsigned long mode;		/* which ea mode (bit mask) */
    unsigned short extension;	/* extension word */
    unsigned short eamode;	/* 6 bit mode to be inserted into opcode */
    int reg;			/* register number */
    int movec_xlate;		/* xlate code for the movec instruction */
    int parts;			/* flags indicating which parts of am found */
    int index_scale;		/* index register scale */
    int num_exprs;		/* index to next available expression stack */
    int state;			/* operand evaluation state */
} EA;

enum ea_parts {
   EAPARTS_BASER=	0x001,	/* have a base register */
   EAPARTS_BASED=	0x002,	/* have a base displacement */
   EAPARTS_INDXR=	0x004,	/* have an index register */
   EAPARTS_INDXD=	0x008	/* have an index (outer) displacement */
};

enum ea_state {
   EASTATE_done,		/* done with argument */
   EASTATE_bd,			/* getting base displacement */
   EASTATE_br,			/* getting base register */
   EASTATE_ir,			/* getting index register */
   EASTATE_od			/* getting outer displacement */
};

extern EA source,dest;

enum extensions {
   EXT_IIS_NOP		=0x0000,	/* No memory indirection */
   EXT_IIS_IPI0OD	=0x0001,	/* Indirect Preindexed with null Outer Displacement */
   EXT_IIS_IPIWOD	=0x0002,	/* Indirect Preindexed with Word Outer Displacement */
   EXT_IIS_IPILOD	=0x0003,	/* Indirect Preindexed with Long Outer Displacement */
   EXT_IIS_IPO0OD	=0x0005,	/* Indirect Postindexed with null Outer Displacement */
   EXT_IIS_IPOWOD	=0x0006,	/* Indirect Postindexed with Word Outer Displacement */
   EXT_IIS_IPOLOD	=0x0007,	/* Indirect Postindexed with Long Outer Displacement */
   EXT_IIS_I0OD		=0x0041,	/* memory Indirect with null Outer Displacement */
   EXT_IIS_IWOD		=0x0042,	/* memory Indirect with Word Outer Displacement */
   EXT_IIS_ILOD		=0x0043,	/* memory Indirect with Long Outer Displacement */
   EXT_BDSIZE		=0x0010,	/* 2 bits for base displacement size */
   EXT_BD0		=0x0010,	/* Null Base Displacement */
   EXT_BDW		=0x0020,	/* Word Base Displacement */
   EXT_BDL		=0x0030,	/* Long Base Displacement */
   EXT_BASESUPRESS	=0x0080,	/* supress add of base register */
   EXT_FULLFMT		=0x0100,	/* full format extension word */
   EXT_V_SCALE		=9,		/* bit number of 2 bit scale field (0=1,1=2,2=4 and 3=8) */
   EXT_SCALE		=1<<EXT_V_SCALE,
   EXT_ILONG		=0x0800,	/* index is longword */
   EXT_V_INDEX		=12,		/* bit number of index register field */
   EXT_INDEX		=1<<EXT_V_SCALE /* 4 bits of index register */
};

/* EA types */
#define AREG 0
#define DREG 1
#define WORDREG 0
#define LONGREG 1
#define NOREG (0x7F)

/* Effective address modes */

#define E_Dn	(1<<0)	/* Dn		Data register direct		     */
#define E_An	(1<<1)	/* An		Address register direct		     */
#define E_ATAn	(1<<2)	/* (An)		Address register indirect	     */
#define E_ANI	(1<<3)	/* (An)+	Addr. reg. indirect, post-increment  */
#define E_DAN	(1<<4)	/* -(An)	Addr. reg. indirect, pre-increment   */
#define E_DSP	(1<<5)	/* d(An)	Addr. reg. indirect with displacement*/
   			/* (d,An)	(aka)				     */
#define E_NDX	(1<<6)	/* d(An,Rn) 	Address register indirect with index */
   			/* (d,An,Rn)	(aka)				     */
#define E_ABS	(1<<7)	/* d		Absolute			     */
#define E_PCR	(1<<8)	/* d(PC) 	Program counter relative	     */
   			/* (d,PC)	(aka)				     */
#define E_PCN	(1<<9)	/* d(PC,Rn) 	Program counter with index	     */
   			/* (d,PC,Rn)	(aka)				     */
#define E_IMM	(1<<10)	/* #d		Immediate			     */
#define E_AII	(1<<11)	/* (d,An,Xn)	Address register Indirect with Index displacement */
#define E_MIPOI	(1<<12)	/* ([d,An],Xn,od) Memory Indirect Postindexed	     */
#define E_MIPRI (1<<13) /* ([d,An,Xn],od) Memory Indirect Preindexed	     */
#define E_PIPOI	(1<<14)	/* ([d,An],Xn,od) Program counter Indirect Postindexed	     */
#define E_PIPRI (1<<15) /* ([d,An,Xn],od) Program counter Indirect Preindexed	     */
#define E_SR	(1<<16)	/* SR		Status Register			     */
#define E_USP	(1<<17)	/* USP		User Stack Pointer		     */
#define E_CCR	(1<<18)	/* CCR		Condition Code Register		     */
#define E_SPCL	(1<<19) /* VBR		Vector Base Register		     */
#define E_RLST	(1<<20)	/* Rn/Rn... 	Register List			     */
#define E_NUL	0	/* <null>	No addressing mode		     */
#define E_ANY	(~E_NUL) /* all addressing modes 			     */
#define E_ALL	((1<<16)-1) /* all the normal address modes		     */
#define E_AMA	(E_ALL-E_IMM-E_PCN-E_PCR-E_An-E_Dn-E_AII-E_MIPOI-E_MIPRI-E_PIPOI-E_PIPRI)

enum regs {
Ea_Dn=		000,	/* Dn=	*/
Ea_An=		010,	/* An=	*/
Ea_ATAn=	020,	/* (An) */
Ea_ANI=		030,	/* (An)+ */
Ea_DAN=		040,	/* -(An) */
Ea_DSP=		050,	/* d(An) or (d,An) */
Ea_NDX=		060,	/* d(An,Rn) or (d,An,Rn) */
Ea_ABSW=	070,	/* d [word] */
Ea_ABSL=	071,	/* d [long] */
Ea_PCR=		072,	/* d(PC) or (d,PC) */
Ea_PCN=		073,	/* d(PC,Rn) or (d,PC,Rn) */
Ea_IMM=		074,	/* #n=	*/
REG_Dn=		000,	/* register D0-D7 */
REG_An=		010,	/* register A0-A7 */
REG_SP=		017,	/* normal stack pointer (A7) */
REG_PC,			/* pc */
REG_ZPC,		/* pc (implied 0) */
REG_SR,			/* status register */
REG_USP,		/* user stack pointer */
REG_SSP,		/* supervisor stack pointer */
REG_ISP,		/* Interrupt stack ptr */
REG_MSP,		/* Master stack ptr */
REG_CCR,		/* condition code register */
REG_VBR,		/* Vector Base Register */
REG_SFC,		/* Function register */
REG_DFC,		/* function register */
REG_CACR,		/* Cache Control Register */
REG_CAAR		/* Cache Address register */
};

#define FORCE_WORD 0x40000000
#define FORCE_LONG 0x20000000

#endif  /* _M682K_H_ */
