/*
    opcas.c - Part of macxx, a cross assembler family for various micro-processors
    Copyright (C) 2008 David Shepperd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
                                                   gf
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "op_class.h"

#define STACK_DEBUG 1
#define OP_DEBUG 2
#define DEBUG 0

#define SCC_BIT  0x00200000
#define SRC1_POS 16
#define DST_POS  22
#define MAXREG   31
#define REGMARK 0xFFE0
#define DISP_MASK (-MIN_DISP | MAX_DISP)
#define DEFNAM(name,numb) {"name",name,numb},

/* next four variables are for compatability with V7.4 Macxx */
unsigned short rel_salign = 2; /* default alignment for rel segments */
unsigned short rel_dalign = 2; /* def algnmnt for data in rel segments */
unsigned short abs_salign = 2; /* default alignment for abs segments */
unsigned short abs_dalign = 2; /* def algnmnt for data in abs segments */

char macxx_name[] = "macas";
char *macxx_target = "ASAP";
char *macxx_descrip = "Cross assembler for the ASAP.";

unsigned short macxx_rel_salign = 2; /* default alignment for rel segments */
unsigned short macxx_rel_dalign = 2; /* def algnmnt for data in rel segments */
unsigned short macxx_abs_salign = 2; /* default alignment for abs segments */
unsigned short macxx_abs_dalign = 2; /* def algnmnt for data in abs segments */
unsigned short macxx_min_dalign = 2;

char macxx_mau = 8;         /* # of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = ED_LC|ED_GBL; /* default edmask */
/* default list mask */
unsigned long macxx_lm_default = ~(LIST_ME|LIST_MEB|LIST_MES|LIST_LD|LIST_COD);
int current_radix = 16;     /* default the radix to hex */
char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
char expr_escape = '^';     /* char that escapes an expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */
char open_operand = '[';    /* char that opens an operand indirection */
char close_operand = ']';   /* char that closes an operand indirection */
int max_opcode_length = 16; /* significant length of opcodes and */
int max_symbol_length = 16; /*  symbols */

/* End of processor specific stuff */

int ust_init(void )
{
    return 1;              /* no custom symbol table stuff for ASAP */
}

static char *am_ptr;
static unsigned short which_stack;  /* from whence to finally output */

#define MNBI 0x8000		/* operand must not be indexed */
#define MBI  0x4000		/* operand must be indexed */
#define FIN  0x2000		/* opening @ or ( found */

static void get_ea(unsigned short size);
static void get_dst(void),get_s1(void),get_s2(void);
static int merge_stacks(int a, int b);

void do_opcode(Opcode *opc)
{
    int hiword;
    int err_cnt = error_count[MSG_ERROR];
    EXPR_struct *exp0,*exp1;

    which_stack = 0;
    EXP0.tag = 'l';         /* opcode is default U32 */
    EXP0.tag_len = 1;           /* only 1 longword */
    EXP1.tag = 'u';         /* operand,if any, is U16 */
    EXP0.ptr = 1;           /* first stack has opcode (1 element) */
    EXP1.ptr = 0;           /* assume no other operands */
    exprs_stack[2].ptr = 0;     /* assume no other operands */
    exprs_stack[3].ptr = 0;     /* assume no other operands */
    exprs_stack[2].tag = 0;     /* assume no tags either */
    exprs_stack[3].tag = 0;
    exprs_stack[2].tag_len = 1;     /* but if there is a tag, len = 1 */
    exprs_stack[3].tag_len = 1;
    exp0 = EXP0SP;          /* point to expression stack 0 */
    exp1 = EXP1SP;          /* point to expression stack 1 */
    exp0->expr_code = EXPR_VALUE;   /* set the opcode expression */
    hiword = opc->op_value;     /* stuff in opcode value */
    exp0->expr_value = (hiword & 0xFFE0) << 16;
    am_ptr = inp_ptr;           /* remember where am starts */
    switch (opc->op_class & 7)
    {
    case OPCL_AU :
        /* ALU ops: 
        *	OP	%i,%j,%k	reg.i <- reg.j OP reg.k
        *	OP	%i,%j,k		reg.i <- reg.j OP k (constant)
        */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class AU, value: %04X %s",
                opc->op_value, am_ptr);
#endif

        get_dst();

        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
        }

        get_s1();

        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
        }

        get_s2();
        break;

    case OPCL_LD :
        /* load ops	(incl LEA): 
        *	Lxx	%i,%j[%k]	reg.i <- @(reg.j + (reg.k << size))
        *	Lxx	%i,%j[k]	reg.i <- @(reg.j + (k << size))
        */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class LD, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        get_dst();
        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
        }

        get_ea(hiword & 3);
        break;

    case OPCL_ST :
        /* store ops: 
        *	STxx	%k,%i[%j]	reg.k -> @(reg.i + (reg.j << size))
        *	STxx	%k,%i[j]	reg.k -> @(reg.i + (j << size))
        *			( same as class LD, separate for debug )
        */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class ST, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        js_st:
        get_dst();

        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
        }

        get_ea(hiword & 3);
        break;

    case OPCL_BC :
        /* conditional branches: 
        *	Bcc	where
        */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class BC, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        get_offset:
        get_token();
        if (exprs(1,&EXP1) < 1)
        {       /* expression nfg or not present */
            exp1->expr_code = EXPR_VALUE;   /* make it a 0 */
            exp1->expr_value = -4;      /* make it a br . */
            EXP1.ptr = 1;           /* 1 item on stack */
            bad_token(inp_ptr - 1,"Expression expected");
        }
        else
        {
            exp1 = EXP1SP + EXP1.ptr;
            exp1->expr_code = EXPR_SEG;
            exp1->expr_value = current_offset + BR_OFF;
            (exp1++)->expr_seg = current_section;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value = '-';
            EXP1.ptr += 2;
            EXP1.ptr = compress_expr(&EXP1);
        }
        EXP1.tag = 'O';     /* flag as branch offset */
        exp1 = EXP1SP;
        if ( (EXP1.ptr == 1)
             && (exp1->expr_code == EXPR_VALUE) )
        {
            /* absolute difference, check range */
            if (exp1->expr_value < MIN_DISP ||
                exp1->expr_value > MAX_DISP )
            {
                long toofar;
                toofar = exp1->expr_value;
                if (toofar > 0)
                {
                    toofar -= MAX_DISP ;
                }
                else
                {
                    toofar = -toofar + MIN_DISP;
                }
                sprintf(emsg,"Branch offset 0x%lX byte(s) out of range",toofar);
                bad_token((char *)0,emsg);
                exp1->expr_value = 0-(BR_OFF);
            }
            if ((exp1->expr_value&3) != 0)
            {
                bad_token((char *)0,"Branch to non-long-aligned address");
            }
            /* absolute difference (possibly cooked), mask and shift */
            exp1->expr_value &= (-MIN_DISP|MAX_DISP);
            exp1->expr_value >>= 2;
            EXP1.ptr = 1;
        }
        else
        {
            if (options[QUAL_BOFF])
            {
                EXP_stk *ep2;
                EXPR_struct *exp2;
                /* branch offset is expression, add to expression to mask
                *  and shift down
                */
                ep2 = &EXP2;         /* clone the expression to stack 2 */
                exp2 = EXP2SP;
                memcpy(exp2, (char *)EXP1SP, EXP1.ptr*sizeof(EXPR_struct));
                exp2 += EXP1.ptr;
                exp2->expr_code = EXPR_VALUE;
                (exp2++)->expr_value = 0;        /* pick's argument; 0=top of stack */
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_PICK;    /* dup top of stack */
                exp2->expr_code = EXPR_VALUE;
                (exp2++)->expr_value = 0x007FFFFFL;  /* check for out of range + */
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_GT<<8);
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_XCHG;    /* save answer, get tos */
                exp2->expr_code = EXPR_VALUE;
                (exp2++)->expr_value = 0;        /* pick's argument; 0=top of stack */
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_PICK;    /* dup top of stack */
                exp2->expr_code = EXPR_VALUE;
                (exp2++)->expr_value = -0x00800000L; /* check for out of range - */
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_LT<<8);
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_XCHG;    /* save answer, get tos */
                exp2->expr_code = EXPR_VALUE;
                (exp2++)->expr_value = 3;        /* check for unaligned target */
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_AND;
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_OR<<8);
                exp2->expr_code = EXPR_OPER;
                (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_OR<<8);
                ep2->ptr = exp2 - EXP2SP;
                sprintf(emsg, "%s:%d", current_fnd->fn_name_only, current_fnd->fn_line);
                ep2->tag = 0;
                write_to_tmp(TMP_BOFF, 0, (char *)ep2, 0);
                write_to_tmp(TMP_ASTNG, strlen(emsg)+1, emsg, 1);
                ep2->ptr = 0;        /* done with stack 2 */
            }

            exp1 = EXP1SP + EXP1.ptr;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = DISP_MASK;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value = EXPROPER_AND;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = 2;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value = EXPROPER_SHR;
            EXP1.ptr = exp1 - EXP1SP;

        }

        /* now merge with opcode from stack 0 */
        if ( (which_stack = merge_stacks(1,0)) )
            EXP0.ptr = 0;
        else
            EXP1.ptr = 0;

        break;

    case OPCL_BS :
        /* branch to subroutine: 
        *	Bsr	%link,where
        */
        /* get link register, then merge with Bcc */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class BS, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        get_dst();

        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
        }
        goto get_offset;

    case OPCL_JS :
        /* Jump to subroutine
        *	Jsr	%link,where
        */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class JS, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        goto js_st;
        break;

    case OPCL_PS :
        /* putps, getps */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class PS, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        if ( hiword == (OP_PUTPS << 11) )
        {
            /* PUTPS, get src2 */
            get_s2();
        }
        else
        {
            /* GETPS, get dst */
            get_dst();
        }
        f1_eatit();
        break;  
    case OPCL_IL :
        /* illegal and sysill */
#if DEBUG & OP_DEBUG
        fprintf(stderr,"got class IL, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        f1_eatit();
        break;  
    }                    /* -- switch(class) */
    if (err_cnt != error_count[MSG_ERROR])
    { /* if any errors detected... */
        f1_eatit();               /* ...eat to EOL */
    }
#if DEBUG & STACK_DEBUG
    fprintf(stderr,"First shot, which_stack: %d\n",which_stack);
    dump_stacks();
    if ( which_stack = merge_stacks(1,0) )
    {
        /* merge ended up in stack 1 */
        EXP0.ptr = 0;
        fputs("stack 1 fiddle clearing stack 0\n",stderr);
    }
    else
    {
        /* merge ended up in stack 0 */
        EXP1.ptr = 0;
        fputs("stack 1 fiddle clearing stack 1\n",stderr);
    } /* end of stack[1] fiddling */
#else
    which_stack = merge_stacks(1,0);
#endif

#if DEBUG & STACK_DEBUG
    fprintf(stderr,"After stack[1] fiddling, which_stack: %d\n",which_stack);
    dump_stacks();

    if ( exprs_stack[3].ptr != 0 )
    {
        /* something on stack 3 */
        if ( (uval = merge_stacks(3,which_stack)) == 3 )
        {
            /* merge ended up in stack 3 */
            exprs_stack[which_stack].ptr = 0;
            fprintf(stderr,"stack 3 fiddle clearing stack %d\n",which_stack);
            which_stack = 3;
        }
        else
        {
            /* merge ended up in other stack */
            exprs_stack[3].ptr = 0;
            fputs("stack 3 fiddle clearing stack 3\n",stderr);
        }
    }  /* end of stack[3] fiddling */
#else
    which_stack = merge_stacks(3,which_stack);
#endif

#if DEBUG & STACK_DEBUG
    fprintf(stderr,"After stack[3] fiddling, which_stack: %d\n",which_stack);
    dump_stacks();
    if ( exprs_stack[2].ptr )
    {
        /* something on stack 3 */
        if ( (uval = merge_stacks(2,which_stack)) == 3 )
        {
            /* merge ended up in stack 2 */
            exprs_stack[which_stack].ptr = 0;
            fprintf(stderr,"stack 2 fiddle clearing stack %d\n",which_stack);
            which_stack = 2;
        }
        else
        {
            /* merge ended up in other stack*/
            exprs_stack[2].ptr = 0;
            fputs("stack 2 fiddle clearing stack 2\n",stderr);
        }
    } /* end of stack 2 fiddling */
    fprintf(stderr,"After stack[2] fiddling, which_stack: %d\n",which_stack);
    dump_stacks();
#else
    which_stack = merge_stacks(2,which_stack);
#endif
    exprs_stack[which_stack].ptr = compress_expr(&exprs_stack[which_stack]);

#if DEBUG & STACK_DEBUG
    fprintf(stderr,"After compress(), which_stack: %d\n",which_stack);
    dump_stacks();
#endif
    exprs_stack[which_stack].tag = ( edmask & ED_M68 ) ? 'L' : 'l';
    if (list_bin) compress_expr_psuedo(&exprs_stack[which_stack]);
    p1o_long(&exprs_stack[which_stack]);      /* output inst */
}                   /* -- opc_as() */

static void get_ea(unsigned short size)
{
    long val;
    EXPR_struct *exp1,*exp2;

    exp1 = EXP1SP;
    get_token();
    if ( exprs(1,&EXP1) < 1)
    {  /* bad or no expression */
        /* NOP the instruction by clearing Scc, leaving rdst = %0 */
        EXP0SP->expr_value &= ~SCC_BIT;
        EXP1.ptr = 0;
        bad_token(inp_ptr - 1,"Register expression expected");
        return;
    }
    else if (EXP1.register_reference)
    {
        if ((EXP1.ptr == 1)&&(exp1->expr_code == EXPR_VALUE))
        {
            /* absolute register # */
            if ( (val = exp1->expr_value) < 0 || val > MAXREG )
            {
                bad_token(inp_ptr - 1,"Invalid Register expression"); 
                exp1->expr_value = 0;
            }
            exp1->expr_value = val << SRC1_POS;
        }
        else
        {
            /* non-absolute register expression */
            exp1 = EXP1SP + EXP1.ptr;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = MAXREG;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value  = EXPROPER_AND;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = SRC1_POS;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value = EXPROPER_SHL;
            EXP1.ptr += 4;
            exp1 = EXP1SP;
        } /* end register expression */
        EXP1.tag = 'R';         /* let common code know */
    } /* end register reference */
    else if ((EXP1.ptr == 1)&&(exp1->expr_code == EXPR_VALUE)
             &&(exp1->expr_value==0))
    {
        /* constant 0 is accepted as %0 */
        EXP1.tag = 'R';     /* let common code know */
    }
    else if ( *inp_ptr == open_operand )
    {
        /* he got the syntax right but muffed the semantics */
        EXP1.ptr = 0;
        bad_token(inp_ptr - 1,"expression must resolve to Register or 0");
        return;
    }
    else
    {
        /* special shorthand hack for xxx fred => xxx %0[fred] */
        EXP1.tag = 'u';
        if ( size )
        {
            exp1 = EXP1SP + EXP1.ptr;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = size;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value = EXPROPER_SHR;
            EXP1.ptr += 2;
        }
        EXP1.ptr = compress_expr(&EXP1);
        return;
    }

    if ( *inp_ptr++ != open_operand )
    {
        sprintf(emsg,"%c expected",open_operand);
        bad_token(inp_ptr - 1,emsg);
    }
    exp2 = EXP2SP;
    get_token();
    if ( exprs(1,&EXP2) < 1 )
    {
        /* partially NOP the instruction by clearing Scc */
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register or expression expected");
        return;
    }
    else if ( EXP2.register_reference )
    {
        if ((EXP2.ptr == 1)&&(exp2->expr_code == EXPR_VALUE))
        {
            /* absolute register # */
            if ( (val = exp2->expr_value) < 0 || val > MAXREG )
            {
                bad_token(inp_ptr - 1,"Invalid Register expression"); 
                exp2->expr_value = 0;
            }
            else exp2->expr_value |= REGMARK;
        }
        else
        {
            /* a register valued expression, build expression
            *  to clip and position it.
            */
            exp2 = EXP2SP + EXP2.ptr;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = MAXREG;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value  = EXPROPER_AND;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = REGMARK;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_OR;
            EXP2.ptr += 4;
        }
        EXP2.tag = 'R';     /* let common code know */
    }
    else
    {
        EXP2.tag = 'u';
        if ( size )
        {
            exp2 = EXP2SP + EXP2.ptr;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = size;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_SHR;
            EXP2.ptr += 2;
            EXP2.ptr = compress_expr(&EXP2);
        }
    }

    if ( *inp_ptr++ != close_operand )
    {
        sprintf(emsg,"%c expected",close_operand);
        bad_token(inp_ptr - 1,emsg);
    }
    else
    {
        while ( *inp_ptr && (*inp_ptr == ' ' || *inp_ptr == '\t'))
        {
            ++inp_ptr;
        }
    }
}

static void get_s1( void )
{
    long val;
    EXPR_struct *exp1;

    get_token();
    exp1 = EXP1SP;
    if ( exprs(1,&EXP1) < 1 )
    {
        /* partially NOP the instruction by clearing Scc */
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register expression expected");
        return;
    }
    else if ( EXP1.register_reference )
    {
        if ((EXP1.ptr == 1)&&(exp1->expr_code == EXPR_VALUE))
        {
            /* absolute register # */
            if ( (val = exp1->expr_value) < 0 || val > MAXREG )
            {
                bad_token(inp_ptr - 1,"Invalid Register expression"); 
                exp1->expr_value = 0;
            }
            exp1->expr_value <<= SRC1_POS;
        }
        else
        {
            /* a register valued expression, build expression
            *  to clip and position it.
            */
            exp1 = EXP1SP + EXP1.ptr;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = MAXREG;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value  = EXPROPER_AND;
            exp1->expr_code = EXPR_VALUE;
            (exp1++)->expr_value = SRC1_POS;
            exp1->expr_code = EXPR_OPER;
            (exp1++)->expr_value = EXPROPER_SHL;
            EXP1.ptr += 4;
        }
        EXP1.tag = 'R';     /* let common code know */
    }
    else if ((EXP1.ptr == 1)&&(exp1->expr_code == EXPR_VALUE)
             &&(exp1->expr_value==0))
    {
        /* constant 0 is accepted as %0 */
        EXP1.tag = 'R';     /* let common code know */
    }
    else
    {
        bad_token(inp_ptr - 1,"Expression must resolve to Register or 0");
        EXP1.ptr = 0;
    }
}

static void get_s2( void )
{
    long val;
    EXPR_struct *exp2;

    get_token();
    exp2 = EXP2SP;
    if ( exprs(1,&EXP2) < 1 )
    {
        /* partially NOP the instruction by clearing Scc */
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register or expression expected");
        return;
    }
    else if ( EXP2.register_reference )
    {
        if ((EXP2.ptr == 1)&&(exp2->expr_code == EXPR_VALUE))
        {
            /* absolute register # */
            if ( (val = exp2->expr_value) < 0 || val > MAXREG )
            {
                bad_token(inp_ptr - 1,"Invalid Register expression"); 
                exp2->expr_value = 0;
            }
            else exp2->expr_value |= REGMARK;
        }
        else
        {
            /* a register valued expression, build expression
            *  to clip and position it.
            */
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = MAXREG;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value  = EXPROPER_AND;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = REGMARK;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_OR;
            EXP2.ptr += 4;
        }
        EXP2.tag = 'R';     /* let common code know */
    }
    else
    {
        EXP2.tag = 'u';
    }
}

static void get_dst( void )
{
    long val;
    EXPR_struct *exp3;

    get_token();
    exp3 = EXP3SP;
    if ( exprs(1,&EXP3) < 1 )
    {
        /* partially NOP the instruction by clearing Scc & leaving dst=R0*/
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register expression expected");
        EXP3.ptr = 0;
    }
    else if ( EXP3.register_reference )
    {
        if ((EXP3.ptr == 1)&&(exp3->expr_code == EXPR_VALUE))
        {
            /* absolute register # */
            if ( (val = exp3->expr_value) < 0 || val > MAXREG )
            {
                bad_token(inp_ptr - 1,"Invalid Register expression"); 
                exp3->expr_value = 0;
            }
            exp3->expr_value <<= DST_POS;
        }
        else
        {
            /* a register valued expression, build expression
            *  to clip and position it.
            */
            exp3->expr_code = EXPR_VALUE;
            (exp3++)->expr_value = MAXREG;
            exp3->expr_code = EXPR_OPER;
            (exp3++)->expr_value  = EXPROPER_AND;
            exp3->expr_code = EXPR_VALUE;
            (exp3++)->expr_value = DST_POS;
            exp3->expr_code = EXPR_OPER;
            (exp3++)->expr_value = EXPROPER_SHL;
            EXP3.ptr += 4;
        }
        EXP3.tag = 'R';     /* let common code know */
    }
    else if ((EXP3.ptr == 1)&&(exp3->expr_code == EXPR_VALUE)
             &&(exp3->expr_value==0))
    {
        /* constant 0 is accepted as %0 */
        EXP3.tag = 'R';     /* let common code know */
    }
    else
    {
        bad_token(inp_ptr - 1,"Expression must resolve to Register or 0");
        EXP3.ptr = 0;
    }
}

/* 		merge_stacks()
*	merges two expression stacks, by copying the shorter to the longer
*	and adding an 'or' operator. Returns the number of the resulting stack
*	and sets its stack ptr correctly. There is a slight preference for
*	returning first.
*/
static int merge_stacks(int first, int second)
{
    short flen,slen;
    int final;
    EXP_stk *ep1,*ep2;
    EXPR_struct *esp1,*esp2;
    ep1 = &exprs_stack[first];
    ep2 = &exprs_stack[second];
    esp1 = ep1->stack;
    esp2 = ep2->stack;

/* if either is empty, return the other... */
    if ( (slen = ep2->ptr) == 0) return first;
    if ( (flen = ep1->ptr) == 0) return second;

    if ( flen < slen )
    {
        /* first is shorter, copy it to second */
        memcpy((char *)(esp2+ep2->ptr),(char *)esp1,
               flen * sizeof(EXPR_struct));
        final = second;
        ep1->ptr = 0;       /* first stack is now empty */
        ep1 = ep2;          /* set final pointer to second stack */
        esp1 = esp2;
    }
    else
    {
        /* second is shorter, copy it to first */
        memcpy((char *)(esp1+ep1->ptr),(char *)esp2,
               slen * sizeof(EXPR_struct));
        final = first;
        ep2->ptr = 0;       /* second stack is now empty */
    }
    flen += slen;           /* my own constant subexpression elimination */
    esp1 = ep1->stack + flen;   /* compute new end of stack */
    ep1->ptr = flen+1;      /* set new size of final stack */
    esp1->expr_code = EXPR_OPER;    /* glue on an "or" to merge the items */
    esp1->expr_value = EXPROPER_OR;
    return final;
}

#if STACK_DEBUG
void dump_stack(int stacknum)
{
    int tag,last;
    EXP_stk *eps;
    EXPR_struct *curr,*top;

    eps = &exprs_stack[stacknum];
    tag = eps->tag;

    fprintf(stderr,"\nStack %d Tag '%c' (0x%02X) depth %d\n",
            stacknum,
            (isgraph(tag) ? tag : '.'),
            tag,
            last = eps->ptr);

    curr = eps->stack;
    top = eps->stack+last;

    for ( ; curr < top ; ++curr )
    {
        switch (tag = curr->expr_code)
        {
        case EXPR_VALUE :
            fprintf(stderr," 0x%lX\n",curr->expr_value);
            break;
        case EXPR_OPER :
            tag = curr->expr_value;
            fprintf(stderr," %c (0x%X)\n",(isgraph(tag) ? tag : '.'),tag);
            break;
        case EXPR_SEG :
            fprintf(stderr," %s + 0x%lX\n",
                    (curr->expt.expt_seg)->seg_string,curr->expr_value);
            break;
        case EXPR_SYM :
            fprintf(stderr,"`%s'\n",
                    (curr->expt.expt_sym)->ss_string);
            break;
        default :
            fprintf(stderr,"?? code: 0x%X value: 0x%lX\n",tag,curr->expr_value);
        }   /* end case */
    }       /* end for */
}       /* end routine dump_stack */

void dump_stacks( void )
{
    int i;
    for ( i = 0 ; i < EXPR_MAXSTACKS ; ++i )
    {
        if ( exprs_stack[i].ptr ) dump_stack(i);
    }
}
#endif

#define NTYPE_REG (1)	/* expression resolves to register */
#define NTYPE_S2  (2)	/* expression can always be used in src2 slot */
#define NTYPE_AS2 (4)	/* expression can be used in Arithmetic src2 slot */
#define NTYPE_LS2 (8)	/* expression can be used in Logical src2 slot */
/* the next two (LEA and SHF) are not yet implemented */
#define NTYPE_LEA (0x10)/* expression can be built with LEA */
#define NTYPE_SHF (0x20)/* expression can be built one shift */
#define NTYPE_ABS (0x40)/* expression is absolute */
#define NTYPE_SYM (0x80)/* expression is symbol_ref, bit 16-23 have section */

long op_ntype( Opcode *opc )
{
    long val,type;
    EXPR_struct *exp2;

    get_token();
    exp2 = EXP2SP;
    type = 0;
    if ( exprs(1,&EXP2) < 1 )
    {
        /* no expression */
        bad_token(inp_ptr - 1,"Register or expression expected");
    }
    else if ( EXP2.register_reference )
    {
        return(NTYPE_REG | NTYPE_S2 | NTYPE_AS2 | NTYPE_LS2);
    }
    else if ( (EXP2.ptr == 1) && (exp2->expr_code == EXPR_VALUE))
    {
        /* an absolute value, classify further */
        type = NTYPE_ABS;
        if ( (val = exp2->expr_value) > 0 && ( val < 0xFFE0 ) )
        {
            type |= (NTYPE_S2 | NTYPE_AS2 | NTYPE_LS2);
        }
        if ( (val < 0) && (-val < 0xFFE0) ) type |= NTYPE_AS2;
        if ( (val < 0) && (~val < 0xFFE0) ) type |= NTYPE_LS2;
    }
    else if ( (EXP2.ptr == 1) && (exp2->expr_code == EXPR_SYM))
    {
        type = ((exp2->expt.expt_sym)->ssp_up.ssp_seg)->seg_ident;
        type = (type << 16) | NTYPE_SYM;
    }
    f1_eatit();
    return type;
}

static SS_struct *literal_pool_sym;
static SEG_struct *literal_pool_ptr;
static long literal_pool_register = 26<<SRC1_POS;

int op_using( Opcode *opc )
{
    EXPR_struct *exp3;
    SEG_struct *segp;
    SS_struct *sym=0;
    exp3 = EXP3SP;
    get_dst();
    if (EXP3.ptr == 1 && exp3->expr_code == EXPR_VALUE && EXP3.tag == 'R' && exp3->expr_value != 0)
    {
        if (*inp_ptr++ != ',')
        {
            bad_token(inp_ptr-1, "Comma expected here");
            f1_eatit();
            return 0;
        }
        get_token();
        if (token_type != TOKEN_strng)
        {
            bad_token(tkn_ptr, "Expected a section name here");
            f1_eatit();
            return 0;
        }
        sym = sym_lookup(token_pool, SYM_INSERT_IF_NOT_FOUND);
        if ((new_symbol&SYM_ADD) != 0)
        {
            sym->ss_fnd = current_fnd;
            sym->ss_line = current_fnd->fn_line;
            sym->flg_global = 1;
            sym->ss_string = token_pool;
            token_pool_size -= token_value+1;
            token_pool += token_value+1;
        }
        segp = find_segment(sym->ss_string, seg_list, seg_list_index);
        if (segp == 0)
        {
            segp = get_seg_mem(&sym, sym->ss_string);
            segp->flg_literal = 1;
            segp->seg_salign = macxx_rel_salign;
            segp->seg_dalign = macxx_rel_dalign;
            segp->seg_maxlen = 256*1024-(32*4);
        }
        literal_pool_sym = sym;
        literal_pool_ptr = segp;
        literal_pool_register = exp3->expr_value>>(DST_POS-SRC1_POS);
        return 0;
    }
    bad_token(tkn_ptr, "First argument must a register > 0");
    return 0;
}

int op_ldlit( Opcode *opc )
{
    int which_stack;
    EXPR_struct *exp0, *exp1, *exp;
#if 0
    EXPR_struct *exp2;
#endif
    EXP0.tag = 'l';          /* opcode is default U32 */
    EXP0.tag_len = 1;            /* only 1 longword */
    EXP0.ptr = 1;            /* first stack has opcode (1 element) */
    exp0 = EXP0SP;           /* point to expression stack 0 */
    exp1 = EXP1SP;           /* point to expression stack 1 */
#if 0
    exp2 = EXP2SP;           /* point to expression stack 2 */
#endif
    exp0->expr_code = EXPR_VALUE;    /* set the opcode expression */
    exp0->expr_value = 0x80000000L;  /* opcode for LD */
    EXP1.ptr = 0;            /* assume no other operands */
    EXP2.ptr = 0;            /* assume no other operands */
    EXP3.ptr = 0;            /* assume no other operands */
    EXP2.tag = 0;            /* assume no tags either */
    EXP3.tag = 0;
    EXP2.tag_len = 1;            /* but if there is a tag, len = 1 */
    EXP3.tag_len = 1;
    get_dst();               /* first arg is a register */
    if ( *inp_ptr++ != ',' )
    {
        bad_token(inp_ptr - 1,"Comma expected");
    }
    get_token();             /* pickup next value */
    which_stack = merge_stacks(0,3); /* merge the stacks */
    exp = exprs_stack[which_stack].stack + exprs_stack[which_stack].ptr;
    if (exprs(1, &EXP1) >= 0)
    {      /* expression present */
        long v, newop = 0;
        if (EXP1.ptr == 1 && exp1->expr_code == EXPR_VALUE)
        {
            v = exp1->expr_value;
            if (v >= 0 && v < 0xFFE0L)
            {
                newop = 0x08<<27;       /* ADD opcode */
            }
            else if (v < 0 && v > -0xFFE0L)
            {
                newop = 0x09<<27;       /* SUB opcode */
                v = 0-v;
            }
            else if (v > 0)
            {
                if ((v&3) == 0 && v < 0x3FF80)
                {
                    newop = 0x03<<27;        /* LEA opcode */
                    v >>= 2;         /* scale it by longs */
                }
                else if ((v&1) == 0 && v < 0x1FFC0)
                {
                    newop = 0x04<<27;        /* LEAS opcode */
                    v >>= 1;         /* scale it by shorts */
                }
            }
        }
        if (newop != 0)
        {         /* did we changed the opcode? */
            exp = exprs_stack[which_stack].stack;  /* point to our opcode (stack element 0) */
            exp->expr_value &= ~(0x1F<<27);    /* whack out the old one */
            exp->expr_value |= newop|v;        /* plop in the new one and the argument */
            EXP1.ptr = 0;          /* stack 1 is now empty */
        }
        else
        {              /* else, do the hard way */
            int olist;
            SEG_struct *oldseg;
            EXPR_struct *oexp = exp;       /* remember where we started */
            oldseg = current_section;      /* save current section ptr */
            current_section = literal_pool_ptr;    /* set section to literal pool */
            exp->expr_code = EXPR_VALUE;       /* or in the register */
            (exp++)->expr_value = literal_pool_register;
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_OR;
            exp->expr_code = EXPR_SEG;     /* make expression: "(arg-.LITPOOL.)>>2[.LP]" */
            exp->expr_seg = current_section;
            (exp++)->expr_value = current_offset;
            exp->expr_code = EXPR_SYM;
            (exp++)->expr_sym = literal_pool_sym;
            literal_pool_sym->flg_ref = 1; /* signal we've touched the lit pool name */
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_SUB;
            exp->expr_code = EXPR_VALUE;
            (exp++)->expr_value = 2;
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_SHR;
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_OR;
            exprs_stack[which_stack].ptr += exp-oexp;
            EXP1.tag = ( edmask & ED_M68 ) ? 'L' : 'l';
            EXP1.tag_len = 1;
            olist = list_bin;
            list_bin = 0;          /* don't print anything */
            EXP1.ptr = compress_expr(&EXP1);
            p1o_long(&EXP1);           /* drop expression into literal pool */
            if (current_section->seg_pc > current_section->seg_len)
            {
                current_section->seg_len = current_section->seg_pc;
            }
            list_bin = olist;
            EXP1.ptr = 0;          /* e1 is empty now */
            current_section = oldseg;
        }
    }
    exprs_stack[which_stack].tag = ( edmask & ED_M68 ) ? 'L' : 'l';
    exprs_stack[which_stack].tag_len = 1;    /* only 1 longword */
    exprs_stack[which_stack].ptr = compress_expr(exprs_stack+which_stack);
    if (list_bin) compress_expr_psuedo(exprs_stack+which_stack);
    p1o_long(exprs_stack+which_stack);   /* store opcode */
    return 0;
}

#define MAC_AS
#include "opcommon.h"
