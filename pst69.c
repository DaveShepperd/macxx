/*
    pst69.c - Part of macxx, a cross assembler family for various micro-processors
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


/*

    03/26/2022	- 6809 CPU support added by Tim Giddens
		  using David Shepperd's work as a reference

*/


#define MAC_69
#include "pst_tokens.h"
#include "psttkn69.h"

#define DIRDEF(name,func,flags) extern int func();

#include "dirdefs.h"

#undef DIRDEF



#define OPCDEF(name,clas,value,am) {name,value,OP_CLASS_OPC+(clas),am},


Opcpst perm_opcpst[] = {
OPCDEF("ABX",	00,		0x3A,	IMP	)		/* INHERENT */
OPCDEF("ADCA",	00,		0x89,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADCB",	00,		0xC9,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADDA",	00,		0x8B,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADDB",	00,		0xCB,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADDD",	00,		0xC3,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("ANDA",	00,		0x84,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ANDB",	00,		0xC4,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ANDCC",	00,		0x1C,	I	)		/* IMM */
OPCDEF("ASL",	00,		0x48,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("ASLA",	00,		0x48,	ACC	)		/* A */
OPCDEF("ASLB",	00,		0x58,	ACC	)		/* B */
OPCDEF("ASR",	00,		0x47,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("ASRA",	00,		0x47,	ACC	)		/* A */
OPCDEF("ASRB",	00,		0x57,	ACC	)		/* B */
OPCDEF("BCC",	00,		0x24,	S	)		/* REL */
OPCDEF("BCS",	00,		0x25,	S	)		/* REL */
OPCDEF("BEQ",	00,		0x27,	S	)		/* REL */
OPCDEF("BGE",	00,		0x2C,	S	)		/* REL */
OPCDEF("BGT",	00,		0x2E,	S	)		/* REL */
OPCDEF("BHI",	00,		0x22,	S	)		/* REL */
OPCDEF("BHIS",	00,		0x24,	S	)		/* REL */
OPCDEF("BHS",	00,		0x24,	S	)		/* REL */
OPCDEF("BITA",	00,		0x85,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("BITB",	00,		0xC5,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("BLE",	00,		0x2F,	S	)		/* REL */
OPCDEF("BLO",	00,		0x25,	S	)		/* REL */
OPCDEF("BLOS",	00,		0x23,	S	)		/* REL */
OPCDEF("BLS",	00,		0x23,	S	)		/* REL */
OPCDEF("BLT",	00,		0x2D,	S	)		/* REL */
OPCDEF("BMI",	00,		0x2B,	S	)		/* REL */
OPCDEF("BNE",	00,		0x26,	S	)		/* REL */
OPCDEF("BPL",	00,		0x2A,	S	)		/* REL */
OPCDEF("BRA",	00,		0x20,	S	)		/* REL */
OPCDEF("BRN",	00,		0x21,	S	)		/* REL */
OPCDEF("BSR",	00,		0x8D,	S	)		/* REL */
OPCDEF("BVC",	00,		0x28,	S	)		/* REL */
OPCDEF("BVS",	00,		0x29,	S	)		/* REL */
OPCDEF("CLR",	00,		0x4F,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("CLRA",	00,		0x4F,	ACC	)		/* A */
OPCDEF("CLRB",	00,		0x5F,	ACC	)		/* B */
OPCDEF("CMPA",	00,		0x81,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("CMPB",	00,		0xC1,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("CMPD",	00,		0x83,	MOST69|SPC|SP10	)	/* IMM, DIR, EXT, IND */
OPCDEF("CMPS",	00,		0x8C,	MOST69|SPC|SP11	)	/* IMM, DIR, EXT, IND */
OPCDEF("CMPU",	00,		0x83,	MOST69|SPC|SP11	)	/* IMM, DIR, EXT, IND */
OPCDEF("CMPX",	00,		0x8C,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("CMPY",	00,		0x8C,	MOST69|SPC|SP10	)	/* IMM, DIR, EXT, IND */
OPCDEF("COM",	00,		0x43,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("COMA",	00,		0x43,	ACC	)		/* A */
OPCDEF("COMB",	00,		0x53,	ACC	)		/* B */
OPCDEF("CPA",	00,		0x81,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("CPB",	00,		0xC1,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("CPD",	00,		0x83,	MOST69|SPC|SP10	)	/* IMM, DIR, EXT, IND */
OPCDEF("CPS",	00,		0x8C,	MOST69|SPC|SP11	)	/* IMM, DIR, EXT, IND */
OPCDEF("CPU",	00,		0x83,	MOST69|SPC|SP11	)	/* IMM, DIR, EXT, IND */
OPCDEF("CPX",	00,		0x8C,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("CPY",	00,		0x8C,	MOST69|SPC|SP10	)	/* IMM, DIR, EXT, IND */
OPCDEF("CWAI",	00,		0x3C,	I	)		/* IMM */
OPCDEF("DAA",	00,		0x19,	IMP	)		/* INHERENT */
OPCDEF("DEC",	00,		0x4A,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("DECA",	00,		0x4A,	ACC	)		/* A */
OPCDEF("DECB",	00,		0x5A,	ACC	)		/* B */
OPCDEF("EORA",	00,		0x88,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("EORB",	00,		0xC8,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("EXG",	00,		0x1E,	I	)		/* IMM */
OPCDEF("INC",	00,		0x4C,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("INCA",	00,		0x4C,	ACC	)		/* A */
OPCDEF("INCB",	00,		0x5C,	ACC	)		/* B */
OPCDEF("JMP",	00,		0x4E,	X|E|D|SPDP	)	/* DIR, EXT, IND */
OPCDEF("JSR",	00,		0x8D,	X|E|D	)		/* DIR, EXT, IND */
OPCDEF("LBCC",	00,		0x24,	LONGAM	)		/* REL */
OPCDEF("LBCS",	00,		0x25,	LONGAM	)		/* REL */
OPCDEF("LBEQ",	00,		0x27,	LONGAM	)		/* REL */
OPCDEF("LBGE",	00,		0x2C,	LONGAM	)		/* REL */
OPCDEF("LBGT",	00,		0x2E,	LONGAM	)		/* REL */
OPCDEF("LBHI",	00,		0x22,	LONGAM	)		/* REL */
OPCDEF("LBHIS",	00,		0x24,	LONGAM	)		/* REL */
OPCDEF("LBHS",	00,		0x24,	LONGAM	)		/* REL */
OPCDEF("LBLE",	00,		0x2F,	LONGAM	)		/* REL */
OPCDEF("LBLO",	00,		0x25,	LONGAM	)		/* REL */
OPCDEF("LBLOS",	00,		0x23,	LONGAM	)		/* REL */
OPCDEF("LBLS",	00,		0x23,	LONGAM	)		/* REL */
OPCDEF("LBLT",	00,		0x2D,	LONGAM	)		/* REL */
OPCDEF("LBMI",	00,		0x2B,	LONGAM	)		/* REL */
OPCDEF("LBNE",	00,		0x26,	LONGAM	)		/* REL */
OPCDEF("LBPL",	00,		0x2A,	LONGAM	)		/* REL */
OPCDEF("LBRA",	00,		0x16,	S|SPL	)		/* REL */
OPCDEF("LBRN",	00,		0x21,	LONGAM	)		/* REL */
OPCDEF("LBSR",	00,		0x17,	S|SPL	)		/* REL */
OPCDEF("LBVC",	00,		0x28,	LONGAM	)		/* REL */
OPCDEF("LBVS",	00,		0x29,	LONGAM	)		/* REL */
OPCDEF("LDA",	00,		0x86,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("LDB",	00,		0xC6,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("LDD",	00,		0xCC,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("LDS",	00,		0xCE,	MOST69|SPC|SP10	)	/* IMM, DIR, EXT, IND */
OPCDEF("LDU",	00,		0xCE,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("LDX",	00,		0x8E,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("LDY",	00,		0x8E,	MOST69|SPC|SP10	)	/* IMM, DIR, EXT, IND */
OPCDEF("LEAS",	00,		0x12,	X	)		/* IND */
OPCDEF("LEAU",	00,		0x13,	X	)		/* IND */
OPCDEF("LEAX",	00,		0x10,	X	)		/* IND */
OPCDEF("LEAY",	00,		0x11,	X	)		/* IND */
OPCDEF("LSL",	00,		0x48,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("LSLA",	00,		0x48,	ACC	)		/* A */
OPCDEF("LSLB",	00,		0x58,	ACC	)		/* B */
OPCDEF("LSR",	00,		0x44,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("LSRA",	00,		0x44,	ACC	)		/* A */
OPCDEF("LSRB",	00,		0x54,	ACC	)		/* B */
OPCDEF("MUL",	00,		0x3D,	IMP	)		/* INHERENT */
OPCDEF("NEG",	00,		0x40,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("NEGA",	00,		0x40,	ACC	)		/* A */
OPCDEF("NEGB",	00,		0x50,	ACC	)		/* B */
OPCDEF("NOP",	00,		0x12,	IMP	)		/* INHERENT */
OPCDEF("ORA",	00,		0x8A,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ORB",	00,		0xCA,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("ORCC",	00,		0x1A,	I	)		/* IMM */
OPCDEF("PSHS",	00,		0x34,	I	)		/* IMM */
OPCDEF("PSHU",	00,		0x36,	I	)		/* IMM */
OPCDEF("PULS",	00,		0x35,	I	)		/* IMM */
OPCDEF("PULU",	00,		0x37,	I	)		/* IMM */
OPCDEF("ROL",	00,		0x49,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("ROLA",	00,		0x49,	ACC	)		/* A */
OPCDEF("ROLB",	00,		0x59,	ACC	)		/* B */
OPCDEF("ROR",	00,		0x46,	SHFTAM	)		/* DIR, EXT, IND */
OPCDEF("RORA",	00,		0x46,	ACC	)		/* A */
OPCDEF("RORB",	00,		0x56,	ACC	)		/* B */
OPCDEF("RTI",	00,		0x3B,	IMP	)		/* INHERENT */
OPCDEF("RTS",	00,		0x39,	IMP	)		/* INHERENT */
OPCDEF("SBCA",	00,		0x82,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("SBCB",	00,		0xC2,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("SEX",	00,		0x1D,	IMP	)		/* INHERENT */
OPCDEF("STA",	00,		0x87,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STB",	00,		0xC7,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STD",	00,		0xCD,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STS",	00,		0xCF,	D|X|E|DES|SPC10	)	/* DIR, EXT, IND */
OPCDEF("STU",	00,		0xCF,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STX",	00,		0x8F,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STY",	00,		0x8F,	D|X|E|DES|SPC10	)	/* DIR, EXT, IND */
OPCDEF("SUBA",	00,		0x80,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("SUBB",	00,		0xC0,	MOST69	)		/* IMM, DIR, EXT, IND */
OPCDEF("SUBD",	00,		0x83,	MOST69|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("SWI",	00,		0x3F,	IMP	)		/* INHERENT */
OPCDEF("SWI1",	00,		0x3F,	IMP	)		/* INHERENT */
OPCDEF("SWI2",	00,		0x3F,	SPC10	)		/* INHERENT */
OPCDEF("SWI3",	00,		0x3F,	SPC11	)		/* INHERENT */
OPCDEF("SYNC",	00,		0x13,	IMP	)		/* INHERENT */
OPCDEF("TFR",	00,		0x1F,	I	)		/* IMM */
OPCDEF("TST",	00,		0x4D,	X|E|D|SPDP	)	/* DIR, EXT, IND */
OPCDEF("TSTA",	00,		0x4D,	ACC	)		/* A */
OPCDEF("TSTB",	00,		0x5D,	ACC	)		/* B */
   	{ 0,0,0,0 }
};



#define DIRDEF(name,func,flags) {name,func,flags},

Dirpst perm_dirpst[] = {

#include "dirdefs.h"

   	{ 0,0,0 }
};





