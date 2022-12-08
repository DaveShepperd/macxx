/*
    structs.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _STRUCTS_H_
#define _STRUCTS_H_ 1

#define EOL 254			/* end of line */

#if defined(INTERNAL_PACKED_STRUCTS)
#include "pragma1.h"
#endif

typedef struct my_desc {	/* define a descriptor structure cuz	*/
   unsigned short md_len;	/* the stock descrip definitions cause	*/
#ifdef VMS
   char md_type;		/* the compiler to think that they are	*/
   char md_class;		/* floating point data types.		*/
#endif
   char *md_pointer;		/* The stupid thing.			*/
} MY_desc;

#ifdef VMS
#define DSC$K_DTYPE_T 14
#define DSC$K_CLASS_S  1
#define $DESCRIPTOR(name,string) struct my_desc name = {\
   	sizeof(string)-1,\
   	DSC$K_DTYPE_T,DSC$K_CLASS_S,\
   	string };
#endif

typedef struct fn_struct {
#ifdef VMS
   unsigned short d_length;	/* length of s_buff			*/
   char s_type;			/* descriptor constant (uses 0)		*/
   char s_class;		/* descriptor constant (uses 0)		*/
#endif
   char *fn_buff;		/* pointer to filename buffer		*/
   unsigned short r_length;	/* length of string stored in s_buff	*/
   char *fn_name_only;		/* filename without dvc/dir/ver stuff	*/
   struct fn_struct *fn_next;	/* pointer to next filename desc	*/
   FILE *fn_file;		/* pointer to FILE structure		*/
   struct file_name *fn_nam;	/* pointer to nam_struct 		*/
   char *fn_version;		/* file version number (for debug)	*/
   int macro_level;		/* saved macro level			*/
   unsigned short fn_line;	/* current line number of input file    */
   unsigned char fn_filenum;	/* file number				*/
   unsigned fn_present:1;	/* this file is present (T/F)		*/
   unsigned fn_library:1;	/* this file is a library		*/
   unsigned fn_stdin:1;		/* file is from stdin 			*/
   unsigned :5;			/* fill to next byte boundary		*/
} FN_struct;

typedef struct seg_struct {
   char     *seg_string;	/* pointer to section name */
   unsigned long seg_pc;	/* segment pc */
   unsigned long seg_base;	/* base of segment (lowest address used) */
   unsigned long seg_len;	/* length of group/segment */
   unsigned long seg_maxlen;	/* maximum length for the segment */
   unsigned long rel_offset;	/* offset in relative segment this section begins */
   unsigned short seg_index;	/* segment index */
   unsigned short seg_ident;	/* id number for segment */
   unsigned short seg_salign;	/* alignment of group/segment in memory */
   unsigned short seg_dalign;	/* alignment of data within segment */
   unsigned flg_abs:1;		/* absolute section */
   unsigned flg_zero:1;		/* base page section (.bsect) */
   unsigned flg_ro:1;		/* section is read only */
   unsigned flg_noout:1;	/* don't output anything in this section */
   unsigned flg_ovr:1;		/* common/unique flag */
   unsigned flg_based:1;	/* signal section based */
   unsigned flg_data:1;		/* section is data */
   unsigned flg_subsect:1;	/* section is a subsection */
   unsigned flg_reference:1;	/* section has been referenced */
   unsigned flg_literal:1;	/* section is a literal pool */
} SEG_struct;

typedef struct ss_struct {
   char *ss_string;		/* pointer to ASCII identifier name */
   FN_struct *ss_fnd;		/* pointer to file struct that defined sym */
   struct ss_struct *ss_next; 	/* pointer to next node */
   union {
      struct seg_struct *ssp_seg; /* pointer to segment symbol is relative to */
      struct exp_stk *ssp_expr; /* pointer to expression definition area */
   } ssp_up;
   long ss_value;		/* symbol value or offset from segment */
   unsigned short ss_ident;	/* symbol indentifier */
   unsigned short ss_line;	/* source line that defined the file */
   unsigned int flg_defined:1;	/* symbol defined */
   unsigned int flg_local:1;	/* symbol is a local label */
   unsigned int flg_label:1;	/* symbol is a label */
   unsigned int flg_global:1;	/* symbol is global */
   unsigned int flg_exprs:1;	/* symbol definition is expression */
   unsigned int flg_abs:1;	/* symbol is defined as absolute */
   unsigned int flg_segment:1;	/* struct belongs to a segment */
   unsigned int flg_ident:1;	/* identifier declared for this symbol */
   unsigned int flg_base:1;	/* base page variable */
   unsigned int flg_fwd:1;	/* forward reference */
   unsigned int flg_ref:1;	/* symbol referenced in an expression */
   unsigned int flg_register:1;	/* symbol is a register type */
   unsigned int flg_regmask:1;	/* symbol is a register mask type */
   unsigned int flg_more:1;	/* there're more symbols in this group */
   unsigned int flg_static:1; /* A static (local) symbol */
   unsigned int flg_pass0:1;  /* Symbol defined during pass 0 */
   unsigned char ss_scope;	/* scope level */
   unsigned char ss_type;	/* symbol type (for source code debugging) */
} SS_struct;

#define ss_exprs ssp_up.ssp_expr
#define ss_seg   ssp_up.ssp_seg

extern FN_struct *inp_files,*option_file;

typedef struct ofn_struct {
   FN_struct ofn[OUT_FN_MAX];
} OFN_struct;
extern OFN_struct *out_files;

typedef struct expr_struct {
   EXPR_codes expr_code;	/* expression code */
   unsigned char expr_flags;
#define EXPR_FLG_REG	  1
#define EXPR_FLG_REGMASK  2
   long expr_value;	
   union {
      struct ss_struct *expt_sym;
      struct seg_struct *expt_seg;   
      struct exp_stk *expt_expr;
      unsigned long expt_count;
   } expt;
} EXPR_struct;

#define expr_sym expt.expt_sym
#define expr_seg expt.expt_seg
#define expr_expr expt.expt_expr
#define expr_count expt.expt_count

typedef struct exp_stk {
   long psuedo_value;		/* value if expr with unknowns set to 0 */
   struct expr_struct *stack;	/* expression stack */
   unsigned short tag;		/* expression tag */
   unsigned short tag_len;	/* length of tag */
   short ptr;			/* expression stack pointer */
   short paren_cnt;		/* number of open parens found */
   unsigned int forward_reference:1;
   unsigned int base_page_reference:1;
   unsigned int register_reference:1;
   unsigned int force_byte:1;	/* forced  8 bit mode */
   unsigned int force_short:1;	/* forced 16 bit mode */
   unsigned int force_long:1;	/* forced 32 bit mode */
   unsigned int register_mask:1; /* value is a register mask */
   unsigned int register_scale:2; /* register scale (0=1, 1=2, 2=4, 3=8) */
   unsigned int paren:1;	/* there's at least 1 paren in the expr */
   unsigned int autodec:1;	/* suggested autodecrement */
   unsigned int autoinc:1;	/* suggester autoincrement */
   unsigned int :4;		/* round it up to next short */
} EXP_stk;

#define BOS_CODE  expr_code
#define BOS_VALUE expr_value
#define BOS_SEG   expr_seg

typedef struct tmp_struct {
   unsigned char tf_type;
   unsigned short line_no;
   union {
      struct tmp_struct *tf_lin;
      long tf_len;
   } tf_ll;
} TMP_struct;

#define tf_length tf_ll.tf_len
#define tf_link   tf_ll.tf_lin

#if defined(INTERNAL_PACKED_STRUCTS)
#include "pragma.h"
#endif

#endif /* _STRUCTS_H_ */
