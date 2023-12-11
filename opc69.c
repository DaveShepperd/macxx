/*
	opc69.c - Part of macxx, a cross assembler family for various micro-processors
	Copyright (C) 2008 David Shepperd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


/******************************************************************************
Change Log

	12/07/2023	- Bug fix, improper use of check for DPAGE	T.G.
			  resulting in DPAGE address mode instead of extended
			  address mode when not in DPAGE address range.

	12/06/2023	- Bug fix, allow true 16 bit offsets for	T.G.
			  LEAr instructions

	11/30/2023	- Fixed errors when using -2 pass option	T.G.
			  MACxx being a single pass assembler with a 2 pass
			  option both must create the same obj files.

	12/27/2022	- Fixed dpage problem that was causing LLF Truncation
			  warnings for dpage offsets.
			  Added some Error checking		Tim Giddens

	12/15/2022	- Fixed optimization problem, was not working for all
			  @0(r) instructions.
			  Cleaned up code some.			Tim Giddens

	03/26/2022	- 6809 CPU support added by Tim Giddens
			  using David Shepperd's work as a reference

			  The 6809 has three offset modes
			   5 bit - uses one byte	   -16 to    +15	   n(r)
			   8 bit - uses two bytes	  -128 to   +127	  nn(r)
			  16 bit - uses three bytes	-32768 to +32767	nnnn(r)

			  Optimization:
				Motorola states that any @n(r) or n(r) instruction use 8 bit offset mode
				@(r) and @0(r) are both legal instructions and use an offset of zero to perform
				the same function only difference is that @(r) uses one byte and @0(r) uses
				two bytes.  MAC69 has been optimized to convert all @0(r) instructions into
				@(r) this saves one byte of memory for each occurrence of these instructions.

******************************************************************************/


#if !defined(MAC_69)
	#define MAC_69
#endif

#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"

#include "psttkn69.h"

#include "add_defs.h"
#include "memmgt.h"

#define DEFNAM(name,numb) {"name",name,numb},

/* The following are variables specific to the particular assembler */

char macxx_name[] = "mac69";
char *macxx_target = "6809";
char *macxx_descrip = "Cross assembler for the 6809.";

unsigned short macxx_rel_salign = 0;    /* default alignment for .REL. segment */
unsigned short macxx_rel_dalign = 0;    /* default alignment for data in .REL. segment */
unsigned short macxx_abs_salign = 0;    /* default alignments for .ABS. segment */
unsigned short macxx_abs_dalign = 0;
unsigned short macxx_min_dalign = 0;

char macxx_mau = 8;         	/* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;    /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = ED_AMA | ED_M68; /* default edmask */
unsigned long macxx_lm_default = ~(LIST_ME | LIST_MEB | LIST_MES | LIST_LD | LIST_COD); /* default list mask */

int current_radix = 16;     /* default the radix to hexdecimal */
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
static char *tmp_ptr;

enum
{
	CPU_DEFABS,          /* default to absolute addressing */
	CPU_DEFLNG,          /* default to long addressing */
	CPU_DEFDIR           /* default to direct addressing */
} cpu_defam;

/* End of processor specific stuff */


enum amflag
{
	MNBI = 1,
	FIN = MNBI << 1,
	LFIN = FIN << 1,
	OPENAT = LFIN << 1,
	OPENBKT = OPENAT << 1,
	GOTSTK = OPENBKT << 1
};


typedef struct
{
	int flag;            /* .ne. if value is expression */
	unsigned long value;     /* value if not expression */
	EXP_stk *exptr;      /* ptr to expression stack */
} DPage;



DPage **dpage;
static int dpage_size,dpage_stack;

int ust_init(void)
{
	dpage_size = 16;
	dpage = (DPage **)MEM_alloc(dpage_size * sizeof(DPage *));
	return 0;            /* would fill a user symbol table */
}

static int check_4_dpage(EXP_stk *estk)
{
	DPage *dp;
	EXPR_struct *expr;
	
	expr = estk->stack;
	dp = *(dpage + dpage_stack);
	if ( dp == 0 )
	{
		if ( (estk->ptr == 1) &&           /* or expression is < 256 */
			 (expr->expr_code == EXPR_VALUE) &&
			 ((expr->expr_value & -256) == 0) &&
			 (estk->forward_reference == 0) )
			return 1;
	}
	else
	{
		if ( (estk->ptr == 1) &&           /* or expression is < 256 */
			 (expr->expr_code == EXPR_VALUE) &&
			 ((expr->expr_value & -256) == ((dp->value) << 8)) &&
			 (estk->forward_reference == 0) )
			return 1;
	}
	return 0;

}


static int make_dpage_ref(EXP_stk *estk)
{
	DPage *dp;
	EXP_stk *dpstk;
	EXPR_struct * src,*dst;
	int i;
	dp = *dpage + dpage_stack;
	if ( dp == 0 )
		return 0;   /* nothing to do */
	dst = estk->stack + estk->ptr;
	dpstk = dp->exptr;
	src = dpstk->stack;
	i = dpstk->ptr + estk->ptr + 1;
	if ( i >= EXPR_MAXDEPTH )
	{
		bad_token((char *)0, "Direct page expression too complex");
		return 0;
	}
	memcpy(dst, src, dpstk->ptr * sizeof(EXPR_struct));
	dst->expr_value = ((dst->expr_value) << 8);
	dst += dpstk->ptr;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = '-';
	estk->ptr += dpstk->ptr + 1;
	estk->tag = 'c';
	estk->ptr = compress_expr(estk);
	return 0;

}


int op_dpage(void)
{
	int i, cnt, typ;
	DPage *dp;
	typ = get_token();           /* pickup the next token */
	if ( typ == EOL )
	{
	pop_dpage:
		if ( dpage_stack > 0 )
		{
			if ( dpage_stack < 2 ||
				 *dpage + dpage_stack != *dpage + (dpage_stack - 1) )
				MEM_free((char *)dpage[dpage_stack]);
			*(dpage + dpage_stack) = 0;
			--dpage_stack;
		}
		return 2;
	}
	i = exprs(1, &EXP0);      /* evaluate the expression */
	if ( i == 0 || *inp_ptr == ',' )
	{ /* are we supposed to save the old dpage? */
		int typ;
		++inp_ptr;        /* eat the comma */
		typ = get_token();    /* pick up the next token */
		if ( typ == TOKEN_strng )
		{
			int len = token_value;
			if ( strncmp("SAVE", token_pool, len) == 0 )
			{
				if ( ++dpage_stack >= dpage_size )
				{
					int new = dpage_stack + dpage_stack / 2;
					dpage = (DPage **)MEM_realloc((char *)dpage, new * sizeof(DPage *));
				}
				if ( i == 0 )
				{
					*(dpage + dpage_stack) = *(dpage + (dpage_stack - 1));
					return 2;
				}
			}
			else if ( strncmp("RESTORE", token_pool, len) == 0 )
			{
				if ( i > 0 )
					bad_token((char *)0, "Expression ignored on dpage RESTORE");
				goto pop_dpage;
			}
			else
			{
				bad_token(tkn_ptr, "Expected one of SAVE or RESTORE");
				f1_eatit();
			}
		}
		else if ( typ == EOL )
			goto pop_dpage;
	}
	if ( i > 0 )
	{         /* if 1 or more terms... */
		EXP_stk *exp_ptr;
		cnt = sizeof(DPage) + i * sizeof(EXPR_struct) + sizeof(EXP_stk);
		misc_pool_used += cnt;
		dp = (DPage *)MEM_alloc(cnt);
		*(dpage + dpage_stack) = dp;    /* save it for later */
		exp_ptr = (EXP_stk *)(dp + 1);
		memcpy(exp_ptr, &EXP0, sizeof(EXP_stk));
		exp_ptr->stack = (EXPR_struct *)(exp_ptr + 1);
		memcpy(exp_ptr->stack, EXP0.stack, i * sizeof(EXPR_struct));
		dp->exptr = exp_ptr;
		if ( i == 1 && EXP0SP->expr_code == EXPR_VALUE )
		{
			if ( EXP0SP->expr_value > 65535 || EXP0SP->expr_value < -65536 )
			{
				bad_token((char *)0, "Direct page value must resolve to a 16 bit value");
			}
			else
			{
				dp->value = exp_ptr->stack->expr_value = EXP0SP->expr_value;
			}
		}
		else
		{
			dp->flag = 1;      /* signal that there's an expression */
		}
		return 2;         /* and quit */
	}
	EXP0.psuedo_value = 0;
	return 2;

}


static void do_branch(Opcode *opc)
{
	long offset;
	EXPR_struct *exp_ptr;
	offset = (opc->op_amode & SPL) ? 4 : 2;
	if ( (opc->op_amode & SPL) && (!(opc->op_amode & SP10)) )
		offset = 3;
	get_token();
	if ( exprs(1, &EXP1) < 1 )
	{    /* expression nfg or not present */
		exp_ptr = EXP1.stack;
		exp_ptr->expr_code = EXPR_VALUE; /* make it a 0 */
		exp_ptr->expr_value = -offset;   /* make it a br . */
		EXP1.ptr = 1;
	}
	else
	{
		exp_ptr = EXP1.stack + EXP1.ptr;
		exp_ptr->expr_code = EXPR_SEG;
		exp_ptr->expr_value = current_offset + offset;
		(exp_ptr++)->expr_seg = current_section;
		exp_ptr->expr_code = EXPR_OPER;
		exp_ptr->expr_value = '-';
		EXP1.ptr += 2;
	}
	EXP1.ptr = compress_expr(&EXP1);

	/*  in file pass2.c  tag values */
	/*  lower case 'y' causes low byte then high byte  Upper case 'Y' is high byte then low byte */
	EXP1.tag = (opc->op_amode & SPL) ? ((edmask&ED_M68)?'Y':'y') : 'z'; /* set branch type operand */
	exp_ptr = EXP1.stack;
	if ( EXP1.ptr == 1 && exp_ptr->expr_code == EXPR_VALUE )
	{
		long max_dist;
		max_dist = (opc->op_amode & SPL) ? 32767 : 127;
		if ( exp_ptr->expr_value < -(max_dist + 1) || exp_ptr->expr_value > max_dist )
		{
			long toofar;
			toofar = exp_ptr->expr_value;
			if ( toofar > 0 )
			{
				toofar -= max_dist;
			}
			else
			{
				toofar = -toofar - (max_dist + 1);
			}
			sprintf(emsg, "Branch offset 0x%lX byte(s) out of range", toofar);
			bad_token((char *)0, emsg);
			exp_ptr->expr_value = -offset;
		}
	}
	else
	{
		EXP1.psuedo_value = 0;
	}

	return;
}


static int do_operand(Opcode *opc)
{   /* 1 or 2 operands required*/

	AModes amdcdnum = ILL_NUM;

	int ct;
	int j		= 0;
	int temp	= 0;
	long amflag	= 0;
	int z_page	= 0;	/* zero page */
	long con_offset	= 0;	/* constant offset */

	int indexed_mode	= 0x00;  /* holds current mode flags */

	/* mode flags */
	int flg_1decr		= 0x001; /* one minus sign  - decrement by one */
	int flg_2decr		= 0x002; /* two minus signs - decrement by two */
	int flg_5bit		= 0x004; /*  5 bit constant offset */
	int flg_8bit		= 0x008; /*  8 bit constant offset */
	int flg_16bit		= 0x010; /* 16 bit constant offset */
	int flg_indirect	= 0x020; /* indirect - @(r) */
	int flg_PC		= 0x040; /* offset from Program Counter */
	int flg_ABD		= 0x080; /* Accumulator offset */
	int flg_neg		= 0x000;

	/* indexed/indirect register flags */
	int ireg_X	= 0x0;
	int ireg_Y	= 0x1;
	int ireg_U	= 0x2;
	int ireg_S	= 0x3;

	/* register flags */
	int reg_D	= 0x0;	/* double accumulator D (A & B)	- 16bit register */
	int reg_X	= 0x1;	/* index register X		- 16bit register */
	int reg_Y	= 0x2;	/* index register Y		- 16bit register */
	int reg_U	= 0x3;	/* user stack pointer		- 16bit register */
	int reg_S	= 0x4;	/* hardware stack pointer	- 16bit register */
	int reg_PC	= 0x5;	/* Program Counter PC		- 16bit register */
	int reg_A	= 0x8;	/* accumulator A		-  8bit register */
	int reg_B	= 0x9;	/* accumulator B		-  8bit register */
	int reg_CCR	= 0xA;	/* condition code register	-  8bit register */
	int reg_DPR	= 0xB;	/* direct page register		-  8bit register */


	ct = get_token();		/* pickup the next token */
	switch (ct)
	{
	case EOL:
		{
			return -1;	/* no operand supplied */
		}

	case TOKEN_oper:
		{

			if ( token_value == '-' )
			{
				flg_neg = 1;

				if ( (token_value == '-') && (*inp_ptr == open_operand) && (*(inp_ptr + 2) == close_operand) )
				{
					indexed_mode++;	/* Set one minus sign */
					flg_neg = 0;
					inp_ptr++;	/* eat the open_operand */
				}
				else if ( (token_value == '-') && (*inp_ptr == '-') && (*(inp_ptr + 1) == open_operand) && (*(inp_ptr + 3) == close_operand) )
				{
					indexed_mode++;
					indexed_mode++;	/* Set two minus sign */
					flg_neg = 0;
					inp_ptr++;	/* eat the 2nd '-' */
					inp_ptr++;	/* eat the open_operand */
				}
				else
				{
					inp_ptr--;	/* backup to the minus sign */
				}

			}
			else
			{
				inp_ptr--;		/* backup to the minus sign */
			}

		}

	case TOKEN_pcx:
		{

			if ( token_value == '#' )
			{
				get_token();	/* get the next token */
				if ( exprs(1, &EXP1) < 1 )
					break;	/* quit if expr nfg */

				/* If it is a 2 BYTE immediate mode address */
				if ( (opc->op_amode & SPC) == SPC )
				{
					EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
				}

				return I_NUM;	/* return with immediate mode address */
			}
			if ( token_value == open_operand )
			{
				amflag = FIN;
			}
			else
			{
				if ( token_value == '@' )
				{
					indexed_mode = (indexed_mode | flg_indirect);

					if ( *inp_ptr == open_operand )
					{
						amflag = FIN;
						inp_ptr++;	/* eat the open_operand */
					}
					else if ( *inp_ptr != '-' )
					{
						amflag = OPENAT;
					}
					else
					{
						if ( *inp_ptr == '-' )
						{
							flg_neg = 1;
						}
						if ( (*inp_ptr == '-') && (*(inp_ptr + 1) == open_operand) && (*(inp_ptr + 3) == close_operand) )
						{
							indexed_mode++;	/* Set one minus sign */
							flg_neg = 0;
							inp_ptr++;	/* eat the minus sign */
							inp_ptr++;	/* eat the open_operand */
						}
						else if ( (*inp_ptr == '-') && (*(inp_ptr + 1) == '-') && (*(inp_ptr + 2) == open_operand) && (*(inp_ptr + 4) == close_operand) )
						{
							indexed_mode++;
							indexed_mode++;	/* Set two minus sign */
							flg_neg = 0;
							inp_ptr++;	/* eat the minus sign */
							inp_ptr++;	/* eat the 2nd '-' */
							inp_ptr++;	/* eat the open_operand */
						}

					}

				}

			}


			if ( ((indexed_mode & flg_1decr) || (indexed_mode & flg_2decr)) && (flg_neg == 0) )
			{  /* ----> Start of -(r), --(r), @-(r), and @--(r) opcodes */
				if ( indexed_mode & flg_1decr )
				{
					if ( indexed_mode & flg_indirect )
					{
						temp = 0x93;	/* decrement by one (0x92) not allowed in indirect mode - so dec. by two (0x93)*/
					}
					else
					{
						temp = 0x82;
					}

				}
				if ( indexed_mode & flg_2decr )
				{
					if ( indexed_mode & flg_indirect )
					{
						temp = 0x93;
					}
					else
					{
						temp = 0x83;
					}
				}
				/* Convert to Upper Case */
				j = toupper(*inp_ptr);

				if ( j == 'X' )
				{
					temp = (temp | (ireg_X << 5));
				}
				else if ( j == 'Y' )
				{
					temp = (temp | (ireg_Y << 5));
				}
				else if ( j == 'U' )
				{
					temp = (temp | (ireg_U << 5));
				}
				else if ( j == 'S' )
				{
					temp = (temp | (ireg_S << 5));
				}

				++inp_ptr;	/* eat the reg */
				++inp_ptr;	/* eat the close_operand */

				EXP0SP->expr_value = opc->op_value + 0x20;

				EXP1.psuedo_value = EXP1SP->expr_value = temp;	/* set operand value */
				EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
				EXP1.tag = 'b';
				EXP1.ptr = 1;			/* second stack has operand (1 element) */

				return I_NUM;
			}  /* <---- End of -(r), --(r), @-(r), and @--(r) opcodes */

			get_token();	/* pickup the next token */

		}	/* fall through to rest */
	default:
		{
			tmp_ptr = inp_ptr;
			j = 0;
			while ( !(cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) )
			{
				if ( (*inp_ptr == '(') && ((*(inp_ptr + 2) == ')') || (*(inp_ptr + 3) == ')')) )
				{
					j = 1;
					break;
				}
				inp_ptr++;
			}

			inp_ptr = tmp_ptr;

			if ( token_type == TOKEN_strng )
			{
				/* ---->  Start of  PSHS, PSHU, PULS and PULU opcodes */
				/* There is a better way to do this, but for now it's down and dirty */
				if ( (opc->op_value == 0x34) || (opc->op_value == 0x35) || (opc->op_value == 0x36) || (opc->op_value == 0x37) )
				{
					temp = 0;
					while ( 1 )
					{
						/* Convert to Upper Case */
						j = 0;
						while ( token_pool[j] )
						{
							token_pool[j] = toupper(token_pool[j]);
							++j;
						}

						/* Find - Reg */
						if ( strcmp(token_pool, "CCR") == 0 )
						{
							temp = (temp | 1);
						}
						if ( strcmp(token_pool, "A") == 0 )
						{
							temp = (temp | (1 << 1));
						}
						if ( strcmp(token_pool, "B") == 0 )
						{
							temp = (temp | (1 << 2));
						}
						if ( strcmp(token_pool, "D") == 0 )
						{
							temp = (temp | (1 << 1));
							temp = (temp | (1 << 2));
						}
						if ( strcmp(token_pool, "DPR") == 0 )
						{
							temp = (temp | (1 << 3));
						}
						if ( strcmp(token_pool, "X") == 0 )
						{
							temp = (temp | (1 << 4));
						}
						if ( strcmp(token_pool, "Y") == 0 )
						{
							temp = (temp | (1 << 5));
						}
						if ( strcmp(token_pool, "S") == 0 )
						{
							temp = (temp | (1 << 6));
						}
						if ( strcmp(token_pool, "U") == 0 )
						{
							temp = (temp | (1 << 6));
						}
						if ( strcmp(token_pool, "PC") == 0 )
						{
							temp = (temp | (1 << 7));
						}

						if ( *inp_ptr == ',' )
						{
							inp_ptr++;
							get_token();	/* pickup the next token */
						}
						else
						{
							break;
						}

					}
					EXP1.psuedo_value = EXP1SP->expr_value = temp;	/* set operand value */
					EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
					EXP1.tag = 'b';
					EXP1.ptr = 1;			/* second stack has operand (1 element) */

					return I_NUM;
				}  /* <----  End of PSHS, PSHU, PULS and PULU opcodes */

			}

			if ( (token_type == TOKEN_numb) && (*inp_ptr == open_operand) )
			{
				if ( (token_value == 0) && (*inp_ptr == open_operand) && (*(inp_ptr + 2) == close_operand) )
				{	/*  optimize: if 0(r), 0(r)+, 0(r)++, @0(r), @0(r)+, @0(r)++ */
					/*            convert to (r), (r)+, (r)++, @(r), @(r)+, @(r)++  */
					amflag = FIN;
					inp_ptr++;	/* eat the open_operand */
					get_token();	/* pickup the next token */
				}

			}

			if ( amflag == FIN )
			{  /* ----> Start of (r), (r)+, (r)++, @(r), @(r)+, @(r)++ */

				/* Convert to Upper Case */
				j = 0;
				while ( token_pool[j] )
				{
					token_pool[j] = toupper(token_pool[j]);
					++j;
				}
				if ( *inp_ptr == close_operand )
				{
					++inp_ptr;		/* eat the close_operand */
					if ( (*inp_ptr == '+') && (*(inp_ptr + 1) == '+') )
					{
						if ( indexed_mode & flg_indirect )
						{
							temp = 0x91;
						}
						else
						{
							temp = 0x81;
						}
						++inp_ptr;	/* eat the 1st '+' */
						++inp_ptr;	/* eat the 2nd '+' */
					}
					else if ( *inp_ptr == '+' )
					{
						if ( indexed_mode & flg_indirect )
						{
							temp = 0x91;	/* increment by one (0x90) not allowed in indirect mode */
						}			/* use increment by two (0x91) */
						else
						{
							temp = 0x80;
						}
						++inp_ptr;		/* eat the '+' */
					}
					else
					{
						if ( indexed_mode & flg_indirect )	/* No offset */
						{
							temp = 0x94;
						}
						else
						{
							temp = 0x84;
						}
					}
					if ( strcmp(token_pool, "X") == 0 )
					{
						temp = (temp | (ireg_X << 5));
					}
					else if ( strcmp(token_pool, "Y") == 0 )
					{
						temp = (temp | (ireg_Y << 5));
					}
					else if ( strcmp(token_pool, "U") == 0 )
					{
						temp = (temp | (ireg_U << 5));
					}
					else if ( strcmp(token_pool, "S") == 0 )
					{
						temp = (temp | (ireg_S << 5));
					}

					EXP0SP->expr_value = opc->op_value + 0x20;

					EXP1.psuedo_value = EXP1SP->expr_value = temp;	/* set operand value */
					EXP1SP->expr_code = EXPR_VALUE;		/* set expression component is an absolute value */
					EXP1.tag = 'b';
					EXP1.ptr = 1;				/* second stack has operand (1 element) */

					return I_NUM;

				}
				sprintf(emsg, "using amflag = FIN error - amflag value = %lX Hex", amflag);
				bad_token((char *)0, emsg);
				f1_eatit();	/* eat rest of line */
				return -1;


			}  /* <---- End of (r), (r)+, (r)++, @(r), @(r)+, @(r)++ */

			if ( (((token_type == TOKEN_strng) || (token_type == TOKEN_numb)) && ((*inp_ptr == open_operand) || (j) || (amflag == OPENAT))) || ((token_type == TOKEN_oper) && (j)) )
			{  /* ----> Start of (r) or n(r) or nnnn(r) or @(r) or @n(r) or @nnnn(r) */

				if ( j )
				{
					if ( token_type != TOKEN_oper )
					{
						if ( strcmp(token_pool, "A") == 0 )
						{
							indexed_mode = indexed_mode | flg_ABD;
							if ( indexed_mode & flg_indirect )
							{
								temp = 0x96;
							}
							else
							{
								temp = 0x86;
							}

						}
						else if ( strcmp(token_pool, "B") == 0 )
						{
							indexed_mode = indexed_mode | flg_ABD;
							if ( indexed_mode & flg_indirect )
							{
								temp = 0x95;
							}
							else
							{
								temp = 0x85;
							}
						}
						else if ( strcmp(token_pool, "D") == 0 )
						{

							indexed_mode = indexed_mode | flg_ABD;
							if ( indexed_mode & flg_indirect )
							{
								temp = 0x9B;
							}
							else
							{
								temp = 0x8B;
							}

						}

					}

					if ( (indexed_mode & flg_ABD) == 0 )
					{  /* if not ABD flag do this */
						if ( exprs(1, &EXP2) < 1 )
						{  /* test operand */
							sprintf(emsg, "unknown operand ");
							bad_token((char *)0, emsg);
							f1_eatit();	/* eat rest of line */
							return -1;
						}

						con_offset  = EXP2SP->expr_value;

						if ( (EXP2SP->expr_code != EXPR_VALUE) || (EXP2.ptr > 1) ||
							( ( (getAMATag(current_fnd)) >= 0) && (pass !=0) ) )
						{  /* if unknown value in pass 0 then pass 1 must treat it as unknown */
						   /* if unknown value - treat as 8bit or 16bit mode */

							z_page = 0x1 & (EXP2.base_page_reference);	/* check for base page */
							if ( z_page )
							{
								indexed_mode = indexed_mode | flg_8bit;    /* Really seven bit with negative flag */
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x98;
								}
								else
								{
									temp = 0x88;
								}
							}
							else
							{
								indexed_mode = indexed_mode | flg_16bit;  /* Really fifteen bit with negative flag */
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x99;
								}
								else
								{
									temp = 0x89;
								}
							}
							if ( !pass) /* Tell pass 1 that pass 0 chose an unknown for this instruction*/
							{
								setAMATag(current_fnd,1);
							}

						}
						else
						{  /* if absolute value */

							if ( ((con_offset  > -17) && (con_offset  < 16)) && (!(indexed_mode & flg_indirect)) )
							{  /* -17 >< +16 */
								indexed_mode = indexed_mode | flg_5bit;	/* Really four bit with negative flag */
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x98;	/* Force 8 bits for @n(r) */
									indexed_mode = (indexed_mode & ~flg_5bit) | flg_8bit;
								}
								else
								{
									temp = 0x0;
								}
							}
							else if ( (con_offset  > -129) && (con_offset  < 128) )
							{  /* -129 >< +128 */
								indexed_mode = indexed_mode | flg_8bit;   /* Really seven bit with negative flag */
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x98;
								}
								else
								{
									temp = 0x88;
								}
							}
							else if ( (con_offset > -32769) && (con_offset < 65536) )
							{ /* -32769 >< +32768 or Load Effective Address instruction - Note: In 16 bit math any number
								between 0xFFFF -> 0x8000 is -1 -> -32768 so must be allowed +32768 + 0x8000 = 65536 */
								indexed_mode = indexed_mode | flg_16bit;
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x99;
								}
								else
								{
									temp = 0x89;
								}
							}
							else
							{
								sprintf(emsg, "Only signed 16 bit values allowed - constant offset = %lX Hex", con_offset);
								bad_token((char *)0, emsg);
								f1_eatit();	/* eat rest of line */
								return -1;
							}

							/* offset is now an absolute variable so it might have a value of zero */
							/* If zero offset - set 0(r), @0(r) to (r), @(r) 5bit mode */
							if (con_offset  == 0)
							{
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x94;
								}
								else
								{
									temp = 0x84;
								}
								/* Force 0(r), @0(r) to (r), @(r) */
								indexed_mode = ((indexed_mode & ~flg_8bit & ~flg_16bit) | flg_5bit);
							}

						}


					}
					if ( *inp_ptr == open_operand )
					{
						++inp_ptr;	/* eat the open_operand */
						get_token();	/* pickup the next token */
					}
					else
					{
						get_token();
					}

					/* Convert to Upper Case */
					j = 0;
					while ( token_pool[j] )
					{
						token_pool[j] = toupper(token_pool[j]);
						++j;
					}
					if ( *inp_ptr == close_operand )
					{
						++inp_ptr;	/* eat the close_operand */
					}

					if ( strcmp(token_pool, "X") == 0 )
					{
						temp = temp | (ireg_X << 5);
					}
					else if ( strcmp(token_pool, "Y") == 0 )
					{
						temp = temp | (ireg_Y << 5);
					}
					else if ( strcmp(token_pool, "U") == 0 )
					{
						temp = temp | (ireg_U << 5);
					}
					else if ( strcmp(token_pool, "S") == 0 )
					{
						temp = temp | (ireg_S << 5);
					}
					else if ( strcmp(token_pool, "PC") == 0 )
					{
						indexed_mode = indexed_mode | flg_PC;

						/*  If five bit mode force 8 bit mode */
						if ( indexed_mode & flg_5bit )
						{
							indexed_mode = (indexed_mode & ~flg_5bit) | flg_8bit;
						}

						if ( indexed_mode & flg_8bit )
						{
							if ( indexed_mode & flg_indirect )
							{
								temp = 0x9C;
							}
							else
							{
								temp = 0x8C;
							}
						}
						else if ( indexed_mode & flg_16bit )
						{
							if ( indexed_mode & flg_indirect )
							{
								temp = 0x9D;
							}
							else
							{
								temp = 0x8D;
							}
						}

						temp = temp | (ireg_X << 5);

					}
					else if ( strcmp(token_pool, "PC") != 0 )
					{
						sprintf(emsg, "token_pool Does not match any legit Reg - %s", token_pool);
						bad_token((char *)0, emsg);
						f1_eatit();	/* eat rest of line */
						return -1;
					}

					EXP0SP->expr_value = opc->op_value + 0x20;
					if ( indexed_mode & flg_ABD )
					{  /* if ABD flag */
						if ( strcmp(token_pool, "PC") == 0 )
						{
							sprintf(emsg, "Not A Valid Register for accumulator offset - %s", token_pool);
							bad_token((char *)0, emsg);
							f1_eatit();	/* eat rest of line */
							return -1;
						}
						else
						{
							EXP1.psuedo_value = EXP1SP->expr_value = temp;	/* set Post Byte value */
							EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
							EXP1.tag = 'b';
							EXP1.ptr = 1;			/* second stack has operand (1 element) */
						}
						return I_NUM;
					}
					else
					{
						if ( (EXP2SP->expr_code == EXPR_VALUE) && (EXP2.ptr < 2) )
						{  /* if absolute value */

							if ( indexed_mode & flg_5bit )
							{  /* 5 bit mode */
								EXP1.psuedo_value = EXP1SP->expr_value = temp | (con_offset  & 0x1f);  /* set operand 5bit value */
								EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;			/* second stack has operand (1 element) */
								EXP2.ptr = 0;			/* third stack has no operand (1 element) */
							}
							else if ( indexed_mode & flg_8bit )
							{  /* 8 bit mode */
								EXP1.psuedo_value = EXP1SP->expr_value = temp;	/* set Post Byte value */
								EXP2.psuedo_value = EXP2SP->expr_value = (con_offset  & 0xFF);	/* set operand 8bit value */
								EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
								EXP2SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;			/* second stack has operand (1 element) */
								EXP2.tag = 'b';
								/*               EXP2.ptr = 1;             * third stack has operand (1 element) */
							}
							else
							{  /* 16 bit mode */
								EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set Post Byte value */
								EXP2.psuedo_value = EXP2SP->expr_value = (con_offset  & 0xFFFF);  /* set operand 16bit value */
								EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								EXP2SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;            /* second stack has operand (1 element) */
								EXP2.tag = (edmask & ED_M68) ? 'W' : 'w';
								/*               EXP2.ptr = 1;             * third stack has operand (1 element) */
							}
							return I_NUM;
						}
						else
						{
							if ( indexed_mode & flg_8bit )
							{
								EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set Post Byte value */
								/*         EXP2.psuedo_value = EXP2SP->expr_value = (j & 0xFF) ;   * set operand 8bit value */
								EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								/*         EXP2SP->expr_code = EXPR_VALUE;   * set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;            /* second stack has operand (1 element) */
								EXP2.tag = 'b';
								/*         EXP2.ptr = 1;             * third stack has operand (1 element) */
							}
							else
							{
								EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set Post Byte value */
								/*          EXP2.psuedo_value = EXP2SP->expr_value = (j & 0xFFFF) ;   * set operand 16bit value */
								EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								/*          EXP2SP->expr_code = EXPR_VALUE;   * set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;            /* second stack has operand (1 element) */
								EXP2.tag = (edmask & ED_M68) ? 'W' : 'w';
								/*          EXP2.ptr = 1;             * third stack has operand (1 element) */
							}

							return I_NUM;
						}

					}

				}
				else
				{
					if ( indexed_mode & flg_indirect )
					{
						EXP0SP->expr_value = opc->op_value + 0x20;

						if ( exprs(1, &EXP2) < 1 )
							return -1;   /* test operand */

						EXP1.psuedo_value = EXP1SP->expr_value = 0x9F;  /* set Post Byte value */
						EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
						EXP1.tag = 'b';
						EXP1.ptr = 1;            /* second stack has operand (1 element) */
						EXP2.tag = (edmask & ED_M68) ? 'W' : 'w';
						/*          EXP2.ptr = 1;             * third stack has operand (1 element) */
						return I_NUM;
					}
					else
					{
						sprintf(emsg, "Not an indirect Extended mode - %s", token_pool);
						bad_token((char *)0, emsg);
						f1_eatit();       /* eat rest of line */
					}

				}

			}  /* <---- (r) or n(r) or nnnn(r) or @(r) or @n(r) or @nnnn(r) */

			if ( (token_type == TOKEN_strng) && (*inp_ptr == ',') )
			{
				if ( amflag != 0 )
				{
					bad_token(am_ptr, "Invalid address mode syntax");
					break;
				}
				/* Convert to Upper Case */
				j = 0;
				while ( token_pool[j] )
				{
					token_pool[j] = toupper(token_pool[j]);
					++j;
				}
				/* TFR and EXG opcodes */
				/* There is a better way to do this, but for now it's down and dirty */
				if ( (opc->op_value == 0x1F) || (opc->op_value == 0x1E) )
				{
					j = 0;
					/* Find - Reg */
					if ( strcmp(token_pool, "CCR") == 0 )
					{
						temp = (reg_CCR << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "A") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_A << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "B") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_B << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "DPR") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_DPR << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "X") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_X << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "Y") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_Y << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "S") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_S << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "U") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_U << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "PC") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_PC << 4);
						j = 1;
					}
					else if ( (strcmp(token_pool, "D") == 0) && (*inp_ptr == ',') )
					{
						temp = (reg_D << 4);
						j = 1;
					}

					if ( j )
					{
						++inp_ptr;	/* eat the comma */
						get_token();	/* pickup the next token */

						/* Convert to Upper Case */
						j = 0;
						while ( token_pool[j] )
						{
							token_pool[j] = toupper(token_pool[j]);
							++j;
						}

						if ( strcmp(token_pool, "CCR") == 0 )
						{
							temp = (temp | reg_CCR);
						}
						else if ( strcmp(token_pool, "A") == 0 )
						{
							temp = (temp | reg_A);
						}
						else if ( strcmp(token_pool, "B") == 0 )
						{
							temp = (temp | reg_B);
						}
						else if ( strcmp(token_pool, "DPR") == 0 )
						{
							temp = (temp | reg_DPR);
						}
						else if ( strcmp(token_pool, "X") == 0 )
						{
							temp = (temp | reg_X);
						}
						else if ( strcmp(token_pool, "Y") == 0 )
						{
							temp = (temp | reg_Y);
						}
						else if ( strcmp(token_pool, "S") == 0 )
						{
							temp = (temp | reg_S);
						}
						else if ( strcmp(token_pool, "U") == 0 )
						{
							temp = (temp | reg_U);
						}
						else if ( strcmp(token_pool, "PC") == 0 )
						{
							temp = (temp | reg_PC);
						}
						else if ( strcmp(token_pool, "D") == 0 )
						{
							temp = (temp | reg_D);
						}

						if ( temp > 0xBB )
						{
							sprintf(emsg, "illegal register use - Register values = %X Hex", temp);
							bad_token((char *)0, emsg);
							f1_eatit();	/* eat rest of line */
							return -1;
						}
						if ( ((temp >> 4) < 0x6) && ((temp & 0xF) > 0x7) )
						{
							sprintf(emsg, "can not move 16bit reg to 8bit reg - Register values = %X Hex", temp);
							bad_token((char *)0, emsg);
							f1_eatit();	/* eat rest of line */
							return -1;
						}
						if ( ((temp >> 4) > 0x7) && ((temp & 0xF) < 0x6) )
						{
							sprintf(emsg, "can not move 8bit reg to 16bit reg - Register values = %X Hex", temp);
							bad_token((char *)0, emsg);
							f1_eatit();	/* eat rest of line */
							return -1;
						}

						EXP1.psuedo_value = EXP1SP->expr_value = temp;	/* set operand value */
						EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
						EXP1.tag = 'b';
						EXP1.ptr = 1;			/* second stack has operand (1 element) */

						return I_NUM;

					}

				}

				if ( strcmp(token_pool, "I") == 0 )
				{  /* mode "I"  Immediate addressing */
					++inp_ptr;	/* eat the comma */
					get_token();	/* get the next token */
					if ( exprs(1, &EXP1) < 1 )
						break;	/* quit if expr nfg */

					/* If it is a 2 BYTE immediate mode address */
					if ( (opc->op_amode & SPC) == SPC )
					{
						EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
					}

					return I_NUM;	/* return with immediate mode address */

				}
				else if ( strcmp(token_pool, "D") == 0 )
				{  /* mode "D"  Direct page addressing */
					EXP0SP->expr_value = opc->op_value + 0x10;
					++inp_ptr;	/* eat the comma */
					get_token();	/* get the next token */
					if ( exprs(1, &EXP1) < 1 )
						break;	/* quit if expr nfg */
					return D_NUM;
				}
				else if ( strcmp(token_pool, "E") == 0 )
				{  /* mode "E"  Extended addressing */
					EXP0SP->expr_value = (opc->op_value + 0x30);
					++inp_ptr;	/* eat the comma */
					get_token();	/* get the next token */
					if ( exprs(1, &EXP1) < 1 )
						break;	/* quit if expr nfg */
					EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
					return E_NUM;
				}
				else if ( strcmp(token_pool, "NE") == 0 )
				{  /* mode "NE"  Indirect Extended addressing */
					EXP0SP->expr_value = (opc->op_value + 0x20);
					++inp_ptr;		/* eat the comma */
					get_token();		/* get the next token */
					if ( exprs(1, &EXP2) < 1 )
						return -1;	/* test operand */
					EXP1.psuedo_value = EXP1SP->expr_value = 0x9F;	/* set Post Byte value */
					EXP1SP->expr_code = EXPR_VALUE;	/* set expression component is an absolute value */
					EXP1.tag = 'b';
					EXP1.ptr = 1;			/* second stack has operand (1 element) */
					EXP2.tag = (edmask & ED_M68) ? 'W' : 'w';
					/*          EXP2.ptr = 1;             * third stack has operand (1 element) */
					return I_NUM;

				}

				if ( amdcdnum < UNDEF_NUM )
				{
					bad_token(am_ptr, "Unknown address mode");
					break;
				}
				++inp_ptr;	/* eat the comma */
				get_token();	/* pickup the next token */
				amflag = MNBI;	/* signal nothing else allowed */
			}
			else
			{
				EXPR_struct *exp_ptr;
				
				/* if no address mode specified
				   We know we need an operand, so try to figure out if it's
				   Direct (Absolute Zero Page) or Extended (Absolute)  */

				if ( exprs(1, &EXP1) < 1 )
					break;   /* test operand */

				exp_ptr = EXP1.stack;

				if ( (opc->op_amode & D) &&			/* ( if Direct (Absolute Zero Page) mode is legal and */
					( ((edmask & ED_AMA) == 0) ||		/* ( not AMA mode or*/
					(EXP1.base_page_reference != 0) ||	/* base page expression or*/
					( (EXP1.ptr == 1) &&			/* ( expression and */
					(exp_ptr->expr_code == EXPR_VALUE) &&	/* expression is a number and */
					check_4_dpage(&EXP1) &&			/* it's a DPAGE and */
					(EXP1.forward_reference == 0) ) ) )	/* it's not an unknown size) ) ) */
				{
					/* If no AM option - Default Direct (Absolute Zero Page) */
					if ( ((opc->op_amode) == (SHFTAM)) || (opc->op_amode & SPDP) )
					{
						EXP0SP->expr_value = (opc->op_value - 0x40);
					}
					else
					{
						EXP0SP->expr_value = (opc->op_value + 0x10);
					}
					make_dpage_ref(&EXP1); /* Make a DPAGE reference so MAC69 won't complain about Truncation errors*/
					EXP1.tag = 'c';
				}
				else
				{
					if ( opc->op_amode & D )
					{  /* If no AM option - Check for dpage  */

						if ( check_4_dpage(&EXP1) )
						{
							if ( opc->op_amode & SPDP )
							{
								EXP0SP->expr_value = (opc->op_value - 0x40);
							}
							else
							{
								EXP0SP->expr_value = (opc->op_value + 0x10);
							}

							EXP1.psuedo_value = EXP1SP->expr_value = (EXP1SP->expr_value & 0xFF);	/* set operand value */
							EXP1SP->expr_code = EXPR_VALUE;		/* set expression component is an absolute value */
							EXP1.tag = 'b';
							EXP1.ptr = 1;				/* second stack has operand (1 element) */
						}
						else if ( opc->op_amode & E )
						{  /* If no AM option - Default  Extended (Absolute) */
							EXP0SP->expr_value = opc->op_value + 0x30;
							EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
						}

					}

				}

			}

			if ( EXP1.ptr < 1 )
				break;

			return amdcdnum > UNDEF_NUM ? amdcdnum : UNDEF_NUM; /* give 'em an am */

		}   /* -- case default */

	}  /* -- switch(ct) */

	EXP1.ptr = 0;
	return -1;           /* am nfg */

}


void do_opcode(Opcode *opc)
{

	int err_cnt = error_count[MSG_ERROR];

	EXP0.tag = 'b';		/* opcode is default 8 bit byte */
	EXP0.tag_len = 1;	/* only 1 byte */
	EXP1.tag = 'b';		/* assume operand is byte */
	EXP1.tag_len = 1;	/* and only 1 entry long */
	EXP0SP->expr_code = EXPR_VALUE;
	/* set the opcode expression - expression component is an absolute value */
	EXP0SP->expr_value = opc->op_value;
	EXP0.ptr = 1;		/* first stack has opcode (1 element) */
	EXP1.ptr = 0;		/* assume no operands */
	EXP2.ptr = 0;		/* assume no operands */
	EXP3.ptr = 0;		/* assume no operands */

	am_ptr = inp_ptr;	/* remember where am starts */

	if ( (opc->op_amode & SP10) == SP10 )
	{	/*  Set two byte opcode  */
		opc->op_value = opc->op_value | 0x1000;
		EXP0.psuedo_value = EXP0SP->expr_value = opc->op_value;
		EXP0.tag = (edmask & ED_M68) ? 'W' : 'w';
	}
	if ( (opc->op_amode & SP11) == SP11 )
	{	/*  Set two byte opcode  */
		opc->op_value = opc->op_value | 0x1100;
		EXP0.psuedo_value = EXP0SP->expr_value = opc->op_value;
		EXP0.tag = (edmask & ED_M68) ? 'W' : 'w';
	}
	if ( (opc->op_amode == ACC) || (opc->op_amode == IMP) )
	{
		/* no operands */
	}
	else
	{
		if ( (opc->op_amode & S) == S )
		{
			do_branch(opc);
		}
		else
		{
			do_operand(opc);	/* amdcdnum =   */
		}
	}
	if ( err_cnt != error_count[MSG_ERROR] )
	{	/* if any errors detected... */
		while ( !(cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) )
			++inp_ptr; /* ...eat to EOL */
	}
	/*  When you swap from a byte opcode to a word opcode you need to use this command
		to set eps->stack->psuedo_value so the opcodes list correctly in the .lis file
		compress_expr_psuedo(&EXP0);
	*/
	if ( EXP0.ptr > 0 )
	{
		if ( list_bin )
			compress_expr_psuedo(&EXP0);

		p1o_any(&EXP0);		/* output the opcode */
		if ( EXP1.ptr > 0 )
		{
			if ( list_bin )
				compress_expr_psuedo(&EXP1);
			p1o_any(&EXP1);
		}
		if ( EXP2.ptr > 0 )
		{
			if ( list_bin )
				compress_expr_psuedo(&EXP2);
			p1o_any(&EXP2);
		}

	}

}  /* -- opc69() */


int op_ntype(void)
{
	f1_eatit();
	return 1;
}


#include "opcommon.h"


