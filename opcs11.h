/*
    opcs11.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _OPCS11_H_
#define _OPCS11_H_ 1

/* (Name, class, base, bwl) */
/* Class 0 = no operands */
/* Class 1 = dst only, can be expression */
/* Class 2 = dst only, can only be register */
/* Class 3 = dst only, can only be number from 0-7 */
/* Class 4 = dst only, can only be number from 0-63 */
/* Class 5 = Branch */
/* Class 6 = optional src must be register, dst can be expression (JSR) */
/* Class 7 = src can be expression, dst can be expression */
/* Class 8 = src can be expression, dst can only be register (s is swapped with d) */
/* Class 9 = src is register, dst is branch addr (SOB) */
/* Class 10 = dst only, can only be number from 0-255 */
/* Class 11 = src is register only, dest is expression (XOR) */
/* Class 12 = same as class 1 except dst could be an immediate */
OPDEF("HALT",   0, 0000000, 0 )
OPDEF("WAIT",   0, 0000001, 0 )
OPDEF("RTI",    0, 0000002, 0 )
OPDEF("BPT",    0, 0000003, 0 )
OPDEF("IOT",    0, 0000004, 0 )
OPDEF("RESET",  0, 0000005, 0 )
OPDEF("RTT",    0, 0000006, 0 )
OPDEF("JMP",    1, 0000100, 0 )
OPDEF("RTS",    2, 0000200, 0 )
OPDEF("SPL",    3, 0000230, 0 )
OPDEF("NOP",    0, 0000240, 0 )
OPDEF("CLC",    0, 0000241, 0 )
OPDEF("CLV",    0, 0000242, 0 )
OPDEF("CLZ",    0, 0000244, 0 )
OPDEF("CLN",    0, 0000250, 0 )
OPDEF("CCC",    0, 0000257, 0 )
OPDEF("SEC",    0, 0000261, 0 )
OPDEF("SEV",    0, 0000262, 0 )
OPDEF("SEZ",    0, 0000264, 0 )
OPDEF("SEN",    0, 0000270, 0 )
OPDEF("SCC",    0, 0000277, 0 )
OPDEF("SWAB",   1, 0000300, 0 )
OPDEF("BR",     5, 0000400, 0 )
OPDEF("BNE",    5, 0001000, 0 )
OPDEF("BEQ",    5, 0001400, 0 )
OPDEF("BGE",    5, 0002000, 0 )
OPDEF("BLT",    5, 0002400, 0 )
OPDEF("BGT",    5, 0003000, 0 )
OPDEF("BLE",    5, 0003400, 0 )
OPDEF("JSR",    6, 0004000, 0 )
OPDEF("CLR",    1, 0005000, 0 )
OPDEF("COM",    1, 0005100, 0 )
OPDEF("INC",    1, 0005200, 0 )
OPDEF("DEC",    1, 0005300, 0 )
OPDEF("NEG",    1, 0005400, 0 )
OPDEF("ADC",    1, 0005500, 0 )
OPDEF("SBC",    1, 0005600, 0 )
OPDEF("TST",    1, 0005700, 0 )
OPDEF("ROR",    1, 0006000, 0 )
OPDEF("ROL",    1, 0006100, 0 )
OPDEF("ASR",    1, 0006200, 0 )
OPDEF("ASL",    1, 0006300, 0 )
OPDEF("MARK",   4, 0006400, 0 )
OPDEF("MFPI",   1, 0006500, 0 )
OPDEF("MTPI",  12, 0006600, 0 )
OPDEF("SXT",    1, 0006700, 0 )
OPDEF("MOV",    7, 0010000, 0 )
OPDEF("CMP",    7, 0020000, 0 )
OPDEF("BIT",    7, 0030000, 0 )
OPDEF("BIC",    7, 0040000, 0 )
OPDEF("BIS",    7, 0050000, 0 )
OPDEF("ADD",    7, 0060000, 0 )
OPDEF("MUL",    8, 0070000, 0 )
OPDEF("DIV",    8, 0071000, 0 )
OPDEF("ASH",    8, 0072000, 0 )
OPDEF("ASHC",   8, 0073000, 0 )
OPDEF("XOR",   11, 0074000, 0 )
OPDEF("FADD",   2, 0075000, 0 )
OPDEF("FSUB",   2, 0075010, 0 )
OPDEF("FMUL",   2, 0075020, 0 )
OPDEF("FDIV",   2, 0075030, 0 )
OPDEF("SOB",    9, 0077000, 0 )
OPDEF("BPL",    5, 0100000, 0 )
OPDEF("BMI",    5, 0100400, 0 )
OPDEF("BHI",    5, 0101000, 0 )
OPDEF("BLOS",   5, 0101400, 0 )
OPDEF("BVC",    5, 0102000, 0 )
OPDEF("BVS",    5, 0102400, 0 )
OPDEF("BCC",    5, 0103000, 0 )
OPDEF("BHIS",   5, 0103000, 0 )
OPDEF("BCS",    5, 0103400, 0 )
OPDEF("BLO",    5, 0103400, 0 )
OPDEF("EMT",   10, 0104000, 0 )
OPDEF("TRAP",  10, 0104400, 0 )
OPDEF("CLRB",   1, 0105000, 0 )
OPDEF("COMB",   1, 0105100, 0 )
OPDEF("INCB",   1, 0105200, 0 )
OPDEF("DECB",   1, 0105300, 0 )
OPDEF("NEGB",   1, 0105400, 0 )
OPDEF("ADCB",   1, 0105500, 0 )
OPDEF("SBCB",   1, 0105600, 0 )
OPDEF("TSTB",   1, 0105700, 0 )
OPDEF("RORB",   1, 0106000, 0 )
OPDEF("ROLB",   1, 0106100, 0 )
OPDEF("ASRB",   1, 0106200, 0 )
OPDEF("ASLB",   1, 0106300, 0 )
OPDEF("MTPS",  12, 0106400, 0 )
OPDEF("MFPD",   1, 0106500, 0 )
OPDEF("MTPD",  12, 0106600, 0 )
OPDEF("MFPS",   1, 0106700, 0 )
OPDEF("MOVB",   7, 0110000, 0 )
OPDEF("CMPB",   7, 0120000, 0 )
OPDEF("BITB",   7, 0130000, 0 )
OPDEF("BICB",   7, 0140000, 0 )
OPDEF("BISB",   7, 0150000, 0 )
OPDEF("SUB",    7, 0160000, 0 )

#endif /* _OPCS11_H_ */
