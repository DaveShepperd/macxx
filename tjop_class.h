/*
    tjop_class.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _TJOP_CLASS_H_
#define _TJOP_CLASS_H_ 1

#define OPCL_JR		0x00	/* JR command */
#define OPCL_JMP	0x01	/* JMP command */
#define OPCL_LD		0x02	/* LOAD command */
#define OPCL_ST		0x03	/* STORE command */
#define OPCL_MOVE	0x04	/* MOVE command */
#define OPCL_R		0x05	/* single register operand */
#define OPCL_RR		0x06	/* two register operands */
#define OPCL_nR		0x07	/* two operands, first is immediate, second is register */
#define OPCL_BRR	0x08	/* two operands, (swap src and dest in opcode) */
#define OPCL_TOM	0x80	/* Tom only */
#define OPCL_JERRY	0x40	/* Jerry only */
#define OPCL_TJ		0xC0	/* Tom and Jerry */

#define MAX_DISP	(30)	/* max branch displacement backwards, in bytes */
#define MIN_DISP	(-32)	/* max branch displacement forwards, in bytes */
#define MIN_REG		0	/* smallest register number */
#define MAX_REG		31	/* largest register number */
#define BR_OFF		2	/* number of bytes to add to PC to compute branch offset */

#define AM_SREG		0x0001	/* src register addressing mode */
#define AM_SIREG	0x0002	/* src register indirect */
#define AM_SIIREG	0x0004	/* src register indirect indexed */
#define AM_SIMM		0x0008	/* src immediate */
#define AM_ANYSRC	0x000F	/* any source operand */
#define AM_DREG		0x0010	/* dst register */
#define AM_DIREG	0x0020	/* dst register indirect */
#define AM_DIIREG	0x0040	/* dst register indirect indexed */
#define AM_DIMM		0x0080	/* dst immediate */
#define AM_ANYDST	0x00F0	/* any destination operand */
#define AM_IMM1		0x0100	/* immediate value is 1-32 instead of 0-31 */
#define AM_IMM2		0x0200	/* immediate value is -16 to +15 instead of 0-31 */
#define AM_IMM32	0x0300	/* immediate value is 32 bits long */
#define AM_SHIFT	0x0400	/* shift, immediate is computed 32-n */

enum opcs {
   OPC_CODE=10,			/* Bit position of LSB of opcode */
   OPC_SRC=5,			/* Bit position of LSB of source operand */
   OPC_DST=0,			/* Bit position of LSB of dest operand */
   OPC_JR=53,			/* Jump relative */
   OPC_JMP=52,			/* Jump register indirect */
   OPC_LOADIR=41,		/* Load (r),r */
   OPC_LOADR14n=43,		/* LOAD (R14+n),r */
   OPC_LOADR15n=44,		/* LOAD (R15+n),r */
   OPC_LOADR14R=58,		/* LOAD (R14+r), r */
   OPC_LOADR15R=59,		/* LOAD (R15+r), r */
   OPC_MOVERR=34,		/* MOVE r,r */
   OPC_MOVEPCR=51,		/* MOVE PC,R */
   OPC_MOVEI=38,		/* MOVEI #n, r (n<0 || n>31) */
   OPC_MOVEQ=35,		/* MOVEQ #n, r (0<=n<=31) */
   OPC_NOP=57,			/* NOP */
   OPC_STOREIR=47,		/* STORE r,(r) */
   OPC_STORER14n=49,		/* STORE r,(R14+n) */
   OPC_STORER15n=50,		/* STORE r,(R15+n) */
   OPC_STORER14R=60,		/* STORE r,(R14+r) */
   OPC_STORER15R=61,		/* STORE r,(R15+r) */
   OPC_IMULTN=18,		/* IMULTN r,r */
   OPC_RESMAC=19,		/* RESMAC r */
   OPC_IMACN=20,		/* IMACN r,r */
   OPC_MMULT=54			/* MMULT r,r */
};

#endif /* _TJOP_CLASS_H_ */
