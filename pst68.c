/*
	pst68.c - Part of macxx, a cross assembler family for various micro-processors
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

 *  TG -  edited Dave Shepperd code 12/07/2022 *
 */


/******************************************************************************
Change Log

    03/18/2022	- 6800 CPU support added by Tim Giddens
		  using David Shepperd's work as a reference

******************************************************************************/

#define MAC_68
#include "pst_tokens.h"
#include "psttkn68.h"

#define DIRDEF(name,func,flags) extern int func();

#include "dirdefs.h"

#undef DIRDEF




/*

TG Note:
Looks like BIT 15 of clas must be set for pass1.c to see it as an opcode
pass1.c will call do_opcode(opc) if bit 15 of clas is set

OP_CLASS_OPC is set to 0x8000 in file pst_tokens.h
OPCLPBR=0x0004,	- use program bank reg - in file pst_tokens.h


clas = 0x00 = IMP or ACC = no oprands

clas = 0x03 = S = REL = Branch = one oprand

*/


/*
#define OPCDEF(name,clas,value,am,aam,aaam) {name,value,OP_CLASS_OPC+(clas),am,aam,aaam},
*/

#define OPCDEF(name,clas,value,am) {name,value,OP_CLASS_OPC+(clas),am},



Opcpst perm_opcpst[] = {
OPCDEF("ABA",	00,		0x1B,	IMP	)		/* INHERENT */
OPCDEF("ADCA",	00,		0x89,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADCB",	00,		0xC9,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADDA",	00,		0x8B,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ADDB",	00,		0xCB,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ANDA",	00,		0x84,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ANDB",	00,		0xC4,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ASL",	00,		0x48,	X|E|DES	)		/* EXT, IND */
OPCDEF("ASLA",	00,		0x48,	ACC	)		/* A */
OPCDEF("ASLB",	00,		0x58,	ACC	)		/* B */
OPCDEF("ASR",	00,		0x47,	X|E|DES	)		/* EXT, IND */
OPCDEF("ASRA",	00,		0x47,	ACC	)		/* A */
OPCDEF("ASRB",	00,		0x57,	ACC	)		/* B */
OPCDEF("BCC",	00,		0x24,	S	)		/* REL */
OPCDEF("BCS",	00,		0x25,	S	)		/* REL */
OPCDEF("BEQ",	00,		0x27,	S	)		/* REL */
OPCDEF("BGE",	00,		0x2C,	S	)		/* REL */
OPCDEF("BGT",	00,		0x2E,	S	)		/* REL */
OPCDEF("BHI",	00,		0x22,	S	)		/* REL */
OPCDEF("BITA",	00,		0x85,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("BITB",	00,		0xC5,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("BLE",	00,		0x2F,	S	)		/* REL */
OPCDEF("BLS",	00,		0x23,	S	)		/* REL */
OPCDEF("BLT",	00,		0x2D,	S	)		/* REL */
OPCDEF("BMI",	00,		0x2B,	S	)		/* REL */
OPCDEF("BNE",	00,		0x26,	S	)		/* REL */
OPCDEF("BPL",	00,		0x2A,	S	)		/* REL */
OPCDEF("BRA",	00,		0x20,	S	)		/* REL */
OPCDEF("BSR",	00,		0x8D,	S	)		/* REL */
OPCDEF("BVC",	00,		0x28,	S	)		/* REL */
OPCDEF("BVS",	00,		0x29,	S	)		/* REL */
OPCDEF("CBA",	00,		0x11,	IMP	)		/* INHERENT */
OPCDEF("CLC",	00,		0x0C,	IMP	)		/* INHERENT */
OPCDEF("CLI",	00,		0x0E,	IMP	)		/* INHERENT */
OPCDEF("CLR",	00,		0x4F,	X|E|DES	)		/* EXT, IND */
OPCDEF("CLRA",	00,		0x4F,	ACC	)		/* A */
OPCDEF("CLRB",	00,		0x5F,	ACC	)		/* B */
OPCDEF("CLV",	00,		0x0A,	IMP	)		/* INHERENT */
OPCDEF("CMPA",	00,		0x81,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("CMPB",	00,		0xC1,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("COM",	00,		0x43,	X|E|DES	)		/* EXT, IND */
OPCDEF("COMA",	00,		0x43,	ACC	)		/* A */
OPCDEF("COMB",	00,		0x53,	ACC	)		/* B */
OPCDEF("CPX",	00,		0x8C,	MOST68|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("DAA",	00,		0x19,	IMP	)		/* INHERENT */
OPCDEF("DEC",	00,		0x4A,	X|E|DES	)		/* EXT, IND */
OPCDEF("DECA",	00,		0x4A,	ACC	)		/* A */
OPCDEF("DECB",	00,		0x5A,	ACC	)		/* B */
OPCDEF("DES",	00,		0x34,	IMP	)		/* INHERENT */
OPCDEF("DEX",	00,		0x09,	IMP	)		/* INHERENT */
OPCDEF("EORA",	00,		0x88,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("EORB",	00,		0xC8,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("INC",	00,		0x4C,	X|E|DES	)		/* EXT, IND */
OPCDEF("INCA",	00,		0x4C,	ACC	)		/* A */
OPCDEF("INCB",	00,		0x5C,	ACC	)		/* B */
OPCDEF("INS",	00,		0x31,	IMP	)		/* INHERENT */
OPCDEF("INX",	00,		0x08,	IMP	)		/* INHERENT */
OPCDEF("JMP",	00,		0x4E,	X|E	)		/* EXT, IND */
OPCDEF("JSR",	00,		0x8D,	X|E	)		/* EXT, IND */
OPCDEF("LDAA",	00,		0x86,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("LDAB",	00,		0xC6,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("LDS",	00,		0x8E,	MOST68|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("LDX",	00,		0xCE,	MOST68|SPC	)	/* IMM, DIR, EXT, IND */
OPCDEF("LSR",	00,		0x44,	X|E|DES	)		/* EXT, IND */
OPCDEF("LSRA",	00,		0x44,	ACC	)		/* A */
OPCDEF("LSRB",	00,		0x54,	ACC	)		/* B */
OPCDEF("NEG",	00,		0x40,	X|E|DES	)		/* EXT, IND */
OPCDEF("NEGA",	00,		0x40,	ACC	)		/* A */
OPCDEF("NEGB",	00,		0x50,	ACC	)		/* B */
OPCDEF("NOP",	00,		0x01,	IMP	)		/* INHERENT */
OPCDEF("ORAA",	00,		0x8A,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("ORAB",	00,		0xCA,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("PSHA",	00,		0x36,	ACC	)		/* A */
OPCDEF("PSHB",	00,		0x37,	ACC	)		/* B */
OPCDEF("PULA",	00,		0x32,	ACC	)		/* A */
OPCDEF("PULB",	00,		0x33,	ACC	)		/* B */
OPCDEF("ROL",	00,		0x49,	X|E|DES	)		/* EXT, IND */
OPCDEF("ROLA",	00,		0x49,	ACC	)		/* A */
OPCDEF("ROLB",	00,		0x59,	ACC	)		/* B */
OPCDEF("ROR",	00,		0x46,	X|E|DES	)		/* EXT, IND */
OPCDEF("RORA",	00,		0x46,	ACC	)		/* A */
OPCDEF("RORB",	00,		0x56,	ACC	)		/* B */
OPCDEF("RTI",	00,		0x3B,	IMP	)		/* INHERENT */
OPCDEF("RTS",	00,		0x39,	IMP	)		/* INHERENT */
OPCDEF("SBA",	00,		0x10,	IMP	)		/* INHERENT */
OPCDEF("SBCA",	00,		0x82,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("SBCB",	00,		0xC2,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("SEC",	00,		0x0D,	IMP	)		/* INHERENT */
OPCDEF("SEI",	00,		0x0F,	IMP	)		/* INHERENT */
OPCDEF("SEV",	00,		0x0B,	IMP	)		/* INHERENT */
OPCDEF("STAA",	00,		0x87,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STAB",	00,		0xC7,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STS",	00,		0x8F,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("STX",	00,		0xCF,	D|X|E|DES	)	/* DIR, EXT, IND */
OPCDEF("SUBA",	00,		0x80,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("SUBB",	00,		0xC0,	MOST68	)		/* IMM, DIR, EXT, IND */
OPCDEF("SWI",	00,		0x3F,	IMP	)		/* INHERENT */
OPCDEF("TAB",	00,		0x16,	IMP	)		/* INHERENT */
OPCDEF("TAP",	00,		0x06,	IMP	)		/* INHERENT */
OPCDEF("TBA",	00,		0x17,	IMP	)		/* INHERENT */
OPCDEF("TPA",	00,		0x07,	IMP	)		/* INHERENT */
OPCDEF("TST",	00,		0x4D,	X|E	)		/* EXT, IND */
OPCDEF("TSTA",	00,		0x4D,	ACC	)		/* A */
OPCDEF("TSTB",	00,		0x5D,	ACC	)		/* B */
OPCDEF("TSX",	00,		0x30,	IMP	)		/* INHERENT */
OPCDEF("TXS",	00,		0x35,	IMP	)		/* INHERENT */
OPCDEF("WAI",	00,		0x3E,	IMP	)		/* INHERENT */



   	{ 0,0,0,0 }
};



/*
TG Note:
Add all of the MACxx .xxxxx commands from file psuedo_ops.c
*/


#define DIRDEF(name,func,flags) {name,func,flags},

Dirpst perm_dirpst[] = {

#include "dirdefs.h"

   	{ 0,0,0 }
};





