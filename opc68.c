/*
	opc68.c - Part of macxx, a cross assembler family for various micro-processors
	Copyright (C) 2008 David Shepperd
  TG -  edited Dave Shepperd code 12/07/2022
 
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

	09/29/2025  - Added check_4_abs() function. Emit an error if no operand
			provided but one expected. Handle "op A" and "op B". Fixed bug
			in "xxX #expr" not providing for 16 bit operand. DMS
			
    08/09/2023	- Added support for BR S,+n and BR S,-n syntax  - Tim Giddens
		  All Branch instructions now support this syntax used in
		  very old source code.

    12/10/2022	- Changed default LIST Flags  - Tim Giddens

    03/18/2022	- 6800 CPU support added by Tim Giddens
		  using David Shepperd's work as a reference

******************************************************************************/


#if !defined(MAC_68)
    #define MAC_68
#endif

#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "psttkn68.h"
#include "add_defs.h"
#include "memmgt.h"

#define DEFNAM(name,numb) {"name",name,numb},

/* The following are variables specific to the particular assembler */
const int macxx_name_mask = MACXX_M_68;
const char macxx_name[] = "mac68";
const char *macxx_target = "6800";
const char *macxx_descrip = "Cross assembler for the 6800.";

#if 0
uint16_t macxx_rel_salign = 0;    /* default alignment for .REL. segment */
uint16_t macxx_rel_dalign = 0;    /* default alignment for data in .REL. segment */
uint16_t macxx_abs_salign = 0;    /* default alignments for .ABS. segment */
uint16_t macxx_abs_dalign = 0;
#else
uint16_t macxx_salign = 1;    /* default alignment segments by LLF */
uint16_t macxx_dalign = 1;    /* default alignment data within segment */
#endif
uint16_t macxx_min_dalign = 0;

char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

uint32_t macxx_edm_default = ED_AMA|ED_M68|ED_TRUNC;  /* default edmask */
uint32_t macxx_lm_default = ~(LIST_ME | LIST_MEB | LIST_MES | LIST_LD | LIST_COD | LIST_OCT);  /* default list mask */

int current_radix = 16;     /* default the radix to hex */
char expr_open = '<';       /* char that opens an expression */
char expr_close = '>';      /* char that closes an expression */
/* char expr_escape = '^'; */    /* char that escapes a unary expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */
char open_operand = '(';    /* char that opens an operand indirection */
char close_operand = ')';   /* char that closes an operand indirection */


int max_opcode_length = 6; /* significant length of opcodes */
int max_symbol_length = 6; /* significant length of symbols */

extern int dotwcontext;
extern int no_white_space_allowed;
static char *am_ptr;



enum
{
    CPU_DEFABS,          /* default to absolute addressing */
    CPU_DEFLNG,          /* default to long addressing */
    CPU_DEFDIR           /* default to direct addressing */
} cpu_defam;

/* End of processor specific stuff */



enum amflag
{
    MNBI=1,
    FIN=MNBI<<1,
    LFIN=FIN<<1,
    OPENAT=LFIN<<1,
    OPENBKT=OPENAT<<1,
    GOTSTK=OPENBKT<<1
};


/*
static struct
{
    char *name;
    AModes am_num;
} forced_am[] = {
    {"I",      I_NUM},
    {"D",      D_NUM},
    {"Z",      Z_NUM},
    {"X",      X_NUM},
    {"E",      E_NUM},
    {"A",      A_NUM},
    {"S",      S_NUM},
    {"SPC",  SPC_NUM},
    {"ACC",  ACC_NUM},
    {"IMP",  IMP_NUM},
    {0,0}};
*/


static void do_branch(Opcode *opc)
{
	int s_test;
    int32_t offset;
    EXPR_struct *exp_ptr;
	const char *badExpr=NULL;
	
/* Always 2 */
    offset = 2;
	s_test = 0;  /* set FALSE */

    get_token();
	if ( *inp_ptr == ',' && token_type == TOKEN_strng && token_value == 1 && toupper(token_pool[0]) == 'S' )
	{
		++inp_ptr;	/* eat the comma */
		get_token();
		s_test = 1;  /* set TRUE */
	}
    if (exprs(1,&EXP1) < 1)
    {
		badExpr = "Invalid or no branch target";    /* expression nfg or not present */
    }
    else
    {
		if (s_test)   /* If s_test TRUE make it a br .+n or br .-n syntax */
		{
			EXP1.ptr = compress_expr(&EXP1);
			exp_ptr = EXP1.stack;
			if ( EXP1.ptr != 1 || exp_ptr->expr_code != EXPR_VALUE )
				badExpr = "Branch target must resolve to an absolute value";
			else
			{
				exp_ptr->expr_code = EXPR_SEG;
				exp_ptr->expr_value += current_offset;
				exp_ptr->expr_seg = current_section;
			}
		}
		if ( !badExpr )
		{
			exp_ptr = EXP1.stack + EXP1.ptr;
			exp_ptr->expr_code = EXPR_SEG;
			exp_ptr->expr_value = current_offset + offset;
			(exp_ptr++)->expr_seg = current_section;
			exp_ptr->expr_code = EXPR_OPER;
			exp_ptr->expr_value = '-';
			EXP1.ptr += 2;
		}
    }
	if ( !badExpr )
	{
		EXP1.ptr = compress_expr(&EXP1);
		exp_ptr = EXP1.stack;
		if (EXP1.ptr == 1 && exp_ptr->expr_code == EXPR_VALUE)
		{
			int32_t max_dist;
	
	/* Always 127 */
			max_dist = 127;
	
			if (exp_ptr->expr_value < -(max_dist+1) || exp_ptr->expr_value > max_dist)
			{
				int32_t toofar;
				toofar = exp_ptr->expr_value;
				if (toofar > 0)
				{
					toofar -= max_dist;
				}
				else
				{
					toofar = -toofar-(max_dist+1);
				}
				sprintf(emsg, "Branch offset 0x%X byte(s) out of range", toofar);
				badExpr = emsg;
			}
		}
		else
		{
			EXP1.psuedo_value = 0;
		}
	}
	if ( badExpr )
	{
		bad_token(NULL,badExpr);
		EXP1.ptr = 1;
		exp_ptr = EXP1.stack;
		exp_ptr->expr_code = EXPR_VALUE; /* make it a 0 */
		exp_ptr->expr_value = -offset;   /* make it a br . */
	}

    return;
}

/* This determines whether the am is type X (indexed) or type E (absolute) */
/* Returns: 0 if zpage, 1 if not */
static int check_4_abs(EXP_stk *estk)
{
	EXPR_struct *expr = estk->stack;
	expr = estk->stack;
	int valIsByte = 0;
	
	if ( estk->base_page_reference != 0 )
	{
		return 0;	/* forced z page */
	}
	compress_expr(estk);
	valIsByte = (estk->ptr == 1 && expr->expr_code == EXPR_VALUE && expr->expr_value > -128 && expr->expr_value < 256 );
	if ( valIsByte && !estk->forward_reference )
	{
		return 0;	/* is absolutely z page */
	}
	if ( options[QUAL_2_PASS] )
	{
		if ( !pass && (!valIsByte || estk->forward_reference) )
		{
			setAMATag(current_fnd, 1);   /* Say we chose a word for this instruction because of fwd reference */
			return 1;
		}
		if ( pass && getAMATag(current_fnd) >= 0 )
			return 1;
	}
	/* Default to z page */
	return 0;
}

static int do_operand(Opcode *opc)
{                   /* 1 or 2 operands required*/
    AModes amdcdnum = ILL_NUM;
    int ct;
    int32_t amflag=0;

    ct = get_token();            /* pickup the next token */
    switch (ct)
    {
    case EOL: {
            return amdcdnum;         /* no operand supplied */
         }
    case TOKEN_pcx: {
            if (token_value == '#')
            {
                get_token();        /* get the next token */
                if (exprs(1,&EXP1) < 1)
					break; /* quit if expr nfg */
				if ( (opc->op_amode&SPC) )
					EXP1.tag = (edmask & ED_M68) ? 'W':'w';
				return I_NUM;       /* return with immediate mode address */
            }
            if (token_value == open_operand)
            {
                amflag = FIN;
            }
            else
            {
                if (token_value == '@')
                {
                    amflag = OPENAT;
                }
            }
            if (amflag == 0)
				break;    /* give 'em an illegal am */
            ct = get_token();       /* pickup the next token */
        }             /* fall through to rest */
    default: {               /* -+ if MOS format */
                if (token_type == TOKEN_strng && *inp_ptr == ',')
                {
                    char c;
                    if (amflag != 0)
                    {
                        bad_token(am_ptr,"Invalid address mode syntax");
                        break;
                    }
                    c = *token_pool;
                    c = _toupper(c);

                    if (c == 'I')
                    {
                        amdcdnum = I;
                        if ((opc->op_amode & SPC) == SPC)
                        {
                            EXP1.tag = (edmask & ED_M68) ? 'W':'w';
                        }

                    }
                    else if (c == 'D')
                    {
                        amdcdnum = D;
                        EXP0SP->expr_value = opc->op_value + 0x10;

                    }
                    else if (c == 'X')
                    {
                        amdcdnum = X;
                        EXP0SP->expr_value = opc->op_value + 0x20;
                    }
                    else if (c == 'E')
                    {
                        amdcdnum = E;
                        EXP0SP->expr_value = opc->op_value + 0x30;
                        EXP1.tag = (edmask & ED_M68) ? 'W':'w';

                    }
#if 0
                    else if (c == 'A')
                    {
                        amdcdnum = A;
                    }
#endif

                    if (amdcdnum < UNDEF_NUM)
                    {
                        bad_token(am_ptr,"Unknown address mode");
                        break;
                    }
                    ++inp_ptr;       /* eat the comma */
                    get_token();     /* pickup the next token */
                    amflag = MNBI;       /* signal nothing else allowed */

					if (exprs(1,&EXP1) < 1)
						break;
                }
                else
                {
                    /* if no address mode specified */
					do
					{
						if ( (opc->op_amode&DES) && token_type == TOKEN_strng && token_value == 1 )
						{
							char reg = toupper(token_pool[0]);
							if ( reg == 'A' )
								amdcdnum = IMP;
							else if ( reg == 'B' )
							{
								amdcdnum = IMP;
								EXP0SP->expr_value += 0x10;
							}
							break;	/* from while(0) */
						}
						if ( exprs(1, &EXP1) < 1 )
							break;

						if ( (opc->op_amode) == (X|E) )
						{        /* If no AM option - Default JSR and JMP to Extended (Absolute) */
							EXP0SP->expr_value = opc->op_value + 0x30;
							EXP1.tag = (edmask & ED_M68) ? 'W':'w';	/* jmp/jsr w/o am is always absolute */
							amdcdnum = E;
							break;	/* from while(0) */
						}

						if ( (opc->op_amode) == (X|E|DES) )
                    {        /* If no AM option - Default  Extended (Absolute) */
							EXP0SP->expr_value = opc->op_value + 0x30;
							EXP1.tag = (edmask & ED_M68) ? 'W':'w';
							amdcdnum = E;
							break;	/* from while(0) */
						}

						if ( (opc->op_amode) == (D|X|E|DES) )
						{        /* If no AM option - Could be D (+0x10) or E (+0x30) */
							if ( !check_4_abs(&EXP1) )
							{
								/* zero page */
								EXP0SP->expr_value = opc->op_value + 0x10;
								amdcdnum = D;
							}
							else
							{
								EXP0SP->expr_value = opc->op_value + 0x30;
								EXP1.tag = (edmask & ED_M68) ? 'W':'w';
								amdcdnum = E;
							}
							break;	/* from while(0) */

						}

						if ( (opc->op_amode) == MOST68 )
						{        /* If no AM option - Could be D (+0x10) or E (+0x30) */
							if ( !check_4_abs(&EXP1) )
							{
								/* zero page */
								EXP0SP->expr_value = opc->op_value + 0x10;
								amdcdnum = D;
							}
							else
							{
								EXP0SP->expr_value = opc->op_value + 0x30;
								EXP1.tag = (edmask & ED_M68) ? 'W':'w';
								amdcdnum = E;
							}
							break;	/* from while(0) */
						}

						if ( (opc->op_amode) == (MOST68|SPC) )
						{        /* If no AM option - Could be # (+0), D (+0x10), X (+0x20), E (+0x30) */
							if ( !check_4_abs(&EXP1) )
							{
								/* zero page */
								EXP0SP->expr_value = opc->op_value + 0x10;
								amdcdnum = D;
							}
							else
							{
								EXP0SP->expr_value = opc->op_value + 0x30;
								EXP1.tag = (edmask & ED_M68) ? 'W':'w';
								amdcdnum = E;
							}
							break;	/* from while(0) */

						}
					} while (0);
					if ( amdcdnum < 0 )
						break;	/* from switch(ct) */
                }

                return amdcdnum > UNDEF_NUM ? amdcdnum : UNDEF_NUM; /* give 'em an am */

        }             /* -- case default */
    }                /* -- switch(ct) */

    EXP1.ptr = 0;
    return -1;           /* am nfg */
}



void do_opcode(Opcode *opc)
{
    int err_cnt = error_count[MSG_ERROR];
	
    EXP0.tag = 'b';          /* opcode is default 8 bit byte */
    EXP0.tag_len = 1;            /* only 1 byte */
    EXP1.tag = 'b';          /* assume operand is byte */
    EXP1.tag_len = 1;            /* and only 1 entry long */
    EXP0SP->expr_code = EXPR_VALUE;  /* set the opcode expression */
    EXP0SP->expr_value = opc->op_value;
    EXP0.ptr = 1;            /* first stack has opcode (1 element) */
    EXP1.ptr = 0;            /* assume no operands */
    EXP2.ptr = 0;            /* assume no operands */
    EXP3.ptr = 0;            /* assume no operands */

    am_ptr = inp_ptr;            /* remember where am starts */

    if ( (opc->op_amode == ACC)||(opc->op_amode == IMP) )
    {
        /* no operands */
    }
    else
    {
        if (opc->op_amode == S)
        {
            do_branch(opc);
        }
        else
        {
            if ( do_operand(opc) <= 0 )
				bad_token(NULL,"Invalid or missing operand");
        }

    }


    if (err_cnt != error_count[MSG_ERROR])
    { /* if any errors detected... */
        while (!(cttbl[(int)*inp_ptr] & (CT_EOL|CT_SMC))) ++inp_ptr; /* ...eat to EOL */
    }

    if (EXP0.ptr > 0)
    {
        p1o_any(&EXP0);           /* output the opcode */
        if (EXP1.ptr > 0)
        {
            if (list_bin) compress_expr_psuedo(&EXP1);
            p1o_any(&EXP1);
        }
        if (EXP2.ptr > 0)
        {
            if (list_bin) compress_expr_psuedo(&EXP2);
            p1o_any(&EXP2);
        }
    }

}                   /* -- opc68() */



int ust_init( void )
{
/*
	if (image_name != 0)
    {
        char *s;
        s = strchr(image_name->name_only,'8');
    }
*/
    return 0;            /* would fill a user symbol table */
}


int op_ntype( void )
{
    f1_eatit();
    return 1;
}



#include "opcommon.h"


