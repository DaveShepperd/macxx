/*
	pst8080.c - Part of macxx, a cross assembler family for various micro-processors
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

#include "pst_tokens.h"
#include "psttknz80.h"

#if !defined(MAC_Z80)
	#define MAC_Z80 (1)
#endif

#define DIRDEF(name,func,flags) extern int func();

#include "dirdefs.h"

#undef DIRDEF

#define OPCDEF(name, value, class) {name,value,OP_CLASS_OPC+class, 0},

Opcpst perm_opcpst[] =
{
OPCDEF("LD",    0x00, CLASS_LD)
OPCDEF("MOV",   0x00, CLASS_LD)
OPCDEF("PUSH",	0x00, CLASS_PSH)
OPCDEF("POP",	0x01, CLASS_POP)
OPCDEF("EX",	0x00, CLASS_EX)
OPCDEF("EXX",	0xD9, CLASS_NO)
OPCDEF("LDI", 0xA0ED, CLASS_NO)
OPCDEF("LDIR",0xB0ED, CLASS_NO)
OPCDEF("LDD", 0xA8ED, CLASS_NO)
OPCDEF("LDDR",0xB8ED, CLASS_NO)
OPCDEF("CPI", 0xA1ED, CLASS_NO)
OPCDEF("CPIR",0xB1ED, CLASS_NO)
OPCDEF("CPD", 0xA9ED, CLASS_NO)
OPCDEF("CPDR",0xB9ED, CLASS_NO)
OPCDEF("ADD",   0x00, CLASS_ADD)
OPCDEF("ADC",   0x00, CLASS_ADC)
OPCDEF("SUB",   0x00, CLASS_SUB)
OPCDEF("SBC",   0x00, CLASS_SBC)
OPCDEF("AND",   0x00, CLASS_AND)
OPCDEF("OR",    0x00, CLASS_OR)
OPCDEF("XOR",   0x00, CLASS_XOR)
OPCDEF("CP",    0x00, CLASS_CP)
OPCDEF("INC",   0x00, CLASS_INC)
OPCDEF("DEC",   0x00, CLASS_DEC)
OPCDEF("DAA",	0x27, CLASS_NO)
OPCDEF("CPL",	0x2F, CLASS_NO)
OPCDEF("NEG", 0x44ED, CLASS_NO)
OPCDEF("CCF",	0x3F, CLASS_NO)
OPCDEF("SCF",	0x37, CLASS_NO)
OPCDEF("NOP",	0x00, CLASS_NO)
OPCDEF("HALT",	0x76, CLASS_NO)
OPCDEF("DI",	0xF3, CLASS_NO)
OPCDEF("EI",	0xFB, CLASS_NO)
OPCDEF("IM",	0x00, CLASS_IM)
OPCDEF("RL",	0x00, CLASS_RL)
OPCDEF("RLA",	0x17, CLASS_NO)
OPCDEF("RLC",	0x00, CLASS_RLC)
OPCDEF("RLCA",	0x07, CLASS_NO)
OPCDEF("RR",	0x00, CLASS_RR)
OPCDEF("RRA",	0x1F, CLASS_NO)
OPCDEF("RRC",	0x00, CLASS_RRC)
OPCDEF("RRCA",	0x0F, CLASS_NO)
OPCDEF("SLA",	0x00, CLASS_SLA)
OPCDEF("SRA",	0x00, CLASS_SRA)
OPCDEF("SRL",	0x00, CLASS_SRL)
OPCDEF("RLD", 0x6FED, CLASS_NO)
OPCDEF("RRD", 0x67ED, CLASS_NO)
OPCDEF("BIT",   0x00, CLASS_BIT)
OPCDEF("SET",   0x00, CLASS_SET)
OPCDEF("RES",   0x00, CLASS_RES)
OPCDEF("JP",    0x00, CLASS_JP)
OPCDEF("JR",    0x00, CLASS_JR)
OPCDEF("BR",    0x00, CLASS_JR)
OPCDEF("DJNZ",  0x00, CLASS_DJNZ)
OPCDEF("CALL",  0x00, CLASS_CALL)
OPCDEF("RET",   0x00, CLASS_RET)
OPCDEF("RETI",0x4DED, CLASS_NO)
OPCDEF("RETN",0x45ED, CLASS_NO)
OPCDEF("RST", 	0xC7, CLASS_RST)
OPCDEF("IN",    0x00, CLASS_IN)
OPCDEF("OUT",   0x00, CLASS_OUT)
OPCDEF("INI", 0xA2ED, CLASS_NO)
OPCDEF("INIR",0xB2ED, CLASS_NO)
OPCDEF("IND", 0xAAED, CLASS_NO)
OPCDEF("INDR",0xBAED, CLASS_NO)
OPCDEF("OUTI",0xA3ED, CLASS_NO)
OPCDEF("OTIR",0xB3ED, CLASS_NO)
OPCDEF("OTDR",0xBBED, CLASS_NO)
OPCDEF("OUTD",0xABED, CLASS_NO)
OPCDEF("OTDR",0xBBED, CLASS_NO)
OPCDEF(0, 0,  0 )		/* End of list */
};

#define DIRDEF(name,func,flags) {name,func,flags},

Dirpst perm_dirpst[] = {

#include "dirdefs.h"

   	{ 0,0,0 }
};





