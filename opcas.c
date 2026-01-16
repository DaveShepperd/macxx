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
#include "asop_class.h"
#include "opcas_dmp.h"

#define OP_DEBUG 0

#define SRC1_POS 16
#define SCC_POS  21
#define SCC_BIT  (1<<SCC_POS)
#define DST_POS  22
#define OPC_POS  27
#define MAXREG   31
#define MAX_PS_BITS (0x3F)
#define MAX_S2_VALUE (0xFFE0)
#define REGMARK 0xFFE0
#define DISP_MASK (-MIN_DISP | MAX_DISP)
#define DEFNAM(name,numb) {"name",name,numb},

/* next four variables are for compatability with V7.4 Macxx */
uint16_t rel_salign = 2; /* default alignment for rel segments */
uint16_t rel_dalign = 2; /* def algnmnt for data in rel segments */
uint16_t abs_salign = 2; /* default alignment for abs segments */
uint16_t abs_dalign = 2; /* def algnmnt for data in abs segments */

const int macxx_name_mask = MACXX_M_AS;
const char macxx_name[] = "macas";
const char *macxx_target = "ASAP";
const char *macxx_descrip = "Cross assembler for the ASAP.";

uint16_t macxx_salign = 2;    /* default alignment segments by LLF */
uint16_t macxx_dalign = 2;    /* default alignment data within segment */
uint16_t macxx_min_dalign = 2;

char macxx_mau = 8;         /* # of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

uint32_t macxx_edm_default = ED_LC|ED_GBL|ED_TRUNC; /* default edmask */
uint32_t macxx_lm_default = ~(LIST_ME|LIST_MEB|LIST_MES|LIST_LD|LIST_COD|LIST_OCT); /* default list mask */

int current_radix = 10;     /* default the radix to decimal */
char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
/* char expr_escape = '^'; */     /* char that escapes a unary expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */
char open_operand = '[';    /* char that opens an operand indirection */
char close_operand = ']';   /* char that closes an operand indirection */
int max_opcode_length = 16; /* significant length of opcodes and */
int max_symbol_length = 16; /*  symbols */

static char *am_ptr;
static SS_struct *literal_pool_sym;
static SEG_struct *literal_pool_ptr;
static int32_t literal_pool_register = 26<<SRC1_POS;

/* End of processor specific stuff */

int ust_init(void )
{
	if ( !options[QUAL_2_PASS] || !pass )
	{
		SS_struct *sym_ptr;
		SEG_struct *segp;
		strcpy(token_pool, ".LITPOOL.");
		token_value = sizeof(".LITPOOL.")-1;
		literal_pool_sym = sym_ptr = sym_lookup(token_pool, SYM_INSERT_IF_NOT_FOUND);
		sym_ptr->ss_fnd = current_fnd;
		sym_ptr->ss_line = current_fnd->fn_line;
		sym_ptr->flg_global = 1;
		literal_pool_ptr = segp = (SEG_struct *)get_seg_mem(&sym_ptr, sym_ptr->ss_string);
		segp->flg_literal = 1;
		segp->seg_salign = macxx_salign;
		segp->seg_dalign = macxx_dalign;
		segp->seg_maxlen = 256*1024-(32*4);
	}
    return 1;              /* no custom symbol table stuff for ASAP */
}

#define MNBI 0x8000		/* operand must not be indexed */
#define MBI  0x4000		/* operand must be indexed */
#define FIN  0x2000		/* opening @ or ( found */

static int get_ea(uint16_t size);
static int get_dst(void),get_s1(void),get_s2(void);
static void merge_stacks(int a, int b);

static void addOutOfRangeTest( EXP_stk *dstStk, int32_t lowLimit, int32_t hiLimit, int align )
{
	EXPR_struct *dst;
	
	dst = dstStk->stack+dstStk->ptr;
	if ( align )
	{
		dst->expr_code = EXPR_VALUE;
		dst->expr_value = 0;        /* Duplicate the target address to tos */
		++dst;
		dst->expr_code = EXPR_OPER;
		dst->expr_value = EXPROPER_PICK;
		++dst;
		dst->expr_code = EXPR_VALUE;
		dst->expr_value = align;        /* check for target address alignment */
		++dst;
		dst->expr_code = EXPR_OPER;
		dst->expr_value = EXPROPER_AND;
		++dst;
		dst->expr_code = EXPR_VALUE;
		dst->expr_value = 0;		     /* The TST_NE below compares the result of the AND above and this 0 */
		++dst;
		dst->expr_code = EXPR_OPER;
		dst->expr_value = EXPROPER_TST | (EXPROPER_TST_NE<<8);
		++dst;
		dst->expr_code = EXPR_OPER;
		dst->expr_value = EXPROPER_XCHG;    /* swap top two stack entries (TST_NE result and target address) */
		++dst;
	}
	dst->expr_code = EXPR_VALUE;
	dst->expr_value = 0;        /* Duplicate target address to tos */
	++dst;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = EXPROPER_PICK;
	++dst;
	dst->expr_code = EXPR_VALUE;
	dst->expr_value = hiLimit;  /* check for out of range positive */
	++dst;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = EXPROPER_TST | (EXPROPER_TST_GT<<8);
	++dst;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = EXPROPER_XCHG;    /* swap result with target address */
	++dst;
	dst->expr_code = EXPR_VALUE;
	dst->expr_value = lowLimit; /* check for out of range negative */
	++dst;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = EXPROPER_TST | (EXPROPER_TST_LT<<8);
	++dst;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = EXPROPER_OR;	/* combine result of the two tests */
	++dst;
	if ( align )
	{
		dst->expr_code = EXPR_OPER;	/* combine with result of alignment test */
		dst->expr_value = EXPROPER_OR;
		++dst;
	}
	dstStk->ptr = dst - dstStk->stack;	/* set stack size */
	DUMP_STACK(stdout,"addOutOfRangeTest() dst limit check before compress",dstStk);
	dstStk->ptr = compress_expr(dstStk);	/* squeeze the expression if possible */
	DUMP_STACK(stdout,"addOutOfRangeTest() dst limit check after compress",dstStk);
	snprintf(emsg, sizeof(emsg)-1, "%s:%d", current_fnd->fn_name_only, current_fnd->fn_line);
	dstStk->tag = 0;
	write_to_tmp(TMP_OOR, 0, (char *)dstStk, 0);
	write_to_tmp(TMP_ASTNG, strlen(emsg)+1, emsg, 1);
}

static void mergeSrc2(EXP_stk *exp0Stk, EXP_stk *src2Stk)
{
	EXPR_struct *exp0, *dst;
	int ptrSave;
	
	ptrSave = src2Stk->ptr;
	addOutOfRangeTest(src2Stk,0,MAX_S2_VALUE,0);
	src2Stk->ptr = ptrSave;
	dst = src2Stk->stack+src2Stk->ptr;
	dst->expr_code = EXPR_VALUE;
	(dst++)->expr_value = 0x0000FFFF;
	dst->expr_code = EXPR_OPER;
	(dst++)->expr_value = EXPROPER_AND;
	src2Stk->ptr += 2;

	exp0 = exp0Stk->stack+exp0Stk->ptr;
	exp0->expr_code = EXPR_VALUE;
	exp0->expr_value = 0xFFFF0000;
	++exp0;
	exp0->expr_code = EXPR_OPER;
	exp0->expr_value = EXPROPER_AND;
	++exp0;
	memcpy((uint8_t *)exp0, (uint8_t *)src2Stk->stack, src2Stk->ptr*sizeof(EXPR_struct));
	exp0 += src2Stk->ptr;
	exp0->expr_code = EXPR_OPER;
	exp0->expr_value = EXPROPER_OR;
	++exp0;
	exp0Stk->ptr = exp0 - exp0Stk->stack;
	DUMP_STACK(stdout,"mergeSrc2() Stk0",exp0Stk);
	src2Stk->ptr = 0;
}

void do_opcode(Opcode *opc)
{
    int hiword,abs;
    int err_cnt = error_count[MSG_ERROR];
	int32_t val;
    EXPR_struct *exp0,*exp1,*exp2;

    EXP0.tag = 'l';         /* opcode is default U32 */
    EXP0.tag_len = 1;       /* only 1 longword */
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
    exp0->expr_value = (hiword & 0xFFE0) << 16;	/* Low order 5 bits has the class */
    am_ptr = inp_ptr;           /* remember where am starts */
    switch ( (opc->op_class & 0x1F) )
    {
    case OPCL_AU :
        /* ALU ops: 
        *	OP	%i,%j,%k	reg.i <- reg.j OP reg.k
        *	OP	%i,%j,k		reg.i <- reg.j OP k (constant)
        */
#if OP_DEBUG
        fprintf(stderr,"got class AU, value: %04X %s",
                opc->op_value, am_ptr);
#endif

        if (!get_dst())
		{
			f1_eatit();
			return;
		}

        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
			f1_eatit();
			return;
        }

        if (!get_s1())
		{
			f1_eatit();
			return;
		}

        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
			f1_eatit();
			return;
        }

        if (!get_s2())
		{
			f1_eatit();
			return;
		}
        break;

	case OPCL_LD :
#if OP_DEBUG
		if ( (opc->op_class & 7) == OPCL_LD )
		{
			fprintf(stderr,"got class LD, value: %04X %s",
					opc->op_value, am_ptr);
		}
#endif
		// intentional fall through to OPCL_ST which will then intentionally fall through to OPCL_JS
	case OPCL_ST:
#if OP_DEBUG
		if ( (opc->op_class & 7) == OPCL_ST )
		{
			fprintf(stderr,"got class ST, value: %04X %s",
					opc->op_value, am_ptr);
		}
#endif
		// intentional fall through to OPCL_JS
	case OPCL_JS:
#if OP_DEBUG
		if ( (opc->op_class & 7) == OPCL_ST )
		{
			fprintf(stderr,"got class ST, value: %04X %s",
					opc->op_value, am_ptr);
		}
#endif
		/* load ops: 
		*	LDxx	%k,%i[%j]	reg.k <- @(reg.i + (reg.j << size))
		*	LDxx	%k,%i[j]	reg.k <- @(reg.i + (j << size))
		*   LDxx    %k,where    reg.k <- @where
		*	LEAxx	%k,%i[j]	reg.k <- (reg.i + (j << size))
		*   LEAXx   %k,where    reg.k <- where
		* store ops:
		*	STxx	%k,%i[%j]	reg.k -> @(reg.i + (reg.j << size))
		*	STxx	%k,%i[j]	reg.k -> @(reg.i + (j << size))
		*   STxx    %k,where    reg.k -> @where
		* Jump to subroutine
		*   JSR     %k,%i[%j]   reg.k <- PC+4; LPC <- (reg.i + reg.j << size)
		*   JSR     %k,%i[j]    reg.k <- PC+4; LPC <- (reg.i + j << size)
		*   JSR     %k,where    reg.k <- PC+4; LPC <- where
		*/
        if ( !get_dst() )
		{
			f1_eatit();
			return;
		}
        if ( *inp_ptr++ != ',' )
        {
            bad_token(inp_ptr - 1,"Comma expected");
			f1_eatit();
			return;
        }

        if ( !get_ea(hiword & 3) )
		{
			f1_eatit();
			return;
		}
        break;

	case OPCL_BS:
		/* branch to subroutine: 
		*	Bsr	%link,where
		*/
		/* get link register, then merge with Bcc */
#if OP_DEBUG
		fprintf(stderr,"got class BS, value: %04X %s",
				opc->op_value, am_ptr);
#endif
		if (!get_dst())
		{
			f1_eatit();
			return;
		}

		if ( *inp_ptr++ != ',' )
		{
			bad_token(inp_ptr - 1,"Comma expected");
			f1_eatit();
			return;
		}
		// intentional fall through to OPCL_BC
    case OPCL_BC:
#if OP_DEBUG
		if ( (opc->op_class & 7) == OPCL_BC )
		{
			/* conditional branches: 
			*	Bcc	where
			*/
			fprintf(stderr,"got class BC, value: %04X %s",
					opc->op_value, am_ptr);
		}
#endif
        get_token();
        if (exprs(1,&EXP1) < 1)
        {       /* expression nfg or not present */
            exp1->expr_code = EXPR_VALUE;   /* make it a 0 */
            exp1->expr_value = -4;      /* make it a br . */
            EXP1.ptr = 1;           /* 1 item on stack */
            bad_token(inp_ptr - 1,"Expression expected");
			f1_eatit();
			return;
        }
		exp1 = EXP1SP + EXP1.ptr;
		exp1->expr_code = EXPR_SEG;
		exp1->expr_value = current_offset + BR_OFF;
		exp1->expr_seg = current_section;
		++exp1;
		exp1->expr_code = EXPR_OPER;
		exp1->expr_value = '-';
		++exp1;
		EXP1.ptr += 2;
		EXP1.ptr = compress_expr(&EXP1);
        EXP1.tag = 'O';     /* flag as branch offset */
        exp1 = EXP1SP;
		abs = EXP1.ptr == 1 && exp1->expr_code == EXPR_VALUE;
		val = exp1->expr_value;
        if ( abs )
        {
            /* absolute difference, check range */
            if ( val < MIN_DISP || val > MAX_DISP )
            {
                int32_t toofar;
                toofar = val;
                if (toofar > 0)
                {
                    toofar -= MAX_DISP ;
                }
                else
                {
                    toofar = -toofar + MIN_DISP;
                }
                sprintf(emsg,"Branch offset 0x%X byte(s) out of range",toofar);
                bad_token(NULL,emsg);
				break;	/* from switch */
            }
            if ((val&3) != 0)
            {
                bad_token(NULL,"Branch to non-long-aligned address");
				break;	/* from switch */
            }
            /* absolute difference (possibly cooked), mask and shift */
            val &= (-MIN_DISP|MAX_DISP);
            val >>= 2;
			exp1->expr_value = val;
            EXP1.ptr = 1;
        }
        else
        {
            if (options[QUAL_BOFF])
            {
				int savePtr = EXP1.ptr;
				addOutOfRangeTest(&EXP1, MIN_DISP, MAX_DISP, 3);
				EXP1.ptr = savePtr;
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
        merge_stacks(0,1);
        break;

    case OPCL_PS :
        /* putps, getps */
#if OP_DEBUG
        fprintf(stderr,"got class PS, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        if ( hiword == (OP_PUTPS << 11) )
        {
            /* PUTPS, get src2 */
            if (!get_s2())
			{
				f1_eatit();
				return;
			}
        }
        else
        {
            /* GETPS, get dst */
            if (!get_dst())
			{
				f1_eatit();
				return;
			}
        }
        break;
		
    case OPCL_IL :
        /* illegal and sysill */
#if OP_DEBUG
        fprintf(stderr,"got class IL, value: %04X %s",
                opc->op_value, am_ptr);
#endif
        break;
	case OPCL_SY:
		get_token();
		if ( exprs(1,&EXP2) < 1 )
		{
			/* partially NOP the instruction by clearing Scc */
			EXP0SP->expr_value &= ~SCC_BIT;
			bad_token(inp_ptr - 1,"Register or expression expected");
			break;
		}
		exp2 = EXP2SP;
		val = exp2->expr_value;	/* pickup expression's value */
		abs = (EXP2.ptr == 1 && exp2->expr_code == EXPR_VALUE); /* abs is true if expression absolute */
		if ( abs )
		{
			if ( EXP2.register_reference ) /* any register reference */
			{
				if ( val < 1 || val > 5 )
				{
					bad_token(inp_ptr - 1,"Invalid Register expression. Can only be 1 through 5 inclusive"); 
					break;
				}
				exp2->expr_value |= REGMARK;	/* convert to register indicator */
			}
			else if ( exp2->expr_value < 1 || exp2->expr_value > 255 )
			{
				bad_token(inp_ptr, "Value must lie between 1 and 255 inclusive");
				break;
			}
		}
		else
		{
			int savePtr;
			if ( EXP2.register_reference ) /* any register reference */
			{
				bad_token(NULL,"Register expression has to be absolute");
				break;
			}
			savePtr = EXP2.ptr;
			addOutOfRangeTest(&EXP2, 1, 255, 0);
			EXP2.ptr = savePtr;
		}
		break;			/* from switch(class) */
    }                    /* -- switch(class) */
    if (err_cnt != error_count[MSG_ERROR])
    { /* if any errors detected... */
        f1_eatit();               /* ...eat to EOL */
    }
    merge_stacks(0,1);
    merge_stacks(0,3);
    merge_stacks(0,2);
    EXP0.ptr = compress_expr(&EXP0);
    EXP0.tag = ( edmask & ED_M68 ) ? 'L' : 'l';
    if (list_bin) compress_expr_psuedo(&EXP0);
    p1o_long(&EXP0);      /* output inst */
}                   /* -- opc_as() */

static int get_ea(uint16_t size)
{
    int32_t val;
	int abs,s2IsReg=0;
    EXPR_struct *exp1,*exp2;

	/* Effective address syntax is:
	 *    register
	 * or fred
	 * or register[offset]
	 */
    exp1 = EXP1SP;
    get_token();
    if ( exprs(1,&EXP1) < 1)
    {  /* bad or no expression */
        /* NOP the instruction by clearing Scc, leaving rdst = %0 */
        EXP0SP->expr_value &= ~SCC_BIT;
        EXP1.ptr = 0;
        bad_token(inp_ptr - 1,"Register expression expected");
        return 0;
    }
	val = exp1->expr_value;	/* pickup expression's value */
	abs = (EXP1.ptr == 1 && exp1->expr_code == EXPR_VALUE); /* abs is true if expression absolute */
	if ( abs )
	{
		if ( EXP1.register_reference ) /* if any register reference */
		{
			if ( val >= 0 && val <= MAXREG )
			{
				exp1->expr_value = val << SRC1_POS;
				s2IsReg = 1;
			}
			else
			{
				bad_token(inp_ptr - 1,"Invalid Register expression"); 
				exp1->expr_value = 0;
				return 0;
			}
		}
		else if ( !val )        /* a constant of 0 might mean register 0 */
		{
			while ( *inp_ptr && *inp_ptr != '\n' && isspace(*inp_ptr) )
				++inp_ptr;
			if ( *inp_ptr == open_operand )
				s2IsReg = 1;
		}
	}
	else if ( EXP1.register_reference )
	{
		bad_token(inp_ptr - 1,"Invalid Register expression"); 
		exp1->expr_value = 0;
		return 0;
	}
	/* constant 0 is accepted as %0 */
    if ( !s2IsReg )
    {
		if ( *inp_ptr == open_operand  )
		{
			/* he got the syntax right but muffed the semantics. First term must be a register or a 0 */
			EXP1.ptr = 0;
			bad_token(inp_ptr - 1,"expression must resolve to Register or 0");
			return 0;
		}
		/* special shorthand hack for opc fred => opc %0[fred] */
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
		if ( EXP1.ptr != 1 || EXP1SP->expr_code != EXPR_VALUE || (EXP1SP->expr_value < 0 || EXP1SP->expr_value > MAX_S2_VALUE ) )
		{
			int savePtr;
			savePtr = EXP1.ptr;
			addOutOfRangeTest(&EXP1, 0, MAX_S2_VALUE, 0);
			EXP1.ptr = savePtr;
		}
		return 1;
    }

    if ( *inp_ptr++ != open_operand )
    {
        sprintf(emsg,"%c expected",open_operand);
        bad_token(inp_ptr - 1,emsg);
		return 0;
    }
	/* First term is a legit register. So syntax from here is reg[expr]. open_operand has been eaten by inp_ptr++ above */
    exp2 = EXP2SP;
    get_token();
    if ( exprs(1,&EXP2) < 1 )
    {
        /* partially NOP the instruction by clearing Scc */
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register or expression expected");
        return 0;
    }
	val = exp2->expr_value;	/* pickup expression's value */
	abs = (EXP2.ptr == 1 && exp2->expr_code == EXPR_VALUE); /* abs is true if expression absolute */
    if ( EXP2.register_reference ) /* any register reference */
    {
		if ( !abs )
		{
			bad_token(NULL,"Register expression has to be absolute");
			return 0;
		}
		if ( val < 0 || val > MAXREG )
		{
			bad_token(inp_ptr - 1,"Invalid Register expression"); 
			return 0;
		}
		exp2->expr_value |= REGMARK;	/* convert to register indicator */
    }
    else
    {
        if ( size )
        {
			/* operand has to be shifted right according to size */
            exp2 = EXP2SP + EXP2.ptr;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = size;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_SHR;
            EXP2.ptr += 2;
            EXP2.ptr = compress_expr(&EXP2);
        }
		/* and it has to be checked for being within limits */
		if ( EXP2.ptr != 1 || EXP2SP->expr_code != EXPR_VALUE || (EXP2SP->expr_value < 0 || EXP2SP->expr_value > MAX_S2_VALUE ) )
		{
			int savePtr;
			savePtr = EXP2.ptr;
			addOutOfRangeTest(&EXP2, 0, MAX_S2_VALUE, 0);
			EXP2.ptr = savePtr;
		}
    }

    if ( *inp_ptr++ != close_operand )
    {
        sprintf(emsg,"%c expected",close_operand);
        bad_token(inp_ptr - 1,emsg);
		return 0;
    }
	return 1;
}

static int get_s1( void )
{
	int32_t val;
	int abs=0;
    EXPR_struct *exp1;

    get_token();
    exp1 = EXP1SP;
    if ( exprs(1,&EXP1) < 1 )
    {
        /* partially NOP the instruction by clearing Scc */
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register expression expected");
        return 0;
    }
	val = exp1->expr_value;	/* pickup expression's value */
	abs = (EXP1.ptr == 1 && exp1->expr_code == EXPR_VALUE); /* abs is true if expression absolute */
	if (    EXP1.register_reference /* any register reference */
		 || (abs && !val)			/* or a constant of 0 which means register 0 */
	   )
	{
		if ( !abs || val < 0 || val > MAXREG )
		{
			bad_token(inp_ptr - 1,"Invalid Register expression"); 
			return 0;
		}
		exp1->expr_value = val << SRC1_POS;
		return 1;
	}
	bad_token(inp_ptr - 1,"Expression must resolve to Register or 0");
	EXP1.ptr = 0;
	return 0;
}

static int get_s2( void )
{
	int abs;
    int32_t val;
    EXPR_struct *exp2;

    get_token();
    exp2 = EXP2SP;
    if ( exprs(1,&EXP2) < 1 )
    {
        /* partially NOP the instruction by clearing Scc */
        EXP0SP->expr_value &= ~SCC_BIT;
        bad_token(inp_ptr - 1,"Register or expression expected");
        return 0;
    }
	val = exp2->expr_value;
	abs = (EXP2.ptr == 1) && (exp2->expr_code == EXPR_VALUE);
	if ( EXP2.register_reference )
	{
		if ( !abs || val < 0 || val > MAXREG )
		{
			bad_token(inp_ptr - 1, "Invalid Register expression");
			EXP2.ptr = 0;
			return 0;
		}
		EXP2.ptr = 0;
		EXP0.stack[0].expr_value |= val | REGMARK;
		return 1;
	}
	if ( abs && val >= 0 && val < REGMARK )
    {
		EXP2.ptr = 0;
		EXP0.stack[0].expr_value |= val;
		return 1;
	}
	mergeSrc2(&EXP0,&EXP2);
	return 1;
}

static int get_dst( void )
{
    int32_t val;
	int abs;
    EXPR_struct *exp3;

    get_token();
    exp3 = EXP3SP;
    if ( exprs(1,&EXP3) < 1 )
    {
        bad_token(inp_ptr - 1,"Register expression expected");
		return 0;
    }
	val = exp3->expr_value;
	abs = (EXP3.ptr == 1) && (exp3->expr_code == EXPR_VALUE);
    if ( !EXP3.register_reference || !abs || val < 0 || val > MAXREG )
    {
		bad_token(inp_ptr - 1,"Expression must resolve to Register or 0");
		exp3->expr_value = 0;
		return 0;
    }
	exp3->expr_value <<= DST_POS;
	return 1;
}

/* 		merge_stacks()
*	merges two expression stacks by copying the source into the dest
*	and adding an 'or' operator.
*/
static void merge_stacks(int dest, int source)
{
	EXP_stk *dstStk,*srcStk;
	srcStk = exprs_stack + source;

/* if source has something in it */
    if ( srcStk->ptr )
	{
		EXPR_struct *dst,*src;
		dstStk = exprs_stack + dest;
		dst = dstStk->stack + dstStk->ptr;
		src = srcStk->stack;
		memcpy((uint8_t *)dst, (uint8_t *)src, srcStk->ptr * sizeof(EXPR_struct));
		dst += srcStk->ptr;
		dst->expr_code = EXPR_OPER;    /* glue on an "or" to merge the items */
		dst->expr_value = EXPROPER_OR;
		dstStk->ptr += srcStk->ptr+1;
		srcStk->ptr = 0;
		return;
	}
    return;
}

#define NTYPE_REG (1)	/* expression resolves to register */
#define NTYPE_S2  (2)	/* expression can always be used in src2 slot */
#define NTYPE_AS2 (4)	/* expression can be used in Arithmetic src2 slot */
#define NTYPE_LS2 (8)	/* expression can be used in Logical src2 slot */
/* the next two (LEA and SHF) are not yet implemented */
#define NTYPE_LEA (0x10)/* expression can be built with LEA */
#define NTYPE_SHF (0x20)/* expression can be built one shift */
#define NTYPE_ABS (0x40)/* expression is absolute */
#define NTYPE_SYM (0x80)/* expression is symbol_ref, bit 16-23 have section */

int32_t op_ntype( Opcode *opc )
{
    int32_t val,type;
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

int op_using( Opcode *opc )
{
    EXPR_struct *exp3;
    SEG_struct *segp;
    SS_struct *sym=0;
    exp3 = EXP3SP;
    get_dst();
    if (EXP3.ptr == 1 && exp3->expr_code == EXPR_VALUE && exp3->expr_value != 0)
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
            segp->seg_salign = macxx_salign;
            segp->seg_dalign = macxx_dalign;
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
	int32_t val, newop = 0;
	int abs;
    EXPR_struct *exp0, *exp1, *exp;
	
    EXP0.tag = 'l';          /* opcode is default U32 */
    EXP0.tag_len = 1;            /* only 1 longword */
    EXP0.ptr = 1;            /* first stack has opcode (1 element) */
    exp0 = EXP0SP;           /* point to expression stack 0 */
    exp1 = EXP1SP;           /* point to expression stack 1 */
    exp0->expr_code = EXPR_VALUE;    /* set the opcode expression */
    exp0->expr_value = _BIT_31_; /* 0x80000000; */  /* opcode for LD */
    EXP1.ptr = 0;            /* assume no other operands */
    EXP2.ptr = 0;            /* assume no other operands */
    EXP3.ptr = 0;            /* assume no other operands */
    EXP2.tag = 0;            /* assume no tags either */
    EXP3.tag = 0;
    EXP2.tag_len = 1;            /* but if there is a tag, len = 1 */
    EXP3.tag_len = 1;
	list_stats.pc_flag = 1;
	/* first arg is destination register */
    if (!get_dst())
	{
		f1_eatit();
		return 0;
	}
    if ( *inp_ptr++ != ',' )
    {
        bad_token(inp_ptr - 1,"Comma expected");
		f1_eatit();
		return 0;
    }
    get_token();             /* pickup next value */
    merge_stacks(0,3);		/* merge the stacks (place destination register) */
    exp = EXP0.stack + EXP0.ptr;
	if ( exprs(1, &EXP1) < 0 )
	{
		bad_token(inp_ptr - 1, "Invalid expression");
		f1_eatit();
		return 0;
	}
	val = exp1->expr_value;
	abs = EXP1.ptr == 1 && exp1->expr_code == EXPR_VALUE;
	if (abs)
	{
		if (val >= 0 && val < MAX_S2_VALUE)
		{
			newop = 0x08<<27;       /* convert to ADD opcode */
		}
		else if (val < 0 && val > -MAX_S2_VALUE)
		{
			newop = 0x09<<27;       /* convert to SUB opcode */
			val = 0-val;
		}
		else if (val > 0)
		{
			if ((val&3) == 0 && val < MAX_S2_VALUE*4)
			{
				newop = 0x03<<27;        /* convert to LEA opcode */
				val >>= 2;         /* scale it by longs */
			}
			else if ((val&1) == 0 && val < MAX_S2_VALUE*2)
			{
				newop = 0x04<<27;        /* convert to LEAS opcode */
				val >>= 1;         /* scale it by shorts */
			}
		}
	}
	if (newop != 0)
	{   /* we changed the opcode? */
		exp = EXP0.stack;  /* point to our opcode (stack element 0) */
		exp->expr_value &= ~(0x1F<<27);    /* whack out the old one */
		exp->expr_value |= newop|val;      /* plop in the new one and the argument */
		EXP1.ptr = 0;          /* stack 1 is now empty */
	}
	else
	{   /* else, do the hard way */
		int olist;
		SEG_struct *oldseg;
		EXP_stk *tmpStk = &EXP2;
		EXPR_struct *tmpExp = tmpStk->stack;
		
		EXP0SP->expr_value |= literal_pool_register;
		
		oldseg = current_section;      /* save current section ptr */
		current_section = literal_pool_ptr;    /* set section to literal pool */
		current_offset = (current_offset + 3) & -4;	/* Make sure all litpool elements are longword aligned */
		tmpExp->expr_code = EXPR_SEG;	/* prepare expression: ".LP[(arg-{sym}.LITPOOL.)>>2]" */
		tmpExp->expr_seg = current_section;
		tmpExp->expr_value = current_offset;
		++tmpExp;
		tmpExp->expr_code = EXPR_SYM;
		tmpExp->expr_sym = literal_pool_sym;
		literal_pool_sym->flg_ref = 1; /* signal we've touched the lit pool name */
		++tmpExp;
		tmpExp->expr_code = EXPR_OPER;
		tmpExp->expr_value = EXPROPER_SUB;
		++tmpExp;
		tmpExp->expr_code = EXPR_VALUE;
		tmpExp->expr_value = 2;
		++tmpExp;
		tmpExp->expr_code = EXPR_OPER;
		tmpExp->expr_value = EXPROPER_SHR;
		++tmpExp;
		tmpStk->ptr = tmpExp-tmpStk->stack;
		mergeSrc2(&EXP0,tmpStk);
		
		EXP1.tag = ( edmask & ED_M68 ) ? 'L' : 'l';
		EXP1.tag_len = 1;
		olist = list_bin;
		lm_bits &= ~LIST_BIN;          /* don't print anything */
		EXP1.ptr = compress_expr(&EXP1);
		p1o_long(&EXP1);           /* drop expression into literal pool */
		if (current_section->seg_pc > current_section->seg_len)
		{
			current_section->seg_len = current_section->seg_pc;
		}
		lm_bits |= olist;
		EXP1.ptr = 0;          /* e1 is empty now */
		current_section = oldseg;
	}
    EXP0.tag = ( edmask & ED_M68 ) ? 'L' : 'l';
    EXP0.tag_len = 1;    /* only 1 longword */
    EXP0.ptr = compress_expr(&EXP0);
    if (list_bin) compress_expr_psuedo(&EXP0);
    p1o_long(&EXP0);   /* store opcode */
    return 0;
}

#define MAC_AS
#include "opcommon.h"
