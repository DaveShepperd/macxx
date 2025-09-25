/*
    psttkn65.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _PSTTKN65_H_
#define _PSTTKN65_H_ 1

typedef enum {
   ILL_NUM= -1,
   UNDEF_NUM=0,
   I_NUM=1,
   A_NUM,
   AL_NUM,
   Z_NUM,
   D_NUM=Z_NUM,
   AC_NUM,
   IMP_NUM=AC_NUM,
   NY_NUM,
   NNY_NUM,
   NX_NUM,
   ZX_NUM,
   ZY_NUM,
   AX_NUM,
   ALX_NUM,
   AY_NUM,
   N_NUM,
   ND_NUM,
   NND_NUM,
   NAX_NUM,
   DS_NUM,
   NDSY_NUM,
   OP_TO_HEX_SIZE,
   XYC_NUM=OP_TO_HEX_SIZE,
   R_NUM,
   RL_NUM,
   X_NUM,
   Y_NUM,
   DES_NUM,
   NZ_NUM,
   MAX_NUM
} AModes;

#define   I	(1 << I_NUM)
#define   A	(1 << A_NUM)
#define   AL	(1 << AL_NUM)
#define   D	(1 << Z_NUM)
#define   Z	(1 << Z_NUM)
#define   AC	(1 << AC_NUM)
#define   IMP	(1 << IMP_NUM)
#define   NY	(1 << NY_NUM)
#define   NNY	(1 << NNY_NUM)
#define   NX	(1 << NX_NUM)
#define   ZX	(1 << ZX_NUM)
#define   ZY	(1 << ZY_NUM)
#define   AX	(1 << AX_NUM)
#define   ALX	(1 << ALX_NUM)
#define   AY	(1 << AY_NUM)
#define   R	(1 << R_NUM)
#define   RL	(1 << RL_NUM)
#define   N	(1 << N_NUM)
#define   ND	(1 << ND_NUM)
#define   NND	(1 << NND_NUM)
#define   NAX	(1 << NAX_NUM)
#define   DS	(1 << DS_NUM)
#define   NDSY	(1 << NDSY_NUM)
#define   XYC	(1 << XYC_NUM)
#define   X	(1 << X_NUM)
#define   Y	(1 << Y_NUM)
#define   DES	(1 << DES_NUM)
#define	  NZ (1<<NZ_NUM)

#define   MOST816	(I|A|AL|Z|NY|NNY|NX||NZ|ZX|AX|ALX|AY|ND|NND|DS|NDSY)
#define   MOST65	(I|A|Z|NX|NY|ZX|AX|AY)

#define   SHFTAM  	(A|Z|AC|ZX|AX|DES)
#define   BITAM		(Z|A)
#define   INCAM		(A|Z|ZX|AX|DES)
#define   XYAM    	(I|Z|A)
#define   BITAM816	(BITAM|I|ZX|AX)
#define   INCAM816	(A|Z|AC|ZX|AX|DES)

#endif /* _PSTTKN65_H_ */
