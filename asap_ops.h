/*
    asap_ops.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _ASAP_OPS_H_
#define _ASAP_OPS_H_ 1

#include "op_class.h"


/*	mnem.,mnem.c	class		value		comments	*/
/*branches:				v--cond code field*/
/*all others:				v--actual opcode */
NOC_DEF( "ILLOP","ILLOP.C",OPCL_IL,	0x00)	/* 0 op Illegal for bullet proofing!! */
NOC_DEF( "NOP", "NOP.C", OPCL_IL,	0x08)	/* a NOP */
BR_DEF( "BSP","BSP.C",	OPCL_BC,	0)	/* Strictly Positive (~N & ~Z) */
BR_DEF( "BMZ","BMZ.C",	OPCL_BC,	1)	/* Minus or Zero ~(~N & ~Z) : (N | Z) */
BR_DEF( "BGT","BGT.C",	OPCL_BC,	2)
BR_DEF( "BLE","BLE.C",	OPCL_BC,	3)
BR_DEF( "BGE","BGE.C",	OPCL_BC,	4)
BR_DEF( "BLT","BLT.C",	OPCL_BC,	5)
BR_DEF( "BHI","BHI.C",	OPCL_BC,	6)	/* (unsigned > ) */
BR_DEF( "BLS","BLS.C",	OPCL_BC,	7)	/* Low | same (unsigned <= ) */
BR_DEF( "BCC","BCC.C",	OPCL_BC,	8)
BR_DEF( "BLO","BLO.C",	OPCL_BC,	8)	/* (unsigned < ) aka BCC */
BR_DEF( "BCS","BCS.C",	OPCL_BC,	9)
BR_DEF( "BHS","BHS.C",	OPCL_BC,	9)	/* High | same (unsigned >= ) aka BCS */
BR_DEF( "BPL","BPL.C",	OPCL_BC,	10)
BR_DEF( "BMI","BMI.C",	OPCL_BC,	11)
BR_DEF( "BNE","BNE.C",	OPCL_BC,	12)
BR_DEF( "BEQ","BEQ.C",	OPCL_BC,	13)
BR_DEF( "BVC","BVC.C",	OPCL_BC,	14)
BR_DEF( "BVS","BVS.C",	OPCL_BC,	15)
BR_DEF( "BUGE","BUGE.C",	OPCL_BC,	9) /* aka HS, aka BCS */
BR_DEF( "BUGT","BUGT.C",	OPCL_BC,	6) /* aka HI */
BR_DEF( "BULE","BULE.C",	OPCL_BC,	7) /* aka LS */
BR_DEF( "BULT","BULT.C",	OPCL_BC,	8) /* aka LO, aka BCC */

NOC_DEF( "BSR","BSR.C",	OPCL_BS,	0x02)	/* Also used for getpc */
NOC_DEF( "BRA","BRA.C",	OPCL_BC,	0x02)	/* BSR %0,target */
LS_DEF( "LEA","LEA.C",	OPCL_LD,	0x03,	2) /* load effective (int) address */
LS_DEF( "LEAS","LEAS.C",OPCL_LD,	0x04,	1)
OPCDEF( "SUBR","SUBR.C",OPCL_AU,	0x05)
OPCDEF( "XOR","XOR.C",	OPCL_AU,	0x06)
OPCDEF( "XORN","XORN.C",OPCL_AU,	0x07)

OPCDEF( "ADD","ADD.C",	OPCL_AU,	0x08)
OPCDEF( "SUB","SUB.C",	OPCL_AU,	0x09)
OPCDEF( "ADDC","ADDC.C",OPCL_AU,	0x0A)
OPCDEF( "SUBC","SUBC.C",OPCL_AU,	0x0B)
OPCDEF( "AND","AND.C",	OPCL_AU,	0x0C)
OPCDEF( "ANDN","ANDN.C",OPCL_AU,	0x0D)
OPCDEF( "OR","OR.C",	OPCL_AU,	0x0E)
OPCDEF( "ORN","ORN.C",	OPCL_AU,	0x0F)

LS_DEF( "LD","LD.C",	OPCL_LD,	0x10,	2)
LS_DEF( "LDS","LDS.C",	OPCL_LD,	0x11,	1)
LS_DEF( "LDUS","LDUS.C",OPCL_LD,	0x12,	1)
LS_DEF( "STS","STS.C",	OPCL_ST,	0x13,	1)
LS_DEF( "ST","ST.C",	OPCL_ST,	0x14,	2)
LS_DEF( "LDB","LDB.C",	OPCL_LD,	0x15,	0)
LS_DEF( "LDUB","LDUB.C",OPCL_LD,	0x16,	0)
LS_DEF( "STB","STB.C",	OPCL_ST,	0x17,	0)

OPCDEF( "ASHL","ASHL.C",OPCL_AU,	0x1A)
OPCDEF( "ASHR","ASHR.C",OPCL_AU,	0x18)
OPCDEF( "LSHR","LSHR.C",OPCL_AU,	0x19)
OPCDEF( "SHL","SHL.C",	OPCL_AU,	0x1A)
OPCDEF( "ROTL","ROTL.C",OPCL_AU,	0x1B)
NOC_DEF( "GETPS","GETPS.C",OPCL_PS,	0x1C)
NOC_DEF( "PUTPS","PUTPS.C",OPCL_PS,	0x1D)
LS_DEF( "JSR","JSR.C",	OPCL_JS,	0x1E,	2)
NOC_DEF( "SYSILL","SYSILL.C",OPCL_IL,	0x1F)	/* must be Illegal Op for bullet proofing!! */

#define B_OFF 1
	/* Word offset added to addr of BCC/BSR inst and offset in inst */
#define JSR_OFF 2
	/* word offset added to addr of JSR inst to form return addr */
#define INT_OFF 0
	/* word offset added to addr of jammed inst to form return addr for
	*  interrupts.
    */
#endif      /* _ASAP_OPS_H_ */

