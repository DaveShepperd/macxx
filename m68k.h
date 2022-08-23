/*
    m68k.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _M68K_H_
#define _M68K_H_ 1

#define TRUE 1
#define FALSE 0
#if defined(NULL)
#undef NULL
#endif
#define NULL 0

#define NUM_EA_TMPSTACKS (2)

/*effective address structure */
typedef struct ea {
    unsigned short mode;	/* which ea mode (bit mask) */
    char eamode;		/* 6 bit mode to be inserted into opcode */
    char reg1;			/* first register found */
    short reg2;			/* second register for 2 reg modes */
				/* 0-7 D regs 8-15 A regs */
    struct exp_stk *exp;	/* ptr to operand expression stack */
    struct exp_stk *tmps[NUM_EA_TMPSTACKS];    /* ptrs to temporary stacks */
} EA;

extern EA source,dest;

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
#define E_NDX	(1<<6)	/* d(An,Rn) 	Address register indirect with index */
#define E_ABS	(1<<7)	/* d		Absolute			     */
#define E_PCR	(1<<8)	/* d(PC) 	Program counter relative	     */
#define E_PCN	(1<<9)	/* d(PC,Rn) 	Program counter with index	     */
#define E_IMM	(1<<10)	/* #d		Immediate			     */
#define E_ALL	((1<<11)-1) /* all the normal address modes		     */
#define E_AMA	(E_ALL-E_IMM-E_PCN-E_PCR-E_An-E_Dn)
#define E_MORE	(E_DSP|E_NDX|E_ABS|E_PCR|E_PCN|E_IMM)
#define E_SR	(1<<11)	/* SR		Status Register			     */
#define E_USP	(1<<12)	/* USP		User Stack Pointer		     */
#define E_CCR	(1<<13)	/* CCR		Condition Code Register		     */
#define E_RLST	(1<<14)	/* Rn/Rn... 	Register List			     */
#define E_NUL	0	/* <null>	No addressing mode		     */

#define Ea_Dn	000	/* Dn	*/
#define Ea_An	010	/* An	*/
#define Ea_ATAn 020	/* (An) */	
#define Ea_ANI	030	/* (An)+ */
#define Ea_DAN	040	/* -(An) */
#define Ea_DSP	050	/* d(An) */
#define Ea_NDX	060	/* d(An,Rn) */
#define Ea_ABSW	070	/* d [word] */
#define Ea_ABSL	071	/* d [long] */
#define Ea_PCR	072	/* d(PC)  */
#define Ea_PCN	073	/* d(PC,Rn) */
#define Ea_IMM	074	/* #n	*/
#define REG_Dn	000	/* register D0-D7 */
#define REG_An	010	/* register A0-A7 */
#define REG_PC	020	/* pc */
#define REG_SR	021	/* status register */
#define REG_CCR 022	/* condition code register */
#define REG_SSP 023	/* supervisor stack pointer */
#define REG_USP 024	/* user stack pointer */
#define REG_SP  017	/* normal stack pointer (A7) */
#define FORCE_WORD 0x40000000
#define FORCE_LONG 0x20000000

#endif /* _M68K_H_ */
