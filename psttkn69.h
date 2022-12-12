/*
    psttkn69.h - Part of macxx, a cross assembler family for various micro-processors
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

    03/26/2022	- 6809 CPU support added by Tim Giddens
		  using David Shepperd's work as a reference

******************************************************************************/


#ifndef _PSTTKN69_H_
#define _PSTTKN69_H_ 1



/*

Eight 6809 Addressing modes
I   - IMM - Immediate
ACC - ACC - Accumulator
D   - DIR - Direct (Absolute Zero Page)
E   - EXT - Extended (Absolute)
X   - IND - Indexed
IMP - INH - Implied (Inherent)
S   - REL - Relative
SPC -     - Two Byte Immediate Mode

The opcode syntax for the 6809 assembler is as follows:

LABEL:	OPCODE	OPERAND		;Comments

The available syntax of the operand(s) is as follows:

null		;Inherent mode (i.e. ABX)
label		;PC relative
@#		;extended mode
#data		;immediate mode
(r)		;Register indirect (0 offset)
@(r)		;Indirect register indirect
(r)+		;Register indirect auto-increment by 1
(r)++		;Register indirect auto-increment by 2
@(r)+		;Indirect register indirect auto-increment by 2
@(r)++		;Indirect register indirect auto-increment by 2
n(r)		;Register indexed. (i.e. EA = n + r where:
		;   n => accumulator A,B or D
		;   n => any other expression)
@n(r)		;Indexed indirect. [i.e. EA = (n + r) where:
		;   n => accumulator A,B or D or
		;   n => any other expression]
-(r)		;Auto-decrement by 1. Register indirect.
--(r)		;Auto-decrement by 2. Register indirect.
@-(r)		;Auto-decrement by 2. Indirect register indirect.
@--(r)		;Auto-decrement by 2. Indirect register indirect.

'r' in the above examples may be PC,X,Y,U or S (or the equivalent).

The following is an alternate form of input to specify specific types
of addressing modes:

E,label		;Extended mode
NE,label	;indirect extended
I,data		;Immediate mode
D,label		;Direct page mode
n,label(r)	;and
n,@label(r)	;If n = 5 then forces offset to 5 bit code
		;If n = 8 then forces offset to 8 bit code

*/


typedef enum {
   ILL_NUM= -1,
   UNDEF_NUM=0,
   I_NUM=1,
   D_NUM,
   X_NUM,
   E_NUM,
   S_NUM,
   SPC_NUM,
   SPD_NUM,
   OPC10_NUM,
   OPC11_NUM,
   SPL_NUM,
   ACC_NUM,
   IMP_NUM=ACC_NUM,
   MAX_NUM
} AModes;




#define   DES	(0x8000)

#define   I	(1L << I_NUM)
#define   D	(1L << D_NUM)
#define   X	(1L << X_NUM)
#define   E	(1L << E_NUM)
#define   S	(1L << S_NUM)
#define   SPC	(1L << SPC_NUM)
#define   SPDP	(1L << SPD_NUM)
#define   SP10	(1L << OPC10_NUM)
#define   SPC10	(1L << OPC10_NUM)
#define   SP11	(1L << OPC11_NUM)
#define   SPC11	(1L << OPC11_NUM)
#define   SPL	(1L << SPL_NUM)
#define   ACC	(1L << ACC_NUM)
#define   IMP	(1L << IMP_NUM)

#define   MOST69	(I|D|X|E)
#define   SHFTAM	(D|X|E|SPDP|DES)

#define   LONGAM	(S|SP10|SPL)




#endif /* _PSTTKN69_H_ */
