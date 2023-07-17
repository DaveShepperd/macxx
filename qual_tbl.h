/*
    qual_tbl.h - Part of macxx, a cross assembler family for various micro-processors
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

/**************************************************************
 * Warning, there needs to be a comma at the end of each line
 * except the last one......
 **************************************************************/
 
#if !defined(MAC_PP)
#if !defined(NO_XREF)
QTBL(1, 0, 0, 0, 0, 1, QUAL_CROSS,	"CROSS_REFERENCE", 0),	/* cross reference */
#endif
QTBL(1, 0, 0, 0, 0, 1, QUAL_2_PASS,	"2_PASS", 0),		/* make a full two pass assembler */
QTBL(1, 0, 0, 0, 0, 1, QUAL_BINARY,	"BINARY", 0),		/* output binary files */
QTBL(1, 0, 0, 0, 0, 1, QUAL_CMOS,	"CMOS", 0),		/* 65C02 */
QTBL(1, 0, 0, 0, 0, 1, QUAL_P816,	"816", 0),		/* 65816 */
QTBL(1, 0, 0, 0, 0, 1, QUAL_JERRY,	"JERRY", 0),		/* Jaguar DSP */
QTBL(1, 0, 0, 0, 0, 1, QUAL_MISER,	"MISER", 0),		/* memory miser mode */
QTBL(1, 0, 0, 0, 0, 1, QUAL_BOFF,	"BOFF", 0),		/* MACAS global branch offset test */
QTBL(0, 1, 1, 0, 0, 1, QUAL_TEMP,	"TEMPFILE", 0),		/* use temp files */
#endif
QTBL(1, 0, 0, 0, 0, 1, QUAL_IDE_SYNTAX,	"IDE_ERROR_SYNTAX", 0),		/* output error messages in IDE syntax */
QTBL(0, 1, 0, 1, 0, 1, QUAL_OUTPUT,	"OUTPUT", OUT_FN_OBJ),	/* object file description */
QTBL(0, 1, 0, 1, 0, 1, QUAL_LIST,	"LIST", OUT_FN_LIS),	/* list file description */
QTBL(0, 1, 0, 1, 0, 1, QUAL_DEBUG,	"DEBUG", OUT_FN_DEB),	/* debug file description */
QTBL(0, 0, 1, 0, 0, 0, QUAL_SYML,	"SYMBOL_LENGTH", 0),	/* significant length of symbols */
QTBL(0, 0, 1, 0, 0, 0, QUAL_OPCL,	"OPCODE_LENGTH", 0),	/* significant length of opcodes */
QTBL(1, 0, 0, 0, 0, 1, QUAL_SQUEAK,	"SQUEAK", 0),		/* announce states (debugging) */
QTBL(1, 0, 0, 0, 0, 1, QUAL_OCTAL,	"OCTAL", 0),		/* list file in octal */
QTBL(1, 0, 0, 0, 0, 1, QUAL_RELATIVE,	"RELATIVE", 0),		/* relative assembly */
#if defined(MAC_PP)
QTBL(1, 0, 0, 0, 0, 1, QUAL_LINE,	"LINE", 0),		/* place # line info in output */
#endif
QTBL(0, 0, 0, 0, 1, 0, QUAL_ASSEM,	"ASSEM", 0),		/* insert text in assem stream */
QTBL(0, 0, 0, 0, 0, 0, QUAL_INCLUDE,	"INCLUDE", 0),		/* include file path description */
QTBL(1, 0, 0, 0, 0, 1, QUAL_GRNHILL, "GREENHILLS", 0),
QTBL(1, 0, 0, 0, 0, 1, QUAL_ABBREV, "ABBREVIATE", 0), /* Abbreviate error messages */
QTBL(1, 0, 0, 0, 0, 1, QUAL_IGNORE, "IGNORE", 0) /* Ignore some old undefined psuedo-ops */


