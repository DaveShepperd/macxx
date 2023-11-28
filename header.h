/*
    header.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _HEADER_H_
#define _HEADER_H_ 1

extern short current_procblk;		/* current procedure block number */
extern short current_scopblk;		/* current scope level within block */
extern short current_scope;		/* sum of the two above */
#define SCOPE_PROC (-(1<<3))		/* mask for procblk */

extern char macxx_name[];
extern char *macxx_target;
extern char *macxx_version;
extern char *macxx_descrip;
extern char macxx_mau;      /* number of bits in a minimum addressable unit */
extern char macxx_bytes_mau; /* number of bytes in a minimum addressable unit */
extern char macxx_mau_byte; /* number of mau's in a byte */
extern char macxx_mau_word; /* number of mau's in a word */
extern char macxx_mau_long; /* number of mau's in a long */
extern char macxx_nibbles_byte;
extern char macxx_nibbles_word;
extern char macxx_nibbles_long;
extern unsigned long macxx_lm_default;
extern unsigned long macxx_edm_default;
extern unsigned short macxx_rel_dalign;
extern unsigned short macxx_rel_salign;
extern unsigned short macxx_abs_dalign;
extern unsigned short macxx_abs_salign;
extern unsigned short macxx_min_dalign;

extern int max_symbol_length;
extern int max_opcode_length;

#if defined(MAC_PP)
    #define DEF_OUT def_asm
#else
    #if defined(VMS)
        #define DEF_OUT def_ob
    #else
        #define DEF_OUT def_ol
    #endif
#endif

extern char DEF_OUT[];
extern char def_mac[];
extern char def_MAC[];
extern char def_lis[];
extern char opt_delim[];
extern FILE_name *image_name;
extern char *def_inp_ptr[];

extern int output_mode;		/* output mode */
extern FN_struct *current_fnd; /* global current_fnd for error handlers */
extern FN_struct output_files[OUT_FN_MAX]; /* output file structs */
extern int current_outfile;
extern FN_struct *cmd_fnds;	/* pointer to array of FN_structs for various outputs */
extern int cmd_outputs_index;	/* number of entries in cmd_fnds; */
extern int binary_output_flag;
extern int outx_width;         /* outx default record length */
extern int outx_swidth;
extern long out_pc;
extern int fn_pool_size;
extern char *fn_pool;
extern FN_struct *get_fn_struct(void);
extern void get_fn_pool( void );
extern FN_struct *first_inp;
extern void outid(FILE *fp,int mode);
extern int out_dbgfname(FILE *fp, FN_struct *odfp);
extern FILE *outxsym_fp;

#define lis_fp output_files[OUT_FN_LIS].fn_file
#define obj_fp output_files[OUT_FN_OBJ].fn_file
#define tmp_fp output_files[OUT_FN_TMP].fn_file
#define deb_fp output_files[OUT_FN_DEB].fn_file
extern char *map_subtitle;	/* pointer to map subtitle */

#ifdef MAC_PP
extern char *inp_str_partial;
#endif
extern char *inp_str;		/* temp string area */
extern int   inp_str_size;	/* size of input string */
extern int   max_token;		/* maximum length of any single token found in file */
extern char *token_pool_base;	/* base addr of current token pool */
extern char *token_pool;
extern char *tkn_ptr;
extern char *inp_ptr;
extern long token_value;
extern char next_token;     /* next token character */
extern int next_type;      /* next token character type */
extern int token_type;
extern int token_pool_size; /* size of remaining free token memory */
extern int comma_expected;
extern long misc_pool_used;
extern int debug;
extern int squeak;
extern int inp_len;
extern int include_level;
extern long sym_pool_used;
extern int new_identifier;
extern int strings_substituted;
extern unsigned long record_count;

/* pass0 and pass1 specific variables and functions */
extern int no_white_space_allowed;
extern int expr_message_tag;
#define DEFG_SYMBOL 1
#define DEFG_GLOBAL 2
#define DEFG_LOCAL  4
#define DEFG_LABEL  8
#define DEFG_STATIC 16
#define DEFG_FIXED 32
extern int f1_defg(int gbl_flg);
#if defined(MAC68K) || defined(MAC682K)
#define WST_LABEL    (0) /* no white space before labels */
#define WST_OPC      (1) /* one white space before opcode */
#define WST_OPRAND   (2) /* two white spaces before operand(s) */
#define WST_COMMENTS (3) /* three white spaces to comments */
extern int white_space_section;
extern int dotwcontext;
#endif /* M68k */
extern unsigned long autogen_lsb; /* autolabel for macro processing */
extern int get_text_assems;
extern void purge_data_stacks(const char *name);
extern int squawk_syms;
/*****************************/

extern char expr_open;
extern char expr_close;
extern char expr_escape;

extern char macro_arg_open;
extern char macro_arg_close;
extern char macro_arg_escape;
extern char macro_arg_gensym;
extern char macro_arg_genval;
extern int macro_nesting;
extern int macro_level;

#define ERRMSG_SIZE (512)
extern char emsg[ERRMSG_SIZE];		/* error message buffer */
#define LIS_LINES_PER_PAGE (60)
extern int show_line;
extern int lis_line;
extern int line_errors_index;
extern char *line_errors[];
extern char line_errors_sev[];
extern int error_count[];
#define LIS_TITLE_LEN (133)
extern char lis_title[LIS_TITLE_LEN];
extern char lis_subtitle[LIS_TITLE_LEN];
extern char ascii_date[];
extern void puts_titles(void);

#include "memmgt.h"

extern int new_symbol;	/* flag indicating an symbol added to symbol table */
extern int new_opcode;  /* flag indicating an opcode added to permanent symbol table */
   				        /* value (additive) */
#define SYM_ADD		1	/* symbol added to symbol table */
#define SYM_FIRST	2	/* symbol added is first in hash table */
#define SYM_DUP		4	/* symbol added is duplicate symbol */
extern SS_struct *first_symbol; /* pointer to first if duplicates */

extern unsigned long edmask;	/* enabl/disabl bit mask */
extern unsigned long lismask;	/* list/nlist bit mask */
extern unsigned long autogen_lsb;
extern unsigned long current_lsb;
extern unsigned long next_lsb;

extern long condit_word;
extern long condit_polarity;
extern int condit_level;
extern int condit_nest;
extern unsigned long listing_lnumb;

extern EXP_stk exprs_stack[EXPR_MAXSTACKS];
#define EXP0 exprs_stack[0]
#define EXP1 exprs_stack[1]
#define EXP2 exprs_stack[2]
#define EXP3 exprs_stack[3]
#define EXP4 exprs_stack[4]
#define EXP0SP exprs_stack[0].stack
#define EXP1SP exprs_stack[1].stack
#define EXP2SP exprs_stack[2].stack
#define EXP3SP exprs_stack[3].stack
#define EXP4SP exprs_stack[4].stack
extern int squawk_experr;
#if !defined(MAC_PP)
extern EXPR_struct tmp_org;
extern EXP_stk tmp_org_exp;
#endif

extern SEG_struct *current_section;
#define current_offset current_section->seg_pc
#define current_pc     current_section->seg_pc

extern int current_radix;	/* current input radix */

extern int seg_list_index;
extern SEG_struct **seg_list;
extern int subseg_list_index;
extern SEG_struct **subseg_list;
extern long unix_time;
extern int pass;
extern int quoted_ascii_strings;
extern unsigned short cttbl[];
extern char char_toupper[];
extern char hexdig[];

extern char **cmd_assems;
extern char **cmd_includes;
extern char **cmd_outputs;

extern char *get_token_pool(int amt, int flag);
extern int get_text( void );
extern int get_token( void );
extern void show_bad_token( const char *ptr, const char *msg, int sev );
extern void bad_token( char *ptr, const char *msg );
extern void err_msg( int severity, const char *msg );
extern char *error2str( int num);
extern int flushobj( void );
extern int hashit( char *string, unsigned int hash_size, unsigned int hash_indx);
extern int f1_eatit( void );
extern void f1_eol( void );
extern int endlin( void );
extern SEG_struct *get_seg_mem(SS_struct **ss, char *str);
extern SEG_struct *get_subseg_mem(void);
extern SEG_struct *find_segment(char *name,SEG_struct **segl,int segi);
extern void outseg_def(SEG_struct *seg_ptr);
typedef enum
{
	SYM_DO_NOTHING,				/* if not to insert into symbol table.*/
	SYM_INSERT_IF_NOT_FOUND,	/* if to insert into symbol table if symbol not found */
	SYM_INSERT_IF_FOUND,		/* if to insert into symbol table even if symbol found */
	SYM_INSERT_HIGHER_SCOPE,	/* if to insert into symbol table at higher scope */
	SYM_INSERT_LOCAL			/* if to insert into symbol table as a static local */
} SymInsertFlag_t;
extern SS_struct *sym_lookup( char *strng, SymInsertFlag_t err_flag );
extern SS_struct *do_symbol(SymInsertFlag_t flag);
extern SS_struct *get_symbol_block(int flag);
extern void outsym_def(SS_struct *sym_ptr,int mode);
extern void write_to_tmp(int typ, int itm_cnt, void *itm_ptr, int itm_siz);
extern void clear_outbuf(void);
extern int read_from_tmp( void );
extern void puts_lis(const char *string, int lines );
extern char *do_option(char *str);

extern void init_exprs( void );
extern int p1o_any( EXP_stk *eps );
extern int p1o_byte( EXP_stk *eps );
extern int p1o_word( EXP_stk *eps );
extern int p1o_long( EXP_stk *eps );
extern int p1o_var( EXP_stk *eps );
extern void n_to_list( int nibbles, long value, int tag);
extern void line_to_listing(void);
extern void dump_hex4(long value,char *ptr);
extern void outx_init( void );
extern int list_init(int onoff);

extern void opcinit(void);
extern int op_byte(void);
extern int op_word(void);
extern int dbg_init( void );
extern int dbg_line( int flag);
extern long dbg_flush( int flag );
extern int add_dbg_file( char *name, char *version, int *indx);
extern int f1_org( void );
extern void lap_timer(char *str);
extern int display_help(void);
extern int getcommand(void);
extern void pass0( int file_cnt);
extern void pass1( int file_cnt);
extern SS_struct *hash[HASH_TABLE_SIZE];
extern int dump_subsects(void);
extern int sort_symbols(void);
extern int pass2( void );
extern void display_mem(void);
extern void show_timer( void );
extern void qksort (SS_struct *array[],unsigned int num_elements);
extern int getAMATag(const FN_struct *fnd, int line);
extern void setAMATag(const FN_struct *fnd, int line, int tag);
extern int totalTagsUsed;

#ifndef n_elts
#define n_elts(x) (int)(sizeof(x)/sizeof((x)[0]))
#endif

#endif /* _HEADER_H_ */
