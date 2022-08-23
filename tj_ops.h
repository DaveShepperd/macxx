/*
    tj_ops.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _TJ_OPS_H_
#define _TJ_OPS_H_ 1

/*	tj_ops.h		opcode defn for Tom and Jerry */

OPCDEF( "ABS",	OPCL_TJ|OPCL_R,		 22, AM_DREG)			/* ABS Rd: Absolute value */
OPCDEF( "ADD",	OPCL_TJ|OPCL_RR,	  0, AM_SREG|AM_DREG)		/* ADD Rs, Rd: Add registers */
OPCDEF( "ADC",	OPCL_TJ|OPCL_RR,	  1, AM_SREG|AM_DREG)		/* ADDC Rs, Rd: Add with carry */
OPCDEF( "ADDC",	OPCL_TJ|OPCL_RR,	  1, AM_SREG|AM_DREG)		/* ADDC Rs, Rd: Add with carry */
OPCDEF( "ADDQ",	OPCL_TJ|OPCL_nR,	  2, AM_SIMM|AM_IMM1|AM_DREG)	/* ADDQ #n, Rd: Add immediate value to rn */
OPCDEF( "ADDQMOD",OPCL_JERRY|OPCL_nR,	 63, AM_SIMM|AM_IMM1|AM_DREG)	/* ADDQMOD #n, Rd: ???*/
OPCDEF( "ADDQT",OPCL_TJ|OPCL_nR,	  3, AM_SIMM|AM_IMM1|AM_DREG)	/* ADDQT #n, Rd: Same as ADDQ without changing flags */
OPCDEF( "AND",	OPCL_TJ|OPCL_RR,	  9, AM_SREG|AM_DREG)		/* AND Rs,Rd: Logical and */
OPCDEF( "BCLR",	OPCL_TJ|OPCL_nR,	 15, AM_SIMM|AM_DREG)		/* BCLR #n, Rd: Bit clear */
OPCDEF( "BSET", OPCL_TJ|OPCL_nR,	 14, AM_SIMM|AM_DREG)		/* BSET #n, Rd: Bit set */
OPCDEF( "BTST", OPCL_TJ|OPCL_nR,	 13, AM_SIMM|AM_DREG)		/* BTST #n, Rd: Bit test */
OPCDEF( "CMP",	OPCL_TJ|OPCL_RR,	 30, AM_SREG|AM_DREG)		/* CMP Rs, Rd: Compare */
OPCDEF( "CMPQ", OPCL_TJ|OPCL_nR,	 31, AM_SIMM|AM_IMM2|AM_DREG)	/* CMPQ #n,Rd: Compare immediate */
OPCDEF( "DIV",	OPCL_TJ|OPCL_RR,	 21, AM_SREG|AM_DREG)		/* DIV Rs, Rd: Divide */
OPCDEF( "IMACN",OPCL_TJ|OPCL_RR,	 20, AM_SREG|AM_DREG)		/* IMACN Rs, Rd: Mult/accum */
OPCDEF( "IMULT",OPCL_TJ|OPCL_RR,	 17, AM_SREG|AM_DREG)		/* IMULT Rs, Rd: Integer multiply */
OPCDEF( "IMULTN",OPCL_TJ|OPCL_RR,	 18, AM_SREG|AM_DREG)		/* IMULTN Rs, Rd: Integer multiply-no writeback */
OPCDEF(	"JR",	OPCL_TJ|OPCL_JR, 	 53, 0)				/* JR cc, n */
OPCDEF( "JUMP",	OPCL_TJ|OPCL_JMP,	 52, 0)				/* JUMP (Rd): Jump indirect */
OPCDEF( "JMP",	OPCL_TJ|OPCL_JMP,	 52, 0)				/* JMP (Rd): Jump indirect */
OPCDEF(	"LOAD",	OPCL_TJ|OPCL_LD,	 41, AM_SIREG|AM_SIIREG|AM_DREG)/* LOAD (Rx),Rd: Load instruction */
OPCDEF( "LOADB",OPCL_TJ|OPCL_RR,	 39, AM_SIREG|AM_DREG)		/* LOADB (Rs),Rd: Load byte */
OPCDEF( "LOADP",OPCL_TOM|OPCL_RR,	 42, AM_SIREG|AM_DREG)		/* LOADP (Rs),Rd: Load packed */
OPCDEF( "LOADW",OPCL_TJ|OPCL_RR,	 40, AM_SIREG|AM_DREG)		/* LOADW (Rs),Rd: Load word */
OPCDEF( "MIRROR", OPCL_JERRY|OPCL_R,	 48, AM_DREG)			/* MIRROR Rd; Mirrors bits around bits 16-15 */
OPCDEF( "MMULT",OPCL_TJ|OPCL_RR,	 54, AM_SREG|AM_DREG)		/* MMULT Rs, Rd; Matrix multiply */
OPCDEF( "MOVE", OPCL_TJ|OPCL_MOVE,	 34, AM_SREG|AM_DREG)		/* MOVE Rs, Rd; Move registers */
OPCDEF( "MOVEFA",OPCL_TJ|OPCL_RR,	 37, AM_SREG|AM_DREG)		/* MOVEFA Rs, Rd; Move from alternate */
OPCDEF( "MOVEI", OPCL_TJ|OPCL_nR,	 38, AM_SIMM|AM_IMM32|AM_DREG)	/* MOVEI #n, Rd; Move 32 bit immediate */
OPCDEF( "MOVEQ", OPCL_TJ|OPCL_nR,	 35, AM_SIMM|AM_DREG)		/* MOVEQ #n, Rd; Move 0-31 to r */
OPCDEF( "MOVETA",OPCL_TJ|OPCL_RR,	 36, AM_SREG|AM_DREG)		/* MOVETA Rs,Rd; Move r to alternate */
OPCDEF( "MTOI", OPCL_TJ|OPCL_RR,	 55, AM_SREG|AM_DREG)		/* MTOI Rs, Rd; Mantissa to Integer */
OPCDEF( "MULT", OPCL_TJ|OPCL_RR,	 16, AM_SREG|AM_DREG)		/* MULT Rs, Rd; Multipy */
OPCDEF( "NEG", OPCL_TJ|OPCL_R,		  8, AM_DREG)			/* NEG Rd; 2's compliment r */
OPCDEF( "NOP", OPCL_TJ|OPCL_R,		 57, 0)				/* NOP */
OPCDEF( "NORMI", OPCL_TJ|OPCL_RR,	 56, AM_SREG|AM_DREG)		/* NORMI Rs, Rd; Normalize Integer */
OPCDEF( "NOT", OPCL_TJ|OPCL_R,		 12, AM_DREG)			/* NOT Rd; 1's compliment r */
OPCDEF( "OR", OPCL_TJ|OPCL_RR,		 10, AM_SREG|AM_DREG)		/* OR Rs, Rd; Logical OR */
SOPCDEF("PACK", OPCL_TOM|OPCL_R, (63<<10)|(0<<5), AM_DREG)		/* PACK Rd; Pack into 16 bit CRY */
OPCDEF( "RESMAC", OPCL_TJ|OPCL_R,	 19, AM_DREG)			/* RESMAC Rd; Result of MAC to r */
OPCDEF( "ROR", OPCL_TJ|OPCL_RR,		 28, AM_SREG|AM_DREG)		/* ROR Rs, Rd; Rotate right Rd by Rs */
OPCDEF( "RORQ", OPCL_TJ|OPCL_nR,	 29, AM_SIMM|AM_IMM1|AM_DREG)	/* ROR #n, Rd; Rotate right Rd by n */
OPCDEF( "SAT8", OPCL_TOM|OPCL_R,	 32, AM_DREG)			/* SAT8 Rd; Saturate to 8 bits */
OPCDEF( "SAT16", OPCL_TOM|OPCL_R,	 33, AM_DREG)			/* SAT16 Rd; Saturate to 16 bits */
OPCDEF( "SAT16S", OPCL_JERRY|OPCL_R,	 33, AM_DREG)			/* SAT16 Rd; Saturate to 16 bits */
OPCDEF( "SAT24", OPCL_TOM|OPCL_R,	 62, AM_DREG)			/* SAT24 Rd; Saturate to 24 bits */
OPCDEF( "SAT32S", OPCL_JERRY|OPCL_R,	 42, AM_DREG)			/* SAT16 Rd; Saturate to 16 bits */
OPCDEF( "SH", OPCL_TJ|OPCL_RR,		 23, AM_SREG|AM_DREG)		/* SH Rs, Rd; Logical shift Rd by Rs */
OPCDEF( "SHA", OPCL_TJ|OPCL_RR,		 26, AM_SREG|AM_DREG)		/* SHA Rs, Rd; Arithmetic Rd by Rs */
OPCDEF( "SHARQ", OPCL_TJ|OPCL_nR,	 27, AM_SIMM|AM_IMM1|AM_DREG)	/* SHARQ #n, Rd; Arithmetic shift righ */
OPCDEF( "SHLQ", OPCL_TJ|OPCL_nR,	 24, AM_SIMM|AM_IMM1|AM_DREG|AM_SHIFT)	/* SHLQ #n, Rd; Logical shift left */
OPCDEF( "SHRQ", OPCL_TJ|OPCL_nR,	 25, AM_SIMM|AM_IMM1|AM_DREG)	/* SHRQ #n, Rd; Logical shift right */
OPCDEF(	"STORE", OPCL_TJ|OPCL_ST,	 47, AM_DIREG|AM_DIIREG|AM_SREG)/* STORE Rs, (Rx); STORE instruction */
OPCDEF( "STOREB",OPCL_TJ|OPCL_BRR,	 45, AM_DIREG|AM_SREG)		/* STOREB Rs, (Rd); STORE byte */
OPCDEF( "STOREP",OPCL_TOM|OPCL_BRR,	 48, AM_DIREG|AM_SREG)		/* STOREP Rs, (Rd); STORE packed */
OPCDEF( "STOREW",OPCL_TJ|OPCL_BRR,	 46, AM_DIREG|AM_SREG)		/* STOREW Rs, (Rd); STORE word */
OPCDEF( "SUB", OPCL_TJ|OPCL_RR,		  4, AM_SREG|AM_DREG)		/* SUB Rs, Rd; Subtract Rs from Rd */
OPCDEF( "SBC", OPCL_TJ|OPCL_RR,		  5, AM_SREG|AM_DREG)		/* SUBC Rs, Rd; Subtract with carry */
OPCDEF( "SUBC", OPCL_TJ|OPCL_RR,	  5, AM_SREG|AM_DREG)		/* SUBC Rs, Rd; Subtract with carry */
OPCDEF( "SUBQ", OPCL_TJ|OPCL_nR,	  6, AM_SIMM|AM_DREG|AM_IMM1)	/* SUBQ #n, Rd; Subtract quick */
OPCDEF( "SUBQMOD", OPCL_JERRY|OPCL_nR,	 32, AM_SIMM|AM_IMM1|AM_DREG)	/* SUBQMOD #n, Rd; Subtract quick transparent */
OPCDEF( "SUBQT", OPCL_TJ|OPCL_nR,	  7, AM_SIMM|AM_DREG|AM_IMM1)	/* SUBQT #n, Rd; Subtract quick transparent */
SOPCDEF("UNPACK", OPCL_TOM|OPCL_R, (63<<10)|(1<<5), AM_DREG)		/* UNPACK Rd; */
OPCDEF( "XOR", OPCL_TJ|OPCL_RR,		 11, AM_SREG|AM_DREG)		/* XOR Rs, Rd; Exclusive OR */

#endif /* _TJ_OPS_H_ */
