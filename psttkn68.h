/*
	psttkn68.c - Part of macxx, a cross assembler family for various micro-processors
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

    03/18/2022	- 6800 CPU support added by Tim Giddens
		  using David Shepperd's work as a reference

******************************************************************************/


#ifndef _PSTTKN68_H_
#define _PSTTKN68_H_ 1



typedef enum {
   ILL_NUM= -1,
   UNDEF_NUM=0,
   I_NUM=1,
   D_NUM,
   Z_NUM=D_NUM,
   X_NUM,
   E_NUM,
   A_NUM=E_NUM,
   S_NUM,
   SPC_NUM,
   ACC_NUM,
   IMP_NUM=ACC_NUM,
   MAX_NUM
} AModes;


#define   I	(1L << I_NUM)
#define   D	(1L << D_NUM)
#define   Z	(1L << Z_NUM)
#define   X	(1L << X_NUM)
#define   E	(1L << E_NUM)
#define   A	(1L << A_NUM)
#define   S	(1L << S_NUM)
#define   SPC	(1L << SPC_NUM)
#define   ACC	(1L << ACC_NUM)
#define   IMP	(1L << IMP_NUM)

#define   DES	(0x8000)

#define   MOST68	(I|D|X|E)



#endif /* _PSTTKN68_H_ */
