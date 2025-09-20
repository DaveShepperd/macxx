/*
	psttkn8080.c - Part of macxx, a cross assembler family for various micro-processors
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


/******************************************************************************
Change Log

    09/03/2025	- z80 CPU support added by Dave Shepperd

******************************************************************************/


#ifndef _PSTTKNZ80_H_
#define _PSTTKNZ80_H_ 1

/* The Z80 has numerous address modes */

typedef enum
{
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_H,
    REG_L,
    REG_M,
    REG_A,
    REG_BC,
    REG_DE,
	REG_HL,
	REG_SP,
	REG_AF,
	REG_R,
	REG_I,
	REG_IX,
	REG_IY,
	REG_AFP,
	REG_BCP,
	REG_DEP,
	REG_HLP
} Z80Regs_t;

typedef enum {
	ILL_AM= -1,
/* Make the class match the registers */	
	AM_B=REG_B,
	AM_C,
	AM_D,
	AM_E,
	AM_H,
	AM_L,
	AM_skip,
	AM_A,
	AM_BC,
	AM_DE,
	AM_HL,
	AM_SP,
	AM_AF,
	AM_R,
	AM_I,
	AM_IX,
	AM_IY,
	AM_AFP,
	AM_BCP,
	AM_DEP,
	AM_HLP,
/* End of register match */
	AM_IBC,		/* (BC) */
	AM_IDE,		/* (DE) */
	AM_IHL,		/* (HL) */
	AM_ISP,		/* (SP) */
	AM_nn,		/* plain */
	AM_Inn,		/* 16 bit operand indirect */
	AM_IIX,		/* index indirect */
	AM_IIY,		/* index indirect */
	AM_IC,		/* (c) (just for out command) */
	MAX_AMODE
} AModes;

#define OPC_AM_BIT_SHIFT (MAX_AMODE)

typedef enum
{
	CLASS_NO,	/* Either no operands or no special operand handling */
	CLASS_LD,
	CLASS_PSH,
	CLASS_POP,
	CLASS_EX,
	CLASS_ADD,
	CLASS_ADC,
	CLASS_SUB,
	CLASS_SBC,
	CLASS_AND,
	CLASS_OR,
	CLASS_XOR,
	CLASS_CP,
	CLASS_INC,
	CLASS_DEC,
	CLASS_IM,
	CLASS_RLC,
	CLASS_RL,
	CLASS_RRC,
	CLASS_RR,
	CLASS_SLA,
	CLASS_SRA,
	CLASS_SRL,
	CLASS_BIT,
	CLASS_SET,
	CLASS_RES,
	CLASS_JP,
	CLASS_JR,
	CLASS_RST,
	CLASS_DJNZ,
	CLASS_CALL,
	CLASS_RET,
	CLASS_IN,
	CLASS_OUT,
	CLASS_MAX
} AMClasses_t;

#endif /* _PSTTKNZ80_H_ */
