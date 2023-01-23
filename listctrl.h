/*
    listctrl.h - Part of macxx, a cross assembler family for various micro-processors
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

    01/24/2022	- Changed - Added support for HLLxxF  - Tim Giddens

******************************************************************************/

#ifndef _LISTCTRL_H_
#define _LISTCTRL_H_ 1

#include "lstcnsts.h"

typedef struct
{
	int srcPosition;					/* listing line location for source code */
	int srcPositionQued;				/* queued listing line location for source code */
	int reqNewLine;					/* Request a new listing line */
	unsigned char optTextBuf[OPT_TXT_BUF_SIZE]; /* Buffer for text storage - 1 location + 16 characters + null */
} LIST_Source_t;

typedef struct
{
   char listBuffer[LLIST_MAXSRC+2];	/* Holds line to be printed */
/*   char *line_end;					* Points to end of above line */
   unsigned long line_no;			/* line number of source */
   int include_level;				/* depth of .include */
   int list_ptr;					/* Index into listBuffer between 0 and LLIST_SIZE (left of source string) */
   int pc_flag;						/* flag indicating to print program counter */
   unsigned long pc;				/* value of program counter to print */
   unsigned long expected_pc;		/* value of program counter to print */
   struct seg_struct *expected_seg;	/* pointer to current section */
   int f1_flag;						/* flag to print pf_value at field 1 listBuffer[LLIST_PF1] */
   int f2_flag;						/* flag to print pf_value at field 2 listBuffer[LLIST_PF2] */
   unsigned long pf_value;			/* value of field */
   int getting_stuff;				/* flag indicating accumulating stuff to print */
   int has_stuff;					/* flag indicating has some stuff to print */
} LIST_stat_t;

extern LIST_Source_t list_source;
extern LIST_stat_t list_stats;
extern LIST_stat_t meb_stats;
extern int list_radix;
extern char hexdig[];
extern int list_mask;
extern int list_level;
extern int list_save;
extern int list_que;
extern unsigned long record_count;
extern FILE *save_lisfp;
extern int show_line;
extern void (*reset_list_params)(int onoff);
extern void display_line(LIST_stat_t *lstat);
extern void clear_list(LIST_stat_t *lstat);
extern int fixup_overflow(LIST_stat_t *lstat);
extern int limitSrcPosition(int value);

typedef struct list_bits
{
   unsigned int lm_bin:1;
   unsigned int lm_bex:1;
   unsigned int lm_cnd:1;
   unsigned int lm_cod:1;
   unsigned int lm_com:1;
   unsigned int lm_ld:1;
   unsigned int lm_loc:1;
   unsigned int lm_mc:1;
   unsigned int lm_md:1;
   unsigned int lm_me:1;
   unsigned int lm_meb:1;
   unsigned int lm_mes:1;
   unsigned int lm_seq:1;
   unsigned int lm_src:1;
   unsigned int lm_sym:1;
   unsigned int lm_toc:1;
} LIST_bits;

typedef union list_mask
{
   unsigned short list_mask;
   struct list_bits list_bits;
} LIST_mask;

extern LIST_mask lm_bits,saved_lm_bits,qued_lm_bits;

#define list_bin lm_bits.list_bits.lm_bin
#define list_bex lm_bits.list_bits.lm_bex
#define list_cnd lm_bits.list_bits.lm_cnd
#define list_cod lm_bits.list_bits.lm_cod
#define list_com lm_bits.list_bits.lm_com
#define list_ld  lm_bits.list_bits.lm_ld
#define list_loc lm_bits.list_bits.lm_loc
#define list_mc  lm_bits.list_bits.lm_mc
#define list_md  lm_bits.list_bits.lm_md
#define list_me  lm_bits.list_bits.lm_me
#define list_meb lm_bits.list_bits.lm_meb
#define list_mes lm_bits.list_bits.lm_mes
#define list_seq lm_bits.list_bits.lm_seq
#define list_src lm_bits.list_bits.lm_src
#define list_sym lm_bits.list_bits.lm_sym
#define list_toc lm_bits.list_bits.lm_toc

/**************************************************tg*/
/*    01-24-2022  Added for HLLxxF support by TG
*/
typedef enum
{
	/* Indicies into list_arg[] array */
	ListArg_ID,		/* Format ID */
	ListArg_Dst,	/* destination of message (index to field) */
	ListArg_Len,	/* Field length */
	ListArg_Que=ListArg_Len, /* Flag for queue on SRC option */
	ListArg_Radix,	/* radix to convert */
	ListArg_LZ, 	/* flag, if not-zero, include leading 0's */
	ListArg_Sign,	/* flag, if not-zero, prefix a sign */
	ListArg_NewLine, /* flag, if not-zero, request new line */
	ListArg_MAX		/* Number of arguments */
} ListArg_t;

extern int list_args(int arg[ListArg_MAX], int maxArgs, int cnt, unsigned char *optTextBuf, size_t optTextBufSize);
/* extern char listing_temp[]; */

/*************************************************etg*/


#endif /* _LISTCTRL_H_ */
