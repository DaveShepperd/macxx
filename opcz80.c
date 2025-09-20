/*
	opc8080.c - Part of macxx, a cross assembler family for various micro-processors
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

	04/10/2025	- 8080 CPU support added by Dave Shepperd

******************************************************************************/


#if !defined(MAC_Z80)
	#define MAC_Z80 1
#endif
#include <strings.h>
#include "token.h"
#include "pst_tokens.h"
#include "psttknz80.h"
#include "exproper.h"
#include "listctrl.h"
#include "add_defs.h"
#include "memmgt.h"

#define DEFNAM(name,numb) {"name",name,numb},

/* The following are variables specific to the particular assembler */

const int macxx_name_mask = MACXX_M_Z80;
const char macxx_name[] = "macz80";
const char *macxx_target = "z80";
const char *macxx_descrip = "Cross assembler for the z80.";

uint16_t macxx_salign = 0;    /* default alignment segments by LLF */
uint16_t macxx_dalign = 0;    /* default alignment data within segment */
uint16_t macxx_min_dalign = 0;

char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;    /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

uint32_t macxx_edm_default = ED_TRUNC | ED_DOL_PC | ED_H_HEX | ED_O_OCT | ED_Q_OCT | ED_ALTEXP | ED_PRECED | ED_Q_MARG;  /* default edmask */
uint32_t macxx_lm_default = ~(LIST_ME | LIST_MEB | LIST_MES | LIST_LD | LIST_COD | LIST_OCT);  /* default list mask */

int current_radix = 10;     /* default the radix to decimal */
char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
/* char expr_escape = '^'; */     /* char that escapes a unary expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */

int max_opcode_length = 16; /* significant length of opcodes */
int max_symbol_length = 16; /* significant length of symbols */

extern int dotwcontext;
/* extern int no_white_space_allowed; */
static char *am_ptr;

static const char SourceOnlyAReg[] = "Source can only be the A register";
static const char DestOnlyAReg[] = "Destination can only be the A register";
static const char InvalidSource[] = "Invalid source";
static const char InvalidDestination[] = "Invalid destination";
static const char InvalidOperand[] = "InvalidOperand";

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
	Opcode *opc;
	int error;
	int outOrder;
	EXP_stk *exp0;
	EXPR_struct *expsp0;
	AModes dstMode;
	AModes srcMode;
	EXP_stk *dstStk;
	EXPR_struct *dstSp0;
	EXP_stk *srcStk;
	EXPR_struct *srcSp0;
	EXP_stk *extStk;
	EXPR_struct *extSp0;
} OpcArgs_t;

static const char Internal[] = "INTERNAL: ";

static void badOperand(const char *title1, const char *title2, OpcArgs_t *args)
{
	char errMsg[132];
	snprintf(errMsg, sizeof(errMsg), "%s%s, srcMode=%d, dstMode=%d", title1 ? title1 : "", title2 ? title2 : "", args->srcMode, args->dstMode);
	bad_token(NULL,errMsg);
	args->error = 1;
}

static AModes do_operand(Opcode *opc, EXP_stk *exp)
{
	int ct, openParen=0;
	AModes amode=ILL_AM;
	EXPR_struct *expsp = exp->stack;
	
	exp->tag = 'b';          /* assume operand is unsigned byte */
	exp->tag_len = 1;        /* and only 1 entry long */
	exp->ptr = 0;            /* assume no operands */
	expsp->expr_value = 0;
	ct = get_token();		/* pickup the next token */
	if ( ct == EOL )
		return ILL_AM;
	am_ptr = tkn_ptr;
	if ( *tkn_ptr == '(' )
	{
		char x,y,z,*cPtr = inp_ptr;
		openParen = 1;
		while (myIsspace(*cPtr))
			++cPtr; /* skip over white space */
		x = *cPtr++;
		y = *cPtr++;
		while (myIsspace(*cPtr))
			++cPtr; /* skip over white space */
		z = *cPtr;
		if ( z == '+' || z == '-' || z == ')')
		{
			x = toupper(x);
			if ( x == 'I' )
			{
				y = toupper(y);
				if ( y == 'X' )
					amode = AM_IIX;
				else if ( y == 'Y' )
					amode = AM_IIY;
			}
			if ( amode != ILL_AM )
			{
				if ( z == ')' )
				{
					exp->ptr = 1;
					exp->stack[0].expr_code = EXPR_VALUE;
					exp->stack[0].expr_value = 0;
					inp_ptr = cPtr+1;
					return amode;
				}
				inp_ptr = cPtr;
				get_token();
			}
			exp->tag = 's';          /* assume operand is signed byte */
		}
		else if ( toupper(x) == 'C' && y == ')' )
		{
			amode = AM_IC;
			exp->ptr = 1;
			exp->stack[0].expr_code = EXPR_VALUE;
			exp->stack[0].expr_value = 0;
			inp_ptr = cPtr;
			return amode;
		}
	}
	if ( exprs(1, exp) < 1 )
		return ILL_AM;
	if ( amode == AM_IIX || amode == AM_IIY )
	{
		if ( *inp_ptr != ')' )
		{
			if ( options[QUAL_VERBOSE] )
				bad_token(inp_ptr,"Expected a ')' here");
			f1_eatit();
			return ILL_AM;
		}
		++inp_ptr;	/* Eat the trailing ')' */
		return amode;
	}
	if ( openParen )
	{
		char errMsg[132];
		if ( !exp->register_reference )
			return AM_Inn;
		if ( exp->ptr == 1 && expsp->expr_code == EXPR_VALUE )
		{
			if ( expsp->expr_value == REG_HL )
				return AM_IHL;
			if ( expsp->expr_value == REG_BC )
				return AM_IBC;
			if ( expsp->expr_value == REG_DE )
				return AM_IDE;
			if ( expsp->expr_value == REG_SP )
				return AM_ISP;
		}
		if ( options[QUAL_VERBOSE] )
		{
			snprintf(errMsg, sizeof(errMsg), "do_operand(): Cannot determine mode. token='%s', register=1, value=%d", token_pool, expsp->expr_value);
			bad_token(am_ptr,errMsg);
		}
		return ILL_AM;
	}
	if ( exp->register_reference )
	{
		if ( exp->symIsPrime )
		{
			switch (expsp->expr_value)
			{
			case REG_AF:
				return AM_AFP;
			case REG_BC:
				return AM_BCP;
			case REG_DE:
				return AM_DEP;
			case REG_HL:
				return AM_HLP;
			default:
				break;
			}
		}
		if ( expsp->expr_value >= REG_B && expsp->expr_value <= REG_IY )
			return AM_B + expsp->expr_value;
		return ILL_AM;
	}
	return AM_nn;
}

static void ld_DstIs8BitReg( OpcArgs_t *args )
{
	static const char OurName[] = "ld_DstIs8BitReg(): ";
	
	switch (args->srcMode)
	{
	case AM_A:
	case AM_B:
	case AM_C:
	case AM_D:
	case AM_E:
	case AM_H:
	case AM_L:
		args->expsp0->expr_value = 0x40 | ((args->dstMode) << 3) | args->srcMode;
		args->dstStk->ptr = 0;
		args->srcStk->ptr = 0;
		break;
	case AM_IHL:
		args->expsp0->expr_value = 0x40 | (args->dstMode << 3) | REG_M;
		args->dstStk->ptr = 0;
		args->srcStk->ptr = 0;
		break;
	case AM_IBC:
		if ( args->dstMode != AM_A )
			badOperand(NULL,DestOnlyAReg, args);
		else
		{
			args->expsp0->expr_value = 0x0A;
			args->dstStk->ptr = 0;
			args->srcStk->ptr = 0;
		}
		break;
	case AM_IDE:
		if ( args->dstMode != AM_A )
			badOperand(NULL,DestOnlyAReg, args);
		else
		{
			args->expsp0->expr_value = 0x1A;
			args->dstStk->ptr = 0;
			args->srcStk->ptr = 0;
		}
		break;
	case AM_R:
		if ( args->dstMode != AM_A )
			badOperand(NULL,DestOnlyAReg, args);
		else
		{
			args->expsp0->expr_value = 0x5FED;
			args->dstStk->ptr = 0;
			args->srcStk->ptr = 0;
		}
		break;
	case AM_I:
		if ( args->dstMode != AM_A )
			badOperand(NULL,DestOnlyAReg, args);
		else
		{
			args->expsp0->expr_value = 0x57ED;
			args->dstStk->ptr = 0;
			args->srcStk->ptr = 0;
		}
		break;
	case AM_nn:		/* operand immediate */
		args->expsp0->expr_value = 0x06 | ((args->dstMode) << 3);
		args->dstStk->ptr = 0;
		break;
	case AM_Inn:	/* 16 bit operand indirect */
		if ( args->dstMode != AM_A )
			badOperand(NULL,DestOnlyAReg, args);
		else
		{
			args->expsp0->expr_value = 0x3A;
			args->dstStk->ptr = 0;
			args->srcStk->tag = 'w';
		}
		break;
	case AM_IIX:
		if ( args->dstMode > AM_A )
			badOperand(OurName, InvalidSource, args);
		else
		{
			args->expsp0->expr_value = 0x46DD | (args->dstMode << (3+8));
			args->dstStk->ptr = 0;
		}
		break;
	case AM_IIY:
		if ( args->dstMode > AM_A )
			badOperand(OurName, InvalidSource, args);
		else
		{
			args->expsp0->expr_value = 0x46FD | (args->dstMode << (3+8));
			args->dstStk->ptr = 0;
		}
		break;
	case AM_BC:
	case AM_DE:
	case AM_HL:
	case AM_SP:
	case AM_AF:
	case AM_IX:
	case AM_IY:
		badOperand(OurName, InvalidSource, args);
		break;
	default:
		badOperand(Internal,"ld_DstIs8bitReg(): Unhandled source operand", args);
		break;
	}
	if ( !args->error )
	{
		if ( args->expsp0->expr_value > 255 )
			args->exp0->tag = 'w';
		args->exp0->psuedo_value = args->expsp0->expr_value;
	}
}

static void handle_ld(OpcArgs_t *args)
{
	static const char OurName[] = "handle_ld(): ";
	AModes tmp = ILL_AM;

	args->srcMode = ILL_AM;
	if ( (args->dstMode=do_operand( args->opc, args->dstStk)) != ILL_AM )
	{
		comma_expected = 1;
		args->srcMode = do_operand(args->opc, args->srcStk);
	}
	if ( args->srcMode >= AM_B && args->srcMode <= AM_A )
	{
		if ( args->dstMode == AM_HL )
			tmp = AM_IHL;
		else if ( args->dstMode == AM_BC )
			tmp = AM_IBC;
		else if ( args->dstMode == AM_DE )
			tmp = AM_IDE;
		if ( tmp != ILL_AM )
		{
			show_bad_token(NULL, "Did you forget ()'s on destination?", MSG_WARN);
			args->dstMode = tmp;
			tmp = ILL_AM;
		}
	}
	if ( args->dstMode >= AM_B && args->dstMode <= AM_A )
	{
		if ( args->srcMode == AM_HL )
			tmp = AM_IHL;
		else if ( args->srcMode == AM_BC )
			tmp = AM_IBC;
		else if ( args->srcMode == AM_DE )
			tmp = AM_IDE;
		if ( tmp != ILL_AM )
		{
			show_bad_token(NULL, "Did you forget ()'s on source?", MSG_WARN);
			args->srcMode = tmp;
			tmp = ILL_AM;
		}
	}
	switch (args->dstMode)
	{
	case AM_B:
	case AM_C:
	case AM_D:
	case AM_E:
	case AM_H:
	case AM_L:
	case AM_A:
		args->dstStk->ptr = 0;
		ld_DstIs8BitReg(args);	/* LD r,src ; where r is B-A and src is any legit source */
		break;
	case AM_IBC:		/* LD (BC),A */
		args->dstStk->ptr = 0;
		if ( args->srcSp0->expr_value != REG_A )
		{
			badOperand(OurName, SourceOnlyAReg, args);
			break;
		}
		args->expsp0->expr_value = 0x02;
		args->srcStk->ptr = 0;
		break;
	case AM_IDE:		/* LD (DE),A */
		args->dstStk->ptr = 0;
		if ( args->srcSp0->expr_value != REG_A )
		{
			badOperand(OurName, SourceOnlyAReg, args);
			break;
		}
		args->expsp0->expr_value = 0x12;
		args->srcStk->ptr = 0;
		break;
	case AM_IHL:		/* LD (HL),src ; where src is B-A or n */
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_B:
		case AM_C:
		case AM_D:
		case AM_E:
		case AM_H:
		case AM_L:
		case AM_A:
			args->expsp0->expr_value = 0x40 | (REG_M << 3) | (args->srcSp0->expr_value & 7);
			args->srcStk->ptr = 0;
			break;
		case AM_nn:
			args->expsp0->expr_value = 0x36;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_R:		/* LD R,A */
		args->dstStk->ptr = 0;
		if ( args->srcSp0->expr_value != REG_A )
		{
			badOperand(OurName, SourceOnlyAReg, args);
			break;
		}
		args->expsp0->expr_value = 0x4FED;
		args->srcStk->ptr = 0;
		break;
	case AM_I:		/* LD I,A */
		args->dstStk->ptr = 0;
		if ( args->srcSp0->expr_value != REG_A )
		{
			badOperand(OurName, SourceOnlyAReg, args);
			break;
		}
		args->expsp0->expr_value = 0x47ED;
		args->srcStk->ptr = 0;
		break;
	case AM_AF:		/* LD AF,(SP) */
		args->dstStk->ptr = 0;
		if ( args->srcMode != AM_ISP )
		{
			badOperand(OurName, InvalidSource, args);
			break;
		}
		args->expsp0->expr_value = 0xF1;
		args->srcStk->ptr = 0;
		break;
	case AM_BC:		/* LD BC,src ; where src is (SP) or nn or (nn) */
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_nn:
			args->expsp0->expr_value = 0x01;
			args->srcStk->tag = 'w';
			break;
		case AM_Inn:
			args->expsp0->expr_value = 0x4BED;
			args->srcStk->tag = 'w';
			break;
		case AM_ISP:
			args->expsp0->expr_value = 0xC1;
			args->srcStk->ptr = 0;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_DE:
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_nn:
			args->expsp0->expr_value = 0x11;
			args->srcStk->tag = 'w';
			break;
		case AM_Inn:
			args->expsp0->expr_value = 0x5BED;
			args->srcStk->tag = 'w';
			break;
		case AM_ISP:
			args->expsp0->expr_value = 0xD1;
			args->srcStk->ptr = 0;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_HL:
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_nn:
			args->expsp0->expr_value = 0x21;
			args->srcStk->tag = 'w';
			break;
		case AM_Inn:
			args->expsp0->expr_value = 0x2A;
			args->srcStk->tag = 'w';
			break;
		case AM_ISP:
			args->expsp0->expr_value = 0xE1;
			args->srcStk->ptr = 0;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_SP:
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_HL:
			args->expsp0->expr_value = 0xF9;
			args->srcStk->ptr = 0;
			break;
		case AM_IX:
			args->expsp0->expr_value = 0xF9DD;
			args->srcStk->ptr = 0;
			break;
		case AM_IY:
			args->expsp0->expr_value = 0xF9FD;
			args->srcStk->ptr = 0;
			break;
		case AM_nn:
			args->expsp0->expr_value = 0x31;
			args->srcStk->tag = 'w';
			break;
		case AM_Inn:
			args->expsp0->expr_value = 0x7BED;
			args->srcStk->tag = 'w';
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_IX:
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_nn:
			args->expsp0->expr_value = 0x21DD;
			args->srcStk->tag = 'w';
			break;
		case AM_Inn:
			args->expsp0->expr_value = 0x2ADD;
			args->srcStk->tag = 'w';
			break;
		case AM_ISP:
			args->expsp0->expr_value = 0xE1DD;
			args->srcStk->ptr = 0;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_IY:
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_nn:
			args->expsp0->expr_value = 0x21FD;
			args->srcStk->tag = 'w';
			break;
		case AM_Inn:
			args->expsp0->expr_value = 0x2AFD;
			args->srcStk->tag = 'w';
			break;
		case AM_ISP:
			args->expsp0->expr_value = 0xE1FD;
			args->srcStk->ptr = 0;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_Inn:
		args->srcStk->ptr = 0;
		args->dstStk->tag = 'w';
		switch (args->srcMode)
		{
		case AM_A:
			args->expsp0->expr_value = 0x32;
			args->exp0->tag = 'b';
			break;
		case AM_BC:
			args->expsp0->expr_value = 0x43ED;
			break;
		case AM_DE:
			args->expsp0->expr_value = 0x53ED;
			break;
		case AM_HL:
			args->expsp0->expr_value = 0x22;
			break;
		case AM_SP:
			args->expsp0->expr_value = 0x73ED;
			break;
		case AM_IX:
			args->expsp0->expr_value = 0x22DD;
			break;
		case AM_IY:
			args->expsp0->expr_value = 0x22FD;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_ISP:
		args->dstStk->ptr = 0;
		args->srcStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_AF:
			args->expsp0->expr_value = 0xF6;
			break;
		case AM_BC:
			args->expsp0->expr_value = 0xC6;
			break;
		case AM_DE:
			args->expsp0->expr_value = 0xD6;
			break;
		case AM_HL:
			args->expsp0->expr_value = 0xE6;
			break;
		case AM_IX:
			args->expsp0->expr_value = 0xE6DD;
			break;
		case AM_IY:
			args->expsp0->expr_value = 0xE6FD;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_IIX:
		switch (args->srcMode)
		{
		case AM_A:
		case AM_B:
		case AM_C:
		case AM_D:
		case AM_E:
		case AM_H:
		case AM_L:
			args->expsp0->expr_value = 0x70DD|((args->srcMode&7)<<8);
			args->srcStk->ptr = 0;
			break;
		case AM_nn:
			args->expsp0->expr_value = 0x36DD;
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	case AM_IIY:
		switch (args->srcMode)
		{
		case AM_A:
		case AM_B:
		case AM_C:
		case AM_D:
		case AM_E:
		case AM_H:
		case AM_L:
			args->expsp0->expr_value = 0x70FD|((args->srcMode&7)<<8);
			args->srcStk->ptr = 0;
			break;
		case AM_nn:
			args->expsp0->expr_value = 0x36FD;
			args->exp0->tag = 'w';
			break;
		default:
			badOperand(OurName, InvalidSource, args);
			break;
		}
		break;
	default:
		badOperand(OurName, InvalidDestination, args);
		break;
	}
	return;
}

static void handle_pushPop(OpcArgs_t *args, int id)
{
	static const char HandlePush[] = "handle_push(): ";

	args->srcMode = ILL_AM;
	if ( (args->srcMode=do_operand( args->opc, args->srcStk)) == ILL_AM )
	{
		badOperand(HandlePush,InvalidOperand,args);
		return;
	}
	switch (args->srcMode)
	{
	case AM_AF:
		args->expsp0->expr_value = 0xF1 | id;
		break;
	case AM_BC:
		args->expsp0->expr_value = 0xC1 | id;
		break;
	case AM_DE:
		args->expsp0->expr_value = 0xD1 | id;
		break;
	case AM_HL:
		args->expsp0->expr_value = 0xE1 | id;
		break;
	case AM_IX:
		args->expsp0->expr_value = 0xE1DD | (id << 8);
		break;
	case AM_IY:
		args->expsp0->expr_value = 0xE1FD | (id << 8);
		break;
	default:
		badOperand(HandlePush,InvalidOperand,args);
		break;
	}
	args->srcStk->ptr = 0;
}

static void handle_ex(OpcArgs_t *args)
{
	static const char HandleEx[] = "handle_ex(): ";

	args->srcMode = ILL_AM;
	args->dstMode = do_operand( args->opc, args->dstStk);
	comma_expected = 1;
	if ( (args->srcMode=do_operand( args->opc, args->srcStk)) == ILL_AM )
	{
		badOperand(HandleEx,InvalidSource,args);
		return;
	}
	switch (args->dstMode)
	{
	case AM_AFP:
		if ( args->srcMode != AM_AF )
		{
			badOperand(HandleEx,InvalidSource,args);
			break;
		}
		args->expsp0->expr_value = 0x08;
		break;
	case AM_AF:
		if ( args->srcMode != AM_AFP )
		{
			badOperand(HandleEx,InvalidSource,args);
			break;
		}
		args->expsp0->expr_value = 0x08;
		break;
	case AM_HL:
		if ( args->srcMode != AM_DE )
		{
			badOperand(HandleEx,InvalidSource,args);
			break;
		}
		args->expsp0->expr_value = 0xEB;
		break;
	case AM_DE:
		if ( args->srcMode != AM_HL )
		{
			badOperand(HandleEx,InvalidSource,args);
			break;
		}
		args->expsp0->expr_value = 0xEB;
		break;
	case AM_ISP:
		switch (args->srcMode)
		{
		case AM_HL:
			args->expsp0->expr_value = 0xE3;
			break;
		case AM_IX:
			args->expsp0->expr_value = 0xE3DD;
			break;
		case AM_IY:
			args->expsp0->expr_value = 0xE3FD;
			break;
		default:
			badOperand(HandleEx,InvalidSource,args);
			break;
		}
		break;
	default:
		badOperand(HandleEx,InvalidOperand,args);
		break;
	}
	args->srcStk->ptr = 0;
	args->dstStk->ptr = 0;
}

#define ADD_BASE 0x80
#define ADC_BASE 0x88
#define SBC_BASE 0x98
#define INC_BASE 0x04
#define DEC_BASE 0x05

static void handle_alu(OpcArgs_t *args, int regBase, int imm )
{
	static const char HandleAlu[] = "handle_alu(): ";
	int ct;
	char *inp = inp_ptr;

	ct = get_token();		/* pickup the next token */
	if ( ct == EOL )
	{
		badOperand(HandleAlu,InvalidOperand,args);
		return;
	}
	while ( myIsspace(*inp_ptr) )
		++inp_ptr;
	if ( *inp_ptr == ',' )
	{
		inp_ptr = inp;	/* reset for rescan */
		args->dstMode = do_operand( args->opc, args->dstStk);
		comma_expected = 1;
	}
	else
	{
		/* Default destination to accumulator */
		args->dstMode = AM_A;
		args->dstSp0->expr_code = EXPR_VALUE;
		args->dstSp0->expr_value = REG_A;
		inp_ptr = inp;	/* reset for rescan */
	}
	args->srcMode = do_operand( args->opc, args->srcStk );
	switch (args->dstMode)
	{
	case AM_A:
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_A:
		case AM_B:
		case AM_C:
		case AM_D:
		case AM_E:
		case AM_H:
		case AM_L:
			args->expsp0->expr_value = regBase + args->srcMode;
			args->srcStk->ptr = 0;
			break;
		case AM_IHL:
			args->expsp0->expr_value = regBase+0x06;
			args->srcStk->ptr = 0;
			break;
		case AM_IIX:
			args->expsp0->expr_value = ((regBase+0x06)<<8) | 0xDD;
			break;
		case AM_IIY:
			args->expsp0->expr_value = ((regBase+0x06)<<8) | 0xFD;
			break;
		case AM_nn:
			args->expsp0->expr_value = imm;
			break;
		default:
			badOperand(HandleAlu,InvalidSource,args);
			break;
		}
		break;
	case AM_HL:			/* Dest mode HL */
		args->srcStk->ptr = 0;
		args->dstStk->ptr = 0;
		switch (args->srcMode)
		{
		case AM_BC:		/* Dst mode HL, Src mode BC */
			if ( regBase == ADD_BASE )
				args->expsp0->expr_value = 0x09;
			else if ( regBase == ADC_BASE )
				args->expsp0->expr_value = 0x4AED;
			else if ( regBase == SBC_BASE )
				args->expsp0->expr_value = 0x42ED;
			else
				badOperand(HandleAlu,InvalidSource,args);
			break;
		case AM_DE:		/* Dst mode HL, Src mode DE */
			if ( regBase == ADD_BASE )
				args->expsp0->expr_value = 0x19;
			else if ( regBase == ADC_BASE )
				args->expsp0->expr_value = 0x5AED;
			else if ( regBase == SBC_BASE )
				args->expsp0->expr_value = 0x52ED;
			else
				badOperand(HandleAlu,InvalidSource,args);
			break;
		case AM_HL:		/* Dst mode HL, Src mode HL */
			if ( regBase == ADD_BASE )
				args->expsp0->expr_value = 0x29;
			else if ( regBase == ADC_BASE )
				args->expsp0->expr_value = 0x6AED;
			else if ( regBase == SBC_BASE )
				args->expsp0->expr_value = 0x62ED;
			else
				badOperand(HandleAlu,InvalidSource,args);
			break;
		case AM_SP:		/* Dst mode HL, Src mode SP */
			if ( regBase == ADD_BASE )
				args->expsp0->expr_value = 0x39;
			else if ( regBase == ADC_BASE )
				args->expsp0->expr_value = 0x7AED;
			else if ( regBase == SBC_BASE )
				args->expsp0->expr_value = 0x72ED;
			else
				badOperand(HandleAlu,InvalidSource,args);
			break;
		default:
			badOperand(HandleAlu,InvalidSource,args);
			break;
		}
		break;
	case AM_IX:		/* Dst mode IX */
		args->srcStk->ptr = 0;
		args->dstStk->ptr = 0;
		if ( regBase != ADD_BASE )
		{
			badOperand(HandleAlu,InvalidSource,args);
			break;
		}
		switch (args->srcMode)
		{
		case AM_BC:		/* Dst mode IX, Src mode BC */
			args->expsp0->expr_value = 0x09DD;
			break;
		case AM_DE:		/* Dst mode IX, Src mode DE */
			args->expsp0->expr_value = 0x19DD;
			break;
		case AM_SP:		/* Dst mode IX, Src mode SP */
			args->expsp0->expr_value = 0x39DD;
			break;
		case AM_IX:		/* Dst mode IX, Src mode IX */
			args->expsp0->expr_value = 0x29DD;
			break;
		default:
			badOperand(HandleAlu,InvalidSource,args);
			break;
		}
		break;
	case AM_IY:
		args->srcStk->ptr = 0;
		args->dstStk->ptr = 0;
		if ( regBase != ADD_BASE )
		{
			badOperand(HandleAlu,InvalidSource,args);
			break;
		}
		switch (args->srcMode)
		{
		case AM_BC:		/* Dst mode IY, Src mode BC */
			args->expsp0->expr_value = 0x09FD;
			break;
		case AM_DE:		/* Dst mode IY, Src mode DE */
			args->expsp0->expr_value = 0x19FD;
			break;
		case AM_SP:		/* Dst mode IY, Src mode SP */
			args->expsp0->expr_value = 0x39FD;
			break;
		case AM_IY:		/* Dst mode IY, Src mode IY */
			args->expsp0->expr_value = 0x29FD;
			break;
		default:
			badOperand(HandleAlu,InvalidSource,args);
			break;
		}
		break;
	default:
		badOperand(HandleAlu,InvalidDestination,args);
		break;
	}
}

static void handle_im(OpcArgs_t *args)
{
	static const char HandleIM[] = "handle_im(): ";

	args->dstMode = do_operand( args->opc, args->dstStk);
	if (    args->dstMode == AM_nn
		 && args->dstStk->ptr == 1
		 && args->dstSp0->expr_code == EXPR_VALUE
		 && args->dstSp0->expr_value >= 0
		 && args->dstSp0->expr_value <= 2
		)
	{
		static const unsigned short Opcodes[3] = { 0x46ED, 0x56ED, 0x5EED };
		args->expsp0->expr_value = Opcodes[args->dstSp0->expr_value];
		args->dstStk->ptr = 0;
	}
	else
	{
		badOperand(HandleIM,InvalidDestination,args);
	}
}

static void handle_incDec(OpcArgs_t *args, int regBase)
{
	static const char HandleInc[] = "handle_inc(): ";

	args->dstMode = do_operand( args->opc, args->dstStk);
	switch (args->dstMode)
	{
	case AM_A:
	case AM_B:
	case AM_C:
	case AM_D:
	case AM_E:
	case AM_H:
	case AM_L:
		args->expsp0->expr_value = regBase + (args->dstMode<<3);
		args->dstStk->ptr = 0;
		break;
	case AM_IHL:
		args->expsp0->expr_value = regBase + (0x06<<3);
		args->dstStk->ptr = 0;
		break;
	case AM_IIX:
		args->expsp0->expr_value = ((regBase+(0x06<<3))<<8) | 0xDD;
		break;
	case AM_IIY:
		args->expsp0->expr_value = ((regBase+(0x06<<3))<<8) | 0xFD;
		break;
	case AM_BC:
		args->expsp0->expr_value = (regBase == INC_BASE) ? 0x03 : 0x0B;
		args->dstStk->ptr = 0;
		break;
	case AM_DE:
		args->expsp0->expr_value = (regBase == INC_BASE) ? 0x13 : 0x1B;
		args->dstStk->ptr = 0;
		break;
	case AM_HL:
		args->expsp0->expr_value = (regBase == INC_BASE) ? 0x23 : 0x2B;
		args->dstStk->ptr = 0;
		break;
	case AM_SP:
		args->expsp0->expr_value = (regBase == INC_BASE) ? 0x33 : 0x3B;
		args->dstStk->ptr = 0;
		break;
	case AM_IX:
		args->expsp0->expr_value = (regBase == INC_BASE) ? 0x23DD : 0x2BDD;
		args->dstStk->ptr = 0;
		break;
	case AM_IY:
		args->expsp0->expr_value = (regBase == INC_BASE) ? 0x23FD : 0x2BFD;
		args->dstStk->ptr = 0;
		break;
	default:
		badOperand(HandleInc,InvalidDestination,args);
		break;
	}
}

static void handle_shifts(OpcArgs_t *args, int hdrBase, int regBase )
{
	static const char HandleShifts[] = "handle_shifts(): ";

	args->dstMode = do_operand( args->opc, args->dstStk);
	switch (args->dstMode)
	{
	case AM_A:
	case AM_B:
	case AM_C:
	case AM_D:
	case AM_E:
	case AM_H:
	case AM_L:
		args->exp0->tag = 'w';
		args->expsp0->expr_value = ((regBase + (args->dstMode))<<8) | hdrBase;
		args->dstStk->ptr = 0;
		break;
	case AM_IHL:
		args->exp0->tag = 'w';
		args->expsp0->expr_value = ((regBase+0x06)<<8) | hdrBase;
		args->dstStk->ptr = 0;
		break;
	case AM_IIX:
		args->expsp0->expr_value = 0xDD;
		args->srcSp0->expr_code = EXPR_VALUE;
		args->srcSp0->expr_value = hdrBase;
		args->srcStk->tag = 'b';
		args->srcStk->ptr = 1;
		args->outOrder = 1;
		args->extSp0->expr_code = EXPR_VALUE;
		args->extSp0->expr_value = regBase + 0x06;
		args->extStk->tag = 'b';
		args->extStk->ptr = 1;
		break;
	case AM_IIY:
		args->expsp0->expr_value = 0xFD;
		args->srcSp0->expr_code = EXPR_VALUE;
		args->srcSp0->expr_value = hdrBase;
		args->srcStk->tag = 'b';
		args->srcStk->ptr = 1;
		args->outOrder = 1;
		args->extSp0->expr_code = EXPR_VALUE;
		args->extSp0->expr_value = regBase + 0x06;
		args->extStk->tag = 'b';
		args->extStk->ptr = 1;
		break;
	default:
		badOperand(HandleShifts,InvalidDestination,args);
		break;
	}
}

static void handle_bits(OpcArgs_t *args, int hdrBase, int regBase )
{
	static const char HandleBits[] = "handle_bits(): ";
	int bit;
	
	args->dstMode = do_operand( args->opc, args->dstStk);
	bit = args->dstSp0->expr_value;
	comma_expected = 1;
	args->srcMode = do_operand( args->opc, args->srcStk );
	if (    args->dstMode != AM_nn
		 || args->dstStk->ptr != 1
		 || args->dstSp0->expr_code != EXPR_VALUE
		 || bit < 0
		 || bit > 7
	   )
	{
		badOperand(HandleBits,"Invalid bit number. Can only be absolute 0-7",args);
		return;
	}
	args->dstStk->ptr = 0;
	switch (args->srcMode)
	{
	case AM_A:
	case AM_B:
	case AM_C:
	case AM_D:
	case AM_E:
	case AM_H:
	case AM_L:
		args->expsp0->expr_value = ((regBase | args->srcMode | (bit<<3))<<8) | hdrBase;
		args->srcStk->ptr = 0;
		args->exp0->tag = 'w';
		break;
	case AM_IHL:
		args->expsp0->expr_value = ((regBase | 0x06 | (bit<<3))<<8) | hdrBase;
		args->srcStk->ptr = 0;
		args->exp0->tag = 'w';
		break;
	case AM_IIX:
		args->expsp0->expr_value = 0xDD;
		args->dstSp0->expr_code = EXPR_VALUE;
		args->dstSp0->expr_value = hdrBase;
		args->dstStk->tag = 'b';
		args->dstStk->ptr = 1;
		args->extSp0->expr_code = EXPR_VALUE;
		args->extSp0->expr_value = regBase | 0x06 | (bit<<3);
		args->extStk->tag = 'b';
		args->extStk->ptr = 1;
		break;
	case AM_IIY:
		args->expsp0->expr_value = 0xFD;
		args->dstSp0->expr_code = EXPR_VALUE;
		args->dstSp0->expr_value = hdrBase;
		args->dstStk->tag = 'b';
		args->dstStk->ptr = 1;
		args->extSp0->expr_code = EXPR_VALUE;
		args->extSp0->expr_value = regBase | 0x06 | (bit<<3);
		args->extStk->tag = 'b';
		args->extStk->ptr = 1;
		break;
	default:
		badOperand(HandleBits,InvalidDestination,args);
		break;
	}
}

typedef struct
{
	int bitNum;
	const char *name;
} CC_List_t;

#define JUMP_BASE	(0xC3)
#define CALL_BASE	(0xCD)

static void handle_jumpCall(OpcArgs_t *args, int baseImm, int baseCC)
{
	static const char HandleJump[] = "handle_jumpCall(): ";
	int ct, opCode = 0, bitNum = -1;
	char *inp = inp_ptr;
	
	ct = get_token();		/* pickup the next token */
	if ( ct == EOL )
	{
		badOperand(HandleJump,InvalidOperand,args);
		return;
	}
	if ( ct == TOKEN_strng )
	{
		while ( myIsspace(*inp_ptr) )
			++inp_ptr;
		if ( *inp_ptr == ',' )
		{
			static const CC_List_t CCs[] = 
			{
				{ 0, "NZ" },
				{ 1, "Z" },
				{ 2, "NC"},
				{ 3, "C" },
				{ 4, "PO" },
				{ 5, "PE" },
				{ 6, "P" },
				{ 7, "M" },
				{ -1, NULL }
			};
			const CC_List_t *ccList;
			
			ccList = CCs;
			while ( ccList->name  )
			{
				if ( !strcmp(ccList->name,token_pool) )
				{
					bitNum = ccList->bitNum;
					break;
				}
				++ccList;
			}
			if ( bitNum < 0 || bitNum > 7 )
			{
				badOperand(HandleJump,"Undefined jump/call conditional",args);
				return;
			}
			++inp_ptr;	/* eat the comma */
		}
		else
		{
			inp_ptr = inp;	/*reset for rescan */
		}
	}
	else
	{
		inp_ptr = inp;	/*reset for rescan */
	}
	args->dstMode = do_operand( args->opc, args->dstStk);
	if (    (baseImm != JUMP_BASE && args->dstMode != AM_nn )
		 || (baseImm == JUMP_BASE && args->dstMode != AM_nn && args->dstMode != AM_IHL && args->dstMode != AM_IIX && args->dstMode != AM_IIY)
	   )
	{
		badOperand(HandleJump,InvalidOperand,args);
		return;
	}
	switch (args->dstMode)
	{
	case AM_nn:
		if ( bitNum < 0 )
			opCode = baseImm;
		else
			opCode = baseCC | (bitNum<<3);
		args->dstStk->tag = 'w';
		break;
	case AM_IHL:
		if ( baseImm == JUMP_BASE )
		{
			opCode = 0xE9;
			args->dstStk->ptr = 0;
		}
		break;
	case AM_IIX:
		if ( baseImm == JUMP_BASE )
		{
			opCode = 0xE9DD;
			args->dstStk->ptr = 0;
		}
		break;
	case AM_IIY:
		if ( baseImm == JUMP_BASE )
		{
			opCode = 0xE9FD;
			args->dstStk->ptr = 0;
		}
		break;
	default:
		break;
	}
	if ( !opCode )
		badOperand(HandleJump,InvalidDestination,args);
	else
		args->expsp0->expr_value = opCode;
}

static void handle_branch(OpcArgs_t *args)
{
	static const char HandleBranch[] = "handle_branch(): ";
	int ct, base = 0x18;
	char *inp = inp_ptr;
	EXPR_struct *exp;
	
	ct = get_token();		/* pickup the next token */
	if ( ct == EOL )
	{
		badOperand(HandleBranch,InvalidOperand,args);
		return;
	}
	if ( ct == TOKEN_strng )
	{
		while ( myIsspace(*inp_ptr) )
			++inp_ptr;
		if ( *inp_ptr == ',' )
		{
			static const CC_List_t CCs[] = 
			{
			{ 0x38, "C" },
			{ 0x30, "NC"},
			{ 0x28, "Z" },
			{ 0x20, "NZ" },
			{ -1, NULL }
			};
			const CC_List_t *ccList;
			ccList = CCs;
			while ( ccList->name  )
			{
				if ( !strcmp(ccList->name,token_pool) )
				{
					base = ccList->bitNum;
					break;
				}
				++ccList;
			}
			if ( !ccList->name )
			{
				badOperand(HandleBranch,"Undefined branch conditional",args);
				return;
			}
			++inp_ptr;	/* eat the comma */
		}
		else
		{
			inp_ptr = inp;	/*reset for rescan */
		}
	}
	else
	{
		inp_ptr = inp;	/*reset for rescan */
	}
	args->expsp0->expr_value = base;
	args->dstMode = do_operand( args->opc, args->dstStk);
	if ( args->dstMode != AM_nn )
	{
		badOperand(HandleBranch,InvalidOperand,args);
		return;
	}
	exp = args->dstSp0+1;
	exp->expr_code = EXPR_SEG;
	exp->expr_value = current_offset + 2;
	(exp++)->expr_seg = current_section;
	exp->expr_code = EXPR_OPER;
	exp->expr_value = '-';
	args->dstStk->ptr += 2;
	args->dstStk->tag = 's';
	args->srcStk->ptr = 0;
}

static void handle_djnz(OpcArgs_t *args)
{
	static const char HandleDJNZ[] = "handle_djnz(): ";
	EXPR_struct *exp;

	args->expsp0->expr_value = 0x10;
	args->dstMode = do_operand( args->opc, args->dstStk);
	if ( args->dstMode != AM_nn )
	{
		badOperand(HandleDJNZ,InvalidOperand,args);
		return;
	}
	exp = args->dstSp0+1;
	exp->expr_code = EXPR_SEG;
	exp->expr_value = current_offset + 2;
	(exp++)->expr_seg = current_section;
	exp->expr_code = EXPR_OPER;
	exp->expr_value = '-';
	args->dstStk->ptr += 2;
	args->dstStk->tag = 's';
	args->srcStk->ptr = 0;
}

static void handle_ret(OpcArgs_t *args)
{
	static const char HandleRet[] = "handle_ret(): ";
	int ct, opCode = 0xC9;

	ct = get_token();		/* pickup the next token */
	if ( ct == TOKEN_strng )
	{
		static const CC_List_t CCs[] = 
		{
		{ 0, "NZ" },
		{ 1, "Z" },
		{ 2, "NC"},
		{ 3, "C" },
		{ 4, "PO" },
		{ 5, "PE" },
		{ 6, "P" },
		{ 7, "M" },
		{ -1, NULL }
		};
		const CC_List_t *ccList;
		int bitNum = -1;
		ccList = CCs;
		while ( ccList->name  )
		{
			if ( !strcmp(ccList->name,token_pool) )
			{
				bitNum = ccList->bitNum;
				break;
			}
			++ccList;
		}
		if ( !ccList->name )
		{
			badOperand(HandleRet,"Undefined conditional",args);
			return;
		}
		opCode = 0xC0 | (bitNum<<3);
	}
	args->expsp0->expr_value = opCode;
	args->dstMode = do_operand( args->opc, args->dstStk);
	args->dstStk->ptr = 0;
	args->srcStk->ptr = 0;
}

static void handle_rst(OpcArgs_t *args)
{
	static const char HandleRst[] = "handle_rst(): ";

	args->dstMode = do_operand( args->opc, args->dstStk);
	if (    args->dstMode == AM_nn
		 && args->dstStk->ptr == 1
		 && args->dstSp0->expr_code == EXPR_VALUE
		 && args->dstSp0->expr_value >= 0x00
		 && args->dstSp0->expr_value <= 0x38
		 && !(args->dstSp0->expr_value&0x7)
		)
	{
		args->expsp0->expr_value = 0xC7 | args->dstSp0->expr_value;
		args->dstStk->ptr = 0;
	}
	else
	{
		badOperand(HandleRst,InvalidDestination,args);
	}
}

static void handle_in(OpcArgs_t *args)
{
	static const char HandleIn[] = "handle_in(): ";
	
	args->dstMode = do_operand( args->opc, args->dstStk);
	comma_expected = 1;
	args->srcMode = do_operand( args->opc, args->srcStk);
	args->dstStk->ptr = 0;
	switch (args->dstMode)
	{
	case AM_A:
		if (args->srcMode == AM_Inn)
		{
			args->expsp0->expr_value = 0xDB;
			break;
		}
		// Fall through to normal
	case AM_B:
	case AM_C:
	case AM_D:
	case AM_E:
	case AM_H:
	case AM_L:
		if ( args->srcMode == AM_IC )
		{
			args->expsp0->expr_value = 0xED | ((0x40 | (args->dstMode << 3)) << 8);
			args->dstStk->ptr = 0;
			args->srcStk->ptr = 0;
			break;
		}
		// Fall through
	default:
		badOperand(HandleIn,InvalidSource,args);
		break;
	}
}

static void handle_out(OpcArgs_t *args)
{
	static const char HandleOut[] = "handle_out(): ";

	args->dstMode = do_operand( args->opc, args->dstStk);
	comma_expected = 1;
	args->srcMode = do_operand( args->opc, args->srcStk);
	args->srcStk->ptr = 0;
	switch (args->dstMode)
	{
	case AM_Inn:
		args->expsp0->expr_value = 0xD3;
		break;
	case AM_IC:
		switch (args->srcMode)
		{
		case AM_A:
		case AM_B:
		case AM_C:
		case AM_D:
		case AM_E:
		case AM_H:
		case AM_L:
			args->expsp0->expr_value = 0xED | ((0x41|(args->srcMode<<3))<<8);
			args->dstStk->ptr = 0;
			break;
		default:
			badOperand(HandleOut,InvalidSource,args);
			break;
		}
		break;
	default:
		badOperand(HandleOut,InvalidDestination,args);
		break;
	}
}

static void finishUp(OpcArgs_t *args)
{
	if ( args->error )
	{
		args->expsp0->expr_code = EXPR_VALUE;
		args->expsp0->expr_value = 0;
		args->exp0->tag = 'b';
		args->exp0->ptr = 1;
		args->dstSp0->expr_code = EXPR_VALUE;
		args->dstSp0->expr_value = 0;
		args->dstStk->psuedo_value = 0;
		args->dstStk->tag = 'b';
		args->dstStk->ptr = 1;
		args->srcSp0->expr_code = EXPR_VALUE;
		args->srcSp0->expr_value = 0;
		args->srcStk->psuedo_value = 0;
		args->srcStk->tag = 'b';
		args->srcStk->ptr = 1;
		args->extSp0->expr_code = EXPR_VALUE;
		args->extSp0->expr_value = 0;
		args->extStk->psuedo_value = 0;
		args->extStk->tag = 'b';
		args->extStk->ptr = 1;
	}
	else if ( args->expsp0->expr_value > 255 )
		args->exp0->tag = 'w';
	args->exp0->psuedo_value = args->expsp0->expr_value;
	p1o_any(args->exp0);
	if ( !args->outOrder )
	{
		if ( args->dstStk->ptr )
		{
			compress_expr(args->dstStk);
			if (list_bin)
				compress_expr_psuedo(args->dstStk);
			p1o_any(args->dstStk);
		}
		if ( args->srcStk->ptr )
		{
			compress_expr(args->srcStk);
			if (list_bin)
				compress_expr_psuedo(args->srcStk);
			p1o_any(args->srcStk);
		}
	}
	else
	{
		if ( args->srcStk->ptr )
		{
			compress_expr(args->srcStk);
			if (list_bin)
				compress_expr_psuedo(args->srcStk);
			p1o_any(args->srcStk);
		}
		if ( args->dstStk->ptr )
		{
			compress_expr(args->dstStk);
			if (list_bin)
				compress_expr_psuedo(args->dstStk);
			p1o_any(args->dstStk);
		}
	}
	if ( args->extStk->ptr )
	{
		if (list_bin)
			compress_expr_psuedo(args->extStk);
		p1o_any(args->extStk);
	}
}

void do_opcode(Opcode *opc)
{
	OpcArgs_t args;
	AMClasses_t iClass;
	
	EXP0.tag = 'b';          /* opcode is default 8 bit byte */
	EXP0.tag_len = 1;        /* only 1 byte */
	EXP0.ptr = 1;            /* first stack has opcode (1 element) */
	EXP0SP->expr_code = EXPR_VALUE;  /* set the opcode expression */
	EXP0SP->expr_value = opc->op_value;
	EXP0.psuedo_value = opc->op_value;
	am_ptr = inp_ptr;            /* remember where am starts */

	iClass = opc->op_class&0x3FFF;
	if ( iClass > CLASS_MAX )
		iClass = CLASS_MAX;
	args.opc = opc;
	args.exp0 = &EXP0;
	args.expsp0 = args.exp0->stack;
	args.dstStk = &EXP1;
	args.dstSp0 = args.dstStk->stack;
	args.srcStk = &EXP2;
	args.srcSp0 = args.srcStk->stack;
	args.extStk = &EXP3;
	args.extSp0 = args.extStk->stack;
	args.error = 0;
	args.dstMode = ILL_AM;
	args.srcMode = ILL_AM;
	args.dstStk->ptr = 0;
	args.srcStk->ptr = 0;
	args.extStk->ptr = 0;
	args.outOrder = 0;
	switch (iClass)
	{
		case CLASS_NO:	/* No operands */
			break;
		case CLASS_LD:
			handle_ld(&args);
			break;
		case CLASS_PSH:
			handle_pushPop(&args,0x04);
			break;
		case CLASS_POP:
			handle_pushPop(&args,0x00);
			break;
		case CLASS_EX:
			handle_ex(&args);
			break;
		case CLASS_ADD:
			handle_alu(&args,ADD_BASE,0xC6);
			break;
		case CLASS_ADC:
			handle_alu(&args,ADC_BASE,0xCE);
			break;
		case CLASS_SUB:
			handle_alu(&args,0x90,0xD6);
			break;
		case CLASS_SBC:
			handle_alu(&args,SBC_BASE,0xDE);
			break;
		case CLASS_AND:
			handle_alu(&args,0xA0,0xE6);
			break;
		case CLASS_XOR:
			handle_alu(&args,0xA8,0xEE);
			break;
		case CLASS_OR:
			handle_alu(&args,0xB0,0xF6);
			break;
		case CLASS_CP:
			handle_alu(&args,0xB8,0xFE);
			break;
		case CLASS_INC:
			handle_incDec(&args,INC_BASE);
			break;
		case CLASS_DEC:
			handle_incDec(&args,DEC_BASE);
			break;
		case CLASS_RLC:
			handle_shifts(&args,0xCB,0x00);
			break;
		case CLASS_RRC:
			handle_shifts(&args,0xCB,0x08);
			break;
		case CLASS_RL:
			handle_shifts(&args,0xCB,0x10);
			break;
		case CLASS_RR:
			handle_shifts(&args,0xCB,0x18);
			break;
		case CLASS_SLA:
			handle_shifts(&args,0xCB,0x20);
			break;
		case CLASS_SRA:
			handle_shifts(&args,0xCB,0x28);
			break;
		case CLASS_SRL:
			handle_shifts(&args,0xCB,0x38);
			break;
		case CLASS_IM:
			handle_im(&args);
			break;
		case CLASS_BIT:
			handle_bits(&args,0xCB,0x40);
			break;
		case CLASS_RES:
			handle_bits(&args,0xCB,0x80);
			break;
		case CLASS_SET:
			handle_bits(&args,0xCB,0xC0);
			break;
		case CLASS_JP:
			handle_jumpCall(&args,JUMP_BASE,0xC2);
			break;
		case CLASS_JR:
			handle_branch(&args);
			break;
		case CLASS_RST:
			handle_rst(&args);
			break;
		case CLASS_DJNZ:
			handle_djnz(&args);
			break;
		case CLASS_CALL:
			handle_jumpCall(&args,CALL_BASE,0xC4);
			break;
		case CLASS_RET:
			handle_ret(&args);
			break;
		case CLASS_IN:
			handle_in(&args);
			break;
		case CLASS_OUT:
			handle_out(&args);
			break;
		case CLASS_MAX:
			bad_token(tkn_ptr, "INTERNAL: Undefined opcode class.");
			f1_eatit();
			break;
	}
	finishUp(&args);
}                   /* -- opc80() */

struct rinit
{
    char *name;
    uint32_t value;
};

static const struct rinit reginit[] =
{
    { "B",REG_B},	/* Registers */
    { "C",REG_C},
    { "D",REG_D},
    { "E",REG_E},
    { "H",REG_H},
    { "L",REG_L},
/*    { "M",REG_M}, */
    { "A",REG_A},
    { "BC",REG_BC},
    { "DE",REG_DE},
	{ "HL",REG_HL},
	{ "SP",REG_SP},
	{ "AF",REG_AF},
	{ "R", REG_R},
	{ "I", REG_I},
	{ "IX",REG_IX},
	{ "IY",REG_IY},
	{ NULL, 0 }
};

int ust_init(void)
{
	SS_struct *ptr;
	const struct rinit *ri;

	ri = reginit;
	while ( ri->name != 0 )
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
		ptr->flg_macLocal = 0;       /* not local */
		ptr->flg_gasLocal = 0;
		ptr->flg_global = 0;      /* not global */
		ptr->flg_label = 0;       /* can redefine it */
		ptr->flg_fixed_addr = 1;
		ptr->ss_fnd = 0;          /* no associated file */
		ptr->flg_register = 1;        /* it's a register */
		ptr->flg_abs = 1;         /* it's not relocatible */
		ptr->flg_exprs = 0;       /* no expression with it */
		ptr->ss_seg = 0;          /* not an offset from a segment */
		ptr->ss_value = ri->value;    /* value */
		ri += 1;              /* next */
	}
	return 0;            /* would fill a user symbol table */
}


int op_ntype(void)
{
	f1_eatit();
	return 1;
}



#include "opcommon.h"


