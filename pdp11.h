/*
    pdp11.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _PDP11_H_
#define _PDP11_H_ 1

/******************************************************************************
		pdp11.h
   This is the global data declaration section for pdp11.
******************************************************************************/

#define TRUE 1
#define FALSE 0
#if defined(NULL)
#undef NULL
#endif
#define NULL 0

/*effective address structure */
typedef struct ea {
    struct exp_stk *exp;	/* array of ptrs to expression stacks */
    unsigned long mode;		/* which ea mode (bit mask) */
    unsigned short eamode;	/* 6 bit mode to be inserted into opcode */
} EA;

extern EA source,dest;

/* Effective address modes */

#define E_REG       (1<<0)	/* reg		Register direct		     */
#define E_IREG      (1<<1)	/* (reg)	Register indirect	     */
#define E_IREGAI	(1<<2)	/* (reg)+	Register indirect, auto-increment  */
#define E_IIREGAI   (1<<3)  /* @(reg)+  Register indirect indirect, auto-increment */
#define E_ADIREG	(1<<4)	/* -(reg)	Auto-decrement reg, indirect  */
#define E_IADIREG   (1<<5)  /* @-(reg)  Auto-decrement reg, indirect indirect */
#define E_DSP   	(1<<6)	/* d(reg)	Register + displacement indirect */
#define E_IDSP      (1<<7)  /* @d(reg)  Register + displacement indirect indirect */
#define E_IMM       (1<<8)	/* #d		Immediate			     */
#define E_IND       (1<<9)  /* @d       Indirect */
#define E_ABS	    (1<<10)	/* @#d		Absolute (Indirect immediate)     */
#define E_PCR       (1<<11) /* d(pc)    PC relative */
#define E_IPCR      (1<<12) /* @d(pc)   Indirect PC relative */
#define E_NUL	    0	    /* <null>	No addressing mode		     */
#define E_ANY	(~E_NUL)    /* all addressing modes 			     */
#define E_ALL	((1<<16)-1) /* all the normal address modes		     */

/* Address modes: */
enum regs {
Ea_REG=		000,	/* reg */
Ea_IREG=	010,	/* (reg) */
Ea_IREGAI=	020,	/* (reg)+ */
Ea_IIREGAI= 030,    /* @(reg)+ */
Ea_ADIREG=	040,	/* -(reg) */
Ea_IADIREG= 050,    /* @-(reg) */
Ea_DSP=	    060,	/* disp(reg) [where reg != pc] */
Ea_IDSP=	070,	/* @disp(reg) [where reg != pc] */
Ea_IMM=     027,	/* #imm */
Ea_ABS=     037,    /* @#abs */
Ea_PCR=     067,    /* disp (pc relative) */
Ea_IPCR= 	077 	/* @disp (pc relative) */
};

#endif /* _PDP11_H_ */
