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

#ifndef _LISTCTRL_H_
#define _LISTCTRL_H_ 1

#include "lstcnsts.h"

typedef struct list_stat {
   char *line_ptr;
   char *line_end;
   unsigned long line_no;
   int include_level;
   int list_ptr;
   int pc_flag;
   unsigned long pc;
   unsigned long expected_pc;
   struct seg_struct *expected_seg;
   int f1_flag;
   int f2_flag;
   unsigned long pf_value;
   int getting_stuff;
   int has_stuff;
} LIST_stat;

extern LIST_stat list_stats;
extern char listing_line[];
#ifndef MAC_PP
extern char listing_meb[];
extern LIST_stat meb_stats;
extern int list_radix;
#endif
extern char hexdig[];
extern int list_mask;
extern int list_level;
extern int list_save;
extern int list_que;
extern unsigned long record_count;
extern FILE *save_lisfp;
extern int show_line;
extern void (*reset_list_params)(int onoff);
extern void display_line(LIST_stat *lstat);
extern void clear_list(LIST_stat *lstat);
extern int fixup_overflow(LIST_stat *lstat);

typedef struct list_bits {
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

typedef union list_mask {
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

#endif /* _LISTCTRL_H_ */
