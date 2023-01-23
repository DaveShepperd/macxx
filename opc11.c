/*
    opc11.c - Part of macxx, a cross assembler family for various micro-processors
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
#ifndef MAC_11
    #define MAC_11 1
#endif

#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "pdp11.h"

/* The following are variables specific to the particular assembler */

char macxx_name[] = "mac11";
char *macxx_target = "pdp11";
char *macxx_descrip = "Cross assembler for the PDP11.";

unsigned short macxx_rel_salign = 1;    /* default alignment for .REL. segment */
unsigned short macxx_rel_dalign = 1;    /* default alignment for data in .REL. segment */
unsigned short macxx_abs_salign = 1;    /* default alignments for .ABS. segment */
unsigned short macxx_abs_dalign = 1;    /* default alignments for .ABS. segment */
unsigned short macxx_min_dalign = 0;    /* alignment required by the hardware */
char macxx_mau = 8;             /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 3;        /* For the listing output routines */
char macxx_nibbles_word = 6;
char macxx_nibbles_long = 11;

unsigned long macxx_edm_default = ED_AMA|ED_LC|ED_GBL;  /* default edmask */
unsigned long macxx_lm_default = ~(LIST_ME|LIST_MEB|LIST_MES|LIST_LD|LIST_COD); /* default list mask */

int current_radix = 8;      /* default the radix to octal */
char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
char expr_escape = '^';     /* char that escapes an expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */
char open_operand = '(';    /* char that opens an operand indirection */
char close_operand = ')';   /* char that closes an operand indirection */
int max_opcode_length = 16; /* significant length of opcode names */
int max_symbol_length = 16; /* significant length of symbol names */

/* End of processor specific stuff */

struct rinit
{
    char *name;
    unsigned long value;
};

static struct rinit reginit[] = {
    { "R0",0},
    { "R1",1},
    { "R2",2},
    { "R3",3},
    { "R4",4},
    { "R5",5},
    { "R6",6},
    { "R7",7},
    { "SP",6},
    { "PC",7},
    { "r0",0},
    { "r1",1},
    { "r2",2},
    { "r3",3},
    { "r4",4},
    { "r5",5},
    { "r6",6},
    { "r7",7},
    { "sp",6},
    { "pc",7},
    { 0,0}
};

static void mac11_reset_list(int onoff)
{
    if ( onoff < 0 )
    {
        list_radix = 16;    /* .nlist oct */
        LLIST_OPC = 14;
        LLIST_OPR = 17;
        macxx_nibbles_byte = 2;
        macxx_nibbles_word = 4;
        macxx_nibbles_long = 8;
    }
    else
    {
        list_radix = 8;     /* .list oct */
        LLIST_OPC = 16;
        LLIST_OPR = 24;
        macxx_nibbles_byte = 3;
        macxx_nibbles_word = 6;
        macxx_nibbles_long = 11;
    }
}

int ust_init( void )
{
    SS_struct *ptr;
    struct rinit *ri = reginit;

    while (ri->name != 0)
    {
        if ((ptr = sym_lookup(ri->name,SYM_INSERT_IF_NOT_FOUND)) == 0)
        {
            sprintf(emsg,"Unable to insert symbol %s into symbol table",
                    ri->name);
            bad_token((char *)0,emsg);
            continue;
        }
        ptr->flg_defined = 1;     /* defined */
        ptr->flg_ref = 1;         /* referenced */
        ptr->flg_local = 0;       /* not local */
        ptr->flg_global = 0;      /* not global */
        ptr->flg_label = 1;       /* can't redefine it */
        ptr->ss_fnd = 0;          /* no associated file */
        ptr->flg_register = 1;        /* it's a register */
        ptr->flg_abs = 1;         /* it's not relocatible */
        ptr->flg_exprs = 0;       /* no expression with it */
        ptr->ss_seg = 0;          /* not an offset from a segment */
        ptr->ss_value = ri->value;    /* value */
        ri += 1;              /* next */
    }
    current_section->seg_dalign = 1; /* make default data in code section word aligned */
    quoted_ascii_strings = TRUE;
    reset_list_params = mac11_reset_list;
    mac11_reset_list(1);
    return 0;
}

EA source,dest;

extern int type0(int inst, int bwl);
extern int type1(int inst, int bwl);
extern int type2(int inst, int bwl);
extern int type3(int inst, int bwl);
extern int type4(int inst, int bwl);
extern int type5(int inst, int bwl);
extern int type6(int inst, int bwl);
extern int type7(int inst, int bwl);
extern int type8(int inst, int bwl);
extern int type9(int inst, int bwl);
extern int type10(int inst, int bwl);

static int (*opctbl[])() = { type0,type1,type2,type3,type4,type5,type6,type7,
    type8,type9,type10};

void do_opcode(Opcode *opc)
{
    int i;
    EXP_stk *ep;

    ep = exprs_stack;
    for (i=0;i<EXPR_MAXSTACKS;++i,++ep)
    {
        ep->tag = 'W';          /* select default size */
        ep->tag_len = 1;        /* and 1 element */
        ep->forward_reference = 0;
        ep->register_reference = 0;
        ep->register_mask = 0;
        ep->stack->expr_code = EXPR_VALUE;
        if ( !i )
        {
            ep->ptr = 1;
            ep->stack->expr_value = opc->op_value;
            ep->psuedo_value = opc->op_value;
        }
        else
        {
            ep->ptr = 0;
            ep->stack->expr_value = 0;
            ep->psuedo_value = 0;
        }
    }
    ep = exprs_stack;
    source.eamode = dest.eamode = 0;
    source.mode = dest.mode = 0;
    source.exp = ep+1;    /* source expression loads into stacks 1 */
    dest.exp = ep+2;      /* dest expression loads into stacks 2 */
    if ((current_offset&1) != 0)
    {
        bad_token((char *)0,"Instructions must start on even byte boundary");
        ++current_offset;
    }
    if ((opc->op_class&0xF) > sizeof(opctbl)/sizeof(void *))
    {
        bad_token(tkn_ptr,"Undecodable opcode; MAC11 internal error, please call dial-a-prayer");
    }
    (*opctbl[opc->op_class&0xF])(opc->op_value,opc->op_amode);
    ep = exprs_stack;          /* point to stack 0 */
    ep->stack->expr_value |= (source.eamode<<6) | dest.eamode;
    ep->ptr = compress_expr(ep);
    if (list_bin)
        compress_expr_psuedo(ep);
    p1o_any(ep);              /* output the opcode */
    ++ep;                      /* skip to stack 1 */
    if ( ep->ptr )              /* If there's something there */
    {
        ep->ptr = compress_expr(ep);
        if (list_bin)
            compress_expr_psuedo(ep);
        p1o_any(ep);            /* output it */
    }
    ++ep;                       /* Skip to stack 2 */
    if ( ep->ptr )              /* If there's something there */
    {
        ep->ptr = compress_expr(ep);
        if (list_bin)
            compress_expr_psuedo(ep);
        p1o_any(ep);            /* output it */
    }
}

int op_ntype( void )
{
    f1_eatit();
    return 1;
}

#ifndef MAC_11
    #define MAC_11 1
#endif
#include "opcommon.h"
