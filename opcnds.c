/*
    opcnds.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "opcnds.h"

#define CCN(str,siz,flg) {str,siz,flg}

struct ccn_struct ccns[] = {
    CCN("B",1,CCN_B),
    CCN("F",1,CCN_EQ),
    CCN("L",1,CCN_LT),
    CCN("T",1,CCN_NE),
    CCN("Z",1,CCN_EQ),
    CCN("DF",2,CCN_DF),
    CCN("EQ",2,CCN_EQ),
    CCN("GE",2,CCN_GE),
    CCN("GT",2,CCN_GT),
    CCN("LE",2,CCN_LE),
    CCN("LT",2,CCN_LT),
    CCN("NB",2,CCN_NB),
    CCN("NE",2,CCN_NE),
    CCN("NZ",2,CCN_NE),
    CCN("ABS",3,CCN_ABS),
    CCN("DIF",3,CCN_DIF),
    CCN("IDN",3,CCN_IDN),
    CCN("NDF",3,CCN_NDF),
    CCN("REL",3,CCN_REL),
    CCN("SAME",4,CCN_IDN),
    CCN("TRUE",4,CCN_NE),
    CCN("ZERO",4,CCN_EQ),
    CCN("BLANK",5,CCN_B),
    CCN("EQUAL",5,CCN_EQ),
	CCN("EXIST",5,CCN_EXIST),
    CCN("FALSE",5,CCN_EQ),
    CCN("EXISTS",6,CCN_EXIST),
	CCN("DEFINED",7,CCN_DF),
    CCN("NOT_ZERO",8,CCN_NE),
    CCN("DIFFERENT",9,CCN_DIF),
    CCN("IDENTICAL",9,CCN_IDN),
    CCN("LESS_THAN",9,CCN_LT),
    CCN("NOT_BLANK",9,CCN_NB),
    CCN("NOT_EQUAL",9,CCN_NE),
	CCN("NOT_EXIST",9,CCN_NEXIST),
	CCN("NOT_EXISTS",10,CCN_NEXIST),
    CCN("NOT_DEFINED",11,CCN_NDF),
    CCN("GREATER_THAN",12,CCN_GT),
    CCN("LESS_OR_EQUAL",13,CCN_LE),
    CCN("GREATER_OR_EQUAL",16,CCN_GE),
    {0,0,0}
};
#undef CCN

