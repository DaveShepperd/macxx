/*
    exprs.c - Part of macxx, a cross assembler family for various micro-processors
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

    04/04/2022	- Changed added support for MAC69  - Tim Giddens

    03/26/2022	- Changed added support for MAC68  - Tim Giddens

******************************************************************************/
#if defined(MAC68K)
    #include "m68k.h"
#endif
#if defined(MAC682K)
    #include "m682k.h"
#endif
/*********************************************tg*/
/* 04/04/2022 changed for MAC69 support by Tim Giddens
 * 03/26/2022 changed for MAC68 support by Tim Giddens
#if defined(MAC_65)
    #define EXPR_C 0
#endif
*/
#if defined(MAC_65) || defined(MAC_68) || defined(MAC_69)
    #define EXPR_C 0
#endif
/*********************************************etg*/

#ifndef EXPR_C
    #define EXPR_C 1
#endif

#include "token.h"
#include "exproper.h"
#include "listctrl.h"
#include "memmgt.h"

int exprs_nest;
EXP_stk exprs_stack[EXPR_MAXSTACKS];
#if defined(MAC682K)
int squawk_experr = 1;
#endif

#if defined(MAC68K) || defined(MAC682K)
extern int dotwcontext;
#endif

int quoted_ascii_strings = 0;

void init_exprs( void )
{
    int i;
    EXP_stk *eps;
#if !defined(MAC_PP)
    tmp_org_exp.stack = &tmp_org;
    tmp_org_exp.ptr = 1;
    tmp_org.expr_code = EXPR_SEG;
#endif
    eps = &EXP0;
    for (i=0;i<EXPR_MAXSTACKS;++i,++eps)
    {
        eps->stack = (EXPR_struct *)MEM_alloc(EXPR_MAXDEPTH*sizeof(EXPR_struct));
    }
    return;
}   

int compress_expr_psuedo( EXP_stk *ep )
{
    EXPR_struct *src,*op1,*op2,*texpbase,texp[8];
    int src_terms,dst_terms;
    src_terms = ep->ptr;
    if (src_terms <= sizeof(texp)/sizeof(EXPR_struct))
    {
        texpbase = texp;
    }
    else
    {
        texpbase = (EXPR_struct *)MEM_alloc(src_terms*(int)sizeof(EXPR_struct));
    }
    src = ep->stack;
    op1 = texpbase;      /* point to dst array */
    for (dst_terms=0;src_terms > 0; --src_terms,++src)
    {
        switch (src->expr_code)
        {
        case EXPR_LINK: {
                compress_expr_psuedo(src->expr_expr);
                op1->expr_code = EXPR_VALUE;
                op1->expr_value = src->expr_expr->psuedo_value;
                ++dst_terms;
                ++op1;
                continue;
            }
        case EXPR_SYM:
        case EXPR_SEG:
        case EXPR_VALUE: {
                op1->expr_code = EXPR_VALUE;
                op1->expr_value = src->expr_value;
                ++dst_terms;
                ++op1;
                continue;
            }
        case EXPR_OPER: {
                int oper,tcnt;
                oper = src->expr_value;
                --op1;          /* point to top of stack */
                op2 = op1-1;        /* point to second on stack */
                if (oper == EXPROPER_SWAP || 
                    oper == EXPROPER_NEG ||
                    oper == EXPROPER_COM ||
                    oper == (EXPROPER_TST|(EXPROPER_TST_NOT<<8)))
                {
                    tcnt = 1;
                }
                else
                {
                    tcnt = 2;
                }
                if (dst_terms < tcnt) goto bad_expression;
                dst_terms -= tcnt-1;
                switch (oper&255)
                {      /* dispatch on code */
                case EXPROPER_TST: {
                        int condit;
                        oper = oper >> 8; /* get what to test */
                        if (oper == EXPROPER_TST_NOT)
                        {
                            if (op1->expr_value == 0)
                            {
                                op1->expr_value = 1;
                            }
                            else
                            {
                                op1->expr_value = 0;
                            }
                            ++op1;
                            continue;
                        }
                        else if (oper == EXPROPER_TST_OR ||
                                 oper == EXPROPER_TST_AND)
                        {
                            int o1,o2;
                            o1 = (op1->expr_value != 0) ? 1 : 0;
                            o2 = (op2->expr_value != 0) ? 1 : 0;
                            if (oper == EXPROPER_TST_OR)
                            {
                                op2->expr_value = o1 | o2;
                            }
                            else
                            {
                                op2->expr_value = o1 & o2;
                            }
                            continue;
                        }
                        condit = 0;       /* assume false */
                        op2->expr_value -= op1->expr_value;
                        if (oper == EXPROPER_TST_LT)
                        {
                            if (op2->expr_value < 0)
                                condit = 1;
                        }
                        else if (oper == EXPROPER_TST_GT)
                        {
                            if (op2->expr_value > 0)
                                condit = 1;
                        }
                        else if (oper == EXPROPER_TST_EQ)
                        {
                            if (op2->expr_value == 0)
                                condit = 1;
                        }
                        else if (oper == EXPROPER_TST_LE)
                        {
                            if (op2->expr_value <= 0)
                                condit = 1;
                        }
                        else if (oper == EXPROPER_TST_GE)
                        {
                            if (op2->expr_value >= 0)
                                condit = 1;
                        }
                        op2->expr_value = condit;
                        continue;
                    }
                case EXPROPER_XCHG: {
                        long tmp;
                        tmp = op1->expr_value;
                        op1->expr_value = op2->expr_value;
                        op2->expr_value = tmp;
                        ++op1;
                        continue;
                    }
                case EXPROPER_PICK: {
                        op1->expr_value = (op2-op1->expr_value)->expr_value;
                        ++op1;
                        continue;
                    }
                case EXPROPER_ADD: {
                        op2->expr_value += op1->expr_value;
                        continue;
                    }
                case EXPROPER_SUB: {
                        op2->expr_value -= op1->expr_value;
                        continue;
                    }
                case EXPROPER_MUL: {
                        op2->expr_value *= op1->expr_value;
                        continue;
                    }
                case EXPROPER_DIV: {
                        if (op1->expr_value == 0)
                        {
                            if (op2->expr_value > 0)
                            {
                                op2->expr_value = 0x7FFFFFFFl;
                            }
                            if (op2->expr_value < 0)
                            {
                                op2->expr_value = 0x80000000l;
                            }
                        }
                        else
                        {
                            op2->expr_value /= op1->expr_value;
                        }
                        continue;
                    }
                case EXPROPER_AND: {
                        op2->expr_value &= op1->expr_value;
                        continue;
                    }
                case EXPROPER_OR: {
                        op2->expr_value |= op1->expr_value;
                        continue;
                    }
                case EXPROPER_XOR: {
                        op2->expr_value ^= op1->expr_value;
                        continue;
                    }
                case EXPROPER_USD: {
                        if (op1->expr_value == 0)
                        {
                            if (op2->expr_value != 0)
                            {
                                op2->expr_value = 0xFFFFFFFFl;
                            }
                        }
                        else
                        {
                            op2->expr_value = 
                            (unsigned long)op2->expr_value/(unsigned long)op1->expr_value;
                        }
                        continue;
                    }
                case EXPROPER_MOD: {
                        if (op1->expr_value != 0)
                        {
                            op2->expr_value %= op1->expr_value;
                        }
                        else
                        {
                            op2->expr_value = 0;
                        }
                        continue;
                    }
                case EXPROPER_SHL: {
                        op2->expr_value <<= op1->expr_value;
                        continue;
                    }
                case EXPROPER_SHR: {
                        op2->expr_value >>= op1->expr_value;
                        continue;
                    }
                case EXPROPER_SWAP: {
                        int lo,hi;
                        lo = op1->expr_value & 255;
                        hi = (op1->expr_value >> 8) & 255;
                        op1->expr_value = hi | (lo << 8);
                        ++op1;
                        continue;
                    }
                case EXPROPER_NEG: {
                        op1->expr_value = -op1->expr_value;
                        ++op1;
                        continue;
                    }
                case EXPROPER_COM: {
                        op1->expr_value = ~op1->expr_value;
                        ++op1;
                        continue;
                    }
                default: {
                        op1->expr_value = 0;
                        continue;
                    }
                }           /* -- switch (oper) */
            }          /* -- case EXPR_OPER: */
        default: continue; /* this keeps gcc from bitching */
        }             /* -- switch (expr_code) */
        break;            /* if fall out of switch, fall out of for */
    }                /* -- for (terms) */
    if (dst_terms != 1)
    {
        bad_expression:
        bad_token((char *)0,"Invalid expression syntax");
        ep->psuedo_value = 0;
    }
    else
    {
        ep->psuedo_value = texpbase->expr_value;
    }
    if (texpbase != texp) MEM_free((char *)texpbase);
    return 0;
}

/*******************************************************************
 * Expression evaluators
 */
int compress_expr( EXP_stk *exptr )
/*
 * At entry:
 *	stack = pointer to expression stack to evaluate
 *	size = # of items in expression stack
 * At exit:
 *	expression packed into stack as much as possible
 *	returns new length of expression
 */
{
    EXPR_struct *src, *dst, *op1, *op2=NULL;
    int terms,i,j;

    terms = exptr->ptr;
    if (terms <= 0) return(terms);       /* empty */
    src = dst = exptr->stack;    /* start at bottom of stack */
    j = 0;
    for (i=0; i < terms ; ++src,++i)
    {
        switch (src->expr_code)
        {
        case EXPR_VALUE: {
                dst->expr_code = EXPR_VALUE;
                dst->expr_value = src->expr_value;
#if EXPR_C
                dst->expr_flags = src->expr_flags;
#endif
                ++dst;
                ++j;
                continue;
            }
        case EXPR_LINK: {
                int k;
                EXP_stk *nxt_exp;
                nxt_exp = src->expr_expr;
                op1 = nxt_exp->stack;
                k = compress_expr(nxt_exp);
                nxt_exp->ptr = k;
                if (k <= 0) return k;
                exptr->base_page_reference |= nxt_exp->base_page_reference;
                exptr->register_reference |= nxt_exp->register_reference;
                exptr->forward_reference |= nxt_exp->forward_reference;
                if (k == 1)
                {
                    if (op1->expr_code == EXPR_VALUE)
                    {
                        dst->expr_code = EXPR_VALUE;
                        dst->expr_value = op1->expr_value;
#if EXPR_C
                        dst->expr_flags = op1->expr_flags;
#endif
                        ++dst;
                        ++j;
                        continue;
                    }
                    else if (op1->expr_code == EXPR_SYM || op1->expr_code == EXPR_SEG)
                    {
                        src->expr_code = op1->expr_code;
                        src->expr_sym = op1->expr_sym;
                        src->expr_value = op1->expr_value;
#if EXPR_C
                        src->expr_flags = op1->expr_flags;
#endif
                        --src;
                        --i;
                        continue;
                    }
                }
                dst->expr_code = EXPR_LINK;
                dst->expr_expr = nxt_exp;
                ++dst;
                ++j;
                continue;
            }
        case EXPR_SYM: {
                SS_struct *sym_ptr;
                sym_ptr = src->expr_sym;
#if 0
				if ( squeak )
				{
					printf("compress_expr(): EXPR_SYM: '%s', exprs=%d, val=%08lX, base=%d, fwd=%d, def=%d, reg=%d, abs=%d, lab=%d\n",
						   sym_ptr->ss_string,
						   sym_ptr->flg_exprs,
						   sym_ptr->ss_value,
						   sym_ptr->flg_base,
						   sym_ptr->flg_fwd,
						   sym_ptr->flg_defined,
						   sym_ptr->flg_register,
						   sym_ptr->flg_abs,
						   sym_ptr->flg_label
						   );
				}
#endif
				exptr->base_page_reference |= sym_ptr->flg_base;
                exptr->forward_reference |= sym_ptr->flg_fwd;
                exptr->register_reference |= sym_ptr->flg_register;
#if defined(MAC68K)
                exptr->register_mask |= sym_ptr->flg_regmask;
#endif
                if (sym_ptr->flg_defined)
                {
                    if (sym_ptr->flg_abs)
                    {
                        dst->expr_code = EXPR_VALUE;
                        dst->expr_value = sym_ptr->ss_value;
#if defined(MAC68K)
                        if (sym_ptr->flg_regmask) dst->expr_flags = EXPR_FLG_REGMASK;
                        else if (sym_ptr->flg_register) dst->expr_flags = EXPR_FLG_REG;
#endif
                        ++dst;
                        ++j;
                        continue;
                    }
                    if (sym_ptr->flg_exprs)
                    {
                        src->expr_code = EXPR_LINK;
                        src->expr_expr = sym_ptr->ss_exprs;
                        --i;
                        --src;
                    }
                    else
                    {
                        SEG_struct *sp;
                        if ( (sp = sym_ptr->ss_seg) )
						{
							if (sp->flg_subsect && sp->rel_offset != 0)
							{
								sym_ptr->ss_value += sp->rel_offset;
								sp = sym_ptr->ss_seg = *(seg_list+sp->seg_index);
							}
							if (sp->flg_abs)
							{
								dst->expr_code = EXPR_VALUE;
								dst->expr_value = sym_ptr->ss_value;
							}
							else
							{
								dst->expr_code = EXPR_SEG;
								dst->expr_value = sym_ptr->ss_value;
								dst->expr_seg = sp;
								exptr->base_page_reference |= sp->flg_zero;
							}
							++dst;
							++j;
						}
						else
						{
							sym_ptr->flg_defined = 0;	/* undefine the symbol for future */
							bad_token(NULL,"Fatal internal error. Possible circular symbol assignment.");
							dst->expr_code = EXPR_VALUE;
							dst->expr_value = 0;
							++dst;
							++j;
						}
                    }
                    continue;
                }
                dst->expr_code = EXPR_SYM;
                dst->expr_value = 0;
                dst->expr_sym = sym_ptr;
                ++dst;
                ++j;
                continue;
            }
        case EXPR_OPER: {
                int oper,k;
                oper = src->expr_value;
                op1 = dst -1;       /* point to first argument */
                if (j < 1)
                {        /* always has to be 1 term */
                    j = -1;          /* else underflow */
                    break;
                }
                k = (op1->expr_code == EXPR_VALUE) +
                    (op1->expr_code == EXPR_SEG) * 2;
                if (oper != EXPROPER_COM && 
                    oper != EXPROPER_NEG &&
                    oper != EXPROPER_SWAP &&
                    oper != (EXPROPER_TST | (EXPROPER_TST_NOT<<8)))
                {
                    if (j < 2)
                    {     /* some things need 2 terms */
                        j = -1;
                        break;
                    }
                    op2 = dst -2;        /* point to second argument (if any) */
                    k += (op2->expr_code == EXPR_VALUE) * 4 +
                         (op2->expr_code == EXPR_SEG) * 8;
                }
                else
                {
                    k += 4;          /* pretend second parameter is value */
                }
                if (oper == EXPROPER_DIV && (edmask&ED_USD) != 0)
                {
                    oper = EXPROPER_USD;
                }
                if (k != 5)
                {       /* not simple values */
                    if (oper == EXPROPER_ADD)
                    {
                        if (k == 6)
                        {     /* op1 = seg, op2 = val */
                            op2->expr_value += op1->expr_value;
                            op2->expr_code = op1->expr_code;
                            op2->expr_seg = op1->expr_seg;
#if EXPR_C
                            op2->expr_flags |= op1->expr_flags;
#endif
                            --dst;
                            --j;
                            continue;
                        }
                        if (k == 9)
                        {     /* op1 = val, op2 = seg */
                            op2->expr_value += op1->expr_value;
#if EXPR_C
                            op2->expr_flags |= op1->expr_flags;
#endif
                            --dst;
                            --j;
                            continue;
                        }
                        if (k == 10)
                        {    /* op1 = seg, op2 = seg */
                            op2->expr_value += op1->expr_value;
#if EXPR_C
                            op2->expr_flags |= op1->expr_flags;
                            op1->expr_flags = 0;
#endif
                            op1->expr_value = 0;
                        }         /* fall thru to setup '+' */
                    }
                    if ( oper == EXPROPER_SUB)
                    {
                        if (k == 6)
                        {     /* op1 = seg, op2 = val */
                            op2->expr_value -= op1->expr_value;
                            op1->expr_value = 0;
#if EXPR_C
                            op2->expr_flags |= op1->expr_flags;
#endif
                        }         /* fall through to setup '-' */
                        if (k == 9)
                        {     /* op1 = val, op2 = seg */
                            op2->expr_value -= op1->expr_value;
#if EXPR_C
                            op2->expr_flags |= op1->expr_flags;
#endif
                            --dst;
                            --j;
                            continue;
                        }
                        if (k == 10)
                        {    /* op1 = seg, op2 = seg */
                            op2->expr_value -= op1->expr_value;
#if EXPR_C
                            op2->expr_flags |= op1->expr_flags;
#endif
                            if (op1->expr_seg == op2->expr_seg)
                            { /* if same sec... */
                                op2->expr_code = EXPR_VALUE;    /* then folds to cnst */
                                --dst;
                                --j;
                                continue;
                            }
                            op1->expr_value = 0;
#if EXPR_C
                            op1->expr_flags = 0;
#endif
                        }         /* fall thru to setup '+' */
                    }
                    dst->expr_code = EXPR_OPER;  /* else just pass the oper */
                    dst->expr_value = oper;
                    ++dst;
                    ++j;
                    continue;
                }
                switch (oper&255)
                {      /* simple values, dispatch on code */
                case EXPROPER_TST: {
                        int condit;
                        oper = oper >> 8; /* get what to test */
                        if (oper == EXPROPER_TST_NOT)
                        {
                            if (op1->expr_value == 0)
                            {
                                op1->expr_value = 1;
                            }
                            else
                            {
                                op1->expr_value = 0;
                            }
                            ++dst;     /* compensate for following -- */
                            ++j;
                            break;
                        }
                        if (oper == EXPROPER_TST_OR || oper == EXPROPER_TST_AND)
                        {
                            int o1,o2;
                            o1 = (op1->expr_value != 0) ? 1 : 0;
                            o2 = (op2->expr_value != 0) ? 1 : 0;
                            if (oper == EXPROPER_TST_OR)
                            {
                                op2->expr_value = o1 | o2;
                            }
                            else
                            {
                                op2->expr_value = o1 & o2;
                            }
                            break;
                        }
                        condit = 0;       /* assume false */
                        op2->expr_value -= op1->expr_value;
                        if (oper == EXPROPER_TST_LT)
                        {
                            if (op2->expr_value < 0) condit = 1;
                        }
                        else if (oper == EXPROPER_TST_GT)
                        {
                            if (op2->expr_value > 0) condit = 1;
                        }
                        else if (oper == EXPROPER_TST_EQ)
                        {
                            if (op2->expr_value == 0) condit = 1;
                        }
                        else if (oper == EXPROPER_TST_NE)
                        {
                            if (op2->expr_value != 0) condit = 1;
                        }
                        else if (oper == EXPROPER_TST_LE)
                        {
                            if (op2->expr_value <= 0) condit = 1;
                        }
                        else if (oper == EXPROPER_TST_GE)
                        {
                            if (op2->expr_value >= 0) condit = 1;
                        }
                        op2->expr_value = condit;
#if EXPR_C
                        op2->expr_flags = 0;
#endif
                        break;
                    }
                case EXPROPER_ADD: {
                        op2->expr_value += op1->expr_value;
#if EXPR_C
                        op2->expr_flags |= op1->expr_flags;
#endif
                        break;
                    }
                case EXPROPER_SUB: {
#if defined(MAC68K)
                        if ((op2->expr_flags&EXPR_FLG_REG) != 0 &&
                            (op1->expr_flags&EXPR_FLG_REG) != 0)
                        {
                            unsigned long hi,lo;
                            if (exptr->register_scale != 0)
                            {
                                bad_token((char *)0,"Index SCALE ignored in expression");
                                exptr->register_scale = 0;
                            }
                            if (op2->expr_value > op1->expr_value)
                            {
                                hi = op2->expr_value;
                                lo = op1->expr_value;
                            }
                            else
                            {
                                hi = op1->expr_value;
                                lo = op2->expr_value;
                            }
                            if (hi > 15 || lo > 15)
                            {
                                bad_token((char *)0,"Invalid term in register mask");
                                break;
                            }
                            hi = (1<<hi);
                            hi += hi-1;
                            lo = (1<<lo)-1;
                            op2->expr_value = hi-lo;
                            exptr->register_mask = 1;
                            op2->expr_flags = EXPR_FLG_REGMASK;
                            break;
                        }
#endif
                        op2->expr_value -= op1->expr_value;
                        break;
                    }
                case EXPROPER_MUL: {
#if defined(MAC682K)
                        if (op2->expr_flags != 0)
                        {
                            if (exptr->register_scale != 0)
                            {
                                bad_token((char *)0,"Register term has multiple SCALE factors");
                            }
                            else
                            {
                                if ((op1->expr_flags&(EXPR_FLG_REG|EXPR_FLG_REGMASK)) != 0)
                                {
                                    bad_token((char *)0,"Multiplying two register terms produces useless results");
                                }
                                else
                                {
                                    long rv;
                                    rv = op1->expr_value;
                                    if (op1->expr_code != EXPR_VALUE ||
                                        (rv != 1 && rv != 2 && rv != 4 && rv != 8))
                                    {
                                        bad_token((char *)0,"Index SCALE must be an absolute value of 1,2,4 or 8");
                                    }
                                    else
                                    {
                                        if (rv == 2) exptr->register_scale = 1;
                                        else if (rv == 4) exptr->register_scale = 2;
                                        else exptr->register_scale = 3;
                                    }
                                }
                            }
                            break;
                        }
#endif
                        op2->expr_value *= op1->expr_value;
                        break;
                    }
                case EXPROPER_DIV: {
                        if (op1->expr_value == 0)
                        {
                            err_msg(MSG_WARN,"Attempted divide by 0");
                            if (op2->expr_value > 0)
                            {
                                op2->expr_value = 0x7FFFFFFFl;
                            }
                            if (op2->expr_value < 0)
                            {
                                op2->expr_value = 0x80000000l;
                            }
                        }
                        else
                        {
                            op2->expr_value /= op1->expr_value;
                        }
                        break;
                    }
                case EXPROPER_AND: {
                        op2->expr_value &= op1->expr_value;
                        break;
                    }
                case EXPROPER_OR: {
#if defined(MAC68K)
                        if (op1->expr_flags != 0 && op2->expr_flags != 0)
                        {
                            if (exptr->register_scale != 0)
                            {
                                bad_token((char *)0,"Index SCALE ignored in expression");
                            }
                            if ((op1->expr_flags&EXPR_FLG_REG) != 0)
                            {
                                op1->expr_value = 1<<op1->expr_value;
                            }
                            if ((op2->expr_flags&EXPR_FLG_REG) != 0)
                            {
                                op2->expr_value = 1<<op2->expr_value;
                            }
                            op2->expr_value |= op1->expr_value;
                            op2->expr_flags = EXPR_FLG_REGMASK;
                            exptr->register_mask = 1;
                            break;
                        }
#endif
                        op2->expr_value |= op1->expr_value;
                        break;
                    }
                case EXPROPER_XOR: {
                        op2->expr_value ^= op1->expr_value;
                        break;
                    }
                case EXPROPER_USD: {
                        if (op1->expr_value == 0)
                        {
                            err_msg(MSG_WARN,"Attempted divide by 0");
                            if (op2->expr_value != 0)
                            {
                                op2->expr_value = 0xFFFFFFFFl;
                            }
                        }
                        else
                        {
                            op2->expr_value = 
                            (unsigned long)op2->expr_value/(unsigned long)op1->expr_value;
                        }
                        break;
                    }
                case EXPROPER_MOD: {
                        if (op1->expr_value != 0)
                        {
                            op2->expr_value %= op1->expr_value;
                        }
                        else
                        {
                            err_msg(MSG_WARN,"Attempted modulo by 0");
                        }
                        break;
                    }
                case EXPROPER_SHL: {
                        op2->expr_value <<= op1->expr_value;
                        break;
                    }
                case EXPROPER_SHR: {
                        op2->expr_value >>= op1->expr_value;
                        break;
                    }
                case EXPROPER_SWAP: {
                        int lo,hi;
                        lo = op1->expr_value & 255;
                        hi = (op1->expr_value >> 8) & 255;
                        op1->expr_value = hi | (lo << 8);
                        ++dst;        /* compensate for following -- */
                        ++j;
                        break;
                    }
                case EXPROPER_NEG: {
                        op1->expr_value = -op1->expr_value;
                        ++dst;        /* compensate for following -- */
                        ++j;
                        break;
                    }
                case EXPROPER_COM: {
                        op1->expr_value = ~op1->expr_value;
                        ++dst;        /* compensate for following -- */
                        ++j;
                        break;
                    }
                case EXPROPER_PICK: {
                        EXPR_struct *pck;
                        int pick;
                        pick = j-2;       /* maximum that can be picked */
                        if (op1->expr_value > pick)
                        {
                            sprintf(emsg,"Tried to PICK %ld'th item from an expr stack of %d items",
                                    op1->expr_value, pick);
                            bad_token((char *)0, emsg);
                            op1->expr_value = 0;
                            op1->expr_code = EXPR_VALUE;
                        }
                        else
                        {
                            pck = op2 - op1->expr_value;
                            if (pck->expr_code == EXPR_OPER)
                            {
                                bad_token((char *)0, "Tried to PICK an operator term in expression.");
                                op1->expr_code = EXPR_VALUE;
                                op1->expr_value = 0;
                            }
                            else
                            {
                                op1->expr_code = pck->expr_code;
                                op1->expr_value = pck->expr_value;
                                op1->expr_count = pck->expr_count;
                            }
                        }
                        ++dst;        /* compensate for following -- */
                        ++j;
                        break;
                    }
                case EXPROPER_XCHG: {
                        long tval,tcnt;
                        char tcode;
                        if (op1->expr_code == EXPR_OPER || 
                            op2->expr_code == EXPR_OPER)
                        {
                            bad_token((char *)0, "Tried to XCHG two operators in expression");
                        }
                        else
                        {
                            tval = op2->expr_value;
                            tcnt = op2->expr_count;
                            tcode = op2->expr_code;
                            op2->expr_code = op1->expr_code;
                            op2->expr_value = op1->expr_value;
                            op2->expr_count = op1->expr_count;
                            op1->expr_code = tcode;
                            op1->expr_value = tval;
                            op1->expr_count = tcnt;
                        }
                        ++dst;        /* compensate for following -- */
                        ++j;
                        break;
                    }
                default: {
                        dst->expr_code = src->expr_code;
                        dst->expr_value = src->expr_value;
                        dst->expr_sym = src->expr_sym;
                        ++dst;
                        ++j;
                        continue;
                    }
                }
                --dst;
                --j;
                continue;
            }
        case EXPR_SEG: {
                if (src->expr_seg->flg_abs)
                {   /* if ABS section */
                    dst->expr_code = EXPR_VALUE;
                    dst->expr_seg = 0;
                }
                else
                {
                    dst->expr_code = EXPR_SEG;
                    dst->expr_seg = src->expr_seg;
                    exptr->base_page_reference |= src->expr_seg->flg_zero;
                }
                dst->expr_value = src->expr_value;
                ++dst;
                ++j;
                continue;
            }
        default: {
                sprintf(emsg,"Internal error decoding expression. Code = %02X",
                        src->expr_code);
                err_msg(MSG_FATAL,emsg);
                break;
            }
        }
        if (j < 0)
        {
            if (j == -1)
            {
                err_msg(MSG_WARN,"Expression stack underflow.");
            }
            else
            {
                err_msg(MSG_WARN,"Expression stack overflow.");
            }
            exptr->ptr = 0;
            return 0;
        }
    }
    return(j);
}

static int do_unary(int *sexptr, EXP_stk *eps )
{
    EXPR_struct *expr_ptr;
    char c;

    c = *inp_ptr++;
    c = _toupper(c);
    if ( c == 0 )
    {
        --inp_ptr;        /* don't go off end of rcd */
        bad_token(tkn_ptr,"Premature end of line");
        return(eps->ptr);
    }
    if (c == 'C' || c == '~' || c == '^' || c == 'V')
    {
        if ( get_token() == EOL)
        {
            bad_token(tkn_ptr,"Premature end of line");
            return(eps->ptr);
        }
        if (do_exprs(0,eps) < 0) return(eps->ptr = -1); /* recurse for 1 term */
        expr_ptr = eps->stack + eps->ptr;
        if (c == 'C') token_value = EXPROPER_COM; /* compliment */
        if (c == '~') token_value = EXPROPER_SWAP;    /* swap bytes */
        if (c == 'V')
        {
            expr_ptr->expr_code = EXPR_VALUE;
            (expr_ptr++)->expr_value  = 255;
            ++eps->ptr;
            *sexptr += 1;
            token_value = EXPROPER_AND;        /* low byte */
        }
        if (c == expr_escape)
        {
            expr_ptr->expr_code = EXPR_VALUE;
            (expr_ptr++)->expr_value  = 8;
            ++eps->ptr;
            *sexptr += 1;
            token_value = EXPROPER_SHR;        /* high byte */
        }
        expr_ptr->expr_code = EXPR_OPER;
        expr_ptr->expr_value  = token_value;
        ++eps->ptr;
        *sexptr += 1;
        return(eps->ptr);
    }
    else
    {          /* if not C, ~, ^ or V */
        int save_radix,new_radix;
        new_radix = 0;
        save_radix = current_radix;
        if (c == 'H') new_radix = 16;
        else if (c == 'D') new_radix = 10;
        else if (c == 'X') new_radix = 16;
        else if (c == 'O') new_radix = 8;
        else if (c == 'B') new_radix = 2;
        else if (c == 'W')
        {
            eps->force_short = 1;
            new_radix = -1;
        }
        else if (c == 'L')
        {
            eps->force_long = 1;
            new_radix = -1;
        }
        if (new_radix == 0)
        {
            bad_token(tkn_ptr,"Unrecognised unary operator");
            return(eps->ptr);
        }
        if (new_radix > 0) current_radix = new_radix;
        if ( get_token() == EOL)
        {
            bad_token(tkn_ptr,"Premature end of line");
            current_radix = save_radix;
            return(eps->ptr);
        }
        if (do_exprs(0,eps) < 0)
        {    /* get one term */
            current_radix = save_radix;
            return(eps->ptr = -1);
        }
        current_radix = save_radix;
        *sexptr += 1; /* indicate we have a term */
    }            /* -- one of COBDHX~^V */
    return(eps->ptr);
}           /* --do_unary() */

/*******************************************************************
 * Expression evaluator
 */
int do_exprs( int flag, EXP_stk *eps )
/*
 * At entry:
 *	token_pool has current token, inp_ptr points to next item
 *	flag - 0 if to do only a single term
 * 	eps - pointer to exp_stack
 * At exit:
 *	expression stack filled with expression
 *	returns number of items on expression stack
 */
{
    SS_struct *sym_ptr;
    EXPR_struct *expr_ptr;
    int sexptr = 0;
    int c;
#if EXPR_C
    int expr_flags_init=0;
#endif

    ++exprs_nest;
/*     printf("Into do_exprs(). token=%s, line=%s\n", token_pool, inp_ptr ); */
    while (1)
    {
        if (eps->ptr >= EXPR_MAXDEPTH)
        {
            bad_token(tkn_ptr,"Too many terms in expression");
            return(eps->ptr = -1);
        }
        expr_ptr = eps->stack + eps->ptr; /* compute start place for eval */
#if EXPR_C
        expr_ptr->expr_flags = expr_flags_init;
        expr_flags_init = 0;
#endif
        switch (token_type)
        {
        case TOKEN_local: 
        case TOKEN_strng: {
#ifndef MAC_PP
                if (*token_pool == '.' && token_value == 1)
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
                }
                else
                {
#endif
                    if (token_type == TOKEN_strng) *(token_pool+max_symbol_length) = 0;
                    if ((sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND)) == 0)
                    {  /* process symbol name */
                        return (eps->ptr = -1);
                    }
					if ( sym_ptr->flg_fwd )
						eps->forward_reference = 1;
                    if (sym_ptr->flg_defined)
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
                        expr_ptr->expr_code = EXPR_SYM;
                        expr_ptr->expr_sym = sym_ptr;
                        expr_ptr->expr_value = 0;
                        sym_ptr->flg_ref = 1;     /* signal symbol referenced */
						if ( !pass )
						{
							/* in pass 0, if symbol not defined, then this is a forward reference */
							if ( !sym_ptr->flg_defined )
							{
								sym_ptr->flg_fwd = 1;
								eps->forward_reference = 1;	/* signal this expression contains a forward reference */
							}
						}
#if defined(MAC68K) || defined(MAC682K)
                        if ( !sym_ptr->flg_global && sym_ptr->ss_string[0] == '.' 
                             && ( sym_ptr->ss_string[1] == 'L' || sym_ptr->ss_string[1] == 'l') )
                        {
                            sym_ptr->flg_local = 1; /* make .Lxxx local symbols */
                        }
#endif
                    }
#ifndef MAC_PP
                }
#endif
                ++eps->ptr;
                ++sexptr;
                break;          /* exit from switch */
            }
        case TOKEN_numb: {
                expr_ptr->expr_code = EXPR_VALUE;
                expr_ptr->expr_value  = token_value;
                ++eps->ptr;
                ++sexptr;
                break;          /* exit from switch */
            }
        case TOKEN_oper: {
                int sp_save,oper_save;
                unsigned short ct;

                c = token_value;        /* get current char */
                ct = cttbl[c];      /* get current char type */
                if ( (ct & CT_UOP) )
                {      /* unary operator? */
                    if (sexptr == 0)
                    {   /* is this the first item? */
                        if (c == '+' || c == '-')
                        {   /* is it a plus or minus? */
                            expr_ptr->expr_code = EXPR_VALUE;
                            (expr_ptr++)->expr_value  = 0;
                            ++eps->ptr;
                            ++sexptr;      /* fake it by making a 0 n +/- */
                            goto binary_operators;
                        }
#if EXPR_C
                        if (c == '%')
                        {   /* register indicator? */
                            eps->register_reference |= 1;
                            if (*inp_ptr == '%')
                            { /* double %'s? */
                                bad_token(inp_ptr,"Too many register flags");
                                ++inp_ptr;      /* eat them */
                            }
                            if ( get_token() == EOL)
                            {
                                bad_token(tkn_ptr,"Premature end of line");
                                return(eps->ptr);
                            }
                            expr_flags_init = EXPR_FLG_REG;
                            continue;          /* and loop */
                        }
#endif
                        if (c == expr_escape)
                        {   /* first thing a unary? */
                            if (do_unary(&sexptr,eps) <= 0) return(-1);
                            break;
                        }
                    }
#if EXPR_C
                    if (c == '~' || c == '!')
                    {  /* 1's compliment or negate? */
                        if (c == '!' && *inp_ptr == '=')
                        {
                            if (sexptr == 0)
                            {
                                bad_token(tkn_ptr,"Can't start a term with a relational");
                                return(eps->ptr = -1);
                            }
                            ++inp_ptr;         /* eat the '=' */
                            token_value = EXPROPER_TST | (EXPROPER_TST_NE<<8);
                            goto binary_operators;
                        }
                        if ( get_token() == EOL)
                        {
                            bad_token(tkn_ptr,"Premature end of line");
                            return(eps->ptr);
                        }
                        if (do_exprs(0,eps) < 0) return(eps->ptr = -1); /* recurse for 1 term */
                        expr_ptr = eps->stack + eps->ptr;
                        expr_ptr->expr_code = EXPR_OPER;
                        if (c == '!')
                        {
                            expr_ptr->expr_value = EXPROPER_TST | (EXPROPER_TST_NOT<<8);
                        }
                        else
                        {
                            expr_ptr->expr_value  = c;
                        }
                        ++eps->ptr;
                        ++sexptr;
                        break;             /* exit from switch */
                    }
#endif
                    if (c == '\'')
                    {
                        expr_ptr->expr_code = EXPR_VALUE;
                        if (quoted_ascii_strings)
                        {
                            int ii;
                            expr_ptr->expr_value = 0;
                            for (ii=0; ii<4; ++ii, ++inp_ptr)
                            {
                                c = *inp_ptr;   /* get the char */
                                if (c == '\'') break;
                                if (!isprint(c))
                                {
                                    bad_token(inp_ptr, "Illegal character in string");
                                    ii = -1;
                                    break;
                                }
                                expr_ptr->expr_value <<= 8;
                                expr_ptr->expr_value |= (c&0xFF);
                            }
                            if (ii > 0 && *inp_ptr != '\'')
                            {
                                bad_token(inp_ptr, "Expected a closing quote");
                            }
                            else
                            {
                                ++inp_ptr;      /* eat the closing quote */
                            }
                        }
                        else
                        {
                            inp_ptr = tkn_ptr+1;   /* un-eat any white space */
                            expr_ptr->expr_value  = *inp_ptr++;
                        }
                        ++eps->ptr;
                        ++sexptr;
                        while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
                        break;        /* exit from switch */
                    }
                    if (c == '"')
                    {
                        inp_ptr = tkn_ptr+1;  /* un-eat any white space */
                        expr_ptr->expr_code = EXPR_VALUE;
                        ct = *inp_ptr++;
                        ct = ct + (*inp_ptr++ << 8);
                        expr_ptr->expr_value  = ct;
                        ++eps->ptr;
                        ++sexptr;
                        while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
                        break;        /* exit from switch */
                    }
                    if (c == expr_open)
                    {
#if defined(MAC682K)
                        ++eps->paren_cnt; /* count the paren */
                        if (!squawk_experr)
                        {
                            if (eps->paren_cnt == 1 && (*inp_ptr == '[' || *inp_ptr == ','))
                            {
                                eps->ptr = 0;   /* say there's no terms */
                                flag = 0;   /* pickup no more terms */
                                break;  /* and quit the switch */
                            }
                        }
#endif
                        if (get_token() == EOL)
                        {
                            bad_token(tkn_ptr,"Unbalanced expression nesting");
                            return(eps->ptr = -1);
                        }
                        if (do_exprs(1,eps) < 0) return(eps->ptr = -1); /* recurse */
                        if (*inp_ptr != expr_close)
                        {
#if defined(MAC682K)
                            if (!squawk_experr)
                            {
                                if (eps->paren_cnt == 1 && *inp_ptr == ',')
                                {
                                    flag = 0;    /* pickup no more terms */
                                    break;   /* and quit the switch */
                                }
                            }
#endif
                            bad_token(tkn_ptr,"Unbalanced expression nesting");
                            return(eps->ptr = -1);
                        }
                        while (++inp_ptr,isspace(*inp_ptr)); /* skip over white space */
#if defined(MAC682K) || defined(MAC68K)
                        --eps->paren_cnt; /* found a matching close paren */
                        if (inp_ptr[0] == '.')
                        { /* look for a trailing .W, .B or .L */
                            char fc;
                            int skp=0;
                            fc = _toupper(inp_ptr[1]);
                            if (fc == 'W')
                            {
                                eps->force_short = 1;
                                skp = 2;
                            }
                            else if (fc == 'L')
                            {
                                eps->force_long = 1;
                                skp = 2;
                            }
                            else if (fc == 'B')
                            {
                                eps->force_byte = 1;
                                skp = 2;
                            }
                            inp_ptr += skp;
                        }
                        else
                        {
                            if (eps->register_reference) eps->paren = 1;   /* signal there was a paren involved */
                        }
#endif
                        ++sexptr;     /* signal we have a term */
                        break;        /* exit from switch */
                    }
                }
                else
                {            /* --uop, must be bop */
                    if (sexptr == 0)
                    {   /* is this the first item? */
                        expr_ptr->expr_code = EXPR_VALUE;
                        expr_ptr->expr_value  = 0;
                        ++eps->ptr;
                        ++sexptr;     /* fake it by making a 0 n ? */
                        bad_token(tkn_ptr,"Can't start expression with binary operator");
                    }
                }
#if EXPR_C
                if (c == '>')
                {
                    if (*inp_ptr == '>')
                    {
                        ++inp_ptr;        /* >> is shift right, eat extra char */
                        token_value = EXPROPER_SHR;
                    }
                    else if (*inp_ptr == '=')
                    {
                        ++inp_ptr;         /* >= is gtreq, eat extra char */
                        token_value = EXPROPER_TST | (EXPROPER_TST_GE<<8);           
                    }
                    else
                    {
                        token_value = EXPROPER_TST | (EXPROPER_TST_GT<<8);
                    }
                    goto binary_operators;
                }
                if (c == '<')
                {
                    if (*inp_ptr == '<')
                    {
                        ++inp_ptr;        /* << is shift left, eat extra char */
                        token_value = EXPROPER_SHL;
                    }
                    else if (*inp_ptr == '=')
                    {
                        ++inp_ptr;         /* <= is lteq, eat extra char */
                        token_value = EXPROPER_TST | (EXPROPER_TST_LE<<8);           
                    }
                    else
                    {
                        token_value = EXPROPER_TST | (EXPROPER_TST_LT<<8);
                    }
                    goto binary_operators;
                }
                if (c == '=' && *inp_ptr == '=')
                {
                    ++inp_ptr;       /* == is equal, eat extra char */
                    token_value = EXPROPER_TST | (EXPROPER_TST_EQ<<8);          
                    goto binary_operators;
                }
                if (c == '|' && *inp_ptr == '|')
                {
                    ++inp_ptr;
                    token_value = EXPROPER_TST | (EXPROPER_TST_OR<<8);
                    goto binary_operators;
                }
                if (c == '&' && *inp_ptr == '&')
                {
                    ++inp_ptr;
                    token_value = EXPROPER_TST | (EXPROPER_TST_AND<<8);
                    goto binary_operators;
                }
#endif
                binary_operators:
                while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
                c = *inp_ptr;       /* pickup next char */
                if ((cttbl[c] & (CT_UOP|CT_ALP|CT_NUM)) == 0)
                {
#if defined(MAC682K)
                    if (token_value == '+' && !squawk_experr && eps->paren && !eps->autodec)
                    {
                        eps->autoinc = 1;
                        flag = 0;     /* pickup no more terms */
                        break;        /* and take a normal exit */
                    }
#endif
                    bad_token(tkn_ptr,"Expression syntax error");
                    return(eps->ptr);
                }
                oper_save = token_value;    /* save this operator */
#if !EXPR_C
                if (oper_save == '!') oper_save = EXPROPER_OR;
                if (oper_save == '?') oper_save = EXPROPER_XOR;
                if (oper_save == '{') oper_save = EXPROPER_SHL;
                if (oper_save == '}') oper_save = EXPROPER_SHR;
#else
                if (oper_save == '%') oper_save = EXPROPER_MOD;
#endif
                sp_save = eps->ptr;     /* save current sp */
                if ( get_token() == EOL)
                {
                    bad_token(tkn_ptr,"Premature end of line");
                    return(eps->ptr);
                }
#if defined(MAC68K)
                if (oper_save == '/' && eps->register_reference)
                {
                    if (do_exprs(1,eps) < 0) return(eps->ptr = -1); /* recurse */
                    oper_save = EXPROPER_OR;
                }
                else
                {
                    if (do_exprs(0,eps) < 0) return(eps->ptr = -1); /* recurse for 1 term */
                }
#else
                if (do_exprs(0,eps) < 0) return(eps->ptr = -1); /* recurse for 1 term */
#endif
#if defined(MAC682K)
                if (!squawk_experr && oper_save == '-' && eps->ptr == 2 && eps->register_reference
                    && eps->paren && eps->stack->expr_code == EXPR_VALUE &&
                    eps->stack->expr_value == 0)
                {
                    eps->autodec = 1; /* this is a -(r) autodecrement syntax */
                    --sexptr;    /* take the leading 0 off the top of the stack*/
                    eps->ptr = 1;    /* now there's only 1 term instead of 2 */
                    memcpy(eps->stack,eps->stack+1,sizeof(EXPR_struct)); /* replace the const 0 with reg term */
                    break;       /* fall out of switch statement */
                }
#endif
                expr_ptr = eps->stack + eps->ptr;
                if (sp_save == eps->ptr)
                {
                    inp_ptr = tkn_ptr;
                    return(eps->ptr = -1);
                }
                expr_ptr->expr_code = EXPR_OPER;
                expr_ptr->expr_value  = oper_save;
                ++eps->ptr;
                ++sexptr;
                break;      /* exit from switch */
            }          /* -- TOKEN_oper */
        }             /* -- switch (token_type) */
        c = *inp_ptr;      /* pick up next char */
#if defined(MAC68K) || defined(MAC682K)
/*        printf("Falling out of do_exprs(). token=\"%s\", inp_ptr=\"%s\", rest of line=%s",
            token_pool, inp_ptr, tkn_ptr );                                       */
        if ( dotwcontext && c == '.' )
        {
            /* ("Found a .X at the end of %s. Rest=%s\n", 
                tkn_ptr, inp_ptr ); */                           
            c = inp_ptr[1];
            c = _toupper(c);
            if ( c == 'W' || c == 'L' || c == 'B' )
            {
                if ( c == 'B' )
                {
                    eps->force_byte = 1;
                }
                else if ( c == 'W' )
                {
                    eps->force_short = 1;
                }
                else
                {
                    eps->force_long = 1;
                }
                inp_ptr += 2;
            }
            c = *inp_ptr;
        }
#endif
        if (flag)
        {
            if ( (cttbl[c]&CT_BOP) )
            {
                if ( get_token() == EOL)
                {
                    bad_token(tkn_ptr,"Premature end of line");
                    return(eps->ptr);
                }
                continue;
            }
        }
        --exprs_nest;
        return(eps->ptr);
    }
}

/*******************************************************************
 * Expression evaluator. Calls do_exprs() which recursively calls
 * itself to evaluate an expression. This stub routine checks that
 * the expression is balanced etc.
 */
int exprs( int relative, EXP_stk *eps )
/*
 * At entry:
 *	relative = 0 if expression must be absolute
 *	eps = pointer to expression stack to process
 *	token_pool has first token to process.
 *	inp_ptr points to first char of next term.
 * At exit:
 *	eps->stack filled with RPN expression.
 *	inp_ptr updated to point to first char of next non-expression
 *		term.
 */
{
    char *stp = tkn_ptr;
    exprs_nest = 0;      /* start with no nesting */
    eps->ptr = 0;
    eps->forward_reference = 0;  /* assume no forward ref */
    eps->base_page_reference = 0; /* no base page reference */
    eps->register_reference = 0; /* and no register reference */
#if defined(MAC68K)
    eps->force_short = 0;    /* neither short */
    eps->force_long = 0;     /* nor long */
    eps->register_mask = 0;  /* not a register mask */
#if defined(MAC682K)
    eps->force_byte = 0;
    eps->register_scale = 0;
    eps->paren = 0;
    eps->autodec = 0;
    eps->autoinc = 0;
    eps->paren_cnt = 0;
#endif
#endif
    eps->psuedo_value = 0;   
    if (do_exprs(1,eps) <0)
    {    /* call expression evaluator */
        eps->ptr = 0;     /* bad expression */
        return(-1);
    }
    else
    {
        if (exprs_nest != 0)
        {    /* unbalance */
            bad_token(stp,"Expression is unbalanced");
            eps->ptr = 0;      /* eat the whole expression */
            return(-1);
        }
    }
    if (eps->ptr == 0) return 0;
    eps->ptr = compress_expr(eps); /* squoosh it */
    if (list_bin) compress_expr_psuedo(eps);
    if (relative) return(eps->ptr);
    if (eps->ptr == 1 && eps->stack->expr_code == EXPR_VALUE)
    {
        return(1);
    }
    bad_token(stp,"Expression must resolve to an absolute value");
    return(-1);
}

void dump_expr(int elem)
{
    int i,j;
    EXP_stk *eps;
    EXPR_struct *eptr;
    eps = &exprs_stack[elem];
    eptr = eps->stack;
    j = eps->ptr;
    fprintf(stderr,"\tExpression stack %d:\n",j);
    fprintf(stderr,"\t\tfwd = %d\n\t\tbase = %d\n\t\treg = %d\n",
            eps->forward_reference,eps->base_page_reference,
            eps->register_reference);
#ifdef MAC68K
    fprintf(stderr,"\t\tshort = %d\n\t\tlong = %d\n",eps->force_short,
            eps->force_long);
#endif
    fprintf(stderr,"\t\tpsuedo_value = %08lX\n\t\tExpression: ",eps->psuedo_value);
    for (i=0;i<j;++i,++eptr)
    {
        switch (eptr->expr_code)
        {
        case EXPR_SYM:
            fprintf(stderr,"{%s} ",eptr->expr_sym->ss_string);
            break;
        case EXPR_VALUE:
            fprintf(stderr,"%ld ",eptr->expr_value);
            break;
        case EXPR_OPER:
            if ((eptr->expr_value&255) == EXPROPER_TST)
            {
                fprintf(stderr,"%c%c ",
                        (int)eptr->expr_value,(int)eptr->expr_value>>8);
            }
            else
            {
                fprintf(stderr,"%c ",(int)eptr->expr_value);
            }
            break;
        default:
            fprintf(stderr,"\n\t\tUndefined expr code: %d\n\t\t",eptr->expr_code);
        }
        continue;
    }
    if (eps->tag != 0) fprintf(stderr,":%c %d\n",eps->tag,eps->tag_len);
    return;
}
