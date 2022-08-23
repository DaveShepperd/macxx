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

#ifndef _OP_CLASS_H_
#define _OP_CLASS_H_ 1

#define OPCL_AU 0
#define OPCL_LD 1
#define OPCL_ST 2
#define OPCL_BC 3
#define OPCL_BS 4
#define OPCL_JS 5
#define OPCL_PS 6
#define OPCL_IL 7

#define BR_OFF 0
#define MAX_DISP (8388604L)
#define MIN_DISP (-8388608L)

#define OP_PUTPS 0x1d

#endif /* _OP_CLASS_H_ */
