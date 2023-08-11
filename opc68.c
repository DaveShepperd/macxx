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

char macxx_name[] = "mac68";
char *macxx_target = "6800";
char *macxx_descrip = "Cross assembler for the 6800.";

unsigned short macxx_rel_salign = 0;    /* default alignment for .REL. segment */
unsigned short macxx_rel_dalign = 0;    /* default alignment for data in .REL. segment */
unsigned short macxx_abs_salign = 0;    /* default alignments for .ABS. segment */
unsigned short macxx_abs_dalign = 0;
unsigned short macxx_min_dalign = 0;

char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = ED_AMA|ED_M68;  /* default edmask */
unsigned long macxx_lm_default = ~(LIST_ME | LIST_MEB | LIST_MES | LIST_LD | LIST_COD);  /* default list mask */

int current_radix = 16;     /* default the radix to hex */
char expr_open = '<';       /* char that opens an expression */
char expr_close = '>';      /* char that closes an expression */
char expr_escape = '^';     /* char that escapes an expression term */
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
    long offset;
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
			long max_dist;
	
	/* Always 127 */
			max_dist = 127;
	
			if (exp_ptr->expr_value < -(max_dist+1) || exp_ptr->expr_value > max_dist)
			{
				long toofar;
				toofar = exp_ptr->expr_value;
				if (toofar > 0)
				{
					toofar -= max_dist;
				}
				else
				{
					toofar = -toofar-(max_dist+1);
				}
				sprintf(emsg, "Branch offset 0x%lX byte(s) out of range", toofar);
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




static int do_operand(Opcode *opc)
{                   /* 1 or 2 operands required*/
    AModes amdcdnum = ILL_NUM;
    int ct;
    long amflag=0;

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
                if (exprs(1,&EXP1) < 1) break; /* quit if expr nfg */
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
            if (amflag == 0) break;    /* give 'em an illegal am */
            get_token();       /* pickup the next token */
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
                    else if (c == 'A')
                    {
                        amdcdnum = A;
                    }


                    if (amdcdnum < UNDEF_NUM)
                    {
                        bad_token(am_ptr,"Unknown address mode");
                        break;
                    }
                    ++inp_ptr;       /* eat the comma */
                    get_token();     /* pickup the next token */
                    amflag = MNBI;       /* signal nothing else allowed */
                }
                else
                {
                    /* if no address mode specified */

                    if ( (opc->op_amode) == (X|E) )
                    {        /* If no AM option - Default JSR and JMP to Extended (Absolute) */
                        EXP0SP->expr_value = opc->op_value + 0x30;
                        EXP1.tag = (edmask & ED_M68) ? 'W':'w';
                    }

                    if ( (opc->op_amode) == (X|E|DES) )
                    {        /* If no AM option - Default  Extended (Absolute) */
                        EXP0SP->expr_value = opc->op_value + 0x30;
                        EXP1.tag = (edmask & ED_M68) ? 'W':'w';
                    }

                    if ( (opc->op_amode) == (D|X|E|DES) )
                    {        /* If no AM option - Default  Direct (Absolute Zero Page) */
                        EXP0SP->expr_value = opc->op_value + 0x10;

                    }

                    if ( (opc->op_amode) == MOST68 )
                    {        /* If no AM option - Default  Direct (Absolute Zero Page) */
                        EXP0SP->expr_value = opc->op_value + 0x10;

                    }

                    if ( (opc->op_amode) == (MOST68|SPC) )
                    {        /* If no AM option - Default  Direct (Absolute Zero Page) */
                        EXP0SP->expr_value = opc->op_value + 0x10;

                    }

                }

                if (exprs(1,&EXP1) < 1) break;

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
            do_operand(opc);
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


