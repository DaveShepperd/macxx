/*
    le_itfc.c - Part of macxx, a cross assembler family for various micro-processors
    Copyright (C) 2025 David Shepperd

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

#ifndef MAC_PP
#include "token.h"
#include "exproper.h"
#include "listctrl.h"
#include "memmgt.h"
#include <stdlib.h>
#include "le_itfc.h"

#if defined(MAC_65) || defined(MAC_68) || defined(MAC_69) 
    #define EXPR_C 0
#endif

#ifndef EXPR_C
    #define EXPR_C 1
#endif

static ExprsDef_t *exprsDef;

static ExprsErrs_t copyTerm(ExprsDef_t *exprs, const ExprsTerm_t *term)
{
	char eBuf[128];
	const char *str;
	EXPR_struct *expr_ptr;
	EXP_stk *eps = (EXP_stk *)exprs->userArg1;
	SS_struct *sym_ptr;
	ExprsErrs_t err=EXPR_TERM_GOOD;
	
	expr_ptr = eps->stack+eps->ptr;
	memset(expr_ptr,0,sizeof(EXPR_struct));
	if ( (edmask&ED_ALTVER) )
		printf("copyTerm(): Entry. termType=%d, flags=0x%X, value=0x%X\n", term->termType, term->flags, term->term.u32 );
	if ( (term->flags&EXPRS_TERM_FLAG_REGISTER) )
		eps->register_reference |= 1;
	eps->force_byte |= (term->flags & EXPRS_TERM_FLAG_BYTE) ? 1 : 0;
	eps->force_short |= (term->flags & EXPRS_TERM_FLAG_WORD) ? 1 : 0;
	eps->force_long |= (term->flags & EXPRS_TERM_FLAG_LONG) ? 1 : 0;
	switch (term->termType)
	{
	case EXPRS_TERM_INTEGER:
		expr_ptr->expr_code = EXPR_VALUE;
		expr_ptr->expr_value  = term->term.s32;
		break;          /* exit from switch */
/*	case EXPRS_TERM_SYMBOL_COMPLEX: */
	case EXPRS_TERM_SYMBOL:
		{
			int sLen;
			int lclPC = 0;

			str = libExprsStringPoolTop(exprs) + term->term.string;
			sLen = strlen(str);
/*			printf("copyTerm(): Found type EXPRS_TERM_SYMBOL. sLen=%d, name='%s', flags=0x%X\n", sLen, str, term->flags); */
			if ( sLen == 1 )
			{
				lclPC = (*str == '.');
				if ( !lclPC && (edmask & ED_DOL_PC) )
					lclPC = (*str == '$');
			}
			if (lclPC)
			{
				current_section->flg_reference = 1;
				if (current_section->flg_abs)
				{
					expr_ptr->expr_code = EXPR_VALUE;
					expr_ptr->expr_value = current_offset;
				}
				else
				{
					expr_ptr->expr_code = EXPR_SEG;
					expr_ptr->expr_seg = current_section;
					expr_ptr->expr_value = current_offset;
				}
				break;
			}
			if ( (term->flags & EXPRS_TERM_FLAG_LOCAL_SYMBOL) )
			{
				mklocal(str,strlen(str));
			}
			else
			{
				if ( sLen > max_symbol_length )
					sLen = max_symbol_length;
				if ( !(edmask & ED_LC) )
				{
					int ii;
					char *dst = token_pool;
					const char *src = str;
					for (ii=0; ii < sLen; ++ii)
						*dst++ = toupper(*src++);
				}
				else
					memcpy(token_pool, str, sLen);
				token_pool[sLen] = 0;
				token_type =TOKEN_strng;
				token_value = sLen;
			}
			if ((sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND)) == 0)
			{  /* process symbol name */
				err = EXPR_TERM_BAD_UNDEFINED_SYMBOL;
				break;
			}
			if ( sym_ptr->flg_fwdReference )
				eps->forward_reference = 1;
			if ( sym_ptr->flg_defined )
			{
#if EXPR_C
				if (sym_ptr->flg_register)
				{
					eps->register_reference = 1;
					if (sym_ptr->flg_regmask)
					{
						expr_ptr->expr_flags = EXPR_FLG_REGMASK;
						eps->register_mask = 1;
					}
					else
					{
						expr_ptr->expr_flags = EXPR_FLG_REG;
					}
				}
#endif
				if (sym_ptr->flg_exprs)
				{
					expr_ptr->expr_code = EXPR_LINK;
					expr_ptr->expr_expr = sym_ptr->ss_exprs;
					expr_ptr->expr_value = 0;
				}
				else
				{
					SEG_struct *seg_ptr;
					seg_ptr = sym_ptr->ss_seg;
					expr_ptr->expr_value = sym_ptr->ss_value;
					if (seg_ptr == 0 || seg_ptr->flg_abs)
					{
						expr_ptr->expr_code = EXPR_VALUE;
					}
					else
					{
						expr_ptr->expr_code = EXPR_SEG;
						if (seg_ptr->flg_subsect && seg_ptr->rel_offset != 0)
						{
							sym_ptr->ss_seg = expr_ptr->expr_seg = *(seg_list+seg_ptr->seg_index);
							expr_ptr->expr_value += seg_ptr->rel_offset;
							sym_ptr->ss_value = expr_ptr->expr_value;
							sym_ptr->ss_seg = expr_ptr->expr_seg;
						}
						else
						{
							expr_ptr->expr_seg = seg_ptr;
						}
					}
				}
			}
			else
			{
				/* Symbol not defined */
				sym_ptr->flg_fwdReference = 1;	/* Signal this symbol has a forward reference */
				sym_ptr->flg_ref = 1;			/* signal symbol referenced */
				expr_ptr->expr_code = EXPR_SYM;
				expr_ptr->expr_sym = sym_ptr;
				expr_ptr->expr_value = 0;
				eps->forward_reference = 1;     /* signal this expression contains a forward reference */
#if defined(MAC_68K)
				if ( !sym_ptr->flg_global && sym_ptr->ss_string[0] == '.' 
					 && ( sym_ptr->ss_string[1] == 'L' || sym_ptr->ss_string[1] == 'l') )
				{
					char *endp=NULL;
					strtol(sym_ptr->ss_string+2,&endp,0);
					if ( endp && !*endp )
						sym_ptr->flg_gasLocal = 1; /* make .Lnnn GNU assembler "local" symbols */
				}
#endif
			}
			break;          /* exit from switch */
		}
	case EXPRS_TERM_POS:	/* + (unary term in this case) */
		return EXPR_TERM_GOOD;	/* Do nothing with this */
	case EXPRS_TERM_NEG:	/* - (unary term in this case) */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_NEG;
		break;
	case EXPRS_TERM_HIGH_BYTE: /* high byte */
		/* convert 'term (high_byte)' to 'term (swap_bytes) 255 &' */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_SWAP;
		++expr_ptr;
		expr_ptr->expr_code = EXPR_VALUE;
		expr_ptr->expr_value = 0xff;
		++expr_ptr;
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_AND;
		eps->ptr += 2;
		break;
	case EXPRS_TERM_LOW_BYTE:/* low byte */
		/* convert 'term (low_byte)' to 'term 255 &' */
		expr_ptr->expr_code = EXPR_VALUE;
		expr_ptr->expr_value = 0xff;
		++expr_ptr;
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_AND;
		eps->ptr += 1;
		break;
	case EXPRS_TERM_SWAP:	/* exchange bytes */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_SWAP;
		break;
	case EXPRS_TERM_MUL:		/* * */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_MUL;
		break;
	case EXPRS_TERM_DIV:		/* / */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_DIV;
		break;
	case EXPRS_TERM_MOD:		/* % */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_MOD;
		break;
	case EXPRS_TERM_ADD:		/* + (binary terms in this case) */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_ADD;
		break;
	case EXPRS_TERM_SUB:		/* - (binary terms in this case) */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_SUB;
		break;
	case EXPRS_TERM_SHL:		/* << */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_SHL;
		break;
	case EXPRS_TERM_SHR:		/* >> */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_SHR;
		break;
	case EXPRS_TERM_GT:		/* > */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_GT<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_GE:		/* >= */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_GE<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_LT:		/* < */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_LT<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_LE:		/* <= */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_LE<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_EQ:		/* == */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_EQ<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_NE:		/* != */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_NE<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_AND:		/* & */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_AND;
		break;
	case EXPRS_TERM_XOR:		/* ^ */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_XOR;
		break;
	case EXPRS_TERM_OR:		/* | */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_OR;
		break;
	case EXPRS_TERM_LAND:	/* && */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_AND<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_LOR:		/* || */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_OR<<8)|EXPROPER_TST;
		break;
	case EXPRS_TERM_COM:		/* ~ */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = EXPROPER_COM;
		break;
	case EXPRS_TERM_NOT:		/* ! */
		expr_ptr->expr_code = EXPR_OPER;
		expr_ptr->expr_value = (EXPROPER_TST_NOT<<8)|EXPROPER_TST;
		break;
#if 0
	case EXPRS_TERM_POW:		/* ** */
		show_bad_token(NULL,"libExprs(): unsupported POWER term type", MSG_ERROR);
		err = EXPR_TERM_BAD_UNSUPPORTED;
		break;
	case EXPRS_TERM_FUNCTION:/* Function call (not supported yet) */
		show_bad_token(NULL,"libExprs(): unsupported FUNCTION term type", MSG_ERROR);
		err = EXPR_TERM_BAD_UNSUPPORTED;
		break;
	case EXPRS_TERM_ASSIGN:	/* = */
		show_bad_token(NULL,"libExprs(): unsupported ASSIGNMENT term type", MSG_ERROR);
		err = EXPR_TERM_BAD_UNSUPPORTED;
		break;
	case EXPRS_TERM_FLOAT:
		show_bad_token(NULL,"libExprs(): unsupported FLOAT term type", MSG_ERROR);
		err = EXPR_TERM_BAD_UNSUPPORTED;
		break;
	case EXPRS_TERM_STRING:
		show_bad_token(NULL,"libExprs(): unsupported STRING term type", MSG_ERROR);
		err = EXPR_TERM_BAD_UNSUPPORTED;
		break;
#endif
	default:
		snprintf(eBuf, sizeof(eBuf), "libExprs(): Undefined termtype %d", term->termType);
		show_bad_token(NULL,eBuf,MSG_ERROR);
		err = EXPR_TERM_BAD_UNDEFINED;
		break;
	}
	if ( err == EXPR_TERM_GOOD )
		++eps->ptr;
	return err;
}

int le_itfc(int flag, EXP_stk *eps)
{
    int cnt;
	ExprsErrs_t eErrs;
	char eBuf[128];
	uint32_t flags;
	
	if ( !exprsDef )
	{
		exprsDef = libExprsInit(NULL,0,0);
		if ( !exprsDef )
			return (eps->ptr = -1);
		exprsDef->mOpenDelimiter = expr_open;
		exprsDef->mCloseDelimiter = expr_close;
	}
	flags = 
		 EXPRS_FLG_USE_RADIX
		|EXPRS_FLG_NO_FLOAT
		|EXPRS_FLG_NO_STRING
		|EXPRS_FLG_DOT_DECIMAL
		|EXPRS_FLG_NO_POWER
		|EXPRS_FLG_SINGLE_QUOTE
		|EXPRS_FLG_NO_ASSIGNMENT
		|EXPRS_FLG_LOCAL_SYMBOLS
		|EXPRS_FLG_DOT_SYMBOL
		|EXPRS_FLG_PCNT_IS_REGISTER
		|EXPRS_FLG_OPEN_IS_END
		|EXPRS_FLG_CLOSE_IS_END
		|EXPRS_FLG_NO_DOUBLE_PLAIN
		;
	if ( (edmask & ED_H_HEX) )
		flags |= EXPRS_FLG_H_HEX;
	if ( (edmask & ED_DOL) )
		flags |= EXPRS_FLG_PRE_DOLLAR_HEX;
	if ( (edmask & ED_O_OCT) )
		flags |= EXPRS_FLG_O_OCTAL;
	if ( (edmask & ED_Q_OCT) )
		flags |= EXPRS_FLG_Q_OCTAL;
	if ( !(edmask & ED_PRECED) )
		flags |= EXPRS_FLG_NO_PRECEDENCE;
	if ( no_white_space_allowed )
		flags |= EXPRS_FLG_WS_DELIMIT;
	if ( (macxx_name_mask&(MACXX_M_65|MACXX_M_68|MACXX_M_69|MACXX_M_11)) )
		flags |= EXPRS_FLG_SPECIAL_UNARY|EXPRS_FLG_NO_LOGICALS;
	if ( (macxx_name_mask&(MACXX_M_68K|MACXX_M_11)) )
		flags |= EXPRS_FLG_PCNT_REGISTER;
	if ( (macxx_name_mask&(MACXX_M_68K)) )
		flags |= EXPRS_FLG_LEN_QUALIFIERS;
	libExprsSetFlags(exprsDef, flags, NULL);
	libExprsSetRadix(exprsDef, current_radix, NULL);
	libExprsSetVerbose(exprsDef,(edmask&ED_ALTVER) ? 1 : 0,NULL);
	eErrs = libExprsParseToRPN(exprsDef, actualTknPtr, 0);
	if ( eErrs > EXPR_TERM_END )
	{
		snprintf(eBuf,sizeof(eBuf),"libExprsParseToRPN failed: %d = %s", eErrs, libExprsGetErrorStr(eErrs));
		show_bad_token(inp_ptr, eBuf, MSG_FATAL);
		return (eps->ptr = -1);
	}
	cnt = exprsDef->mStack.mTermsPool.mNumUsed;
	if ( cnt >= EXPR_MAXDEPTH)
	{
		bad_token(tkn_ptr,"Too many terms in expression");
		return (eps->ptr = -1);
	}
/*	printf("After libExprsParseToRPN('%s'): Num terms = %d\n", inp_ptr, cnt); */
	inp_ptr += exprsDef->mCurrPtr-inp_ptr;
	exprsDef->userArg1 = eps;
	eErrs = libExprsWalkParsedStack(exprsDef, copyTerm, 0);
	if ( eErrs )
	{
		snprintf(eBuf,sizeof(eBuf),"libExprsWalkParsedStack() returned error %d: %s", eErrs, libExprsGetErrorStr(eErrs));
		show_bad_token(NULL, eBuf, MSG_ERROR);
		return (eps->ptr = -1);
	}
/*	printf("After libExprsWalkParseStack(): eps->ptr = %d\n", eps->ptr); */
	return eps->ptr;
}
#endif	/* !defined(MAC_PP) */
