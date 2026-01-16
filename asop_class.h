/*
    op_class.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _ASOP_CLASS_H_
#define _ASOP_CLASS_H_ 1

#define OPCL_AU 0	/* ALU function */
#define OPCL_LD 1	/* LDx */
#define OPCL_ST 2	/* STx */
#define OPCL_BC 3	/* Branch on condition */
#define OPCL_BS 4	/* BSR */
#define OPCL_JS 5	/* JSR */
#define OPCL_PS 6	/* GET/PUT PS*/
#define OPCL_IL 7	/* SYSILL */
#define OPCL_SY 8	/* SYSCALL */

#define BR_OFF 0
#define MAX_DISP (8388604)	/* 0x007FFFFC */
#define MIN_DISP (-8388608)	/* 0x00800000 */

#define OP_PUTPS 0x1d

#endif /* _ASOP_CLASS_H_ */
