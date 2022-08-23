/*
    segdef.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _SEGDEF_H_
#define _SEGDEF_H_ 1

/************************************************************************
 * These are the meaning of the bits in the flags field of the VLDA_sym *
 * and VLDA_seg structs as used by MACxx, LLF, ANOTE, etc. in the .VLDA *
 * and .OD file formats.						*
 ************************************************************************/

#define VSYM_SYM	0x0001	/* if 1, struct defines a symbol */
#define VSYM_DEF	0x0002	/* symbol is defined */
#define VSYM_ABS	0x0004	/* symbol is absolute */
#define VSYM_LCL	0x0008	/* symbol is local */
#define VSYM_EXP	0x0010	/* symbol is defined via expression */

#define VSEG_SYM	0x0001	/* if 0, struct defines a segment */
#define VSEG_DEF	0x0002	/* segment is defined (always 1) */
#define VSEG_ABS	0x0004	/* segment is absolute */
#define VSEG_BASED	0x0008	/* segment is based */
#define VSEG_OVR	0x0010	/* segment is overlaid */
#define VSEG_BPAGE	0x0020	/* segment is base page */
#define VSEG_DATA	0x0040	/* segment is a "data" segment */
#define VSEG_RO		0x0080	/* segment is read only */
#define VSEG_NOOUT	0x0100	/* segment has no code or data in it */
#define VSEG_REFERENCE	0x0200	/* segment is referenced or has code in it */
#define VSEG_LITERAL	0x0400	/* segment is a literal pool */

#endif  /* _SEGDEF_H_ */
