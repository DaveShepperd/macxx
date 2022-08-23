/*
    m68types.c - Part of macxx, a cross assembler family for various micro-processors
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
        code emitters
    There are 19 different types of instructions in the 68000.
    An instruction type may ultimately be one of several different
    forms. An instruction type with a number in its name is a basic
    type. If a type routine has a letter in its name it is a sub type.
    Basic types will, if necessary, call subtypes to finish formating
    an instruction.
******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include "token.h"
#include "m68k.h"
#include "exproper.h"
#include "listctrl.h"
#include "pst_tokens.h"

/* extern unsigned short eafield[]; */

static char ea_to_tag[] = {'b','I','L'};
int typeF( unsigned short opcode, EA *tsea, int bwl, EA *tdea);

#define AMFLG_MINUS     (1) /* Have a leading minus: -(x) */
#define AMFLG_DISPLAC   (2) /* Have a displacement: n(x) */
#define AMFLG_OPENPAREN (4) /* Started with an open paren */

#define ONEEA_RET_FAIL (0)
#define ONEEA_RET_SUCC (1)
#if 0
#define M68DBG(x) printf x
#else
#define M68DBG(x) do { ; } while (0)
#endif

static int get_indxea(int amflg, int treg, EA *amp )
{
    EXP_stk *tmps;
    EXPR_struct *exp;
    int ireg;

    M68DBG(("Made it to get_indexea. amflg=%d, treg=%d, tkn_ptr='%s', inp_ptr='%s'\n", amflg, treg, tkn_ptr, inp_ptr));
    tmps = amp->tmps[0];
    exp = tmps->stack;
    tmps->ptr = 0;
    /* All syntaxes end with ,x) or ,x:size) */
    get_token();                /* Prime the pump */
    ireg = exprs(0,tmps);       /* Get index register */
    if ( ireg >= 0 )
        ireg = tmps->stack->expr_value&~(FORCE_WORD|FORCE_LONG);
    tmps->ptr = 0;
    if ( ireg < 0 || !tmps->register_reference || ireg > REG_An+7 )
    {
        bad_token(tkn_ptr,"Index register must be one of D0-D7 or A0-A7");
        return ONEEA_RET_FAIL;
    }
    /* In MIT format, we might be parked on a ':' */
    if ( *inp_ptr == ':' )
    {
        int cc;
        ++inp_ptr;          /* eat size marker */
        cc = _toupper(*inp_ptr);
        if ( cc == 'W' || cc == 'L' )
        {
            if ( cc == 'L' )
                tmps->force_long = 1;
            ++inp_ptr;      /* eat W or L */
        }
        else
        {
            bad_token(inp_ptr,"Expected a W or L here");
            return ONEEA_RET_FAIL;
        }
    }
    amp->reg2 = ((ireg&15)<<4) | ((tmps->force_long) ? 8 : 0);
    /* In all syntaxes, we should be parked on a closing paren */
    if ( *inp_ptr != ')' )
    {
        bad_token(inp_ptr,"Expected a ')' here");
        return ONEEA_RET_FAIL;
    }
    ++inp_ptr;              /* eat closing paren */
    if (treg == REG_PC)
    {
        amp->eamode = Ea_PCN;
        amp->mode = E_PCN;
    }
    else
    {
        amp->reg1 = treg&7;
        amp->eamode = Ea_NDX;
        amp->mode = E_NDX;
    }
    tmps = amp->exp;
    tmps->tag = 's';     /* indexed displacements are signed bytes */
    exp = tmps->stack;
    if ( !(amflg&AMFLG_DISPLAC) )
    {
        exp = tmps->stack;
        tmps->ptr = 1;
        exp->expr_value = 0;
        exp->expr_code = EXPR_VALUE;
    }
    else
    {
	if ( treg == REG_PC )
	{
	    tmps->ptr = compress_expr(tmps);
	    if ( tmps->ptr != 1 || exp->expr_code != EXPR_VALUE )
	    {
	        exp = tmps->stack + tmps->ptr;
	        exp->expr_code = EXPR_SEG;
	        exp->expr_value = current_offset+2;
/*			printf("In get_indexea(). current_offset=0x%08lX\n", current_offset ); */
	        (exp++)->expr_seg = current_section;
	        exp->expr_code = EXPR_OPER;
	        exp->expr_value = '-';
	        tmps->ptr += 2;
	        tmps->ptr = compress_expr(tmps);
	        if (list_bin) compress_expr_psuedo(tmps);
	    }
	}
        if (tmps->ptr == 1 && exp->expr_code == EXPR_VALUE)
        {
            if (exp->expr_value < -128 || exp->expr_value > 127)
            {
                bad_token((char *)0,"Displacement offset s/b -128 <= disp <= 127");
                exp->expr_value = 0;
            }
        }
    }
    M68DBG(("Exit from get_indexea() with ONEEA_RET_SUCC (%d)\n", ONEEA_RET_SUCC));
    return ONEEA_RET_SUCC;
}

static int get_mitsyntax( int areg, EA *amp )
{
    int legit = 0, amflg=0;
    char *cp, epsTag='I';
    EXP_stk *eps;

	M68DBG(("Made it to get_mitsyntax. areg=%d, tkn_ptr='%s', inp_ptr='%s'\n", areg, tkn_ptr, inp_ptr));
    /* Could be r@ or r@- or r@+ or r@num or r@(num) or r@(num,x) or r@(num,x:size) */
    /* areg has r, eps is empty, inp_ptr points to the @ */
    ++inp_ptr;      /* Eat @ */
    if ( (areg >= REG_An && areg <= REG_An+7) || areg == REG_PC )
        legit = 1;
    if ( (*inp_ptr != '+' && *inp_ptr != '-') && areg == REG_PC )
        legit = 1;
    if ( !legit )
    {
        bad_token(tkn_ptr,"Register indirect only valid with A0-A7 or PC");
        return ONEEA_RET_FAIL;
    }
    if ( *inp_ptr == '+' )
    {
        /* Postincrement */
        ++inp_ptr;  /* Eat + */
        amp->reg1 = areg&7;
        amp->eamode = Ea_ANI;
        amp->mode = E_ANI;
        return ONEEA_RET_SUCC;
    }
    if ( *inp_ptr == '-' )
    {
        /* Predecrement */
        ++inp_ptr;  /* Eat - */
        amp->reg1 = areg&7;
        amp->eamode = Ea_DAN;
        amp->mode = E_DAN;
        return ONEEA_RET_SUCC;
    }
    if ( *inp_ptr == '(' )
    {
        cp = inp_ptr;           /* remember this incase of rescan */
        amflg = AMFLG_OPENPAREN; /* remember we started with an open paren */
        ++inp_ptr;              /* eat ( */
    }
    eps = amp->exp;
    get_token();                /* prime the pump */
    if ( exprs(1,eps) < 0 )
    {
        return ONEEA_RET_FAIL;
    }
    if ( (amflg&AMFLG_OPENPAREN) ) /* If we had an open paren */
    {
        if (  *inp_ptr == ',' )  /* and stopped on a comma */
        {
            /* Is a r@(num,x) */
            ++inp_ptr;              /* eat it */
            amflg |= AMFLG_DISPLAC;
            return get_indxea(amflg,areg,amp);
        }
        inp_ptr = cp;               /* else we have to rescan the expression */
        if ( exprs(1,eps) < 0 )
        {
            return ONEEA_RET_FAIL;
        }
    }
    /* It's a r@num  */
    if (areg == REG_PC)
    {
		EXPR_struct *exp;

        amp->mode = E_PCR;
        amp->eamode = Ea_PCR;
		eps->ptr = compress_expr(eps);
		exp = eps->stack+eps->ptr;
		if ( eps->ptr != 1 || exp->expr_code != EXPR_VALUE )
		{
			epsTag = 'I';  /* displacements are signed shorts */
			exp = eps->stack + eps->ptr;
			exp->expr_code = EXPR_SEG;
			exp->expr_value = current_offset+2;
/*			printf("In get_mitsyntax(). current_offset=0x%08lX\n", current_offset ); */
			(exp++)->expr_seg = current_section;
			exp->expr_code = EXPR_OPER;
			exp->expr_value = '-';
			eps->ptr += 2;
			eps->ptr = compress_expr(eps);
			if (list_bin) compress_expr_psuedo(eps);
		}
    }
    else
    {
        amp->mode = E_DSP;
        amp->eamode = Ea_DSP;
        amp->reg1 = areg&7;
    }
    eps->tag = epsTag;     /* displacements are either signed words or signed bytes */
    return ONEEA_RET_SUCC;
}

static int get_regea(int amflg, EA *amp )
{
    EXP_stk *eps, *tmps;
    EXPR_struct *exp;
    char *cp, epsTag='I';
    int treg;
    int legit = 0;

    eps = amp->exp;
    exp = eps->stack;
    tmps = amp->tmps[0];

    /* Could be r or (r) or -(r) or (r)+ or num(r) or num(r,x) */
    /* or (num,r) or (num,r,x) */
    /* or all permutations of r@ */
    /* eps has either r or num */
    cp = inp_ptr;
    M68DBG(("Reached get_regea. amflg=%d. tkn_ptr='%s', inp_ptr='%s'\n", amflg, tkn_ptr, inp_ptr ));
    if ( (amflg&AMFLG_DISPLAC) )
    {
        /* Could be num(r) or (num,r) or num(r,x) or (num,r,x) */
        /* eps has the num in it. We don't yet have our r */
        get_token();
        if ( exprs(0,tmps) < 0 )
        {
            return ONEEA_RET_FAIL;
        }
        /* Now r is in tmps */
        treg = tmps->stack->expr_value & ~(FORCE_WORD|FORCE_LONG);
        /* treg now has the register, eps has the displacement */
        if ( treg >= REG_An && treg <= REG_An+7 )
            legit = 1;
        if ( (amflg&(AMFLG_MINUS|AMFLG_DISPLAC)) == (AMFLG_DISPLAC) && treg == REG_PC )
            legit = 1;
        if ( !legit )
        {
            bad_token(tkn_ptr,"Register indirect only valid with A0-A7 or PC");
            return ONEEA_RET_FAIL;
        }
        tmps->ptr = 0;
        if ( *inp_ptr == ',' )
        {
            ++inp_ptr;          /* eat comma */
            return get_indxea(amflg,treg,amp);      /* Process index register */
        }
        if ( *inp_ptr != ')' )
        {
            bad_token(inp_ptr,"Expected ')' here");
            return ONEEA_RET_FAIL;
        }
        ++inp_ptr;              /* eat closing paren */
        if (treg == REG_PC)
        {
            amp->mode = E_PCR;
            amp->eamode = Ea_PCR;
			exp = eps->stack+eps->ptr;
			if ( eps->ptr != 1 || exp->expr_code != EXPR_VALUE )
			{
	
				epsTag = 'I';  /* displacements are signed shorts */
				/* This one is for num(pc) or pc relative */
				exp->expr_code = EXPR_SEG;
				exp->expr_value = current_offset+2;
/*				printf("In get_regea(). current_offset=0x%08lX\n", current_offset ); */
				(exp++)->expr_seg = current_section;
				exp->expr_code = EXPR_OPER;
				exp->expr_value = '-';
				eps->ptr += 2;
				eps->ptr = compress_expr(eps);
				if (list_bin) compress_expr_psuedo(eps);
			}
        }
        else
        {
            amp->mode = E_DSP;
            amp->eamode = Ea_DSP;
            amp->reg1 = treg&7;
        }
        eps->tag = epsTag;     /* displacements are signed words */
        M68DBG(("At FIXME. treg=%d, ptr=%d, disp=%ld, tkn_ptr=%s",
               treg, eps->ptr, exp->expr_value, tkn_ptr ));
        return ONEEA_RET_SUCC;
    }
    /* Could be r or (r) or -(r) or (r)+ or (r,x) */
    /* or all permutations of r@ */
    /* r is in eps */
    if ( eps->ptr != 1 || exp->expr_code != EXPR_VALUE )
    {
        bad_token(tkn_ptr,"Not an absolute register expression");
        return ONEEA_RET_FAIL;
    }
    treg = exp->expr_value & ~(FORCE_WORD|FORCE_LONG);
    eps->ptr = 0;           /* clobber this expression */
    if ( *inp_ptr == '@' )
    {
        if ( amflg )
        {
            bad_token(inp_ptr, "Operand syntax error. No '-' or '(' expected with this" );
            return ONEEA_RET_FAIL;
        }
        return get_mitsyntax(treg,amp);
    }
    /* Could be r or (r) or -(r) or (r)+ or (r,x) */
    /* r is in treg */
    if ( (amflg&(AMFLG_OPENPAREN|AMFLG_MINUS)) )
    {
        /* Could be (r) or -(r) or (r)+ or (r,x) */
        if ( *inp_ptr != ',' && *inp_ptr != ')' )
        {
            bad_token(inp_ptr, "Operand syntax error. Expected a ',' or ')' here." );
            return ONEEA_RET_FAIL;
        }
        if ( treg >= REG_An && treg <= REG_An+7 )
            legit = 1;
        if ( !(amflg&AMFLG_MINUS) && treg == REG_PC )
            legit = 1;
        if ( !legit )
        {
            bad_token(tkn_ptr,"Register indirect only valid with A0-A7 or PC");
            return ONEEA_RET_FAIL;
        }
        if ( *inp_ptr == ')' )
        {
            ++inp_ptr;      /* Eat ) */
            if ( *inp_ptr == '+' )
            {
                if ( (amflg&AMFLG_MINUS) )
                {
                    bad_token(inp_ptr,"Cannot have both predecrement and post increment");
                    return ONEEA_RET_FAIL;
                }
                /* Postincrement */
                ++inp_ptr;  /* Eat + */
                amp->reg1 = treg&7;
                amp->eamode = Ea_ANI;
                amp->mode = E_ANI;
                return ONEEA_RET_SUCC;
            }
            if ( (amflg&AMFLG_MINUS) )
            {
                amp->reg1 = treg&7;
                amp->eamode = Ea_DAN;
                amp->mode = E_DAN;
                return ONEEA_RET_SUCC;
            }
            /* Just (r) */
            if ( treg == REG_PC )
            {
                /* r == pc is special case */
                /* Fake a 0(r) */
                eps->ptr = 1;
                exp->expr_value = 0;
                exp->expr_code = EXPR_VALUE;
                amp->mode = E_PCR;
                amp->eamode = Ea_PCR;
            }
            else
            {
                amp->mode = E_ATAn;
                amp->eamode = Ea_ATAn;
                amp->reg1 = treg&7;
            }
            return ONEEA_RET_SUCC;
        }
        /* Is (r,x) */
        ++inp_ptr;          /* Eat comma */
        return get_indxea(0,treg,amp);
    }
    /* Could be r or mask */
    /* r is in treg, mask is in eps */
    if (eps->register_mask)
    {
        amp->mode = E_RLST;
    }
    else if (treg < REG_An+0)
    {
        amp->reg1 = treg&7;
        amp->mode = E_Dn;
        amp->eamode = Ea_Dn;
    }
    else if (treg < REG_An+8)
    {
        amp->reg1 = treg&7;
        amp->mode = E_An;
        amp->eamode = Ea_An;
    }
    else if (treg == REG_SR)
    {
        amp->mode = E_SR;
    }
    else if (treg == REG_USP)
    {
        amp->mode = E_USP;
    }
    else if (treg == REG_CCR)
    {
        amp->mode = E_CCR;
    }
    else
    {
        M68DBG(("Failed to find register type. treg=0x%08X. inp_ptr=%s",
                treg, inp_ptr));
        bad_token(cp,"Not a valid register in this context");
    }
    return ONEEA_RET_SUCC;
}

/* Common to all syntaxes:
 * Dn - data register
 * An - Address register
 * #d - immediate
 * d - absolute address (converted to pc relative if .enabl PC_RELATIVE)
 * r-r - register mask
 *
 * Atari syntax:
 * (An) - Address indirect
 * (An)+ - Address indirect, auto-increment
 * -(An) - auto-decrement, Adress indirect
 * d(An) - Address indirect + displacement
 * d(An,Dn) - Address indirect + displacement + index
 *
 * Motorola syntax:
 * (An) - Address indirect
 * (An)+ - Address indirect, auto-increment
 * -(An) - auto-decrement, Adress indirect
 * (d,An) - Address indirect + displacement
 * (d,An,Xn) - Adress indirect + displacement + index
 * (num).W or (num).L for forced word or long displacement
 *
 * MIT syntax (produced by gcc; but gcc also emits Motorola (like) syntax for some instructions):
 * An@ - Address indirect
 * An@- - Auto-decrement, address indirect
 * An@+ - Address indirect, auto-increment
 * Apc@(num) - Address indirect + displacement
 * Apc@(num,reg[:size]) - Index + displacement (optional size is one of w or l
 * num.W or num.L for forced word or long displacement
 * (Apc = any address register or PC)
 */

int get_oneea( EA *amp, int bwl )
{
    EXP_stk *eps;
    EXPR_struct *exp;
    int amflg;
    char *cp;

	M68DBG(("Made it to get_oneea. bwl=%d, tkn_ptr='%s', inp_ptr='%s'\n", bwl, tkn_ptr, inp_ptr));
    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
        ++inp_ptr;      /* Eat WS */
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
        return 0;       /* At EOL */
    eps = amp->exp;
    eps->tag = 'I';     /* assume word mode extra bytes */
    eps->tag_len = 1;    
    eps->forward_reference = 0;
    eps->register_reference = 0;
    eps->register_mask = 0;
    eps->psuedo_value = 0;
    exp = eps->stack;
    amp->tmps[0]->ptr = 0;  /* make sure these are empty too */
    amp->tmps[1]->ptr = 0;
    amflg = 0;          /* no special flags present */
    if (*inp_ptr == '#')
    {  /* immediate? */
        ++inp_ptr;      /* yep, eat the char */
        get_token();        /* prime the pump */
        exprs(1,eps);       /* pickup the expression */
        amp->mode = E_IMM;
        amp->eamode = Ea_IMM;
        amp->exp->tag = ea_to_tag[bwl];
        return ONEEA_RET_SUCC;
    }
    cp = inp_ptr;       /* remember where we started */
    if ( *inp_ptr == '-' )
    {
        ++inp_ptr;      /* eat - */
        amflg = AMFLG_MINUS;    /* remember we started with a -( */
        if ( !options[QUAL_GRNHILL] )
        {
            while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
                ++inp_ptr;      /* Eat WS */
        }
        if ( *inp_ptr != '(' )
        {
            amflg = 0;
            inp_ptr = cp;
        }
    }
    if ( *inp_ptr == '(' )
    {
        ++inp_ptr;
        amflg |= AMFLG_OPENPAREN;
        if ( !options[QUAL_GRNHILL] )
        {
            while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
                ++inp_ptr;      /* Eat WS */
        }
    }
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
        return ONEEA_RET_FAIL;       /* At EOL */
    get_token();        /* Prime the pump */
    if ( exprs(1,eps) < 0 )
    {
        return ONEEA_RET_FAIL;      /* Syntax error */
    }
    M68DBG(("After exprs(). amflg=%d, register_reference=%d, ptr=%d, val=%ld, inp_ptr='%s'\n",
            amflg, eps->register_reference, eps->ptr, exp->expr_value, inp_ptr ));
    if ( eps->register_reference )
    {
        return get_regea(amflg,amp);    /* expression is a register */
    }
    /* It's not a register expression so it could be num or num() or a (num,) */
    if ( *inp_ptr == ',')
    {
        if ( (amflg&(AMFLG_OPENPAREN|AMFLG_MINUS)) == AMFLG_OPENPAREN )
        {
            /* It's a (num,) which is Motorola syntax */
            amflg |= AMFLG_DISPLAC;
            ++inp_ptr;          /* eat the comma */
            return get_regea(amflg,amp);
        }
        if ( (amflg&AMFLG_MINUS) )
        {
            /* Is -(num,) which is illegal */
            bad_token(tkn_ptr,"Operand syntax error");
            return ONEEA_RET_FAIL;
        }
    }
    /* Could be num or num() or (num */
    if ( (amflg&(AMFLG_MINUS|AMFLG_OPENPAREN)) )
    {
        /* Since we've clipped off a minus and/or an open paren.. */
        inp_ptr = cp;       /* We need to rescan and redo the entire expression */
        eps->ptr = 0;
        get_token();                /* reprime the pump */
        M68DBG(("Rescan: amflg=%d, token_pool=\"%s\", inp_ptr=%s",
                amflg, token_pool, inp_ptr));
        if ( exprs(1,eps) < 0 )     /* reprocess the expression */
        {
            M68DBG(("rescan exprs() failed. inp_ptr=%s", inp_ptr));
            return ONEEA_RET_FAIL;
        }
        amflg = 0;                  /* just cleanup */
        M68DBG(("rescan exprs() success. ptr=%d, val=%ld, inp_ptr=%s",
                eps->ptr, exp->expr_value, inp_ptr));
    }
    eps->force_short |= eps->base_page_reference;
    if ( *inp_ptr == '(' )
    {
        /* It could be num() Atari or Motorola syntax */
        ++inp_ptr;                  /* eat ( */
        amflg = AMFLG_DISPLAC|AMFLG_OPENPAREN;
        return get_regea(amflg,amp);
    }
    /* It can only be num at this point */
    eps->tag_len = 1;
    if ((edmask&ED_PCREL) != 0)
    {
        amp->mode = E_PCR;
        amp->eamode = Ea_PCR;
        eps->tag = 'I';  /* displacements are signed words */
        exp = eps->stack + eps->ptr;
        exp->expr_code = EXPR_SEG;
        exp->expr_value = current_offset+2;
/*		printf("In get_oneea(). current_offset=0x%08lX\n", current_offset ); */
        (exp++)->expr_seg = current_section;
        exp->expr_code = EXPR_OPER;
        exp->expr_value = '-';
        eps->ptr += 2;
        eps->ptr = compress_expr(eps);
        if (list_bin) compress_expr_psuedo(eps);
    }
    else
    {
        amp->mode = E_ABS;
        if (!eps->force_long && (eps->force_short || (eps->ptr == 1 &&
                                                      exp->expr_code == EXPR_VALUE &&
                                                      exp->expr_value >= -32678 &&
                                                      exp->expr_value < 32768)))
        {
            eps->tag = 'I';
            amp->eamode = Ea_ABSW;
        }
        else
        {
            eps->tag = 'L';
            amp->eamode = Ea_ABSL;
        }
    }
    return ONEEA_RET_SUCC;
}

static int get_twoea(int bwl)
{
    int sts;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
    {
        sts = get_oneea(&source,bwl);
        M68DBG(("Made it to get_twoea(). Called get_oneea() which returned %d. Source reg=%d, eamode=%d, mode=%d. inp_ptr=%s\n",
                sts, source.reg1, source.eamode, source.mode, inp_ptr ));
        if ( sts == ONEEA_RET_SUCC )
        {
            if (*inp_ptr == ',')
            {
                ++inp_ptr;
                sts = get_oneea(&dest,bwl);
                if ( sts == ONEEA_RET_SUCC )
				{
					M68DBG(("After second call to get_oneea() returned %d. Dest reg=%d, eamode=%d, mode=%d. inp_ptr=%s\n",
							sts, dest.reg1, dest.eamode, dest.mode, inp_ptr ));
                    return 1;
				}
            }
            else
            {
                if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
                {
                    show_bad_token(inp_ptr,"Expected a comma here, one assumed",MSG_WARN);
                    if (get_oneea(&dest,bwl) == ONEEA_RET_SUCC) return 1;
                }
            }
        }
    }
    bad_token(inp_ptr,"Instruction requires two operands");
    return 0;
}

static void bad_sourceam( void )
{
    bad_token((char *)0,"Invalid address mode for first operand");
    source.mode = E_Dn;
    source.eamode = Ea_Dn;
    source.reg1 = 0;
    source.exp->ptr = 0;
    source.exp->stack->expr_code = EXPR_VALUE;
    source.exp->stack->expr_value = 0;
    source.exp->tag = 'W';
    source.exp->tag_len = 1;
    return;
}

static void bad_destam( void )
{
    bad_token((char *)0,"Invalid address mode for second operand");
    dest.mode = E_Dn;
    dest.eamode = Ea_Dn;
    dest.reg1 = 0;
    dest.exp->ptr = 0;
    dest.exp->stack->expr_code = EXPR_VALUE;
    dest.exp->stack->expr_value = 0;
    dest.exp->tag = 'W';
    dest.exp->tag_len = 1;
    return;
}

static int immediate_mode(unsigned short opcode,int bwl)
{
    if ((source.mode&E_IMM) == 0) bad_sourceam();
    EXP0SP->expr_value = opcode | (bwl<<6) | dest.eamode | dest.reg1;
    EXP0.ptr = 1;
    source.exp->tag = ea_to_tag[bwl&3];
    if (source.exp->ptr < 1) source.exp->ptr = 1;
    return 1;
}

/*****************************************************************************
        type0
    Type 0.
    Instructions:
    ABCD,CMPM,SBCD,ADDX,SUBX
*****************************************************************************/
int type0(int inst, int bwl)
{
    int rm;
    static unsigned short optabl0[] = {
        0xc100,     /* ABCD */
        0xb108,     /* CMPM */
        0x8100,     /* SBCD */
        0xd100,     /* ADDX */
        0x9100};    /* SUBX */

    if (get_twoea(bwl) == 0)
    {
        rm = 0;
    }
    else
    {
        rm = bwl << 6;
        if (inst == 2)
        {
            if (source.mode != E_ANI || dest.mode != E_ANI)
            {
                bad_token((char *)0,"Source and dest can only be (An)+");
                source.mode = dest.mode = E_ANI;
            }
        }
        else
        {
            if ((source.mode&~(E_Dn|E_DAN)) != 0 ||
                (source.mode&(E_Dn|E_DAN)) == 0)
            {
                bad_sourceam();
            }
            rm |= (source.mode == E_Dn) ? 0 : 8;
        }
        if (source.mode != dest.mode)
        {
            bad_token((char *)0,"Source and dest modes must both be the same");
        }
    }
/*        9    4    0
 * __________________
 * |   | Rd|  |m| Rs|
 * ------------------
 */
    EXP0SP->expr_value = optabl0[inst-1]|rm|(dest.reg1<<9)|source.reg1;
    EXP1.ptr = EXP2.ptr = EXP3.ptr = 0;
    return 1;
}

/******************************************************************************
        type1
    type 1
    instructions:
    ADD,SUB,ADDA,SUBA,ADDI,SUBI,ADDQ,SUBQ
******************************************************************************/
int type1(int inst, int bwl)
{
    EXP_stk *eps;
    EXPR_struct *eptr;
    static unsigned short optabl1[] = {
        0xD000,     /* add */
        0xD000,     /* adda */
        0x0600,     /* addi */
        0x5000,     /* addq */
        0x9000,     /* sub */
        0x9000,     /* suba */
        0x0400,     /* subi */
        0x5100      /* subq */
    };
    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = optabl1[(int)inst];
        EXP0.ptr = 1;
        return 0;
    }
    eps = source.exp;
    eptr = eps->stack;
    if (bwl == 0)
    {
        if (source.eamode == E_An || dest.eamode == Ea_An)
        {
            show_bad_token((char *)0,"Byte operations are not allowed on address registers",MSG_WARN);
            bwl = 1;
        }
    }
    if ((inst&3) == 0)
    {        /* generic add/sub */
        if ((source.mode&E_IMM) != 0)
        { /* immediate mode? */
            if (!(eps->force_short|eps->force_long) && eps->ptr == 1 && eptr->expr_code == EXPR_VALUE &&
                (eptr->expr_value > 0 && eptr->expr_value <= 8))
            {
                inst |= 3;      /* switch to addQ/subQ */
            }
            else
            {
                inst = (inst&4) + 1;    /* assume ADDA/SUBA */
                if (dest.mode != E_An) ++inst;  /* switch to ADDI/SUBI */
            }
        }
        else
        {            /* -- if (immediate mode) */
            if (dest.mode == E_An)
            {
                inst = (inst&4) + 1;    /* switch to ADDA/SUBA */
            }
            else
            {
                if ((dest.mode&E_Dn) == 0)
                {
                    if ((dest.mode&~E_AMA) != 0 || ((dest.mode&E_AMA) == 0) )
                    {
                        bad_destam();
                    }
                }
                if ((dest.mode&E_Dn) != 0)
                {
                    if ((source.mode&~E_ALL) != 0 || ((source.mode&E_ALL) == 0))
                    {
                        bad_sourceam();
                    }
                }
                else
                {
                    if ((source.mode&E_Dn) == 0) bad_sourceam();
                    bwl |= 4;
                }
                EXP0SP->expr_value = optabl1[(int)inst] | (bwl<<6);
                if ((bwl&4) != 0 || bwl == 3)
                {
                    EXP0SP->expr_value |= (source.reg1<<9) | (dest.eamode+dest.reg1);
                }
                else
                {
                    EXP0SP->expr_value |= (dest.reg1<<9) | (source.eamode+source.reg1);
                }
                EXP0.ptr = 1;
                return 1;
            }
        }
    }
    switch (inst&7)
    {
    case 1:         /* adda */
    case 5: {       /* suba */
            if ((source.mode&~E_ALL) != 0 || (source.mode&E_ALL) == 0)
            {
                bad_sourceam();
            }
            if ((dest.mode&E_An) == 0)
            {
                bad_destam();
            }
            bwl = (bwl == 1) ? 3 : 7;
            EXP0SP->expr_value = optabl1[(int)inst] | (bwl<<6) | (dest.reg1<<9) | source.eamode | source.reg1;
            EXP0.ptr = 1;
            return 1;
        }
    case 2:         /* addi */
    case 6: {       /* subi */
            if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
            immediate_mode(optabl1[(int)inst],bwl);
            return 1;
        }
    case 3:         /* addq */
    case 7: {       /* subq */
            if ((source.mode&E_IMM) == 0) bad_sourceam();
            if ((dest.mode&(E_AMA|E_Dn|E_An)) == 0) bad_destam();
            typeF(optabl1[(int)inst],&source,bwl,&dest);
            return 1;
        }           /* -- case 3/7	*/
    }               /* -- switch	*/
    return 0;
}               /* -- type1()	*/

/******************************************************************************
        type2
    type 2
    instructions:
    ANDI,EORI,AND,EOR,OR,ORI
******************************************************************************/
int type2( int inst, int bwl)
{
    static unsigned short optabl2[] = {
        0xc000,         /* and  */
        0x0200,         /* andi */
        0xB100,         /* eor  */
        0x0A00,         /* eori */
        0x8000,         /* or   */
        0x0000          /* ori  */
    };

    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = optabl2[(int)inst];
        EXP0.ptr = 1;
        return 0;
    }
    if ((inst&1) == 0)
    {        /* if generic and/eor/or */
        if ((source.mode&E_IMM) != 0)
        { /* immediate mode? */
            inst |= 1;          /* switch to ANDI/EORI/ORI */
        }
        else
        {            /* -- immediate mode */
            if ((dest.mode&E_Dn) == 0)
            {
                if ((dest.mode&~E_AMA) != 0 || ((dest.mode&E_AMA) == 0) )
                {
                    bad_destam();
                }
            }
            if ((inst&~1) == 2)
            {       /* eor only allows Dn source */
                if ((source.mode&E_Dn) == 0) bad_sourceam();
                if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
                bwl |= 4;            /* make it <ea>?Dn -> <ea> */
            }
            else
            {
                if ((dest.mode&E_Dn) != 0)
                {
                    if ((source.mode&~(E_ALL-E_An)) != 0 ||
                        ((source.mode&(E_ALL-E_An)) == 0))
                    {
                        bad_sourceam();
                    }
                }
                else
                {
                    if ((source.mode&E_Dn) == 0) bad_sourceam();
                    bwl |= 4;
                }
            }
            EXP0SP->expr_value = optabl2[(int)inst] | (bwl<<6);
            if ((bwl&4) != 0 || bwl == 3)
            {
                EXP0SP->expr_value |= (source.reg1<<9) | (dest.eamode+dest.reg1);
            }
            else
            {
                EXP0SP->expr_value |= (dest.reg1<<9) | (source.eamode+source.reg1);
            }
            EXP0.ptr = 1;
            return 1;
        }
    }
    if ((source.mode&E_IMM) == 0)
    {     /* has to be immediate mode */
        bad_sourceam();
        source.exp->ptr = 1;            /* make it immediate */
        inst |= 1;
    }
    if ((dest.mode&(E_CCR|E_SR)) != 0)
    {    /* special case output? */
        static unsigned short optabl2s[] = {
            0x023c,     /* and #n,CCR */
            0x027c,     /* and #n,SR  */
            0x0a3c,     /* eor #n,CCR */
            0x0a7c,     /* eor #n,SR  */
            0x003c,     /* or  #n,CCR */
            0x007c      /* or  #n,SR  */
        };
        if ((dest.mode&E_CCR) != 0) inst &= ~1;
        if ((inst&1) == 0)
        {
            if (bwl != 0) show_bad_token((char *)0,"Only byte operations performed on CCR",MSG_WARN);
            source.exp->tag = 'b';
        }
        else
        {
            if (bwl != 1) show_bad_token((char *)0,"Only word operations performed on SR",MSG_WARN);
            source.exp->tag = 'W';
        }       
        EXP0SP->expr_value = optabl2s[(int)inst];
        EXP0.ptr = 1;
        return 1;
    }               /* -- if (special registers) */
    if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
    EXP0SP->expr_value = optabl2[(int)inst] | (bwl<<6) | (dest.eamode+dest.reg1);
    EXP0.ptr = 1;
    return 1;
}

/******************************************************************************
        type3
    type 3
    Instructions
    ASL,ASR,LSL,LSR,ROL,ROR,ROXL,ROXR
******************************************************************************/
int type3( int inst, int bwl )
{
    static unsigned short optabl3m[] = {
        0xe1c0,     /* asl m */
        0xe0c0,     /* asr m */
        0xe3c0,     /* lsl m */
        0xe2c0,     /* lsr m */
        0xe7c0,     /* rol m */
        0xe6c0,     /* ror m */
        0xe5c0,     /* roxl m */
        0xe4c0};    /* roxr m */

    static unsigned short optabl3r[] = {
        0xe100,     /* asl */
        0xe000,     /* asr */
        0xe108,     /* lsl */
        0xe008,     /* lsr */
        0xe118,     /* rol */
        0xe018,     /* ror */
        0xe110,     /* roxl */
        0xe010};    /* roxr */


    dest.mode = E_NUL;  /* incase there's only 1 operand */
    if (get_oneea(&source,bwl) != ONEEA_RET_SUCC)
    {
        bad_token((char *)0,"Requires at least one operand");
        source.mode = E_Dn;     /* pretend it's D0 */
    }
    else
    {
        if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
        {
            if (*inp_ptr != ',')
            {
                show_bad_token(inp_ptr,"Comma expected here, one assumed",MSG_WARN);
            }
            else
            {
                ++inp_ptr;
            }
            get_oneea(&dest,bwl);
        }
    }
    if (dest.mode == E_NUL)
    {       /* one operand mode */
        if ((source.mode&~(E_AMA|E_Dn)) != 0 || (source.mode&(E_AMA|E_Dn)) == 0)
        {
            bad_sourceam();
        }
        if ((source.mode&E_Dn) != 0)
        {  /* one operand data reg? */
            EXP0SP->expr_value = optabl3r[(int)inst]|(1<<9)|(bwl<<6)|source.reg1;
        }
        else
        {
            if (bwl != 1)
            {
                show_bad_token((char *)0,"Only words can be shifted with this address mode",MSG_WARN);
            }
            EXP0SP->expr_value = optabl3m[(int)inst] | source.eamode | source.reg1;
        }
        EXP0.ptr = 1;
        return 1;
    }               /* -- one operand mode */
    if ((dest.mode&E_Dn) == 0) bad_destam();    /* dest must be Dn */
    if ((source.mode&E_IMM) != 0)
    { /* source immediate? */
        typeF(optabl3r[(int)inst],&source,bwl,&dest);   /* do 'quick' format */
        return 1;
    }
    if ((source.mode&E_Dn) == 0) bad_sourceam(); /* else source must be Dn */
    EXP0SP->expr_value = optabl3r[(int)inst] | (source.reg1<<9) | (bwl<<6) |
                         0x20 | dest.reg1;
    EXP0.ptr = 1;
    return 1;
}

/******************************************************************************
        type4
    type 4
    Instructions:
    BCC,BCS,BEQ,BGE,BGT,BHI,BLE,
    BLS,BLT,BMI,BNE,BPL,BVC,BVS,
    BRA,BSR
******************************************************************************/
/*ARGSUSED*/
int type4(int inst, int bwl)
{
    int t,opcr;
    EXP_stk *eps;
    EXPR_struct *expr;
    static unsigned short optabl4[] = { 0,
        0x6400,0x6500,0x6700,0x6c00,
        0x6e00,0x6200,0x6f00,0x6300,
        0x6d00,0x6b00,0x6600,0x6a00,
        0x6800,0x6900,0x6000,0x6100};

    opcr = edmask&ED_PCREL;
    edmask &= ~ED_PCREL;        /* default to ABS on branch */
    t = get_oneea(&source,bwl);
    edmask |= opcr;
    if (t != ONEEA_RET_SUCC || (source.mode&E_ABS) == 0)
    {
        if (t != ONEEA_RET_SUCC)
        {
            bad_token((char *)0,"Instruction requires an operand");
        }
        else
        {
            bad_token((char *)0,"Illegal address mode for branch instructions");
        }
        source.exp->ptr = 0;
        EXP0SP->expr_value = optabl4[(int)inst] | 0xFE;
        EXP0.ptr = 1;
        return 1;
    }
    eps = source.exp;
    expr = eps->stack + eps->ptr;
    expr->expr_code = EXPR_SEG;
    expr->expr_value = current_offset+2;
    (expr++)->expr_seg = current_section;
    expr->expr_code = EXPR_OPER;
    expr->expr_value = '-';
    eps->ptr += 2;
    eps->ptr = compress_expr(eps);
    if (list_bin) compress_expr_psuedo(eps);
    expr = eps->stack;
    eps->tag = 'I';         /* assume signed word mode */
    if (eps->ptr == 1 && expr->expr_code == EXPR_VALUE)
    {
        if (expr->expr_value == 0 || expr->expr_value < -128 || expr->expr_value > 127)
        {
            if (expr->expr_value != 0)
            {
                if (bwl == 3 || expr->expr_value < -32768 || expr->expr_value > 32767)
                {
                    long toofar;
                    toofar = expr->expr_value;
                    if (toofar > 0)
                    {
                        toofar -= (bwl==3) ? 127 : 32767;
                    }
                    else
                    {
                        toofar = -toofar- ((bwl==3) ? 128 : 32768);
                    }
                    sprintf(emsg,"Branch offset 0x%lX byte(s) out of range",toofar);
                    bad_token((char *)0,emsg);
                    expr->expr_value = -2;
                }
                if (bwl == 3) goto abs_br_byte_mode;
            }
            EXP0SP->expr_value = optabl4[(int)inst];
            EXP0.ptr = 1;
        }
        else
        {        /* -- byte offset */
            abs_br_byte_mode:
            eps->psuedo_value = expr->expr_value = optabl4[(int)inst] | (expr->expr_value&0xFF);
            eps->tag = 'W'; /* set word mode */
            EXP0.ptr = 0;
        }
    }
    else
    {            /* -- got an absolute value */
        EXP0.ptr = 1;
        if (bwl == 3)
        {     /* forced byte mode? */
            EXP0.psuedo_value = EXP0SP->expr_value = optabl4[(int)inst]>>8;
            EXP0.tag = 'b';
            eps->tag = 'z'; /* branch offset type */
        }
        else
        {
            EXP0.psuedo_value = EXP0SP->expr_value = optabl4[(int)inst];
        }
    }
    return 1;
}

/******************************************************************************
        type5
    type 5
    Instructions:
    BCHG,BCLR,BSET,BTST
******************************************************************************/
/*ARGSUSED*/
int type5(int inst, int bwl) 
{
    static unsigned short optabl5r[] = {
        0x0140,
        0x0180,
        0x01c0,
        0x0100};

    static unsigned short optabl5m[] = {
        0x0840,
        0x0880,
        0x08c0,
        0x0800};

    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = optabl5r[(int)inst];
        EXP0.ptr = 1;
        return 0;
    }
    if ((source.mode&(E_IMM|E_Dn)) == 0) bad_sourceam();
    if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
    EXP0.ptr = 1;
    if ((source.mode&E_Dn) != 0)
    {  /* if source is a D register */
        if ((dest.mode&E_Dn) != 0 && bwl == 1)
        { /* xxx.B command? */
            show_bad_token((char *)0,"Instruction only allows long operation on D regs",MSG_WARN);
        }
        EXP0SP->expr_value = optabl5r[(int)inst] | (source.reg1<<9) | 
                             dest.eamode | dest.reg1;
    }
    else
    {            /* if not use immediate form */
        if (bwl == 2)
        {         /* xxx.L command? */
            show_bad_token((char *)0,"Instruction only allows byte operation with this address mode",MSG_WARN);
        }
        EXP0SP->expr_value = optabl5m[(int)inst] | (dest.eamode + dest.reg1);
        source.exp->tag = 'b';  /* byte in low bits of a word */
    }
    return 1;
}

/******************************************************************************
        type6
    type 6
    Instructions:
    NOP,RESET,RTE,RTR,TRS,TRAPV
******************************************************************************/
/*ARGSUSED*/
int type6(int inst, int bwl, EA *tsea, EA *tdea) 
{
    static unsigned short optabl6[] = { 0,
        0x4e71,
        0x4e70,
        0x4e73,
        0x4e77,
        0x4e75,
        0x4e76};

    EXP0SP->expr_value = optabl6[(int)inst];
    EXP0.ptr = 1;
    return 1;
}

#define JMP_OPCODE (0x4ec0)
#define JSR_OPCODE (0x4e80)

/******************************************************************************
            type7
    type 7
    Instructions:
    PEA,JMP,JSR,NBCD,NEG,NEGX,NOT,CLR,
    SCC,SCS,SEQ,SF,SGE,SGT,SHI,SLE,
    SLS,SLT,SMI,SNE,SPL,ST,SVC,SVS
    TAS,TST
******************************************************************************/
/*ARGSUSED*/
int type7(int inst, int bwl) 
{
    static unsigned short optabl7[] = { 0,
        0x4840,JMP_OPCODE,JSR_OPCODE,0x4800,
        0x4400,0x4000,0x4600,0x4200,
        0x54c0,0x55c0,0x57c0,0x51c0,
        0x5cc0,0x5ec0,0x52c0,0x5fc0,
        0x53c0,0x5dc0,0x5bc0,0x56c0,
        0x5ac0,0x50c0,0x58c0,0x59c0,
        0x4ac0,0x4a00};

    if (get_oneea(&dest,bwl) != ONEEA_RET_SUCC)
    {
        bad_token((char *)0,"Instruction requires 1 operand");
        if (inst < 4)
        {
            dest.exp->ptr = 1;
            dest.mode = E_ABS;
            dest.eamode = Ea_ABSL;
            dest.exp->ptr = 1;
            dest.exp->tag = 'L';
        }
        else
        {
            dest.mode = E_Dn;
        }
    }
    if (inst < 4)
    {
        if ((dest.mode&(E_ATAn|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) == 0)
        {
            bad_destam();
            dest.exp->ptr = 1;
            dest.mode = E_ABS;
            dest.eamode = Ea_ABSL;
            dest.exp->ptr = 1;
            dest.exp->tag = 'L';
        }
    }
    else
    {
        if ((dest.mode&(E_AMA|E_Dn)) == 0)
        {
            bad_destam();
        }
    }
    EXP0SP->expr_value = optabl7[(int)inst] | (bwl<<6) | dest.eamode | dest.reg1;
    EXP0.ptr = 1;
    return 1;
}

/******************************************************************************
        type8
    type8
    Instructions:
    CMP,CMPA,CMPI
******************************************************************************/
/*ARGSUSED*/
int type8(int inst, int bwl)
{

    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = 0xb000;
        EXP0.ptr = 1;
        return 0;
    }
    if (inst != 0)
    {        /* user specified a specific class of insts */
        if (inst == 1)
        {    /* CMPA */
            if ((dest.mode&E_An) == 0) bad_destam();
        }
        else
        {        /* CMPI */
            if ((source.mode&E_IMM) == 0)
            {
                bad_sourceam();
                source.mode = E_IMM;
                source.eamode = Ea_IMM;
                source.exp->stack->expr_code = EXPR_VALUE;
                source.exp->stack->expr_value = 0;
                source.exp->ptr = 1;
            }
            if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
            immediate_mode(0x0c00,bwl);
            return 1;
        }
    }
    if ((dest.mode&(E_Dn|E_An)) == 0)
    {
        if ((source.mode&E_ANI) != 0 && (dest.mode&E_ANI) != 0)
        {
            EXP0SP->expr_value = 0xb108 | (dest.reg1<<9) | (bwl<<6) | source.reg1;
            EXP0.ptr = 1;
            return 1;
        }
        if ((source.mode&E_IMM) == 0)
        {
            bad_sourceam();
            source.mode = E_IMM;
            source.eamode = Ea_IMM;
            source.exp->stack->expr_code = EXPR_VALUE;
            source.exp->stack->expr_value = 0;
            source.exp->ptr = 1;
        }
        if ((dest.mode&E_AMA) == 0) bad_destam();
    }
    else
    {
        if ((dest.mode&E_An) != 0)
        {
            if (bwl == 0)
            {
                show_bad_token((char *)0,"Byte mode compare not allowed on A register",MSG_WARN);
                bwl = 1;
            }
            bwl = (bwl == 2) ? 7 : 3;
        }
        else if ((source.mode&E_IMM) != 0)
        {
            immediate_mode(0x0c00,bwl);
            return 1;
        }
        EXP0SP->expr_value = 0xb000 | (dest.reg1<<9) | (bwl<<6) | source.eamode | source.reg1;
        EXP0.ptr = 1;
        return 1;
    }
    immediate_mode(0x0c00,bwl);
    return 1;
}

/******************************************************************************
        type9
    type 9
    Instructions:
    DBCC,DBCS,DBEQ,DBF,DBGE,DBGT,DBHI,DBLE
    DBLS,DBLT,DBMI,DBNE,DBPL,DBT,DBVC,DBVS
******************************************************************************/
/*ARGSUSED*/
int type9(int inst, int bwl)
{
    static unsigned short optabl9[] = { 0,
        0x54c8,0x55c8,0x57c8,0x51c8,0x5cc8,
        0x5ec8,0x52c8,0x5fc8,0x53c8,
        0x5dc8,0x5bc8,0x56c8,0x5ac8,0x50c8,
        0x58c8,0x59c8};
    EXP_stk *eps;
    EXPR_struct *expr;

    EXP0SP->expr_value = optabl9[(int)inst];
    if ((get_twoea(bwl) == 0) || dest.mode != E_ABS)
    {
        bad_token((char *)0,"Illegal address mode for branch instructions");
        dest.exp->ptr = 1;
        dest.exp->stack->expr_value = -2;
        dest.exp->tag = 'I';
        return 1;
    }
    if (source.mode != E_Dn)
    {
        bad_token((char *)0,"Source operand must be a D register");
        source.mode = E_Dn;
        source.reg1 = 0;
        source.exp->ptr = 0;
    }
    EXP0SP->expr_value |= source.reg1;
    eps = dest.exp;
    expr = eps->stack + eps->ptr;
    expr->expr_code = EXPR_SEG;
    expr->expr_value = current_offset+2;
    (expr++)->expr_seg = current_section;
    expr->expr_code = EXPR_OPER;
    expr->expr_value = '-';
    eps->ptr += 2;
    eps->ptr = compress_expr(eps);
    eps->tag = 'I';
    if (list_bin) compress_expr_psuedo(eps);
    expr = eps->stack;
    if (eps->ptr == 1 && expr->expr_code == EXPR_VALUE)
    {
        if (expr->expr_value < -32768 || expr->expr_value > 32767)
        {
            long toofar;
            toofar = expr->expr_value;
            if (toofar > 0)
            {
                toofar -= 32767;
            }
            else
            {
                toofar -= 32768;
            }
            sprintf(emsg,"Branch offset 0x%lX byte(s) out of range",toofar);
            bad_token((char *)0,emsg);
            expr->expr_value = -2;
        }
    }
    return 1;
}

/******************************************************************************
        type10
    type 10
    Instructions:
    DIVS,DIVU,MULS,MULU,CHK
******************************************************************************/
/*ARGSUSED*/
int type10(int inst, int bwl) 
{
    static unsigned short optabl10[] = { 0,
        0x81c0,
        0x80c0,
        0xc1c0,
        0xc0c0,
        0x4180};

    EXP0SP->expr_value = optabl10[(int)inst];
    if (get_twoea(bwl) == 0) return 0;
    if ((source.mode&(E_ALL-E_An)) == 0) bad_sourceam();
    if ((dest.mode&E_Dn) == 0) bad_destam();
    EXP0SP->expr_value |= (dest.reg1<<9) | source.eamode | source.reg1;
    return 1;
}

/******************************************************************************
        type11
    type 11
    Instructions:
    EXG
******************************************************************************/
/*ARGSUSED*/
int type11(int inst, int bwl) 
{
    int omode,temp;
    unsigned short opcode;
    EXP0SP->expr_value = 0xC100;
    if (get_twoea(bwl) == 0) return 0;
    if ((source.mode&(E_Dn|E_An)) == 0) bad_sourceam();
    if ((dest.mode&(E_Dn|E_An)) == 0) bad_destam();
    if (source.mode == E_Dn)
    {
        if (dest.mode == E_Dn)
        {
            omode = 8;
        }
        else
        {
            temp = source.reg1;
            source.reg1 = dest.reg1;
            dest.reg1 = temp;
            omode = 0x11;
        }
    }
    else
    {
        if (dest.mode == E_Dn)
        {
            omode = 0x11;
        }
        else
        {
            omode = 9;
        }
    }
    opcode = 0xc100 | (omode << 3);
    opcode |= dest.reg1 << 9;
    opcode |= source.reg1;
    EXP0SP->expr_value = opcode;
    return 1;
}


/******************************************************************************
        type12
    type 12
    Instructions:
    EXT,SWAP,UNLK
******************************************************************************/
/*ARGSUSED*/
int type12(int inst, int bwl) 
{
    static unsigned short optabl12[] = { 0,
        0x4800,
        0x4840,
        0x4e58};
    unsigned short opcode,sam;

    opcode = optabl12[(int)inst];
    get_oneea(&source,bwl);
    if (inst < 3)
    {
        sam = E_Dn;
    }
    else
    {
        sam = E_An;
    }
    if (source.mode != sam) bad_sourceam();
    if (inst == 1)
    {
        if (bwl == 2) opcode |= 0xc0;
        else opcode |= 0x80;
    }
    opcode |= source.reg1;
    EXP0SP->expr_value = opcode;
    return 1;
}

/******************************************************************************
        type13
    type 13
    Instructions:
    LEA
******************************************************************************/
/*ARGSUSED*/
int type13(int inst, int bwl)
{
    EXP0SP->expr_value = 0x41c0;
    if (get_twoea(bwl) == 0) return 0;
    if ((source.mode&(E_ATAn|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) == 0)
    {
        bad_sourceam();
        source.eamode = Ea_ATAn;
    }
    if (dest.mode != E_An)
    {
        bad_destam();
        dest.eamode = Ea_An;
    }
    EXP0SP->expr_value |= (dest.reg1<<9) | source.eamode | source.reg1;
    return 1;
}

/******************************************************************************
        type14
    type 14
    Instructions:
    LINK
******************************************************************************/
/*ARGSUSED*/
int type14(int inst, int bwl) 
{
    get_twoea(bwl);
    if ((source.mode&E_An) == 0) bad_sourceam();
    if ((dest.mode&E_IMM) == 0)
    {
        bad_destam();
        dest.mode = E_IMM;
        dest.eamode = Ea_IMM;
        dest.exp->stack->expr_code = EXPR_VALUE;
        dest.exp->stack->expr_value = 0;
        dest.exp->ptr = 1;
    }
    dest.exp->tag = 'I';
    EXP0SP->expr_value = 0x4e50 | source.reg1;
    return 1;
}

/******************************************************************************
        type15
    type 15
    Instructions:
        MOVE MOVEA MOVEQ
******************************************************************************/
/*ARGSUSED*/
int type15(int inst, int bwl) 
{
    unsigned short opcode;
    static short stran[] = {1,3,2};

    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = 0x1000;
        return 1;
    }
    if (inst != 0)
    {            /* if not generic move */
        if (inst == 1)
        {
            if (dest.mode != E_An)
            {    /* movea */
                bad_destam();       /*  must have An for dest */
                dest.mode = E_An;
                dest.eamode = Ea_An;
                dest.exp->ptr = 0;
                dest.reg1 = 0;
            }
        }
        else
        {
            if (source.mode != E_IMM)
            { /* moveq */
                bad_sourceam();     /*   must have #n for source */
                if (dest.mode != E_Dn)
                { /* and dest must be data reg */
                    bad_destam();
                }
                EXP0SP->expr_value = (0xe040>>8) | (dest.reg1<<(9-8));
                EXP0.tag = 'b';
                source.exp->tag = 'b';
                return 0;
            }
        }
    }
    if ((dest.mode&~E_ALL) != 0 || (source.mode&~E_ALL) != 0)
    {
        if ((source.mode&~E_ALL) != 0)
        {
            if ((source.mode&E_USP) != 0)
            {
                if ((dest.mode&E_An) == 0) bad_destam();
                opcode = 0x4e68 | dest.reg1;
            }
            else
            {
                if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
                if (bwl != 1)
                {
                    show_bad_token((char *)0,"Only word mode moves allowed from CCR/SR",MSG_WARN);
                }
                if (source.mode == E_CCR)
                {
                    opcode = 0x42c0 | dest.eamode | dest.reg1;
                }
                else if (source.mode == E_SR)
                {
                    opcode = 0x40c0 | dest.eamode | dest.reg1;
                }
                else
                {
                    bad_sourceam();
                    opcode = 0;
                }
            }
            EXP0SP->expr_value = opcode;
            return 1;
        }
        if (dest.mode == E_USP)
        {
            if ((source.mode&(E_An)) == 0) bad_sourceam();
            opcode = 0x4e60 | source.reg1;
        }
        else
        {
            if ((source.mode&(E_ALL-E_An)) == 0) bad_sourceam();
            if (bwl != 1)
            {
                show_bad_token((char *)0,"Only word mode moves allowed to CCR/SR",MSG_WARN);
            }
            if (dest.mode == E_CCR)
            {
                opcode = 0x44c0 | source.eamode | source.reg1;
            }
            else if (dest.mode == E_SR)
            {
                opcode = 0x46c0 | source.eamode | source.reg1;
            }
            else
            {
                bad_destam();
                opcode = 0;
            }
        }
        EXP0SP->expr_value = opcode;
        return 1;
    }
    if (dest.mode == E_An)
    {
        if (bwl == 0)
        {
            bad_token((char *)0,"Byte mode moves are not allowed to A registers");
            bwl = 1;
        }
        EXP0SP->expr_value = 0x2040 | (stran[(int)bwl]<<12) | (dest.reg1<<9) | source.eamode | source.reg1;
        return 1;
    }
    if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
    if (dest.mode == E_Dn && source.mode == E_IMM && bwl == 2)
    {
        EXP_stk *eps;
        EXPR_struct *exp;
        eps = source.exp;
        exp = eps->stack;
        if (!(eps->force_short|eps->force_long) && eps->ptr == 1 && exp->expr_code == EXPR_VALUE &&
            (exp->expr_value >= -128 && exp->expr_value < 128))
        {
            EXP0SP->expr_value = 0x7000 | (dest.reg1<<9) | (exp->expr_value&255);
            eps->ptr = 0;
            return 1;
        }
    }
    opcode = ((dest.eamode<<3)&0700) | (((dest.eamode|dest.reg1)<<9)&07000); 
    EXP0SP->expr_value = (stran[(int)bwl]<<12) | opcode | source.eamode | source.reg1;
    return 1;
}

/******************************************************************************
        type16
    type 16
    Instructions:
    MOVEM
******************************************************************************/
/* below is nybble flip  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
static char flip[] =    {0, 8, 4,0xC,2,0xA,6,0xE,1, 9, 5,0xD,3,0xB,7,0xF};
/*ARGSUSED*/
int type16(int inst, int bwl) 
{
    int dr;
    unsigned short reg_mask;
    EXP_stk *temp;
    EA *mask,*eadd;

    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if ( (source.mode&E_IMM) )
    {
        if ( !(dest.mode&(E_ATAn|E_DAN|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) )
        {
            bad_destam();
            dest.exp->ptr = 1;
            dest.exp->stack->expr_code = EXPR_VALUE;
            dest.exp->stack->expr_value = 0;
        }
        dr = 0<<10;
        mask = &source;
        eadd = &dest;
    }
    else
    {
        if ( (dest.mode&E_IMM) )
        {
            if ( !(source.mode&(E_ATAn|E_ANI|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) )
            {
                bad_sourceam();
                source.eamode = Ea_ATAn;
            }
            mask = &dest;
            eadd = &source;
            dr = 1<<10;
        }
        else
        {
            if ((source.mode&(E_RLST|E_An|E_Dn)) == 0)
            {
                if ((dest.mode&(E_RLST|E_An|E_Dn)) == 0)
                {
                    bad_destam();
                    dest.exp->ptr = 1;
                    dest.exp->stack->expr_code = EXPR_VALUE;
                    dest.exp->stack->expr_value = 0;
                }
                if ((source.mode&(E_ATAn|E_ANI|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) == 0)
                {
                    bad_sourceam();
                    source.eamode = Ea_ATAn;
                }
                dr = 1<<10;
                mask = &dest;
                eadd = &source;
            }
            else
            {
                mask = &source;
                eadd = &dest;
                dr = 0;
                if ((dest.mode&(E_ATAn|E_DAN|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) == 0)
                {
                    bad_destam();
                    dest.eamode = Ea_ATAn;
                }
            }
        }
    }
    if ((mask->mode&E_An) != 0)
    {
        reg_mask = 1<<(mask->reg1+8);
    }
    else if ((mask->mode&E_Dn) != 0)
    {
        reg_mask = 1<<(mask->reg1);
    }
    else
    {
        reg_mask = mask->exp->stack->expr_value;
    }
    if (dest.mode == E_DAN && !(mask->mode&E_IMM) )
    { /* for -(An) destination mode reverse register mask */
        reg_mask = (flip[(reg_mask >> 12) & 0xF]) |
                   (flip[(reg_mask >> 8) & 0xF] << 4) |
                   (flip[(reg_mask >> 4) & 0xF] << 8) |
                   (flip[reg_mask & 0xF] << 12);
    }
    EXP0SP->expr_value = 0x4880 | dr | (bwl == 2 ? 0x40 : 0) | eadd->eamode | eadd->reg1;
    mask->exp->psuedo_value = mask->exp->stack->expr_value = reg_mask;
    mask->exp->ptr = 1;
    mask->exp->tag = 'W';
    mask->exp->tag_len = 1;
    if (dr != 0)
    {
        dr = source.reg2;
        temp = source.exp;
        source.exp = dest.exp;
        source.reg2 = dest.reg2;
        dest.exp = temp;
        dest.reg2 = dr;
    }
    return 1;
}

/******************************************************************************
        type17
    type 17
    Instructions:
    MOVEP
******************************************************************************/
/*ARGSUSED*/
int type17(int inst,int bwl) 
{
    unsigned short opcode;

    if (get_twoea(bwl) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if (source.mode == E_Dn)
    {
        if (dest.mode != E_DSP)
        {
            bad_destam();
            dest.exp->ptr = 1;
            dest.exp->stack->expr_code = EXPR_VALUE;
            dest.exp->stack->expr_value = 0;
        }
        opcode = 8 | (source.reg1<<9) | ((bwl == 1) ? (6<<6):(7<<6)) | dest.reg1;
    }
    else
    {
        if (source.mode != E_DSP)
        {
            bad_sourceam();
            source.exp->ptr = 1;
        }
        if (dest.mode != E_Dn) bad_destam();
        opcode = 8 | (dest.reg1<<9) | ((bwl == 1) ? (4<<6):(5<<6)) | source.reg1;
    }                   
    EXP0SP->expr_value = opcode;
    return 0;
}

/******************************************************************************
        type18
    type 18
    Instructions:
    STOP
******************************************************************************/
/*ARGSUSED*/
int type18(int inst, int bwl) 
{

    EXP0SP->expr_value = 0x4e72;
    if (get_oneea(&source,bwl) != ONEEA_RET_SUCC)
    {
        source.exp->ptr = 1;
        return 1;
    }
    if (source.mode != E_IMM)
    {
        bad_sourceam();
        source.mode = E_IMM;
        source.eamode = Ea_IMM;
        source.exp->stack->expr_code = EXPR_VALUE;
        source.exp->stack->expr_value = 0;
        source.exp->ptr = 1;
    }
    return 1;
}

/******************************************************************************
        type19
    type 19
    Instructions:
    TRAP
******************************************************************************/
/*ARGSUSED*/
int type19(int inst, int bwl) 
{
    EXP_stk *eps;
    EXPR_struct *exp;

    if (get_oneea(&source,bwl) != ONEEA_RET_SUCC)
    {
        return 1;
    }
    if (source.mode != E_IMM)
    {
        bad_sourceam();
        source.mode = E_IMM;
        source.eamode = Ea_IMM;
        source.exp->stack->expr_code = EXPR_VALUE;
        source.exp->stack->expr_value = 0;
        source.exp->ptr = 1;
    }
    eps = source.exp;
    exp = eps->stack;
    if (eps->ptr == 1 && exp->expr_code == EXPR_VALUE)
    {
        if ((unsigned long)exp->expr_value > 15)
        {
            show_bad_token((char *)0,"Trap vector out of range",MSG_ERROR);
            exp->expr_value = 0;
        }
        EXP0SP->expr_code = EXPR_VALUE;
        EXP0SP->expr_value = 0x4e40 | exp->expr_value;
        EXP0.ptr = 1;
        eps->ptr = 0;
    }
    else
    {
        int tptr,tag_save,tagl_save;
        EXPR_struct *texp;
        tptr = eps->ptr;
        exp += tptr;
        texp = exp;

        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 0;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_PICK;
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 15;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = (EXPROPER_TST_GT<<8) | EXPROPER_TST;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_XCHG;
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 0;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = (EXPROPER_TST_LT<<8) | EXPROPER_TST;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = (EXPROPER_TST_OR<<8) | EXPROPER_TST;
        eps->ptr += 8;
        tag_save = eps->tag;
        tagl_save = eps->tag_len;
        eps->tag = eps->tag_len = 0;
        write_to_tmp(TMP_TEST,0,eps,0);
        sprintf(emsg,"TRAP vector out of range at line %d in %s",
                current_fnd->fn_line,current_fnd->fn_name_only);
        write_to_tmp(TMP_ASTNG,strlen(emsg)+1,emsg,sizeof(char));
        eps->tag = tag_save;
        eps->tag_len = tagl_save;
        exp = texp;
        eps->ptr = tptr;        
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 15;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_AND;
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 0x4e40;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_OR;
        eps->ptr += 4;
        EXP0.ptr = 0;
    }
    return 1;
}    

/******************************************************************************
            typeF
    Type F instructions are of the form (quick immediate):
     ____________________
    |    | cnt|  |i|  |rx|
     --------------------
******************************************************************************/
int typeF( unsigned short opcode, EA *tsea, int bwl, EA *tdea)
{
    EXP_stk *eps;
    EXPR_struct *exp,*texp;
    int tptr;
    eps = tsea->exp;
    exp = eps->stack;
    if (eps->ptr == 1 && exp->expr_code == EXPR_VALUE)
    {
        if (exp->expr_value < 1 || exp->expr_value > 8)
        {
            show_bad_token((char *)0,"Quick immediate value out of range",MSG_ERROR);
        }
        EXP0SP->expr_code = EXPR_VALUE;
        EXP0SP->expr_value = opcode | (bwl<<6) | 
                             ((exp->expr_value&7)<<9) | tdea->eamode | tdea->reg1;
        EXP0.ptr = 1;
        eps->ptr = 0;
    }
    else
    {
        tptr = eps->ptr;
        exp += tptr;
        texp = exp;

        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 0;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_PICK;    /* picks 0'th element (DUP) */
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 8;        /* check against high limit */
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = (EXPROPER_TST_GT<<8) | EXPROPER_TST;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_XCHG;    /* swap top two elements */
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 1;        /* check against low limit */
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = (EXPROPER_TST_LT<<8) | EXPROPER_TST;
        exp->expr_code = EXPR_OPER;     /* logical OR of both tests */
        (exp++)->expr_value = (EXPROPER_TST_OR<<8) | EXPROPER_TST;
        eps->ptr += 8;
        eps->tag = eps->tag_len = 0;        /* no tag or tag length */
        write_to_tmp(TMP_TEST,0,eps,0);     /* write test statement */
        sprintf(emsg,"Quick immediate value out of range at line %d in %s",
                current_fnd->fn_line,current_fnd->fn_name_only); /* and message */
        write_to_tmp(TMP_ASTNG,strlen(emsg)+1,emsg,sizeof(char));
        exp = texp;         /* remove the test stuff from stack */
        eps->ptr = tptr;        
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 7;    /* mask the value */
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_AND;
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = 9;    /* and shift it 9 bits left */
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_SHL;
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = eps->psuedo_value = opcode | (bwl<<6) | 
                              tdea->eamode | tdea->reg1;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_OR;  /* and or in the opcode */
        eps->ptr += 6;          /* say there's 6 more items on stack */
        eps->tag = 'W';         /* opcode is a word */
        eps->tag_len = 1;       /* length of 1 */
        EXP0.ptr = 0;           /* no opcode stack */
    }
    return 1;
}

/******************************************************************************
        type20
    type 20
    Instructions:
    JBCC,JBCS,JBEQ,JBGE,JBGT,JBHI,JBLE,
    JBLS,JBLT,JBMI,JBNE,JBPL,JBVC,JBVS,
    JBRA,JBSR
******************************************************************************/
#define JBRA_INDX (15)
#define JBSR_INDX (16)
#define BRA_OPCODE (0x6000)

static int type20_jmp( int inst )
{
    /* Opposite sense branches */
    static unsigned short optab20_inv[] =
    {   0,              /*  0 Not defined */
        0x6500,         /*  1 BCS BLO (BCC BHS) */
        0x6400,         /*  2 BCC BHS (BCS BLO)*/
        0x6600,         /*  3 BNE (BEQ) */
        0x6d00,         /*  4 BLT (BGE) */
        0x6f00,         /*  5 BLE (BGT) */
        0x6300,         /*  6 BLS (BHI) */
        0x6e00,         /*  7 BGT (BLE) */
        0x6200,         /*  8 BHI (BLS) */
        0x6c00,         /*  9 BGE (BLT) */
        0x6a00,         /* 10 BPL (BMI) */
        0x6700,         /* 11 BEQ (BNE) */
        0x6Bb0,         /* 12 BMI (BPL) */
        0x6900,         /* 13 BVS (BVC) */
        0x6800,         /* 14 BVC (BVS) */
        JMP_OPCODE,     /* 15 JMP (BRA) */
        JSR_OPCODE      /* 16 JSR (BSR) */
    };
    EXP_stk *eps;
    EXPR_struct *expr;

    eps = exprs_stack;
    expr = eps->stack;
    expr->expr_code = EXPR_VALUE;
    eps->ptr = 1;
    eps->tag = 'W';     /* Unsigned word */
    if ( inst < JBRA_INDX )
    {
        eps->psuedo_value = expr->expr_value = optab20_inv[inst]|6;
        p1o_any(eps);       /* Output a branch over a jmp */
        eps->psuedo_value = expr->expr_value = JMP_OPCODE | source.eamode | source.reg1;
    }
    else
    {
        eps->psuedo_value = expr->expr_value = optab20_inv[inst] | source.eamode | source.reg1;
    }
    source.exp->tag = 'L';
    return 1;           /* jmp/jsr target is in source */
}

/*ARGSUSED*/
int type20(int inst, int bwl)
{
    int t,opcr;
    EXP_stk *eps, tps;
    EXPR_struct *expr;

    static unsigned short optab20[] = 
    {   0,              /*  0 Not defined */
        0x6400,         /*  1 BCC BHS */
        0x6500,         /*  2 BCS BLO */
        0x6700,         /*  3 BEQ */
        0x6c00,         /*  4 BGE */
        0x6e00,         /*  5 BGT */
        0x6200,         /*  6 BHI */
        0x6f00,         /*  7 BLE */
        0x6300,         /*  8 BLS */
        0x6d00,         /*  9 BLT */
        0x6b00,         /* 10 BMI */
        0x6600,         /* 11 BNE */
        0x6a00,         /* 12 BPL */
        0x6800,         /* 13 BVC */
        0x6900,         /* 14 BVS */
        0x6000,         /* 15 BRA */
        0x6100          /* 16 BSR */
    };

    opcr = edmask&ED_PCREL;
    edmask &= ~ED_PCREL;        /* default to ABS on branch */
    t = get_oneea(&source,bwl);
    edmask |= opcr;
    eps = exprs_stack;
    if ( t != ONEEA_RET_SUCC || (source.mode&(E_ATAn|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) == 0 )
    {
        if ( t != ONEEA_RET_SUCC )
            bad_token((char *)0,"Instruction requires an operand");
        else
            bad_sourceam();
        source.exp->ptr = 0;
        eps->stack->expr_value = optab20[JBRA_INDX] | 0xFE;
        eps->ptr = 1;
        return 1;
    }
    if ( !(source.mode&E_ABS) )
    {
        /* If complex address mode, always use a branch over a jmp */
        return type20_jmp(inst);
    }
    /* See if we can use a short branch */
    eps = source.exp;
    tps = *eps;             /* save a copy of the stack */
    tps.stack = source.tmps[0]->stack; /* But use one of the actual temporary stacks */
    memcpy(tps.stack, eps->stack, eps->ptr*sizeof(EXPR_struct));
    expr = eps->stack + eps->ptr;
    expr->expr_code = EXPR_SEG;
    expr->expr_value = current_offset+2;
    (expr++)->expr_seg = current_section;
    expr->expr_code = EXPR_OPER;
    expr->expr_value = '-';
    eps->ptr += 2;
    eps->ptr = compress_expr(eps);
    if (list_bin) 
        compress_expr_psuedo(eps);
    expr = eps->stack;
    eps->tag = 'I';         /* assume signed word mode */
    if (eps->ptr == 1 && expr->expr_code == EXPR_VALUE)
    {
        /* Computed to absolute value */
        if ( expr->expr_value < -32768 || expr->expr_value > 32767 )
        {
            /* It's too far. Have to use a jmp */
            eps->ptr = tps.ptr;     /* restore the stack */
            memcpy(eps->stack, tps.stack, eps->ptr*sizeof(EXPR_struct));
            if (list_bin) 
                compress_expr_psuedo(eps);
            return type20_jmp(inst);
        }
        if ( expr->expr_value != 0 && expr->expr_value >= -128 && expr->expr_value < 128)
        {
            /* Can use a byte offset */
            eps->psuedo_value = expr->expr_value = optab20[(int)inst] | (expr->expr_value&0xFF);
            eps->tag = 'W'; /* set word mode */
            EXP0.ptr = 0;
        }
        else
        {
            /* Use a word offset */
            EXP0.ptr = 1;
            EXP0.psuedo_value = EXP0SP->expr_value = optab20[(int)inst];
        }
        return 1;
    }
    expr = tps.stack;
    if (tps.ptr == 1 && expr->expr_code == EXPR_SYM )
    {
        SS_struct *sym;
        sym = expr->expr_sym;
        if ( sym->flg_local )
        {
            /* Use a word offset */
            EXP0.ptr = 1;
            EXP0.psuedo_value = EXP0SP->expr_value = optab20[(int)inst];
            return 1;
        }
    }
    eps->ptr = tps.ptr;     /* restore the stack */
    memcpy(eps->stack, tps.stack, eps->ptr*sizeof(EXPR_struct));
    if (list_bin) 
        compress_expr_psuedo(eps);
    return type20_jmp(inst);
}

int op_ntype(Opcode *opc)
{
    f1_eatit();
    return 0;
}
