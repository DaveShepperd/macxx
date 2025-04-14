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

    04/10/2025	- 8080 CPU support added by Dave Shepperd

******************************************************************************/

#include "pst_tokens.h"
#include "psttkn8080.h"

#if !defined(MAC_8080)
	#define MAC_8080
#endif

#define DIRDEF(name,func,flags) extern int func();

#include "dirdefs.h"

#undef DIRDEF

/* Class, except for the OP_CLASS_OPC bit, isn't used in this module */
/* Instead, both dst and src allowable address modes are encoded in the one amodes field */
/* The 8080 instruction syntax is: op dst,src */
#define OPCDEF(name, value, dstam, srcam) {name,value,OP_CLASS_OPC,(srcam)|(dstam<<16)},

Opcpst perm_opcpst[] =
{
/* Sorted by opcode value */
OPCDEF("NOP",	0x00 /*0b00000000*/, NO, NO )	/* NOP */
OPCDEF("LXI",	0x01 /*0b00000001*/, RP, D16 )	/* LXI RP,data (16 bit) */
OPCDEF("STAX",	0x02 /*0b00000010*/, BD, NO )	/* STAX RP */
OPCDEF("INX",	0x03 /*0b00000011*/, RP, NO )	/* INX RP */
OPCDEF("INR",	0x04 /*0b00000100*/, R, NO )	/* INR R */
OPCDEF("DCR",	0x05 /*0b00000101*/, R, NO )	/* DCR R */
OPCDEF("MVI",	0x06 /*0b00000110*/, R, D8 )	/* MVI R, data (8 bit) */
OPCDEF("RLC",	0x07 /*0b00000111*/, NO, NO )	/* RLC */
OPCDEF("DAD",	0x09 /*0b00001001*/, RP, NO )	/* DAD RP */
OPCDEF("LDAX",	0x0A /*0b00001010*/, BD, NO )	/* LDAX RP */
OPCDEF("DCX",	0x0B /*0b00001011*/, RP, NO )	/* DCX RP */
OPCDEF("RRC",	0x0F /*0b00001111*/, NO, NO )	/* RRC */
OPCDEF("RAL",	0x17 /*0b00010111*/, NO, NO )	/* RAL */
OPCDEF("RAR",	0x1F /*0b00011111*/, NO, NO )	/* RAR */
OPCDEF("SHLD", 	0x22 /*0b00100010*/, NO, D16 )	/* SHLD addr */
OPCDEF("DAA",	0x27 /*0b00100111*/, NO, NO )	/* DAA */
OPCDEF("LHLD",	0x2A /*0b00101010*/, NO, D16 )	/* LHLD addr */
OPCDEF("CMA",	0x2F /*0b00101111*/, NO, NO )	/* CMA */
OPCDEF("STA", 	0x32 /*0b00110010*/, NO, D16 )	/* STA addr */
OPCDEF("STC",	0x37 /*0b00110111*/, NO, NO )	/* STC */
OPCDEF("LDA", 	0x3A /*0b00111010*/, NO, D16 )	/* LDA addr */
OPCDEF("CMC",	0x3F /*0b00111111*/, NO, NO )	/* CMC */
OPCDEF("MOV",	0x40 /*0b01000000*/, R, R )		/* MOV r,r */
OPCDEF("HLT",	0x76 /*0b01110110*/, NO, NO )	/* HLT */
OPCDEF("ADD",	0x80 /*0b10000000*/, NO, R )	/* ADD R */
OPCDEF("ADC",	0x88 /*0b10001000*/, NO, R )	/* ADC R */
OPCDEF("SUB",	0x90 /*0b10010000*/, NO, R )	/* SUB R */
OPCDEF("SBB",	0x98 /*0b10011000*/, NO, R )	/* SBB R */
OPCDEF("ANA",	0xA0 /*0b10100000*/, NO, R )	/* ANA R */
OPCDEF("XRA",	0xA8 /*0b10101000*/, NO, R )	/* XRA R */
OPCDEF("ORA",	0xB0 /*0b10110000*/, NO, R )	/* ORA R */
OPCDEF("CMP",	0xB8 /*0b10111000*/, NO, R )	/* CMP R */
OPCDEF("RNZ",	0xC0 /*0b11000000*/, NO, NO )	/* RNZ */
OPCDEF("POP",	0xC1 /*0b11000001*/, PSW, NO )	/* POP RP */
OPCDEF("JNZ",	0xC2 /*0b11000010*/, NO, D16 )	/* JNZ addr (16 bit) */
OPCDEF("JMP",	0xC3 /*0b11000011*/, NO, D16 )	/* JMP addr (16 bit) */
OPCDEF("CNZ",	0xC4 /*0b11000100*/, NO, D16 )	/* CNZ addr (16 bit) */
OPCDEF("PUSH",	0xC5 /*0b11000101*/, PSW, NO )	/* PUSH RP (special RP) */
OPCDEF("ADI", 	0xC6 /*0b11000110*/, NO, D8 )	/* ADI data (8 bit) */
OPCDEF("RST", 	0xC7 /*0b11000111*/, D3, NO )	/* RST num */
OPCDEF("RZ",	0xC8 /*0b11001000*/, NO, NO )	/* RZ */
OPCDEF("RET",	0xC9 /*0b11001001*/, NO, NO )	/* RET */
OPCDEF("JZ",	0xCA /*0b11001010*/, NO, D16 )	/* JZ addr (16 bit) */
OPCDEF("CZ",	0xCC /*0b11001100*/, NO, D16 )	/* CZ addr (16 bit) */
OPCDEF("CALL",	0xCD /*0b11001101*/, NO, D16 )	/* CALL addr (16 bit) */
OPCDEF("ACI",	0xCE /*0b11001110*/, NO, D8 )	/* ACI data (8 bit) */
OPCDEF("RNC",	0xD0 /*0b11010000*/, NO, NO )	/* RNC */
OPCDEF("JNC",	0xD2 /*0b11010010*/, NO, D16 )	/* JNC addr (16 bit) */
OPCDEF("OUT",	0xD3 /*0b11010011*/, NO, D8 )	/* OUT addr (8 bit) */
OPCDEF("CNC",	0xD4 /*0b11010100*/, NO, D16 )	/* CNC addr (16 bit) */
OPCDEF("SUI",	0xD6 /*0b11010110*/, NO, D8 )	/* SUI data (8 bit) */
OPCDEF("RC",	0xD8 /*0b11011000*/, NO, NO )	/* RC */
OPCDEF("JC",	0xDA /*0b11011010*/, NO, D16 )	/* JC addr (16 bit) */
OPCDEF("IN",	0xDB /*0b11011101*/, NO, D8 )	/* IN addr (8 bit) */
OPCDEF("CC",	0xDC /*0b11011100*/, NO, D16 )	/* CC addr (16 bit) */
OPCDEF("SBI",	0xDE /*0b11011110*/, NO, D8 )	/* SBI data (8 bit) */
OPCDEF("RPO",	0xE0 /*0b11100000*/, NO, NO )	/* RPO */
OPCDEF("JPO",	0xE2 /*0b11100010*/, NO, D16 )	/* JPO addr (16 bit) */
OPCDEF("XTHL",	0xE3 /*0b11100011*/, NO, NO )	/* XTHL */
OPCDEF("CPO",	0xE4 /*0b11100100*/, NO, D16 )	/* CPO addr (16 bit) */
OPCDEF("ANI",	0xE6 /*0b11100110*/, NO, D8 )	/* ANI data (8 bit) */
OPCDEF("RPE",	0xE8 /*0b11101000*/, NO, NO )	/* RPE */
OPCDEF("PCHL",	0xE9 /*0b11101001*/, NO, NO )	/* PCHL */
OPCDEF("JPE",	0xEA /*0b11101010*/, NO, D16 )	/* JPE addr (16 bit) */
OPCDEF("XCHG",	0xEB /*0b11101011*/, NO, NO )	/* XCHG */
OPCDEF("CPE",	0xEC /*0b11101100*/, NO, D16 )	/* CPE addr (16 bit) */
OPCDEF("XRI",	0xEE /*0b11101110*/, NO, D8 )	/* XRI data (8 bit) */
OPCDEF("RP",	0xF0 /*0b11110000*/, NO, NO )	/* RP */
OPCDEF("JP",	0xF2 /*0b11110010*/, NO, D16 )	/* JP addr (16 bit) */
OPCDEF("DI",	0xF3 /*0b11110011*/, NO, NO )	/* DI */
OPCDEF("CP",	0xF4 /*0b11110100*/, NO, D16 )	/* CP addr (16 bit) */
OPCDEF("ORI",	0xF6 /*0b11110110*/, NO, D8 )	/* ORI data (8 bit) */
OPCDEF("RM",	0xF8 /*0b11111000*/, NO, NO )	/* RM */
OPCDEF("SPHL",	0xF9 /*0b11111001*/, NO, NO )	/* SPHL */
OPCDEF("JM",	0xFA /*0b11111010*/, NO, D16 )	/* JM addr (16 bit) */
OPCDEF("EI",	0xFB /*0b11111011*/, NO, NO )	/* EI */
OPCDEF("CM",	0xFC /*0b11111100*/, NO, D16 )	/* CM addr (16 bit) */
OPCDEF("CPI",	0xFE /*0b11111110*/, NO, D8 )	/* CPI data (8 bit) */

OPCDEF(0, 0,  0, 0 )		/* End of list */
};

#define DIRDEF(name,func,flags) {name,func,flags},

Dirpst perm_dirpst[] = {

#include "dirdefs.h"

   	{ 0,0,0 }
};





