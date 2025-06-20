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

    04/10/2025	- 8080 CPU support added by Dave Shepperd

******************************************************************************/


#ifndef _PSTTKN8080_H_
#define _PSTTKN8080_H_ 1

/* The 8080 has no operator specified address modes */

typedef enum {
	ILL_NUM= -1,
	NO=0,		/* No operand */
	R,			/* register */
	RP,			/* register pair */
	PSW,		/* register pair (special for push/pop) */
	BD,			/* register pair (special for STAX) */
	D3,			/* 3 bit operand */
	D8,			/* 8 bit operand */
	D16,		/* 16 bit operand */
	MAX_NUM
} AModes;

#define OPC_AM_BIT_SHIFT (MAX_NUM)

#endif /* _PSTTKN8080_H_ */
