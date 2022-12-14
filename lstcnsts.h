/*
    lstcnsts.h - Part of macxx, a cross assembler family for various micro-processors
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

    01/19/2022	- Changed - Added support for HLLxxF  - Tim Giddens

******************************************************************************/

#ifndef _LSTCNSTS_H_
#define _LSTCNSTS_H_ 1

/* Constants to use for placement of data on listing_line */

#if MACXX_BIG_ENDIAN != 0
#define LIST_BIN 0x8000		/* binary */
#define LIST_BEX 0x4000		/* multi-line binary */
#define LIST_CND 0x2000		/* conditionals */
#define LIST_COD 0x1000		/* display words/longs as appears in mem */
#define LIST_COM 0x0800		/* comments */
#define LIST_LD  0x0400		/* listing directives */
#define LIST_LOC 0x0200		/* location */
#define LIST_MC  0x0100		/* macro calls */
#define LIST_MD  0x0080		/* macro definitions */
#define LIST_ME  0x0040		/* macro expansions */
#define LIST_MEB 0x0020		/* macro expansions, binary only */
#define LIST_MES 0x0010		/* source code on macro expansions */
#define LIST_SEQ 0x0008		/* listing line number */
#define LIST_SRC 0x0004		/* source code */
#define LIST_SYM 0x0002		/* symbol table */
#define LIST_TOC 0x0001		/* table of contents */
#else
#define LIST_BIN 0x0001		/* binary */
#define LIST_BEX 0x0002		/* multi-line binary */
#define LIST_CND 0x0004		/* conditionals */
#define LIST_COD 0x0008		/* display words/longs as appears in mem */
#define LIST_COM 0x0010		/* comments */
#define LIST_LD  0x0020		/* listing directives */
#define LIST_LOC 0x0040		/* location */
#define LIST_MC  0x0080		/* macro calls */
#define LIST_MD  0x0100		/* macro definitions */
#define LIST_ME  0x0200		/* macro expansions */
#define LIST_MEB 0x0400		/* macro expansions, binary only */
#define LIST_MES 0x0800		/* source code on macro expansions */
#define LIST_SEQ 0x1000		/* listing line number */
#define LIST_SRC 0x2000		/* source code */
#define LIST_SYM 0x4000		/* symbol table */
#define LIST_TOC 0x8000		/* table of contents */
#endif

#ifndef MAC_PP
#define LLIST_SIZE	40	/* listing field size (area to the left of source text) */
#define LLIST_SEQ	0	/* statement number */
#define LLIST_LOC	9	/* program counter */
#define LLIST_USC	22	/* unsatisified conditinal's X */
#define LLIST_PF1	18	/* print field 1 */
#define LLIST_PF2	18	/* print field 2 */
#define LLIST_RPT	21	/* repeat count */
#define LLIST_CND	21	/* conditional level */
extern int LLIST_OPC;
extern int LLIST_OPR;
#if 0
#define LLIST_OPC	14	/* opcode */
#define LLIST_OPR	17	/* operands */
#endif
#else
#define LLIST_SIZE	16
#define LLIST_SEQ	0	/* statement number */
#define LLIST_USC	10	/* unsatisified conditinal's X */
#define LLIST_PF1	7	/* print field 1 */
#define LLIST_PF2	7	/* print field 2 */
#define LLIST_RPT	10	/* repeat count */
#define LLIST_CND	9	/* conditional level */
#endif
/**************************************************tg*/
/*  01-19-2022  changed to support real input line size of 255 characters - Tim Giddens   
    01-19-2022  Added support for HLLxxF (up to 9, three space indents)

 True 255 (0-254) so (254 + 2 (CR & LF)) Characters per line support
 plus 40 Characters for LLIST_SIZE
 Support for Formatted Higher Level Language MACROS (HLLxxF)
 plus max of 27 Characters for HLLxxF

removed
#define LLIST_MAXSRC	(256)	* maximum length of listing line */

/* added */
#define LLIST_MAXSRC	(256+LLIST_SIZE+27)	/* maximum length of listing line */
#if 0	/* dms: moved to LIST_stat_t struct and renamed */
extern int LLIST_SRC;				/* listing line location for source code */
extern int LLIST_SRC_QUED;			/* queued listing line location for source code */
extern int LLIST_REQ_NEWL;			/* Request a new listing line */
extern unsigned char LLIST_TXT_BUF[18];		/* Buffer for text storage - 1 location + 16 characters + null */
#else
#define LLIST_SRC	40				/* column position for source to begin */
#define LLIST_MAX_SRC_OFFSET (LLIST_SRC+40)	/* Maximum number of columns source may be repositioned */
#define OPT_TXT_BUF_SIZE (1+16+1)	/* Buffer for text storage - 1 location + 16 characters + null */
#endif
/*************************************************etg*/

#endif /* _LSTCNSTS_H_ */
