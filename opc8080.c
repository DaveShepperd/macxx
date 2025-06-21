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


#if !defined(MAC_8080)
	#define MAC_8080
#endif

#include "token.h"
#include "pst_tokens.h"
#include "psttkn8080.h"
#include "exproper.h"
#include "listctrl.h"
#include "add_defs.h"
#include "memmgt.h"

#define DEFNAM(name,numb) {"name",name,numb},

/* The following are variables specific to the particular assembler */

char macxx_name[] = "mac8080";
char *macxx_target = "8080";
char *macxx_descrip = "Cross assembler for the 8080.";

unsigned short macxx_salign = 0;    /* default alignment segments by LLF */
unsigned short macxx_dalign = 0;    /* default alignment data within segment */
unsigned short macxx_min_dalign = 0;

char macxx_mau = 8;         /* number of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;    /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = ED_TRUNC | ED_DOL_PC | ED_H_HEX | ED_O_OCT | ED_Q_OCT | ED_ALTEXP | ED_PRECED;  /* default edmask */
unsigned long macxx_lm_default = ~(LIST_ME | LIST_MEB | LIST_MES | LIST_LD | LIST_COD);  /* default list mask */

int current_radix = 10;     /* default the radix to decimal */
char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
/* char expr_escape = '^'; */     /* char that escapes a unary expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */

int max_opcode_length = 6; /* significant length of opcodes */
int max_symbol_length = 6; /* significant length of symbols */

extern int dotwcontext;
/* extern int no_white_space_allowed; */
static char *am_ptr;

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

static int do_operand(Opcode *opc, int rqd_mode)
{
	int ct, regNum, relOK;
	
	ct = get_token();		/* pickup the next token */
	if ( ct == EOL )
		return ILL_NUM;
	relOK = (rqd_mode == D3 || rqd_mode == R || rqd_mode == RP || rqd_mode == PSW ) ? 0 : 1;
	if ( exprs(relOK,&EXP1) < 1 )
		return ILL_NUM;
	if ( rqd_mode == D3 || rqd_mode == D8 || rqd_mode == D16 )
	{
		if ( rqd_mode == D16 )
			EXP1.tag = 'w';	/* operand is 16 bits */
		else if ( rqd_mode == D3 )
		{
			if ( (EXP1SP->expr_value > 7 || EXP1SP->expr_value < 0) )
			{
				bad_token(am_ptr,"Parameter for RST instruction out of range. Can only be absolute 0-7");
				EXP1.stack[0].expr_value = 0;
				EXP1.stack[0].expr_code = EXPR_VALUE;
			}
			EXP1.ptr = 0;	/* Don't output an expression for this case */
			rqd_mode = R;	/* But the bits go where a dst register would go */
		}
		return rqd_mode;
	}
	if ( !EXP1.register_reference )
	{
		bad_token(am_ptr,"Parameter must be a register type");
		regNum = 0;
	}
	switch (rqd_mode)
	{
	case R:
		if ( EXP1SP->expr_value >= 0 && EXP1SP->expr_value <= 7 )
			regNum = EXP1SP->expr_value;
		else
		{
			bad_token(am_ptr,"Register parameter out of range. Can only have values of 0 through 7");
			regNum = 0;
		}
		break;
	case RP:
		regNum = EXP1SP->expr_value;
		switch (regNum)
		{
		case 0:	/* B */
		case 8:	/* BC */
			regNum = 0;
			break;
		case 2:	/* D */
		case 9:	/* DE */
			regNum = 2;
			break;
		case 4:	/* H */
		case 0xA: /* HL */
			regNum = 4;
			break;
		case 0xB: /* SP */
			regNum = 6;
			break;
		default:
			bad_token(am_ptr,"Register parameter out of range. Can only have values of 0,2,4,8,9,0xA or 0xB");
			regNum = 0;
			break;
		}
		break;
	case PSW:
		regNum = EXP1SP->expr_value;
		switch (regNum)
		{
		case 0:	/* B */
		case 8:	/* BC */
			regNum = 0;
			break;
		case 2:	/* D */
		case 9:	/* DE */
			regNum = 2;
			break;
		case 4:	/* H */
		case 0xA: /* HL */
			regNum = 4;
			break;
		case 0x0C:/* PSW */
			regNum = 6;
			break;
		default:
			bad_token(am_ptr,"Register parameter out of range. Can only have values of 0,2,4,8,9,0xA or 0xC");
			regNum = 0;
			break;
		}
		break;
	case BD:
		regNum = EXP1SP->expr_value;
		switch (regNum)
		{
		case 0:	/* B */
		case 8:	/* BC */
			regNum = 0;
			break;
		case 2:	/* D */
		case 9:	/* DE */
			regNum = 2;
			break;
		default:
			bad_token(am_ptr,"Register parameter out of range. Can only have values of 0,2,8 or 9");
			regNum = 0;
			break;
		}
		break;
	default:
		regNum = -1;
	}
	if ( regNum < 0 )
	{
		bad_token(am_ptr,"Invalid or undefined register value");
		regNum = 0;
	}
	EXP1.stack[0].expr_value = regNum;
	EXP1.stack[0].expr_code = EXPR_VALUE;
	EXP1.ptr = 0;
	return rqd_mode;
}



void do_opcode(Opcode *opc)
{
	int err_cnt = error_count[MSG_ERROR];
	int srcAmode, dstAmode, opResult, srcBits=0, dstBits=0;
	
	EXP0.tag = 'b';          /* opcode is default 8 bit byte */
	EXP0.tag_len = 1;            /* only 1 byte */
	EXP1.tag = 'b';          /* assume operand is byte */
	EXP1.tag_len = 1;            /* and only 1 entry long */
	EXP0SP->expr_code = EXPR_VALUE;  /* set the opcode expression */
	EXP0SP->expr_value = opc->op_value;
	EXP0.ptr = 1;            /* first stack has opcode (1 element) */
	EXP1.ptr = 0;            /* assume no operands */

	am_ptr = inp_ptr;            /* remember where am starts */

	/* dst address modes are indicated in the upper 16 bits of op_amode */
	/* src address modes are indicated in the lower 16 bits of op_amode */
	/* The opcode syntax is op dst, src */
	if ( (opc->op_amode&(1<<(2*OPC_AM_BIT_SHIFT))) && !(edmask&ED_8085) )
	{
		bad_token(tkn_ptr,"Opcode reserved for 8085. Use '.ENABL M8085' to enable its use.");
		f1_eatit();
		return;
	}
	dstAmode = (opc->op_amode >> OPC_AM_BIT_SHIFT)&((1<<OPC_AM_BIT_SHIFT)-1);
	if ( dstAmode )
	{
		/* The PST is setup so the dst is only ever a register or register pair */
		/* Get the dst operand */
		opResult = do_operand(opc,dstAmode);
		if ( opResult >= 0 && (opResult == R || opResult == RP || opResult == BD || opResult == PSW || opResult == D3) )
		{
			/* It's legit, so merge the bits into the opcode byte */
			dstBits = EXP1SP->expr_value;
			EXP0SP->expr_value |= dstBits<<3;
		}
		if ( *inp_ptr == ',' )
			++inp_ptr;	/* eat comma if there is one */
	}
	srcAmode = opc->op_amode&((1<<OPC_AM_BIT_SHIFT)-1);
	if ( srcAmode )
	{
		/* Get the src operand */
		opResult = do_operand(opc,srcAmode);
		if ( opResult >= 0 && srcAmode == R )
		{
			/* It's legit, so merge the register bits into the opcode byte */
			srcBits = EXP1SP->expr_value;
			EXP0SP->expr_value |= srcBits;
		}
	}
	/* Do some simple checking  */
	if ( dstAmode == R && srcAmode == R && srcBits == 6 && dstBits == 6 )
	{
		bad_token(NULL,"MOV M,M == HLT");
	}
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
	}

}                   /* -- opc80() */

struct rinit
{
    char *name;
    unsigned long value;
};

static const struct rinit reginit[] =
{
    { "B",0},	/* Registers */
    { "C",1},
    { "D",2},
    { "E",3},
    { "H",4},
    { "L",5},
    { "M",6},
    { "A",7},
    { "BC",8},	/* Register pairs */
    { "DE",9},
	{ "HL",0xA},
	{ "SP",0xB},
	{ "PSW",0xC},
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


