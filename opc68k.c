/*
    opc68k.c - Part of macxx, a cross assembler family for various micro-processors
    Copyright (C) 2008 David Shepperd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Gengeral Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define MAC68K 1
#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "m68k.h"
#include <assert.h>

/* The following are variables specific to the particular assembler */

char macxx_name[] = "mac68k";
char *macxx_target = "68000";
char *macxx_descrip = "Cross assembler for the 68000.";

unsigned short macxx_rel_salign = 1;    /* default alignment for .REL. segment */
unsigned short macxx_rel_dalign = 1;    /* default alignment for data in .REL. segment */
unsigned short macxx_abs_salign = 1;    /* default alignments for .ABS. segment */
unsigned short macxx_abs_dalign = 1;
unsigned short macxx_min_dalign = 1;
char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = ED_AMA|ED_M68|ED_LC|ED_GBL|ED_DOL;  /* default edmask */
/* default list mask */
unsigned long macxx_lm_default = ~(LIST_ME|LIST_MEB|LIST_MES|LIST_LD|LIST_COD);
int current_radix = 10;     /* default the radix to decimal */
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
int max_opcode_length = 16; /* significant length of opcodes */
int max_symbol_length = 16; /* significant length of symbols */
extern int dotwcontext;
extern int no_white_space_allowed;

/* End of processor specific stuff */

struct rinit
{
    char *name;
    unsigned long value;
};

static struct rinit reginit[] = {
    { "D0",REG_Dn+0},
    { "D1",REG_Dn+1},
    { "D2",REG_Dn+2},
    { "D3",REG_Dn+3},
    { "D4",REG_Dn+4},
    { "D5",REG_Dn+5},
    { "D6",REG_Dn+6},
    { "D7",REG_Dn+7},
    { "A0",REG_An+0},
    { "A1",REG_An+1},
    { "A2",REG_An+2},
    { "A3",REG_An+3},
    { "A4",REG_An+4},
    { "A5",REG_An+5},
    { "A6",REG_An+6},
    { "A7",REG_An+7},
    { "PC",REG_PC},
    { "SR",REG_SR},
    { "CCR",REG_CCR},
    { "SSP",REG_SSP},
    { "USP",REG_USP},
    { "SP",REG_SP},
    { "D0.W",(REG_Dn+0)|FORCE_WORD},
    { "D1.W",(REG_Dn+1)|FORCE_WORD},
    { "D2.W",(REG_Dn+2)|FORCE_WORD},
    { "D3.W",(REG_Dn+3)|FORCE_WORD},
    { "D4.W",(REG_Dn+4)|FORCE_WORD},
    { "D5.W",(REG_Dn+5)|FORCE_WORD},
    { "D6.W",(REG_Dn+6)|FORCE_WORD},
    { "D7.W",(REG_Dn+7)|FORCE_WORD},
    { "A0.W",(REG_An+0)|FORCE_WORD},
    { "A1.W",(REG_An+1)|FORCE_WORD},
    { "A2.W",(REG_An+2)|FORCE_WORD},
    { "A3.W",(REG_An+3)|FORCE_WORD},
    { "A4.W",(REG_An+4)|FORCE_WORD},
    { "A5.W",(REG_An+5)|FORCE_WORD},
    { "A6.W",(REG_An+6)|FORCE_WORD},
    { "A7.W",(REG_An+7)|FORCE_WORD},
    { "D0.L",(REG_Dn+0)|FORCE_LONG},
    { "D1.L",(REG_Dn+1)|FORCE_LONG},
    { "D2.L",(REG_Dn+2)|FORCE_LONG},
    { "D3.L",(REG_Dn+3)|FORCE_LONG},
    { "D4.L",(REG_Dn+4)|FORCE_LONG},
    { "D5.L",(REG_Dn+5)|FORCE_LONG},
    { "D6.L",(REG_Dn+6)|FORCE_LONG},
    { "D7.L",(REG_Dn+7)|FORCE_LONG},
    { "A0.L",(REG_An+0)|FORCE_LONG},
    { "A1.L",(REG_An+1)|FORCE_LONG},
    { "A2.L",(REG_An+2)|FORCE_LONG},
    { "A3.L",(REG_An+3)|FORCE_LONG},
    { "A4.L",(REG_An+4)|FORCE_LONG},
    { "A5.L",(REG_An+5)|FORCE_LONG},
    { "A6.L",(REG_An+6)|FORCE_LONG},
    { "A7.L",(REG_An+7)|FORCE_LONG},
    { "d0",REG_Dn+0},
    { "d1",REG_Dn+1},
    { "d2",REG_Dn+2},
    { "d3",REG_Dn+3},
    { "d4",REG_Dn+4},
    { "d5",REG_Dn+5},
    { "d6",REG_Dn+6},
    { "d7",REG_Dn+7},
    { "a0",REG_An+0},
    { "a1",REG_An+1},
    { "a2",REG_An+2},
    { "a3",REG_An+3},
    { "a4",REG_An+4},
    { "a5",REG_An+5},
    { "a6",REG_An+6},
    { "a7",REG_An+7},
    { "pc",REG_PC},
    { "sr",REG_SR},
    { "ccr",REG_CCR},
    { "ssp",REG_SSP},
    { "usp",REG_USP},
    { "sp",REG_SP},
    { "d0.w",(REG_Dn+0)|FORCE_WORD},
    { "d1.w",(REG_Dn+1)|FORCE_WORD},
    { "d2.w",(REG_Dn+2)|FORCE_WORD},
    { "d3.w",(REG_Dn+3)|FORCE_WORD},
    { "d4.w",(REG_Dn+4)|FORCE_WORD},
    { "d5.w",(REG_Dn+5)|FORCE_WORD},
    { "d6.w",(REG_Dn+6)|FORCE_WORD},
    { "d7.w",(REG_Dn+7)|FORCE_WORD},
    { "a0.w",(REG_An+0)|FORCE_WORD},
    { "a1.w",(REG_An+1)|FORCE_WORD},
    { "a2.w",(REG_An+2)|FORCE_WORD},
    { "a3.w",(REG_An+3)|FORCE_WORD},
    { "a4.w",(REG_An+4)|FORCE_WORD},
    { "a5.w",(REG_An+5)|FORCE_WORD},
    { "a6.w",(REG_An+6)|FORCE_WORD},
    { "a7.w",(REG_An+7)|FORCE_WORD},
    { "d0.l",(REG_Dn+0)|FORCE_LONG},
    { "d1.l",(REG_Dn+1)|FORCE_LONG},
    { "d2.l",(REG_Dn+2)|FORCE_LONG},
    { "d3.l",(REG_Dn+3)|FORCE_LONG},
    { "d4.l",(REG_Dn+4)|FORCE_LONG},
    { "d5.l",(REG_Dn+5)|FORCE_LONG},
    { "d6.l",(REG_Dn+6)|FORCE_LONG},
    { "d7.l",(REG_Dn+7)|FORCE_LONG},
    { "a0.l",(REG_An+0)|FORCE_LONG},
    { "a1.l",(REG_An+1)|FORCE_LONG},
    { "a2.l",(REG_An+2)|FORCE_LONG},
    { "a3.l",(REG_An+3)|FORCE_LONG},
    { "a4.l",(REG_An+4)|FORCE_LONG},
    { "a5.l",(REG_An+5)|FORCE_LONG},
    { "a6.l",(REG_An+6)|FORCE_LONG},
    { "a7.l",(REG_An+7)|FORCE_LONG},
    { 0,0}
};

int ust_init( void )
{
    SS_struct *ptr;
    extern SS_struct *sym_lookup();
    struct rinit *ri = reginit;

    while (ri->name != 0)
    {
        if ((ptr = sym_lookup(ri->name,1)) == 0)
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
    quoted_ascii_strings = TRUE;
    return 0;
}

EA source,dest;
extern int type0( int inst, int bwl);
extern int type1( int inst, int bwl);
extern int type2( int inst, int bwl);
extern int type3( int inst, int bwl);
extern int type4( int inst, int bwl);
extern int type5( int inst, int bwl);
extern int type6( int inst, int bwl);
extern int type7( int inst, int bwl);
extern int type8( int inst, int bwl);
extern int type9( int inst, int bwl);
extern int type10( int inst, int bwl);
extern int type11( int inst, int bwl);
extern int type12( int inst, int bwl);
extern int type13( int inst, int bwl);
extern int type14( int inst, int bwl);
extern int type15( int inst, int bwl);
extern int type16( int inst, int bwl);
extern int type17( int inst, int bwl);
extern int type18( int inst, int bwl);
extern int type19( int inst, int bwl);
extern int type20( int inst, int bwl);

static int (*opctbl[])() = { type0,type1,type2,type3,type4,type5,type6,type7,
    type8,type9,type10,type11,type12,type13,type14,type15,
    type16,type17,type18,type19,type20};

void do_opcode(Opcode *opc)
{
    EXP_stk *eptr, *sptr, *dptr;
    int ii;

    eptr = exprs_stack;
    assert( EXPR_MAXSTACKS >= 7 );
    for (ii=0; ii < EXPR_MAXSTACKS;++ii,++eptr)
    {
        eptr->ptr = 0;
        eptr->tag = 'W';
        eptr->tag_len = 1;
        eptr->stack->expr_code = EXPR_VALUE;
        eptr->stack->expr_value = 0;
/*        printf("Stack %d (%p) ptr=%d\n", ii, (void *)eptr, eptr->ptr ); */
    }
    if ( options[QUAL_GRNHILL] )
        no_white_space_allowed = 1;
    eptr = exprs_stack;
    eptr->ptr = 1;
    ++eptr;
    memset(&source,0,sizeof(source));
    memset(&dest,0,sizeof(dest));
    source.exp = eptr++;
    source.tmps[0] = eptr++;
    source.tmps[1] = eptr++;
    dest.exp = eptr++;
    dest.tmps[0] = eptr++;
    dest.tmps[1] = eptr++;
    dotwcontext = 1;
    if ((current_offset&1) != 0)
    {
        bad_token((char *)0,"Instructions must start on even byte boundary");
        ++current_offset;
    }
/*
    printf("Calling opcode %d. src=%p, ptr=%d, dst=%p, ptr=%d. inp_ptr=%s",
        opc->op_class&0x1F,
        (void *)source.exp, source.exp->ptr,
        (void *)dest.exp, dest.exp->ptr,
        inp_ptr );
*/
    (*opctbl[opc->op_class&0x1F])((char)opc->op_value,(char)opc->op_amode);
    eptr = exprs_stack;
    sptr = source.exp;
    dptr = dest.exp;
    if ( 0 )
    {
        char tmp[512];
        snprintf(tmp,sizeof(tmp),"eptr=%d, etag=%c, sptr=%d, stag=%c, dptr=%d, dtag=%c\n",
                eptr->ptr, isprint(eptr->tag) ? eptr->tag : '.',
                sptr->ptr, isprint(sptr->tag) ? sptr->tag : '.',
                dptr->ptr, isprint(dptr->tag) ? dptr->tag : '.'
                );
        show_bad_token(NULL,tmp,MSG_WARN);

    }
    if (eptr->ptr > 0)
    {
        /* output opcode word */
        eptr->psuedo_value = eptr->stack->expr_value;
        p1o_any(eptr);
    }
    if (sptr->ptr > 0)
    {
        int tag;
        tag = sptr->tag;
        if (tag == 'b' || tag == 'z' || tag == 's' )
        {
            if (eptr->tag == 'W')
            {
                eptr->ptr = 1;
                eptr->psuedo_value = eptr->stack->expr_value = source.reg2;
                eptr->stack->expr_code = EXPR_VALUE;
                eptr->tag = 'b';
                eptr->tag_len = 1;
                p1o_byte(eptr);        
            }
            p1o_byte(sptr);
        }
        else
        {
            p1o_any(sptr);
        }
    }
    if (dptr->ptr > 0)
    {
        int tag;
        tag = dptr->tag;
        if (tag == 'b' || tag == 'z' || tag == 's' )
        {
            eptr->ptr = 1;
            eptr->psuedo_value = eptr->stack->expr_value = dest.reg2;
            eptr->stack->expr_code = EXPR_VALUE;
            eptr->tag = 'b';
            eptr->tag_len = 1;
            p1o_byte(eptr);       
            p1o_byte(dptr);
        }
        else
        {
            p1o_any(dptr);
        }
    }
    dotwcontext = 0;
}

#include "opcommon.h"
