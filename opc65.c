/*
	opc65.c - Part of macxx, a cross assembler family for various micro-processors
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

/******************************************************************************
Change Log

    08/09/2023	- Added support for BR S,+n and BR S,-n syntax  - Tim Giddens
		  All Branch instructions now support this syntax used in
		  very old source code.

    12/10/2022	- Changed default LIST Flags  - Tim Giddens

******************************************************************************/
#if !defined(MAC_65)
	#define MAC_65
#endif

#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "psttkn65.h"
#include "add_defs.h"
#include "memmgt.h"

#define DEFNAM(name,numb) {"name",name,numb},

/* The following are variables specific to the particular assembler */

char macxx_name[] = "mac65";
char *macxx_target = "6502";
char *macxx_descrip = "Cross assembler for the 6502, 65C02 and 65816.";

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

unsigned long macxx_edm_default = ED_AMA;   /* default edmask */
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
int max_opcode_length = 6;  /* significant length of opcodes and */
int max_symbol_length = 6;  /*  symbols */
static char *am_ptr;
static long cpu_index,cpu_acc;
static int cpu_hist_nest;
enum
{
	CPU_DEFABS,          /* default to absolute addressing */
	CPU_DEFLNG,          /* default to long addressing */
	CPU_DEFDIR           /* default to direct addressing */
} cpu_defam;

/* End of processor specific stuff */

extern unsigned char opc_to_hex[][OP_TO_HEX_SIZE];

enum amflag
{
	MNBI = 1,
	FIN = MNBI << 1,
	LFIN = FIN << 1,
	OPENAT = LFIN << 1,
	OPENBKT = OPENAT << 1,
	GOTSTK = OPENBKT << 1
};

static long ndxtbl[] = { ZX_NUM, AX_NUM, ZY_NUM, AY_NUM };

static struct
{
	char *name;
	AModes am_num;
} forced_am[] = {
	{ "I",    I_NUM },
	{ "A",    A_NUM },
	{ "AL",   AL_NUM },
	{ "D",    D_NUM },
	{ "NY",   NY_NUM },
	{ "LNY",  NNY_NUM },
	{ "XN",   NX_NUM },
	{ "DX",   ZX_NUM },
	{ "DY",   ZY_NUM },
	{ "AX",   AX_NUM },
	{ "ALX",  ALX_NUM },
	{ "AY",   AY_NUM },
	{ "R",    R_NUM },
	{ "RL",   RL_NUM },
	{ "DN",   ND_NUM },
	{ "AN",   N_NUM },
	{ "DLN",  NND_NUM },
	{ "AXN",  NAX_NUM },
	{ "DS",   DS_NUM },
	{ "DSNY", NDSY_NUM },
	{ "XYC",  XYC_NUM },
	{ "L",    DES_NUM },
	{ 0, 0 } };

typedef struct
{
	int flag;            /* .ne. if value is expression */
	unsigned long value;     /* value if not expression */
	EXP_stk *exptr;      /* ptr to expression stack */
} DPage;

typedef struct
{
	int flag;            /* .ne. if value is expression */
	unsigned long value;     /* value if not expression */
	EXP_stk *exptr;      /* ptr to expression stack */
} Bank;

DPage **dpage;
static int dpage_size,dpage_stack;
Bank **bank;
static int bank_size,bank_stack;
static EXP_stk *tmp_expr;
static int tmp_expr_size;

int ust_init(void)
{
	if ( image_name != 0 )
	{
		char *s;
		s = strchr(image_name->name_only, '8');
		if ( s != 0 && strncmp(s, "816", 3) == 0 )
			options[QUAL_P816] = 1;
	}
	if ( options[QUAL_P816] )
	{
		bank_size = dpage_size = 16;
		dpage = (DPage **)MEM_alloc(dpage_size * sizeof(DPage *));
		bank  = (Bank **)MEM_alloc(bank_size * sizeof(Bank *));
		misc_pool_used += (sizeof(Bank *) + sizeof(DPage *)) * dpage_size;
	}
	return 0;            /* would fill a user symbol table */
}

static void do_branch(Opcode *opc)
{
	int s_test;
	long offset;
	EXPR_struct *exp_ptr;
	const char *badExpr=NULL;

	offset = (opc->op_amode & RL) ? 3 : 2;
	s_test = 0;  /* set FALSE */
	get_token();
	if ( *inp_ptr == ',' && token_type == TOKEN_strng && token_value == 1 && toupper(token_pool[0]) == 'S' )
	{
		++inp_ptr;	/* eat the comma */
		get_token();
		s_test = 1;  /* set TRUE */
	}
	if ( exprs(1, &EXP1) < 1 )
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
			{
				badExpr = "Branch target must resolve to an absolute value";
			}
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
		EXP1.tag = (opc->op_amode & RL) ? 'y' : 'z'; /* set branch type operand */
		exp_ptr = EXP1.stack;
		if ( EXP1.ptr == 1 && exp_ptr->expr_code == EXPR_VALUE )
		{
			long max_dist;
			max_dist = (opc->op_amode & RL) ? 32767 : 127;
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
				badExpr = emsg;
			}
		}
		else
		{
			EXP1.psuedo_value = 0;
		}
		if ( !badExpr && options[QUAL_P816] )
		{
			if ( EXP1.ptr + 6 > EXPR_MAXDEPTH )
			{
				badExpr = "Branch address expression too complex";
			}
			else
			{
				exp_ptr = EXP2.stack;
				if ( EXP1.ptr == 1 && EXP1.stack->expr_code == EXPR_VALUE )
				{
					exp_ptr->expr_code = EXPR_SEG;
					exp_ptr->expr_value = EXP1.stack->expr_value + current_offset + offset;
					(exp_ptr++)->expr_seg = current_section;
				}
				else
				{
					memcpy(exp_ptr, EXP1.stack, EXP1.ptr * sizeof(EXPR_struct));
					exp_ptr += EXP1.ptr;
					exp_ptr->expr_code = EXPR_SEG;
					exp_ptr->expr_value = current_offset + offset;
					(exp_ptr++)->expr_seg = current_section;
					exp_ptr->expr_code = EXPR_OPER;
					(exp_ptr++)->expr_value = '+';
				}
				exp_ptr->expr_code = EXPR_VALUE;
				(exp_ptr++)->expr_value = 0x00FF0000;
				exp_ptr->expr_code = EXPR_OPER;
				(exp_ptr++)->expr_value = '&';
				exp_ptr->expr_code = EXPR_SEG;
				exp_ptr->expr_value = 0;
				(exp_ptr++)->expr_seg = current_section;
				exp_ptr->expr_code = EXPR_VALUE;
				(exp_ptr++)->expr_value = 0x00FF0000;
				exp_ptr->expr_code = EXPR_OPER;
				(exp_ptr++)->expr_value = '&';
				exp_ptr->expr_code = EXPR_OPER;
				(exp_ptr++)->expr_value = EXPROPER_TST | (EXPROPER_TST_NE << 8);
				EXP2.ptr = exp_ptr - EXP2.stack;
				snprintf(emsg, ERRMSG_SIZE, "%s:%d: Branch or jmp out of program bank",
						 current_fnd->fn_buff,current_fnd->fn_line);
				write_to_tmp(TMP_TEST, 0, &EXP2, 0);
				write_to_tmp(TMP_ASTNG, strlen(emsg) + 1, emsg, 1);
				EXP2.ptr = 0;          /* don't use this expression any more */
			}
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
{                   /* 0, 1 or 2 operands */
	AModes amdcdnum = ILL_NUM;
	int ct;
	long amflag = 0;
	int str=0;
	
	ct = get_token();            /* pickup the next token */
	switch (ct)
	{
	case EOL:
		{
			return AC_NUM;         /* no operand supplied */
		}
	case TOKEN_pcx:
		{
			if ( token_value == '#' )
			{
				get_token();        /* get the next token */
				if ( exprs(1, &EXP1) < 1 )
					break; /* quit if expr nfg */
				return I_NUM;       /* return with immediate mode address */
			}
			if ( token_value == open_operand )
			{
				amflag = FIN;
			}
			else
			{
				if ( options[QUAL_P816] )
				{
					if ( token_value == '[' )
					{
						amflag = FIN | OPENBKT;
					}
				}
				else if ( token_value == '@' )
				{
					amflag = OPENAT;
				}
			}
			if ( amflag == 0 )
				break;    /* give 'em an illegal am */
			get_token();       /* pickup the next token */
		}
		str = 1; /* force fall through to default */
		/* fall through to default */
	case TOKEN_strng:
		if ( !str && (edmask & ED_MOS) && ((1L << AC_NUM) & opc->op_amode) && token_type == TOKEN_strng && token_value == 1 && _toupper(*token_pool) == 'A' )
		{
			return AC_NUM;
		}
		/* else fall through to default */
	default:
		{
			if ( options[QUAL_P816] || (edmask & ED_MOS) )
			{ /* if mostech format */
				if ( exprs(1, &EXP1) < 1 )
					break;
				if ( amflag & FIN )
				{     /* item started with a '(' */
					int tst;
					tst = (amflag & OPENBKT) ? ']' : close_operand;
					if ( *inp_ptr == tst )
					{
						while ( ++inp_ptr,isspace(*inp_ptr) ); /* eat ) and ws */
						if ( *inp_ptr == ',' )
						{    /* followed with a comma */
							++inp_ptr;         /* eat the comma */
							get_token();       /* pickup the address mode */
							if ( token_type == TOKEN_strng &&
								 token_value == 1 &&
								 _toupper(*token_pool) == 'Y' )
							{
								return (amflag & OPENBKT) ? NNY_NUM : NY_NUM; /* syntax [nn],y */
							}
							bad_token(am_ptr, (amflag & OPENBKT) ?
									  "Invalid syntax. Expected [nn],Y form" :
									  "Invalid syntax. Expected (nn),Y form");
							break;
						}
						else
						{
							if ( amflag & OPENBKT )
								return NND_NUM; /* plain direct long indirect [d] or [a] */
							return N_NUM;      /* plain indirect (d) or (a) */
						}
					}
					else
					{             /* -- ended with ']' or ')' */
						if ( *inp_ptr == ',' )
						{    /* perhaps a (nn,X) format */
							int reg;
							++inp_ptr;         /* eat the comma */
							get_token();       /* pickup the next item */
							reg = _toupper(*token_pool);
							if ( token_type == TOKEN_strng &&
								 token_value == 1 &&
								 *inp_ptr == close_operand &&
								 (amflag & OPENBKT) == 0 &&
								 (reg == 'X' || reg == 'S') )
							{
								++inp_ptr;      /* eat the closing ')' */
								if ( reg == 'X' )
									return (opc->op_amode & NAX) ? NAX_NUM : NX_NUM;
								amflag |= GOTSTK;
							}
							else
							{
								bad_token(am_ptr, "Invalid syntax. Expected form (nn,X)");
								break;
							}
						}
						else
						{
							bad_token(am_ptr, "Invalid syntax. Expected form (nn) or (nn,X)");
							break;
						}
					}                /* -- if closed with a ')' */
					if ( amflag & GOTSTK )
					{
						if ( *inp_ptr != ',' )
						{
							bad_token(inp_ptr, "Expected (nn,S),Y syntax");
							break;
						}
						while ( ++inp_ptr,isspace(*inp_ptr) );   /* eat comma and ws */
						get_token();      /* pickup the next item */
						if ( token_type != TOKEN_strng || token_value != 1 ||
							 _toupper(*token_pool) != 'Y' )
						{
							bad_token(tkn_ptr, "Expected form (D,S),Y");
							break;
						}
						return NDSY_NUM;
					}
					sprintf(emsg, "Expected a closing '%c' character", tst);
					bad_token(inp_ptr, emsg);
					break;
				}
				else
				{            /* -+ if opened with a '(' */
					if ( *inp_ptr == ',' )
					{   /* stop on a comma? */
						char xy;
						while ( ++inp_ptr,isspace(*inp_ptr) );   /* eat it and ws */
						get_token();      /* pick up address mode */
						xy = *token_pool;
						xy = _toupper(xy);
						if ( token_type == TOKEN_strng && token_value == 1 )
						{
							switch (xy)
							{
							case 'X':
								return X_NUM;  /* syntax is nn,X */
							case 'Y':
								return Y_NUM;  /* syntax is nn,Y */
							case 'S':
								return DS_NUM; /* syntax is nn,S */
							}
						}
						bad_token(am_ptr, (options[QUAL_P816]) ?
								  "Expected indexed form of nn,X or nn,Y or nn,S" :
								  "Expected indexed form of nn,X or nn,Y");
						break;
					}                /* -- end on comma */
					return 0;            /* no am specified */
				}                   /* -- didn't open on '(' */
			}
			else
			{               /* -+ if MOS format */
				if ( token_type == TOKEN_strng && *inp_ptr == ',' )
				{
					char c;
					if ( squeak )
						printf("opc65:do_operand() entry 1: token_type=TOKEN_strng, *inp_ptr == ',', amflag=0x%04lX\n", amflag);
					if ( amflag != 0 )
					{
						bad_token(am_ptr, "Invalid address mode syntax");
						break;
					}
					c = *token_pool;
					c = _toupper(c);
					if ( token_value == 1 )
					{
						if ( c == 'I' )
							amdcdnum = I_NUM;
						else if ( c == 'Z' )
							amdcdnum = Z_NUM;
						else if ( c == 'A' )
							amdcdnum = A_NUM;
						else if ( c == 'N' )
							amdcdnum = N_NUM;
						else if ( c == 'X' )
							amdcdnum = X_NUM;
						else if ( c == 'Y' )
							amdcdnum = Y_NUM;
						if ( squeak )
							printf("opc65:do_operand() entry 2: token_value==1, Found c='%c', amdcdnum=%d\n", c, amdcdnum);
					}
					else
					{
						if ( token_value == 2 )
						{
							char c1;
							c1 = *(token_pool + 1);
							c1 = _toupper(c1);
							if ( c == 'Z' )
							{
								if ( c1 == 'X' )
									amdcdnum = ZX_NUM;
								else if ( c1 == 'Y' )
									amdcdnum = ZY_NUM;
							}
							else if ( c == 'A' )
							{
								if ( c1 == 'X' )
									amdcdnum = AX_NUM;
								else if ( c1 == 'Y' )
									amdcdnum = AY_NUM;
								else if ( c1 == 'N' )
									amdcdnum = N_NUM;
							}
							else if ( c == 'N' )
							{
								if ( c1 == 'X' )
									amdcdnum = NX_NUM;
								else if ( c1 == 'Y' )
									amdcdnum = NY_NUM;
							}
							if ( squeak )
								printf("opc65:do_operand() entry 2: token_value==2, Found c='%c', c1='%c', amdcdnum=%d\n", c, c1, amdcdnum);
						}
					}
					if ( amdcdnum < UNDEF_NUM )
					{
						bad_token(am_ptr, "Unknown address mode");
						break;
					}
					++inp_ptr;       /* eat the comma */
					get_token();     /* pickup the next token */
					amflag = MNBI;       /* signal nothing else allowed */
				}
				else
				{
					if ( squeak )
						printf("opc65:do_operand() entry 1: token_type=%d, *inp_ptr == '%s', amflag=0x%04lX\n", token_type, inp_ptr, amflag);
				}
				if ( exprs(1, &EXP1) < 1 )
					break;
				if ( (amflag & MNBI) == 0 )
				{
					if ( *inp_ptr == open_operand )
					{
						char xy;
						++inp_ptr;        /* eat the ( */
						get_token();
						xy = *token_pool;
						xy = _toupper(xy);
						if ( token_type == TOKEN_strng &&
							 token_value == 1 &&
							 (xy == 'X' || xy == 'Y') &&
							 *inp_ptr == close_operand )
						{
							++inp_ptr;     /* eat closing ) */
							if ( amflag & OPENAT )
								return (xy == 'X') ? NX_NUM : NY_NUM;
							return (xy == 'X') ? X_NUM : Y_NUM;
						}
						if ( amflag & OPENAT )
							bad_token(am_ptr, "Illegal index syntax. Expected @n(X) or @n(Y)");
						else
							bad_token(am_ptr, "Illegal index syntax. Expected n(X) or n(Y)");
						break;
					}        /* -- if not open '(' */
					else if ( amflag == OPENAT )
						return N_NUM;
				}           /* -- if MNBI */
				return amdcdnum > UNDEF_NUM ? amdcdnum : UNDEF_NUM; /* give 'em an am */
			}          /* -- if MOS format */
		}             /* -- case default */
	}                /* -- switch(ct) */
	EXP1.ptr = 0;
	return -1;           /* am nfg */
}

static int bad_amode(void)
{
	EXP0SP->expr_code = EXPR_VALUE;
	EXP0.ptr = 1;
	if ( options[QUAL_P816] )
	{
		EXP0.tag_len = 1;
		EXP0.tag = 'l';
		EXP0.psuedo_value = EXP0SP->expr_value = 0xEAEAEAEA;  /* give 'em a bunch of nop's */
	}
	else
	{
		EXP0.tag = 'x';
		EXP0.tag_len = 24;
		EXP0.psuedo_value = EXP0SP->expr_value = 0xEAEAEA;    /* give 'em a bunch of nop's */
	}
	EXP1.ptr = 0;        /* and no operands */
	EXP2.ptr = 0;        /* and no operands */
	EXP3.ptr = 0;        /* and no operands */
	return 0;            /* no address mode */
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
	dst += dpstk->ptr;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = '-';
	estk->ptr += dpstk->ptr + 1;
	estk->tag = 'c';
	estk->ptr = compress_expr(estk);
	return 0;
}

static int make_bank_ref(EXP_stk *estk)
{
	Bank *bp;
	EXP_stk *bpstk;
	EXPR_struct * src,*dst;
	bp = *(bank + bank_stack);
	if ( bp == 0 )
		return 0;   /* nothing to do */
	dst = estk->stack + estk->ptr;
	bpstk = bp->exptr;
	src = bpstk->stack;
	if ( bpstk->ptr + estk->ptr + 1 >= EXPR_MAXDEPTH )
	{
		bad_token((char *)0, "Absolute address expression too complex");
		return 0;
	}
	memcpy(dst, src, bpstk->ptr * sizeof(EXPR_struct));
	dst += bpstk->ptr;
	dst->expr_code = EXPR_OPER;
	dst->expr_value = '-';
	estk->ptr += bpstk->ptr + 1;
	estk->ptr = compress_expr(estk);
	return 0;
}

static int make_pbr_ref(EXP_stk *estk)
{
	EXPR_struct *dst;
	dst = estk->stack + estk->ptr;
	if ( estk->ptr + 4 >= EXPR_MAXDEPTH )
	{
		bad_token((char *)0, "Absolute address expression too complex");
		return 0;
	}
	dst->expr_code = EXPR_SEG;
	dst->expr_value = 0;
	(dst++)->expr_seg = current_section;
	dst->expr_code = EXPR_VALUE;
	(dst++)->expr_value = 0xFF0000;
	dst->expr_code = EXPR_OPER;
	(dst++)->expr_value = '&';
	dst->expr_code = EXPR_OPER;
	(dst++)->expr_value = '-';
	estk->ptr += 4;
	return 0;
}

static int check_4_dpage(EXP_stk *estk)
{
	DPage *dp;
	EXP_stk * testk,*dexp;
	EXPR_struct * expr,*texpr,*tp;
	
	expr = estk->stack;
	dp = *(dpage + dpage_stack);
	if ( estk->base_page_reference )
		return 1; /* base page reference */
	if ( squeak )
		printf("check_4_dpage(): #expr:%d, sptr->code=%d, sptr->value=%08lX, fwd=%d\n", estk->ptr, expr->expr_code, expr->expr_value, estk->forward_reference);
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
		int i;
		dexp = dp->exptr;
		i = (dexp->ptr + estk->ptr + 1) * sizeof(EXPR_struct) + sizeof(EXP_stk);
		if ( i > tmp_expr_size )
		{
			if ( tmp_expr != 0 )
				MEM_free((char *)tmp_expr);
			tmp_expr = (EXP_stk *)MEM_alloc(i + 8 * sizeof(EXPR_struct));
			tmp_expr_size = i + 8 * sizeof(EXPR_struct);
		}
		testk = tmp_expr;
		memcpy(testk, estk, sizeof(EXP_stk));
		texpr = testk->stack = (EXPR_struct *)(testk + 1);
		memcpy(texpr, expr, estk->ptr * sizeof(EXPR_struct));
		tp = texpr + estk->ptr;
		memcpy(tp, dexp->stack, dexp->ptr * sizeof(EXPR_struct));
		tp += dexp->ptr;
		tp->expr_code = EXPR_OPER;
		tp->expr_value = '-';
		testk->ptr += dexp->ptr + 1;
		i = compress_expr(testk);
		if ( i == 1 && (texpr->expr_code == EXPR_VALUE && (texpr->expr_value & -256) == 0) &&
			 testk->forward_reference == 0 )
		{
			return 1;
		}
	}
	return 0;
}

static int check_4_abs(EXP_stk *estk)
{
	Bank *bp;
	EXP_stk * testk,*bexp;
	EXPR_struct * expr,*texpr,*tp;
	expr = estk->stack;
	bp = *(bank + bank_stack);
	if ( bp == 0 )
	{
		if ( (estk->ptr == 1) &&           /* or expression is < 65535 */
			 (expr->expr_code == EXPR_VALUE) &&
			 ((expr->expr_value & -65536L) == 0) &&
			 (estk->forward_reference == 0) )
			return 1;
	}
	else
	{
		int i;
		bexp = bp->exptr;
		i = (bexp->ptr + estk->ptr + 1) * sizeof(EXPR_struct) + sizeof(EXP_stk);
		if ( i > tmp_expr_size )
		{
			if ( tmp_expr != 0 )
				MEM_free((char *)tmp_expr);
			tmp_expr = (EXP_stk *)MEM_alloc(i + 8 * sizeof(EXPR_struct));
			tmp_expr_size = i + 8 * sizeof(EXPR_struct);
		}
		testk = tmp_expr;
		memcpy(testk, estk, sizeof(EXP_stk));
		texpr = testk->stack = (EXPR_struct *)(testk + 1);
		memcpy(texpr, expr, estk->ptr * sizeof(EXPR_struct));
		tp = texpr + estk->ptr;
		memcpy(tp, bexp->stack, bexp->ptr * sizeof(EXPR_struct));
		tp += bexp->ptr;
		tp->expr_code = EXPR_OPER;
		tp->expr_value = '-';
		testk->ptr += bexp->ptr + 1;
		i = compress_expr(testk);
		if ( i == 1 && (texpr->expr_code == EXPR_VALUE && (texpr->expr_value & -65536L) == 0) &&
			 testk->forward_reference == 0 )
		{
			return 1;
		}
	}
	return 0;
}

static int check_am(Opcode *opc, AModes amdcdnum, AModes forced_am_num)
{
	long amdcd;
	EXPR_struct *exp_ptr;
#if 0
	if ( squeak )
		printf("check_am(): opc=%s, amodes=%08lX, amdcdnum=%d, forced_am_num=%d\n", opc->op_name, opc->op_amode, amdcdnum, forced_am_num);
#endif
	if ( forced_am_num != 0 )
	{
		if ( forced_am_num == DES_NUM )
		{   /* he only wants long mode */
			switch (amdcdnum)
			{
			case UNDEF_NUM:
			case A_NUM:
			case AL_NUM:
				amdcdnum = AL_NUM;
				break;
			case X_NUM:
			case AX_NUM:
			case ALX_NUM:
				amdcdnum = ALX_NUM;
				break;
			default:
				bad_token((char *)0, "No \"LONG\" addressing for this address mode");
			}
		}
		else if ( forced_am_num == D_NUM )
		{
			switch (amdcdnum)
			{
			case UNDEF_NUM:
			case D_NUM:
				amdcdnum = D_NUM;
				break;
			case X_NUM:
				amdcdnum = ZX_NUM;
				break;
			case Y_NUM:
				amdcdnum = ZY_NUM;
				break;
			default:
				bad_token((char *)0, "No \"DIRECT\" addressing for this address mode");
			}
		}
		else if ( forced_am_num == A_NUM )
		{
			switch (amdcdnum)
			{
			case UNDEF_NUM:
			case A_NUM:
				amdcdnum = A_NUM;
				break;
			case X_NUM:
				amdcdnum = AX_NUM;
				break;
			case Y_NUM:
				amdcdnum = AY_NUM;
				break;
			default:
				bad_token((char *)0, "No \"ABSOLUTE\" addressing for this address mode");
			}
		}
		else
		{
			if ( amdcdnum != UNDEF_NUM && amdcdnum != forced_am_num )
			{
				bad_token((char *)0, "Conflicting address modes selected.");
			}
			amdcdnum = forced_am_num;
		}
	}
	if ( amdcdnum == AC_NUM )
	{        /* if no operand supplied */
		if ( ((1L << amdcdnum) & opc->op_amode) == 0 )
		{
			bad_token((char *)0, "Opcode requires an operand");
			return bad_amode();
		}
		return amdcdnum;
	}
	if ( EXP1.ptr < 0 || amdcdnum < 0 )
	{  /* if bad am or exprs found an error */
		return bad_amode();
	}
	amdcd = amdcdnum ? (1L << amdcdnum) : 0;
	exp_ptr = EXP1.stack;
	if ( options[QUAL_P816] )
	{
		static struct
		{
			AModes numz, numa, numl;
			long maskz, maska, maskl;
		} *tp, test[] = { { Z_NUM, A_NUM, AL_NUM, Z, A, AL },
			{ ZX_NUM, AX_NUM, ALX_NUM, ZX, AX, ALX },
			{ ZY_NUM, AY_NUM, 0, ZY, AY, 0 },
			{ ND_NUM, N_NUM, 0, ND, N, 0 } };

		if ( amdcdnum <= UNDEF_NUM )
			tp = test;
		else if ( amdcdnum == X_NUM )
			tp = test + 1;
		else if ( amdcdnum == Y_NUM )
			tp = test + 2;
		else if ( amdcdnum == N_NUM )
			tp = test + 3;
		else
			tp = 0;
		if ( tp != 0 )
			do
			{
				if ( opc->op_amode & tp->maskz )
				{     /* direct page */
					if ( check_4_dpage(&EXP1) ||
						 ((opc->op_amode & (tp->maska + tp->maskl)) == 0 ||
						  (!check_4_abs(&EXP1) && cpu_defam == CPU_DEFDIR)) )
					{
						amdcd = tp->maskz;
						amdcdnum = tp->numz;
						break;
					}
				}
				if ( opc->op_amode & tp->maska )
				{     /* absolute */
					if ( check_4_abs(&EXP1) ||
						 ((opc->op_amode & tp->maskl) == 0 || cpu_defam == CPU_DEFABS) )
					{
						amdcd = tp->maska;
						amdcdnum = tp->numa;
						break;
					}
				}
				if ( opc->op_amode & tp->maskl )
				{     /* long */
					amdcd = tp->maskl;
					amdcdnum = tp->numl;
				}
			} while ( 0 );
	}
	else
	{
		if ( amdcd == 0 )
		{     /* if no address mode specified */
			if ( squeak )
			{
				printf("check_am(): pass=%d, Checking for Z vs. A mode. EXP1.ptr=%d, EXP1.fwd=%d, exp->code=%d, exp->value=%08lX\n",
					   pass,
					   EXP1.ptr,
					   EXP1.forward_reference,
					   exp_ptr->expr_code,
					   exp_ptr->expr_value
					   );
			}
			if (    (opc->op_amode & Z)         /* if Z mode is legal */
				 && (
					 (
					      (edmask & ED_AMA) == 0)   /* and not AMA mode */
					   || (EXP1.base_page_reference != 0) /* base page expression */
					   || (
						       (EXP1.ptr == 1)        /* or expression is < 256 */
						    && (exp_ptr->expr_code == EXPR_VALUE)
						    && ((exp_ptr->expr_value & -256) == 0)
						    && (EXP1.forward_reference == 0)
						  )
					 )
			  )
			{
				amdcd = Z;      /* force it direct page */
				amdcdnum = Z_NUM;
			}
			else
			{
				if ( !(opc->op_amode & A) )
				{ /* if normal absolute not allowed */
					amdcd = AL;      /* force it absolute long */
					amdcdnum = AL_NUM;
				}
				else
				{
					amdcd = A;       /* else force it absolute */
					amdcdnum = A_NUM;
				}
			}
		}
		else
		{
			if ( (amdcd & (X + Y)) )
			{
				int amindex0;
				amindex0 = (amdcd & X) ? 1 : 3; /* assume AX or AY */
				if ( (EXP1.base_page_reference != 0) /* if it's an explicit base page ref... */
					 || (EXP1.forward_reference == 0  /* ...or it's not a forward reference... */
						 && ((EXP1.ptr == 1)      /* ...and it's expression evals to < 256 */
							 && (exp_ptr->expr_code == EXPR_VALUE)
							 && ((exp_ptr->expr_value & -256) == 0)
							)
						)
				   )
				{
					amindex0 ^= 1;       /* change to Z mode */
				}
				amdcdnum = ndxtbl[amindex0];
				amdcd = 1L << amdcdnum;
				if ( !(amdcd & opc->op_amode) )
				{ /* legal ? */
					amindex0 ^= 1;       /* nope, try the other */
					amdcdnum = ndxtbl[amindex0];
					amdcd = 1L << amdcdnum;
				}
			}
		}
	}
	if ( !(amdcd & opc->op_amode) )
	{
		bad_token(am_ptr, "Illegal address mode for this instruction");
		return bad_amode();
	}
	return amdcdnum;
}

static int compute_opcode(Opcode *opc, int amdcdnum)
{
	int i;
	long amdcd;
	if ( amdcdnum != 0 )
	{
		amdcd = 1L << amdcdnum;
		i = opc->op_value;        /* pickup the value */
		EXP0.psuedo_value = EXP0SP->expr_value = opc_to_hex[i][amdcdnum - 1];
		if ( EXP0.psuedo_value == 0 )
		{
			sprintf(emsg, "Illegal address mode. opcindx=%d, amdcdnum=%d. Please submit an SPR",
					i, amdcdnum);
			bad_token(am_ptr, emsg);
			return 0;
		}
	}
	else
	{
		amdcd = 0;
	}
	if ( options[QUAL_P816] )
	{
		if ( (opc->op_class & OPCLPBR) && (amdcd & (A + NAX + N + ND + NND)) )
		{
			if ( amdcd & (A + NAX) )
				make_pbr_ref(&EXP1);
			EXP1.tag = (edmask & ED_M68) ? 'U' : 'u';
		}
		else if ( amdcd & (Z + NY + NNY + NX + ZX + ZY + ND + N) )
		{
			make_dpage_ref(&EXP1);
			EXP1.tag = 'c';
		}
		else if ( amdcd & (A + AX + AY) )
		{
			make_bank_ref(&EXP1);
			EXP1.tag = (edmask & ED_M68) ? 'U' : 'u';
		}
		else if ( amdcd & (AL + ALX) )
		{
			EXP1.tag = (edmask & ED_M68) ? 'X' : 'x';
			EXP1.tag_len = 24;
		}
		else if ( (amdcd & I) && ((opc->op_class & OPCLINDX) ? (cpu_index & 1) : (cpu_acc & 1)) )
		{
			EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
		}
	}
	else
	{
		if ( amdcd & (A + AX + AY + N) )
		{
			EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
		}
	}
	return 1;
}

void do_opcode(Opcode *opc)
{
	int i, ct;
	AModes amdcdnum = UNDEF_NUM, forced_am_num;
	int err_cnt = error_count[MSG_ERROR];
	EXPR_struct *exp_ptr;
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
	exp_ptr = EXP1SP;
	am_ptr = inp_ptr;            /* remember where am starts */
	forced_am_num = UNDEF_NUM;
	if ( *inp_ptr == '`' )
	{       /* next thing a grave accent? */
		++inp_ptr;            /* yep, eat it */
		ct = get_token();         /* pickup the next item */
		if ( ct != TOKEN_strng )
		{      /* better be a string */
			bad_token(tkn_ptr, "Illegal address mode specifier");
		}
		else
		{
			for ( i = 0; forced_am[i].name; ++i )
			{
				if ( strcmp(token_pool, forced_am[i].name) == 0 )
					break;
			}
			if ( forced_am[i].name == 0 )
			{
				bad_token(tkn_ptr, "Illegal address mode specifier");
			}
			else
			{
				forced_am_num  = forced_am[i].am_num;
				if ( forced_am_num != DES_NUM && ((1L << forced_am_num) & opc->op_amode) == 0 )
				{
					bad_token(tkn_ptr, "Forced address mode not legal for this instruction. Ignored");
					forced_am_num = UNDEF_NUM;
				}
			}
		}
	}
	switch (opc->op_class & 3)
	{
	case OPCL03:          /* branch instructions */
		do_branch(opc);        /* do branch processing (fall thru to break) */
	case OPCL00:          /* no operands required */
		break;
	case OPCL02:
		amdcdnum = do_operand(opc);    /* optional operands */
		if ( squeak )
			printf("opc65: do_opcode(): do_operand() OPCL02 returned %d, forced_am_num=%d, opc: name='%s', amode=0x%08lX\n",
				   amdcdnum, forced_am_num, opc->op_name,  opc->op_amode);
		if ( !compute_opcode(opc, check_am(opc, amdcdnum, forced_am_num)) )
			bad_amode();
		break;
	case OPCL01:
		{
			int ct;
			if ( (opc->op_amode & I) != 0 && *inp_ptr == '#' )
				while ( ++inp_ptr,isspace(*inp_ptr) );
			ct = get_token();
			if ( ct == EOL )
			{
				bad_token((char *)0, "Op code requires an operand");
				EXP1.ptr = -1;      /* make sure there's an operand */
			}
			else
			{
				if ( exprs(1, &EXP1) < 1 )
					break;
			}
			if ( EXP1.ptr < 1 )
			{
				EXP1.ptr = 1;       /* make sure there's an operand */
				exp_ptr->expr_code = EXPR_VALUE;
				exp_ptr->expr_value = 0;
			}
			if ( opc->op_amode & A )
			{
				EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
			}
			else if ( opc->op_amode & AL )
			{
				EXP1.tag = (edmask & ED_M68) ? 'X' : 'x';
				EXP1.tag_len = 24;
			}
			else if ( opc->op_amode & XYC )
			{
				comma_expected = 1;
				ct = get_token();
				if ( ct == EOL )
				{
					bad_token((char *)0, "Opcode requires 2 operands");
					EXP2.ptr = -1;
				}
				else
				{
					exprs(1, &EXP2);
				}
				if ( EXP2.ptr <= 0 )
				{
					EXP2.ptr = 1;        /* make sure there's an operand */
					EXP2.stack->expr_code = EXPR_VALUE;
					EXP2.stack->expr_value = 0;
				}
				EXP2.tag = 'b';
			}              /* -- am XYC */
		}                 /* -- case OPCL1 */
	}                    /* -- switch(class) */
	if ( err_cnt != error_count[MSG_ERROR] )
	{ /* if any errors detected... */
		while ( !(cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) )
			++inp_ptr; /* ...eat to EOL */
	}
	if ( EXP0.ptr > 0 )
	{
		p1o_any(&EXP0);           /* output the opcode */
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
}                   /* -- opc65() */

int op_ntype(void)
{
	f1_eatit();
	return 1;
}

static int get_cpuarg(void)
{
	int tt, ans, radix;
	ans = -1;            /* assume nfg */
	if ( *inp_ptr != '=' )
	{
		bad_token(inp_ptr, "Expected an '=' here");
	}
	else
	{
		while ( ++inp_ptr,isspace(*inp_ptr) );
		comma_expected = 0;
		radix = current_radix;
		current_radix = 10;   /* assume decimal mode */
		tt = get_token();
		if ( tt != TOKEN_numb || (token_value != 8 && token_value != 16) )
		{
			bad_token(tkn_ptr, "Expected the number 8 or 16 here");
		}
		else
		{
			ans = (token_value == 16) ? 1 : 0;
		}
		current_radix = radix;
	}
	return ans;
}

int op_cpu(void)
{
	int tt, indx, accum;
	if ( !options[QUAL_P816] )
	{
		bad_token((char *)0, "Directive not meaningful for other than 65816 processor");
		f1_eatit();
		return 1;
	}
	indx = cpu_index & 1;
	accum = cpu_acc & 1;
	for (;; comma_expected = 1 )
	{
		int len;
		tt = get_token();
		if ( tt == EOL )
			break;
		len = token_value;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a symbolic string here");
		}
		else
		{
			if ( strncmp(token_pool, "SAVE", len) == 0 )
			{
				if ( cpu_hist_nest > 30 )
				{
					bad_token(tkn_ptr, "Nest level greater than 31");
				}
				else
				{
					++cpu_hist_nest;
					cpu_index <<= 1;
					cpu_acc   <<= 1;
				}
			}
			else if ( strncmp(token_pool, "RESTORE", len) == 0 )
			{
				if ( cpu_hist_nest < 1 )
				{
					bad_token(tkn_ptr, "Nest level already at 0");
				}
				else
				{
					--cpu_hist_nest;
					cpu_index >>= 1;
					cpu_acc   >>= 1;
					indx = cpu_index & 1;
					accum = cpu_acc & 1;
				}
			}
			else if ( strncmp(token_pool, "INDEX", len) == 0 )
			{
				int sts;
				sts = get_cpuarg();
				if ( sts >= 0 )
					indx = sts;
			}
			else if ( strncmp(token_pool, "ACCUMULATOR", len) == 0 )
			{
				int sts;
				sts = get_cpuarg();
				if ( sts >= 0 )
					accum = sts;
			}
			else if ( strncmp(token_pool, "LONG", len) == 0 )
			{
				cpu_defam = CPU_DEFLNG;
			}
			else if ( strncmp(token_pool, "DIRECT", len) == 0 )
			{
				cpu_defam = CPU_DEFDIR;
			}
			else if ( strncmp(token_pool, "ABSOLUTE", len) == 0 )
			{
				cpu_defam = CPU_DEFABS;
			}
			else
			{
				bad_token(tkn_ptr, "Undefined argument");
			}
		}
	}
	cpu_index = (cpu_index & ~1) | indx;
	cpu_acc   = (cpu_acc & ~1) | accum;
	EXP0.psuedo_value = 0;
	return 1;
}

#if 0
static struct
{
	int flag;            /* .ne. if value is expression */
	unsigned long value;     /* value if not expression */
	EXP_stk *exptr;      /* ptr to expression stack */
}
DPage;
#endif

int op_bank(void)
{
	int i, cnt, typ;
	Bank *bp;
	EXP0.psuedo_value = 0;
	if ( !options[QUAL_P816] )
	{
		bad_token((char *)0, "Directive not meaningful for other than 65816 processor");
		f1_eatit();
		return 1;
	}
	typ = get_token();           /* pickup the next token */
	if ( typ == EOL )
	{
	pop_bank:
		if ( bank_stack > 0 )
		{
			if ( bank_stack < 2 ||
				 *(bank + bank_stack) != *(bank + bank_stack - 1) )
				MEM_free((char *)bank[bank_stack]);
			*(bank + bank_stack) = 0;
			--bank_stack;
		}
		return 2;
	}
	i = exprs(1, &EXP0);      /* evaluate the expression */
	if ( i == 0 || *inp_ptr == ',' )
	{ /* are we supposed to save the old bank? */
		int typ;
		++inp_ptr;        /* yep, eat the comma */
		typ = get_token();    /* pick up the next token */
		if ( typ == TOKEN_strng )
		{
			int len = token_value;
			if ( strncmp("SAVE", token_pool, len) == 0 )
			{
				if ( ++bank_stack >= bank_size )
				{
					int new = bank_stack + bank_stack / 2;
					bank = (Bank **)MEM_realloc((char *)bank, new * sizeof(Bank *));
				}
				if ( i == 0 )
				{
					*(bank + bank_stack) = *(bank + bank_stack - 1);
					return 2;
				}
			}
			else if ( strncmp("RESTORE", token_pool, len) == 0 )
			{
				if ( i > 0 )
					bad_token((char *)0, "Expression ignored on bank RESTORE");
				goto pop_bank;
			}
			else
			{
				bad_token(tkn_ptr, "Expected one of SAVE or RESTORE");
				f1_eatit();
			}
		}
		else if ( typ == EOL )
			goto pop_bank;
	}
	if ( i > 0 )
	{         /* if 1 or more terms... */
		EXP_stk *exp_ptr;

		cnt = sizeof(Bank) + (i + 2) * sizeof(EXPR_struct) + sizeof(EXP_stk);
		misc_pool_used += cnt;
		bp = (Bank *)MEM_alloc(cnt);
		*(bank + bank_stack) = bp;      /* save it for later */
		exp_ptr = (EXP_stk *)(bp + 1);
		memcpy(exp_ptr, &EXP0, sizeof(EXP_stk));
		exp_ptr->stack = (EXPR_struct *)(exp_ptr + 1);
		memcpy(exp_ptr->stack, EXP0.stack, i * sizeof(EXPR_struct));
		bp->exptr = exp_ptr;
		if ( i == 1 && EXP0SP->expr_code == EXPR_VALUE )
		{
			if ( EXP0SP->expr_value & 0xFF000000 )
			{
				bad_token((char *)0, "Bank address out of range");
			}
			EXP0SP->expr_value &= 0x00FF0000;
			bp->value = exp_ptr->stack->expr_value = EXP0SP->expr_value;
			bp->flag = 0;
		}
		else
		{
			EXPR_struct *ep;
			bp->flag = 1;      /* signal that there's an expression */
			ep = exp_ptr->stack + i;
			ep->expr_code = EXPR_VALUE;
			(ep++)->expr_value = 0x00FF0000;
			ep->expr_code = EXPR_OPER;
			ep->expr_value = '&';
			exp_ptr->ptr = i + 2;        /* set the length */
		}
		return 2;         /* and quit */
	}
	EXP0.psuedo_value = 0;
	return 2;
}

int op_dpage(void)
{
	int i, cnt, typ;
	DPage *dp;
	if ( !options[QUAL_P816] )
	{
		bad_token((char *)0, "Directive not meaningful for other than 65816 processor");
		f1_eatit();
		return 1;
	}
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

int op_triplet(void)
{
	char *otp;
	long epv;
#if 0
	op_chkalgn(1,1);     /* check pc for alignment */
#endif
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	EXP0.tag = (edmask & ED_M68) ? 'X' : 'x';
	EXP0.tag_len = 24;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		EXP0SP->expr_value = EXP0.psuedo_value = 0;
		EXP0.ptr = 1;
		p1o_var(&EXP0);       /* null arg means insert a 0 */
		return 1;
	}
	while ( 1 )
	{           /* for all items on line */
		char c;
		if ( *inp_ptr == ',' )
		{    /* if the next item is a comma */
			while ( c = *++inp_ptr,isspace(c) ); /* skip over white space */
			EXP0SP->expr_code = EXPR_VALUE;
			epv = EXP0SP->expr_value = EXP0.psuedo_value = 0;
			EXP0.ptr = 1;
		}
		else
		{
			if ( get_token() == EOL )
				break;
			otp = tkn_ptr;     /* remember beginning of expression */
			exprs(1, &EXP0);    /* pickup the expression */
			epv = EXP0SP->expr_value;
			if ( *inp_ptr == ',' )
			{ /* if the next item is a comma */
				while ( c = *++inp_ptr,isspace(c) ); /* skip over white space */
			}
			/*        0x00000000            0x00000000 */
			if ( epv > 0x00FFFFFFl || epv < -0x01000000l )
			{
				snprintf(emsg, ERRMSG_SIZE, "Triplet truncation error. Desired: %08lX, stored: %06lX",
						epv, epv & 0x00FFFFFFl);
				show_bad_token(otp, emsg, MSG_WARN);
				EXP0SP->expr_value &= 0x00FFFFFF;
			}
		}
		p1o_var(&EXP0);       /* dump element 0 */
	}
	return 1;
}

int op_address(void)
{
	char *otp;
	long epv;
#if 0
	op_chkalgn(1,1);     /* check pc for alignment */
#endif
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	EXP0.tag = (edmask & ED_M68) ? 'W' : 'w';
	EXP0.tag_len = 1;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		EXP0SP->expr_value = EXP0.psuedo_value = 0;
		EXP0.ptr = 1;
		p1o_word(&EXP0);      /* null arg means insert a 0 */
		return 1;
	}
	while ( 1 )
	{           /* for all items on line */
		char c;
		if ( *inp_ptr == ',' )
		{    /* if the next item is a comma */
			while ( c = *++inp_ptr,isspace(c) ); /* skip over white space */
			EXP0SP->expr_code = EXPR_VALUE;
			epv = EXP0SP->expr_value = EXP0.psuedo_value = 0;
			EXP0.ptr = 1;
		}
		else
		{
			if ( get_token() == EOL )
				break;
			otp = tkn_ptr;     /* remember beginning of expression */
			exprs(1, &EXP0);    /* pickup the expression */
			make_pbr_ref(&EXP0);
			if ( *inp_ptr == ',' )
			{ /* if the next item is a comma */
				while ( c = *++inp_ptr,isspace(c) ); /* skip over white space */
			}
			EXP0.ptr = compress_expr(&EXP0);
			if ( EXP0.ptr == 1 && EXP0SP->expr_code == EXPR_VALUE )
			{
				epv = EXP0SP->expr_value;
				if ( epv > 65535L || epv < -65536L )
				{
					sprintf(emsg, "Address truncation error. Desired: %08lX, stored: %04lX",
							epv, epv & 65535L);
					show_bad_token(otp, emsg, MSG_WARN);
					EXP0SP->expr_value &= 65535;
				}
			}
		}
		p1o_word(&EXP0);      /* dump element 0 */
	}
	return 1;
}

#include "opcommon.h"
