/*
	opc69.c - Part of macxx, a cross assembler family for various micro-processors
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

	03/26/2022	- 6809 CPU support added by Tim Giddens
		  using David Shepperd's work as a reference

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

char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;


/* default edmask */
unsigned long macxx_edm_default = ED_AMA | ED_M68;

/* default list mask */
unsigned long macxx_lm_default = ~(LIST_MES | LIST_LD | LIST_COD);



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



#if 0
static struct
{
	char *name;
	AModes am_num;
}
forced_am[] =
{
	{"I",      I_NUM
	},
	{
		"D",      D_NUM
	},
	{
		"X",      X_NUM
	},
	{
		"E",      E_NUM
	},
	{
		"S",      S_NUM
	},
	{
		"SPC",  SPC_NUM
	},
	{
		"SPD",  SPD_NUM
	},
	{
		"OPC10",  OPC10_NUM
	},
	{
		"OPC11",  OPC11_NUM
	},
	{
		"SPL",  SPL_NUM
	},
	{
		"ACC",  ACC_NUM
	},
	{
		"IMP",  IMP_NUM
	},
	{
		0,0
	}
};
#endif




typedef struct
{
	int flag;            /* .ne. if value is expression */
	unsigned long value;     /* value if not expression */
	EXP_stk *exptr;      /* ptr to expression stack */
} DPage;



DPage **dpage;
static int dpage_size,dpage_stack;
/*
static EXP_stk *tmp_expr;
static int tmp_expr_size;
*/


int ust_init(void)
{
/*
	if (image_name != 0)
	{
		char *s;
		s = strchr(image_name->name_only,'8');
	}
*/
	dpage_size = 16;
	dpage = (DPage **)MEM_alloc(dpage_size * sizeof(DPage *));

	return 0;            /* would fill a user symbol table */
}



/*  This dpage function is most likely not correct but it works */

static int check_4_dpage(EXP_stk *estk)
{
	DPage *dp;
/*    EXP_stk *testk,*dexp; */
	EXPR_struct *expr; /*,*texpr,*tp; */
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
	EXP1.tag = (opc->op_amode & SPL) ? 'Y' : 'z'; /* set branch type operand */

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
	long amflag = 0;

	int z = 0;
	int j = 0;
	long k = 0;
	int temp = 0;

	int indexed_mode   = 0x00;  /* holds current mode flags */

	int flg_1decr      = 0x001; /* one minus sign */
	int flg_2decr      = 0x002; /* two minus signs */
	int flg_5bit       = 0x004; /* 5 bit offset */
	int flg_8bit       = 0x008; /* 8 bit offset */
	int flg_indirect   = 0x010;
	int flg_offset     = 0x020;
	int flg_PC         = 0x040; /* offset from Program Counter */
	int flg_ABD        = 0x080;
	int flg_noffset    = 0x100;
	int flg_neg        = 0x0;

	int ireg_X = 0x0;
	int ireg_Y = 0x1;
	int ireg_U = 0x2;
	int ireg_S = 0x3;
/*    int ireg_PC=0x4; */

	int reg_CCR = 0xA;    /* condition code */
	int reg_A = 0x8;  /* A */
	int reg_B = 0x9;  /* one minus sign */
	int reg_DPR = 0xB;    /* direct page */
	int reg_X = 0x1;  /* X */
	int reg_Y = 0x2;  /* Y */
	int reg_S = 0x4;  /* hardware stack */
	int reg_U = 0x3;  /* user stack */
	int reg_PC = 0x5; /* Program Counter */
	int reg_D = 0x0;  /* D */


	ct = get_token();            /* pickup the next token */
	switch (ct)
	{
	case EOL:
		{
			return -1;         /* no operand supplied */
		}

	case TOKEN_oper:
		{

			if ( token_value == '-' )
			{
				flg_neg = 1;

				if ( (token_value == '-') && (*inp_ptr == open_operand) && (*(inp_ptr + 2) == close_operand) )
				{
					indexed_mode++;      /* Set one minus sign */
					flg_neg = 0;
					inp_ptr++;       /* eat the open_operand */
				}
				else if ( (token_value == '-') && (*inp_ptr == '-') && (*(inp_ptr + 1) == open_operand) && (*(inp_ptr + 3) == close_operand) )
				{
					indexed_mode++;
					indexed_mode++;      /* Set two minus sign */
					flg_neg = 0;
					inp_ptr++;   /* eat the 2nd '-' */
					inp_ptr++;    /* eat the open_operand */
				}
				else
				{
					inp_ptr--;       /* backup to the minus sign */
				}

			}
			else
			{
				inp_ptr--;       /* backup to the minus sign */
			}

		}

	case TOKEN_pcx:
		{

			if ( token_value == '#' )
			{
				get_token();        /* get the next token */
				if ( exprs(1, &EXP1) < 1 )
					break; /* quit if expr nfg */

				/* If it is a 2 BYTE immediate mode address */
				if ( (opc->op_amode & SPC) == SPC )
				{
					EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
				}

				return I_NUM;       /* return with immediate mode address */
			}
			if ( token_value == open_operand )
			{
				amflag = FIN;
			}
			else
			{
				if ( token_value == '@' )
				{
					indexed_mode = indexed_mode | flg_indirect;

					if ( *inp_ptr == open_operand )
					{
						amflag = FIN;
						inp_ptr++;   /* eat the open_operand */
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
							indexed_mode++; /* Set one minus sign */
							flg_neg = 0;
							inp_ptr++;   /* eat the minus sign */
							inp_ptr++;   /* eat the open_operand */
						}
						else if ( (*inp_ptr == '-') && (*(inp_ptr + 1) == '-') && (*(inp_ptr + 2) == open_operand) && (*(inp_ptr + 4) == close_operand) )
						{
							indexed_mode++;
							indexed_mode++; /* Set two minus sign */
							flg_neg = 0;
							inp_ptr++;   /* eat the minus sign */
							inp_ptr++;   /* eat the 2nd '-' */
							inp_ptr++;   /* eat the open_operand */
						}

					}

				}

			}


			/*  -(r) or --(r) or @-(r) or @--(r)  */
			if ( ((indexed_mode & flg_1decr) || (indexed_mode & flg_2decr)) && (flg_neg == 0) )
			{
				if ( indexed_mode & flg_1decr )
				{
					if ( indexed_mode & flg_indirect )
					{
						temp = 0x93;  /* decrement by one (0x92) not allowed in indirect mode - so dec. by two (0x93)*/
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
					temp = temp | (ireg_X << 5);
				}
				else if ( j == 'Y' )
				{
					temp = temp | (ireg_Y << 5);
				}
				else if ( j == 'U' )
				{
					temp = temp | (ireg_U << 5);
				}
				else if ( j == 'S' )
				{
					temp = temp | (ireg_S << 5);
				}

				++inp_ptr;         /* eat the reg */
				++inp_ptr;         /* eat the close_operand */

				EXP0SP->expr_value = opc->op_value + 0x20;

				EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set operand value */
				EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
				EXP1.tag = 'b';
				EXP1.ptr = 1;            /* second stack has operand (1 element) */

				return I_NUM;

			}

			get_token();       /* pickup the next token */

		}             /* fall through to rest */
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
				/* PSHS, PSHU, PULS and PULU opcodes */
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
							temp = temp | 1;
						}
						if ( strcmp(token_pool, "A") == 0 )
						{
							temp = temp | (1 << 1);
						}
						if ( strcmp(token_pool, "B") == 0 )
						{
							temp = temp | (1 << 2);
						}
						if ( strcmp(token_pool, "D") == 0 )
						{
							temp = temp | (1 << 1);
							temp = temp | (1 << 2);
						}
						if ( strcmp(token_pool, "DPR") == 0 )
						{
							temp = temp | (1 << 3);
						}
						if ( strcmp(token_pool, "X") == 0 )
						{
							temp = temp | (1 << 4);
						}
						if ( strcmp(token_pool, "Y") == 0 )
						{
							temp = temp | (1 << 5);
						}
						if ( strcmp(token_pool, "S") == 0 )
						{
							temp = temp | (1 << 6);
						}
						if ( strcmp(token_pool, "U") == 0 )
						{
							temp = temp | (1 << 6);
						}
						if ( strcmp(token_pool, "PC") == 0 )
						{
							temp = temp | (1 << 7);
						}

						if ( *inp_ptr == ',' )
						{
							inp_ptr++;
							get_token();       /* pickup the next token */
						}
						else
						{
							break;
						}

					}
					EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set operand value */
					EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
					EXP1.tag = 'b';
					EXP1.ptr = 1;            /* second stack has operand (1 element) */

					return I_NUM;
				}

			}

			if ( (token_type == TOKEN_numb) && (*inp_ptr == open_operand) )
			{             /* 0(r)+ or 0(r)++ */
				if ( (token_value == 0) && (*inp_ptr == open_operand) && (*(inp_ptr + 2) == close_operand) && (*(inp_ptr + 3) == '+') )
				{
					amflag = FIN;
					inp_ptr++;   /* eat the open_operand */
					get_token();       /* pickup the next token */
				}

			}

			if ( amflag == FIN )
			{    /* (r) or (r)+ or (r)++ or @(r) or @(r)+ or @(r)++ */

				/* Convert to Upper Case */
				j = 0;
				while ( token_pool[j] )
				{
					token_pool[j] = toupper(token_pool[j]);
					++j;
				}
				if ( *inp_ptr == close_operand )
				{
					++inp_ptr;         /* eat the close_operand */
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
						++inp_ptr;         /* eat the 1st '+' */
						++inp_ptr;         /* eat the 2nd '+' */
					}
					else if ( *inp_ptr == '+' )
					{
						if ( indexed_mode & flg_indirect )
						{
							temp = 0x91;  /* increment by one (0x90) not allowed in indirect mode - so inc. by two (0x91)*/
						}
						else
						{
							temp = 0x80;
						}
						++inp_ptr;         /* eat the '+' */
					}
					else
					{
						if ( indexed_mode & flg_indirect ) /* No offset */
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

					EXP0SP->expr_value = opc->op_value + 0x20;

					EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set operand value */
					EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
					EXP1.tag = 'b';
					EXP1.ptr = 1;            /* second stack has operand (1 element) */

					return I_NUM;

				}

/* give error here */

			}

			if ( (((token_type == TOKEN_strng) || (token_type == TOKEN_numb)) && ((*inp_ptr == open_operand) || (j) || (amflag == OPENAT))) || ((token_type == TOKEN_oper) && (j)) )
			{  /* (r) or n(r) or @(r) or @n(r) or @nnnn */

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
					{
						if ( exprs(1, &EXP2) < 1 )
							return -1;   /* test operand */
						k = EXP2SP->expr_value;

						if ( (EXP2SP->expr_code != EXPR_VALUE) || (EXP2.ptr > 1) )
						{
							z = 1;
						}
						else
						{
							z = 0;
							/* -17 >< 16 */
							if ( ((k > -17) && (k < 16)) && (!(indexed_mode & flg_indirect)) )
							{
								indexed_mode = indexed_mode | flg_5bit;   /* Really four bit with negative flag */
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x98;
									indexed_mode = (indexed_mode & ~flg_5bit) | flg_8bit;  /* Force 8 bits for @n(r) */
								}
								else
								{
									temp = 0x0;
								}
							} /* -129 >< 128 */
							else if ( (k > -129) && (k < 128) )
							{
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
							else
							{
								indexed_mode = indexed_mode | flg_offset;
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x99;
								}
								else
								{
									temp = 0x89;
								}
							}

							/* If zero offset - set to NO offset mode */
							if ( (k == 0) && ((indexed_mode & flg_5bit) != 0) )
							{
								temp = 0x84;
								indexed_mode = (indexed_mode & ~flg_5bit) | flg_noffset;  /* Force no offset for 0(r) */

							}

						}

						if ( z )
						{
							z = 1;
							for ( j = 0; j < EXP2.ptr; j++ )
							{
								if ( (EXP2SP[j].expr_code) == 0x20 )
								{
									z = z & (EXP2SP[j].expr_sym->flg_base);
								}
							}
							z = z & (EXP2.base_page_reference);
							if ( z )
							{
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
							else
							{
								indexed_mode = indexed_mode | flg_offset;   /* Really fifteen bit with negative flag */
								if ( indexed_mode & flg_indirect )
								{
									temp = 0x99;
								}
								else
								{
									temp = 0x89;
								}
							}

						}

					}

					if ( *inp_ptr == open_operand )
					{
						++inp_ptr;         /* eat the open_operand */
						get_token();       /* pickup the next token */
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
						++inp_ptr;         /* eat the close_operand */
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
						else if ( indexed_mode & flg_offset )
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
						f1_eatit();       /* eat rest of line */
						return -1;
					}

					EXP0SP->expr_value = opc->op_value + 0x20;
					if ( indexed_mode & flg_ABD )
					{
						if ( strcmp(token_pool, "PC") == 0 )
						{
							sprintf(emsg, "Not A Valid Register for accumulator offset - %s", token_pool);
							bad_token((char *)0, emsg);
							f1_eatit();       /* eat rest of line */
							return -1;
						}
						else
						{

							EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set Post Byte value */
							EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
							EXP1.tag = 'b';
							EXP1.ptr = 1;            /* second stack has operand (1 element) */
						}
						return I_NUM;
					}
					else
					{
						if ( (EXP2SP->expr_code == EXPR_VALUE) && (EXP2.ptr < 2) )
						{

							if ( indexed_mode & flg_noffset )
							{
								EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set operand no offset value */
								EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;            /* second stack has operand (1 element) */
								EXP2.ptr = 0;            /* second stack has operand (1 element) */
							}
							else if ( indexed_mode & flg_5bit )
							{
								EXP1.psuedo_value = EXP1SP->expr_value = temp | (k & 0x1f);  /* set operand 5bit value */
								EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;            /* second stack has operand (1 element) */
								EXP2.ptr = 0;            /* second stack has operand (1 element) */
							}
							else if ( indexed_mode & flg_8bit )
							{
								EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set Post Byte value */
								EXP2.psuedo_value = EXP2SP->expr_value = (k & 0xFF);  /* set operand 8bit value */
								EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								EXP2SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
								EXP1.tag = 'b';
								EXP1.ptr = 1;            /* second stack has operand (1 element) */
								EXP2.tag = 'b';
								/*               EXP2.ptr = 1;             * third stack has operand (1 element) */
							}
							else   /* 16 bit mode */
							{
								EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set Post Byte value */
								EXP2.psuedo_value = EXP2SP->expr_value = (k & 0xFFFF);  /* set operand 16bit value */
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

			}

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
						temp = reg_CCR << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "A") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_A << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "B") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_B << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "DPR") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_DPR << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "X") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_X << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "Y") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_Y << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "S") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_S << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "U") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_U << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "PC") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_PC << 4;
						j = 1;
					}
					else if ( (strcmp(token_pool, "D") == 0) && (*inp_ptr == ',') )
					{
						temp = reg_D << 4;
						j = 1;
					}

					if ( j )
					{
						++inp_ptr;       /* eat the comma */
						get_token();     /* pickup the next token */

						/* Convert to Upper Case */
						j = 0;
						while ( token_pool[j] )
						{
							token_pool[j] = toupper(token_pool[j]);
							++j;
						}

						if ( strcmp(token_pool, "CCR") == 0 )
						{
							temp = temp | reg_CCR;
						}
						else if ( strcmp(token_pool, "A") == 0 )
						{
							temp = temp | reg_A;
						}
						else if ( strcmp(token_pool, "B") == 0 )
						{
							temp = temp | reg_B;
						}
						else if ( strcmp(token_pool, "DPR") == 0 )
						{
							temp = temp | reg_DPR;
						}
						else if ( strcmp(token_pool, "X") == 0 )
						{
							temp = temp | reg_X;
						}
						else if ( strcmp(token_pool, "Y") == 0 )
						{
							temp = temp | reg_Y;
						}
						else if ( strcmp(token_pool, "S") == 0 )
						{
							temp = temp | reg_S;
						}
						else if ( strcmp(token_pool, "U") == 0 )
						{
							temp = temp | reg_U;
						}
						else if ( strcmp(token_pool, "PC") == 0 )
						{
							temp = temp | reg_PC;
						}
						else if ( strcmp(token_pool, "D") == 0 )
						{
							temp = temp | reg_D;
						}

						EXP1.psuedo_value = EXP1SP->expr_value = temp;  /* set operand value */
						EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
						EXP1.tag = 'b';
						EXP1.ptr = 1;            /* second stack has operand (1 element) */

						/* need error checking - make sure not putting a 16 bit reg into a 8 reg, etc */

						return I_NUM;

					}

				}

				if ( strcmp(token_pool, "I") == 0 )
				{
					++inp_ptr;       /* eat the comma */
					get_token();        /* get the next token */
					if ( exprs(1, &EXP1) < 1 )
						break; /* quit if expr nfg */

					/* If it is a 2 BYTE immediate mode address */
					if ( (opc->op_amode & SPC) == SPC )
					{
						EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';
					}

					return I_NUM;       /* return with immediate mode address */

				}
				else if ( strcmp(token_pool, "D") == 0 )
				{
					amdcdnum = D;
					EXP0SP->expr_value = opc->op_value + 0x10;

				}
				else if ( strcmp(token_pool, "E") == 0 )
				{
					amdcdnum = E;
					EXP0SP->expr_value = opc->op_value + 0x30;
					EXP1.tag = (edmask & ED_M68) ? 'W' : 'w';

				}


				/* mode "NE"   */
				else if ( strcmp(token_pool, "NE") == 0 )
				{
					EXP0SP->expr_value = opc->op_value + 0x20;
					++inp_ptr;       /* eat the comma */
					get_token();        /* get the next token */
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
				EXPR_struct *exp_ptr;
				
				/* if no address mode specified
				   We know we need an operand, so try to figure out if it's
				   Direct (Absolute Zero Page) or Extended (Absolute)  */

				if ( exprs(1, &EXP1) < 1 )
					break;   /* test operand */

				exp_ptr = EXP1.stack;

				if ( (opc->op_amode & D) &&         /* if Direct (Absolute Zero Page) mode is legal */
					 (((edmask & ED_AMA) == 0) ||   /* and not AMA mode */
					  (EXP1.base_page_reference != 0) || /* base page expression */
					  ((EXP1.ptr == 1) &&        /* or expression is < 256 */
					   (exp_ptr->expr_code == EXPR_VALUE) &&
					   ((exp_ptr->expr_value & -256) == 0) &&
					   (EXP1.forward_reference == 0))) )
				{
					/* If no AM option - Default  Direct (Absolute Zero Page) */
					if ( ((opc->op_amode) == (SHFTAM)) || (opc->op_amode & SPDP) )
					{
						EXP0SP->expr_value = opc->op_value - 0x40;
					}
					else
					{
						EXP0SP->expr_value = opc->op_value + 0x10;
					}

				}
				else
				{
					if ( opc->op_amode & D )
					{        /* If no AM option - Check for dpage  */

						if ( check_4_dpage(&EXP1) )
						{
							if ( opc->op_amode & SPDP )
							{
								EXP0SP->expr_value = opc->op_value - 0x40;
							}
							else
							{
								EXP0SP->expr_value = opc->op_value + 0x10;
							}

							EXP1.psuedo_value = EXP1SP->expr_value = (EXP1SP->expr_value & 0xFF);  /* set operand value */
							EXP1SP->expr_code = EXPR_VALUE;  /* set expression component is an absolute value */
							EXP1.tag = 'b';
							EXP1.ptr = 1;            /* second stack has operand (1 element) */

						}
						else if ( opc->op_amode & E )
						{        /* If no AM option - Default  Extended (Absolute) */
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
/*    int i,ct;*/

/*    AModes amdcdnum=UNDEF_NUM,forced_am_num; */

	int err_cnt = error_count[MSG_ERROR];
/*    EXPR_struct *exp_ptr; */
	EXP0.tag = 'b';          /* opcode is default 8 bit byte */
	EXP0.tag_len = 1;            /* only 1 byte */
	EXP1.tag = 'b';          /* assume operand is byte */
	EXP1.tag_len = 1;            /* and only 1 entry long */
	EXP0SP->expr_code = EXPR_VALUE;
	/* set the opcode expression *//* expression component is an absolute value */
	EXP0SP->expr_value = opc->op_value;
	EXP0.ptr = 1;            /* first stack has opcode (1 element) */
	EXP1.ptr = 0;            /* assume no operands */
	EXP2.ptr = 0;            /* assume no operands */
	EXP3.ptr = 0;            /* assume no operands */
/*    exp_ptr = EXP1SP; */
	am_ptr = inp_ptr;            /* remember where am starts */
/*    forced_am_num = UNDEF_NUM; */

	if ( (opc->op_amode & SP10) == SP10 )
	{    /*  Set two byte opcode  */
		opc->op_value = opc->op_value | 0x1000;
		EXP0.psuedo_value = EXP0SP->expr_value = opc->op_value;
		EXP0.tag = (edmask & ED_M68) ? 'W' : 'w';
	}

	if ( (opc->op_amode & SP11) == SP11 )
	{    /*  Set two byte opcode  */
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
			/* amdcdnum = */ do_operand(opc);
		}

	}

	if ( err_cnt != error_count[MSG_ERROR] )
	{   /* if any errors detected... */
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

}   /* -- opc69() */



int op_ntype(void)
{
	f1_eatit();
	return 1;
}



#include "opcommon.h"


