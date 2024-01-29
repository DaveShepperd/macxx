/*
    m682types.c - Part of macxx, a cross assembler family for various micro-processors
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
    There are 20+ different types of instructions in the 68020.
    An instruction type may ultimately be one of several different
    forms. An instruction type with a number in its name is a basic
    type. If a type routine has a letter in its name it is a sub type.
    Basic types will, if necessary, call subtypes to finish formating
    an instruction.
******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include "token.h"
#include "m682k.h"
#include "exproper.h"
#include "listctrl.h"
#include "pst_tokens.h"

/* extern unsigned short eafield[]; */
static char ea_to_tag[] = {'b','I','L'};

int typeF( unsigned short opcode, EA *tsea, int bwl, EA *tdea);

typedef enum
{
    AMFLG_none=    0x00,     /* no special addressing */
    AMFLG_autodec= 0x01,     /* found a leading minus (autodecrement mode) */
    AMFLG_autoinc= 0x02,     /* found a trailing plus sign */
    AMFLG_bdisr=   0x08,     /* bd is a register */
    AMFLG_bracket= 0x10,     /* found an open square bracket */
    AMFLG_openpar= 0x20,     /* found an open paren */
    AMFLG_preindx= 0x40,     /* address mode is preindex indirect */
    AMFLG_postindx=0x80      /* address mode is postindex indirect */
} AMflg;

enum gt_arg
{
    GTERM_ANY=0,     /* allow anything */
    GTERM_D=1,       /* D register */
    GTERM_A=2,       /* A register */
    GTERM_PC=4       /* PC register */
};

/***********************************************************************************
 * Pickup a term. Each item in the operand is parsed and verified against the type
 * argument.
 */
static int get_term(EA *amp, int type, EXP_stk *eps, char *msg )

/* At entry:
 *	amp - ptr to operand struct
 *	type - type of argument allowed: see GTERM_xxx enum
 *	eps - ptr to expression stack into which to build the term
 *	msg - ptr to message string to output if an error occurs
 * At exit:
 *	returns true if the term was valid otherwise returns false
 */
{
    int i,treg,err=0;
    if (*inp_ptr == ',')
    {   /* if we're parked on a comma, then we have a null expression */
        eps->ptr = 0;
        return 0;
    }
    if (get_token() == EOL)
    {        /* prime the pump */
        if (msg) bad_token(tkn_ptr,msg);
        return 0;
    }
    i = exprs(type?0:1,eps);     /* anything is legal */
    if (i < 0)
    {
        if (msg) bad_token(tkn_ptr,msg);
        return i;
    }
    if (type == 0) return i;     /* anything is legal */
    if (i == 0) return i;        /* null expression is legal */
    if ( i != 1 || eps->stack->expr_code != EXPR_VALUE || eps->forward_reference )
    {
        if (msg) bad_token(tkn_ptr,msg);
        eps->ptr = 1;
        eps->stack->expr_code = EXPR_VALUE;
        eps->stack->expr_value = (type != 3) ? REG_Dn : REG_An;
        return 1;             /* pretend it worked */
    }
    treg = eps->stack->expr_value&~(FORCE_WORD|FORCE_LONG);
    switch (type)
    {
    default:              /* any type of register allowed */
    case GTERM_D|GTERM_A|GTERM_PC:    /* has to be a D, A or PC register */
        eps->register_reference = 1;   /* force a register reference */
        if (treg > REG_An+7 && treg != REG_PC && treg != REG_ZPC)
        {
            err = 1;
            eps->stack->expr_value = REG_Dn;    /* change to D0 */
        }
        break;
    case GTERM_A|GTERM_D:     /* has to be a D or A register */
        eps->register_reference = 1;   /* force a register reference */
        if (treg > REG_An+7)
        {
            err = 1;
            eps->stack->expr_value = REG_Dn;    /* change to D0 */
        }
        break;
    case GTERM_A|GTERM_PC:        /* has to be an A or PC register */
        eps->register_reference = 1;   /* force a register reference */
        if (treg < REG_An || (treg > REG_An+7 && treg != REG_PC))
        {
            err = 1;
            eps->stack->expr_value = REG_An;    /* change to D0 */
        }
        break;
    case GTERM_A:         /* has to be an A register */
        eps->register_reference = 1;   /* force a register reference */
        if (treg < REG_An || treg > REG_An+7)
        {
            err = 1;
            eps->stack->expr_value = REG_An;    /* change to D0 */
        }
        break;
    case GTERM_D:         /* has to be a D register */
        eps->register_reference = 1;   /* force a register reference */
        if (treg > REG_Dn+7)
        {
            err = 1;
            eps->stack->expr_value = REG_Dn;    /* change to D0 */
        }
        break;
    }
    if (msg && err)
    {
        bad_token(tkn_ptr,msg);
        return 0;
    }
    return 1;                /* it's ok now */
}

/* An entry from this table is obtained by placing the following data in a 4 bit
 * integer and using that value as an index into this array:
 *	bits 1-0: The size of the outer displacement: 0=illegal,1=null,2=word,3=long
 *	bit  2:   1 if postindex address mode
 *	bit  3:   1 if preindex address mode
 * Note that the (binary) combinations 0000 and 11xx are illegal */

static unsigned char index_indirect_modes[16] = {
    0x00,    /* no memory indirection */
    0x41,    /* memory indirect with null outer displacement */
    0x42,    /* memory indirect with word outer displacement */
    0x43,    /* memory indirect with long outer displacement */
    0x05,    /* indirect postindexed with null outer displacement */
    0x05,    /* indirect postindexed with null outer displacement */
    0x06,    /* indirect postindexed with word outer displacement */
    0x07,    /* indirect postindexed with long outer displacement */
    0x01,    /* indirect preindexed with null outer displacement */
    0x01,    /* indirect preindexed with null outer displacement */
    0x02,    /* indirect preindexed with word outer displacement */
    0x03,    /* indirect preindexed with long outer displacement */
    0x04,    /* illegal */
    0x04,    /* illegal */
    0x04,    /* illegal */
    0x04     /* illegal */
};

void fix_pcr(EA *amp,int size)
{
    EXP_stk *eps;
    EXPR_struct *exp;
    eps = amp->exp[0];
    exp = eps->stack + eps->ptr;
    if (size > 0)
    {
        exp->expr_code = EXPR_SEG;
        exp->expr_value = current_offset+size;
        (exp++)->expr_seg = current_section;
    }
    else
    {
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = (size == 0) ? 2 : size;
    }
    exp->expr_code = EXPR_OPER;
    exp->expr_value = '-';
    eps->ptr += 2;
    eps->ptr = compress_expr(eps);
    if (list_bin) compress_expr_psuedo(eps);
    return;
}

void fixup_ea(EA *amp, int amflg, unsigned long valid, int pcoff)
{
    EXP_stk *eps;
    EXPR_struct *exp;
    int ecv,i;
    long tv;

    switch (amp->parts)
    {
    case 0:               /* no parts */
        amp->extension = 0x0150 | EXT_BASESUPRESS;
        amp->mode = E_NDX;
        amp->eamode = Ea_NDX;
        amp->base_reg = amp->index_reg = 0;
        return;
    case EAPARTS_BASER:       /* only a base register */
        amp->mode = E_ATAn;
        amp->eamode = Ea_ATAn;
        amp->base_reg &= 7;    /* base reg can only be 0-7 */
        return;
    case EAPARTS_BASED:       /* base dislacement only */
    case EAPARTS_BASER|EAPARTS_BASED: /* base reg and base dislacement only */
        eps = amp->exp[0];     /* point to base displacement stack */
        exp = eps->stack;
        if (amp->base_reg == REG_ZPC || amp->base_reg == REG_PC)
        {
            if (amp->base_reg == REG_ZPC)
            {
                amp->base_reg = REG_PC;
                amp->parts &= ~EAPARTS_BASER;
            }
            else
            {
                fix_pcr(amp,pcoff);  /* adjust pc relative */
            }
        }
        if ((amp->parts&EAPARTS_BASER) == 0) amp->extension |= EXT_BASESUPRESS;
        tv =  exp->expr_value;
        ecv = (eps->ptr == 1 && exp->expr_code == EXPR_VALUE && !eps->forward_reference);
        if ((valid&(E_NDX|E_PCN)) == 0 || !eps->force_long || (ecv && tv > -32768 && tv < 32768))
        {
            if (amp->base_reg == REG_PC)
            {
                amp->mode = E_PCR;
                amp->eamode = Ea_PCR;
                amp->base_reg = 0;
            }
            else
            {
                amp->mode = E_DSP;
                amp->eamode = Ea_DSP;
                amp->base_reg &= 7;  /* base reg can only be 0-7 */
            }
            eps->tag = 'I';     /* short */
            return;         /* nothing more to do */
        }
        if (amp->base_reg == REG_PC)
        {
            amp->mode = E_PCN;
            amp->eamode = Ea_PCN;
            amp->base_reg = 0;
        }
        else
        {
            amp->mode = E_NDX;
            amp->eamode = Ea_NDX;
            amp->base_reg &= 7;
        }
        amp->extension |= 0x0170;  /* longword base displacement with no index */
        eps->tag = 'L';        /* longword base displacement */
        return;
    case EAPARTS_BASER|EAPARTS_INDXR:
    case EAPARTS_BASED|EAPARTS_INDXR:
    case EAPARTS_BASER|EAPARTS_BASED|EAPARTS_INDXR:
        if (amp->base_reg == REG_PC)
        {
            fix_pcr(amp,pcoff);
            amp->mode = E_PCN;
            amp->eamode = Ea_PCN;
            amp->base_reg = 0;
        }
        else
        {
            amp->mode = E_NDX;
            amp->eamode = Ea_NDX;
            amp->base_reg &= 7;
        }
        eps = amp->exp[0];     /* point to base displacement stack */
        exp = eps->stack;
        if ((amp->parts&EAPARTS_BASED) != 0)
        {
            tv =  exp->expr_value;
            ecv = (eps->ptr == 1 && exp->expr_code == EXPR_VALUE && !eps->forward_reference);
        }
        else
        {
            tv = 0;
            ecv = 1;
        }
        amp->extension |= ((amp->index_reg&15)<<EXT_V_INDEX) | 
                          ((amp->index_reg&FORCE_LONG) ? EXT_ILONG : 0) |
                          (amp->index_scale<<EXT_V_SCALE);
        if ((amflg&(AMFLG_preindx|AMFLG_postindx)) == 0 && (amp->parts&EAPARTS_BASER) != 0 &&
            (eps->force_byte || (ecv && tv > -128 && tv < 128)))
        {
            amp->extension |= (tv&255);
            if (!ecv || tv < -128 || tv > 127)
            {
                eps->tag = 's';      /* byte displacement expression -> obj file */
            }
            else
            {
                eps->ptr = 0;        /* else zap the expression, value in extension word */
            }
        }
        else
        {
            if ((amp->parts&EAPARTS_BASER) == 0) amp->extension |= EXT_BASESUPRESS;
            i = (((amflg&AMFLG_preindx) != 0) << 3) | (((amflg&AMFLG_postindx) != 0) << 2);
            amp->extension |= index_indirect_modes[i];
            if ((!ecv && !eps->force_long) || (ecv && tv > -32768 && tv < 32768))
            {
                amp->extension |= 0x0120; /* word base displacement */
                eps->tag = 'I';      /* signed word */
            }
            else
            {
                amp->extension |= 0x0130; /* long base displacement */
                eps->tag = 'L';      /* signed longword */
            }
        }
        return;
    case EAPARTS_INDXR:       /* only an index register */
    case EAPARTS_INDXD:       /* only an outer displacement */
    case EAPARTS_INDXR|EAPARTS_INDXD: /* only an index register and an outer displacement */
    case EAPARTS_INDXD|EAPARTS_BASER:
    case EAPARTS_INDXD|EAPARTS_BASED:
    case EAPARTS_INDXD|EAPARTS_BASER|EAPARTS_BASED:
    case EAPARTS_INDXD|EAPARTS_INDXR|EAPARTS_BASER:
    case EAPARTS_INDXD|EAPARTS_INDXR|EAPARTS_BASED:
    case EAPARTS_INDXD|EAPARTS_INDXR|EAPARTS_BASER|EAPARTS_BASED:
        amp->mode = E_NDX;     /* assume indexed mode */
        amp->eamode = Ea_NDX;
        if ((amp->parts&(EAPARTS_BASER|EAPARTS_BASED)) != 0)
        {
            eps = amp->exp[0];      /* point to base displacement stack */
            exp = eps->stack;
            if ((amp->parts&EAPARTS_BASER) == 0) amp->extension |= EXT_BASESUPRESS;
            if (amp->base_reg == REG_PC)
            {
                fix_pcr(amp,pcoff);
                amp->mode = E_PCN;
                amp->eamode = Ea_PCN;
            }
            tv =  exp->expr_value;
            ecv = (eps->ptr == 1 && exp->expr_code == EXPR_VALUE && !eps->forward_reference);
            if ((amp->parts&EAPARTS_BASED) == 0)
            {
                ecv = 1;
                tv = 0;
            }
            if ((!ecv && !eps->force_long) || (ecv && tv > -32768 && tv < 32768))
            {
                if (ecv && tv == 0)
                {
                    amp->extension |= 0x0010; /* null base displacement */
                    eps->ptr = 0;
                }
                else
                {
                    amp->extension |= 0x0020; /* else word base displacement */
                    eps->tag = 'I';       /* short */
                }
            }
            else
            {
                amp->extension |= 0x0030;    /* else it's a longword base displacement */
                eps->tag = 'L';
            }
        }
        else
        {           /* now fall through to index handler */
            amp->extension |= 0x0190;   /* no base register or displacement */
        }
        eps = amp->exp[1];     /* point to outer displacement stack */
        exp = eps->stack;
        if ((amp->parts&EAPARTS_INDXD) != 0)
        {
            tv =  exp->expr_value;
            ecv = (eps->ptr == 1 && exp->expr_code == EXPR_VALUE && eps->forward_reference);
        }
        else
        {
            tv = 0;
            ecv = 1;
        }
        if ((!ecv && !eps->force_long) || (ecv && tv > -32768 && tv < 32768))
        {
            if (ecv && tv == 0)
            {
                i = 1;       /* null displacement */
                eps->ptr = 0;    /* zap the expression */
            }
            else
            {
                i = 2;       /* word displacement */
                eps->tag = 'I';  /* expression must resolve to signed word */
            }
        }
        else
        {
            i = 3;      /* longword displacment */
            eps->tag = 'L';     /* signed longword */
        }
        if ((amp->parts&EAPARTS_INDXR) != 0)
        {
            amp->extension |= ((amp->index_reg&15)<<EXT_V_INDEX) | 
                              ((amp->index_reg&FORCE_LONG) ? EXT_ILONG : 0) |
                              (amp->index_scale<<EXT_V_SCALE);
            i |= (((amflg&AMFLG_preindx) != 0) << 3) | (((amflg&AMFLG_postindx) != 0) << 2);
        }
        amp->extension |= index_indirect_modes[i] | 0x0100;
        amp->base_reg &= 7;
        return;
    default:
        bad_token((char *)0,"Undecoded address mode");
        return;
    }                /* --switch (parts) */
    return;
}

static struct
{
    short reg_num;
    short movec;
    long mode;

} reg_xref[] = {
    {REG_SR,0xFFF,E_SR},
    {REG_USP,0x800,E_USP},
    {REG_SSP,0xFFF,E_SPCL},
    {REG_ISP,0x804,E_SPCL},
    {REG_MSP,0x803,E_SPCL},
    {REG_CCR,0xFFF,E_CCR},
    {REG_VBR,0x801,E_SPCL},
    {REG_SFC,0x000,E_SPCL},
    {REG_DFC,0x001,E_SPCL},
    {REG_CACR,0x002,E_SPCL},
    {REG_CAAR,0x802,E_SPCL},
    {0,0}
};

int get_oneea( EA *amp, int bwl, unsigned long valid, int pcoff)
{
    EXP_stk *eps;
    EXPR_struct *exp;
    AMflg amflg;
    char *cp;
    int es;
    long tv;

    amp->mode = E_NUL;       /* assume no mode found */
    amp->eamode = 0;
    amp->base_reg = amp->index_reg = 0;
    amp->extension = 0;
    amp->num_exprs = 0;      /* no expressions at the moment */
    amp->parts = 0;      /* no address parts found yet */
    eps = amp->exp[0];
    exp = eps->stack;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0) return 0;
    amflg = AMFLG_none;      /* no special flags present */
    if (*inp_ptr == '#')
    {   /* immediate? */
        ++inp_ptr;        /* yep, eat the char */
        get_token();      /* prime the pump */
        exprs(1,eps);     /* pickup the expression */
        amp->mode = E_IMM;
        amp->eamode = Ea_IMM;
        eps->tag = ea_to_tag[bwl];
        return 1;
    }
    cp = inp_ptr;
    if (get_token() == EOL)
    {    /* prime the pump */
        bad_token(tkn_ptr,"Expected an address mode here");
        return 0;
    }
    squawk_experr = 0;       /* put expression evaluator in special mode */
    es = exprs(1,eps);       /* get any kind of expression */
    squawk_experr = 1;       /* put expression evaluator back in normal mode */
    if (es < 0 ) return 0;   /* it's nfg */
    if (eps->register_mask)
    {
        amp->mode = E_RLST;   /* it was a register list */
        return 1;         /* return success */
    }
    if (eps->register_reference && eps->paren_cnt == 0)
    { /* if first term a simple register */
        if ( eps->ptr != 1 || exp->expr_code != EXPR_VALUE || eps->forward_reference )
        {
            bad_token(cp,"Register expression must resolve to an absolute value");
            eps->ptr = 0;
            return 0;
        }
        tv = exp->expr_value&~(FORCE_WORD|FORCE_LONG);        /* register number */
        amp->base_reg = tv;
        if (eps->autodec || eps->autoinc)
        { /* auto modes */
            amp->base_reg = tv&7;
            if (tv < REG_An || tv > REG_An+7)
            {
                bad_token(cp,"Autoinc/autodec modes only valid on A registers");
                eps->ptr = 0;
                return 0;
            }
            if (eps->autodec)
            {
                amp->mode = E_DAN;      /* autodec */
                amp->eamode = Ea_DAN;
            }
            else
            {
                amp->mode = E_ANI;      /* autoinc */
                amp->eamode = Ea_ANI;
            }
            return 1;
        }
        if (eps->paren)
        {         /* simple register indirect */
            if (tv > REG_An+7 && tv != REG_PC && tv != REG_ZPC)
            {
                bad_token(cp,"Register indirect only valid with A, D or PC registers");
                eps->ptr = 0;
                return 0;
            }
            if (tv >= REG_An && tv <= REG_An+7)
            {  /* A reg indirect */
                amp->mode = E_ATAn;
                amp->eamode = Ea_ATAn;
                amp->base_reg = tv&7;   /* the base reg is only one of 8 */
                return 1;
            }
            if (tv < REG_An)
            {     /* D reg indirect is special */
                amp->extension = (tv<<EXT_V_INDEX) | ((exp->expr_value&FORCE_LONG) ? EXT_ILONG:0) |
                                 (eps->register_scale<<EXT_V_SCALE) | 0x0110 | EXT_BASESUPRESS |
                                 EXT_IIS_I0OD;
                amp->mode = E_NDX;
                amp->eamode = Ea_NDX;
                amp->base_reg = 0;  /* no base register in this mode */
                return 1;
            }
            amp->extension = 0x0150;
            amp->mode = E_PCN;
            amp->eamode = Ea_PCN;
            amp->base_reg = 0; /* no base register in this mode */
            eps->ptr = 0;      /* zap the expression */
            return 1;
        }
        if ((exp->expr_value&(FORCE_WORD)) != 0)
        {
            show_bad_token(cp,"Forced word mode ignored in this context",MSG_WARN);
        }
        exp->expr_value = tv;
        amp->movec_xlate = 0xFFF;     /* assume invalid special */
        if (tv < REG_An+0)
        {
            amp->mode = E_Dn;      /* D register */
            amp->eamode = Ea_Dn;
        }
        else if (tv < REG_An+8)
        {
            amp->mode = E_An;      /* A register */
            amp->eamode = Ea_An;
        }
        else for (es = 0;reg_xref[es].reg_num;++es)
            {
                if (reg_xref[es].reg_num == tv)
                {
                    amp->mode = reg_xref[es].mode;
                    amp->movec_xlate = reg_xref[es].movec;
                    break;
                }
                if (reg_xref[es].reg_num==0)
                {
                    bad_token(cp,"Not a valid register in this context");
                    return 0;
                }
            }
        amp->reg = tv;        /* remember the register number */
        amp->base_reg &= 7;   /* base reg can only be 0-7 */
        eps->ptr = 0;
        return 1;
    }
    amp->state = EASTATE_bd; /* assume we're to look for a base displacement */
    if (es == 0)
    {       /* expr failed. Maybe a '([' or '(,' expression */
        if (eps->paren_cnt != 1)
        { /* no parens, then this is illegal */
            bad_token(cp,"Expected an address mode expression here");
            return 0;
        }
        if (*inp_ptr == '[')
        {    /* if we stopped on a bracket */
            amflg |= AMFLG_bracket; /* remember that we have a bracket, but get the bd next */
        }
        else if (*inp_ptr == ',')
        { /* if we stopped on a comma, then the bd is null */
            exp->expr_code = EXPR_VALUE;
            exp->expr_value = 0;
            eps->ptr = 1;      /* 1 term with value of 0 */
            amp->parts |= EAPARTS_BASED; /* say we've got a base displacement */
        }
        else
        {
            bad_token(inp_ptr,"Invalid address mode syntax");
            return 0;
        }
    }
    else
    {         /* the first term was found */
        if (*inp_ptr != '(')
        {    /* if we didn't stop on an open paren... */
            if (eps->paren_cnt == 0)
            { /* and there were no imbalanced parens, then we have the whole am */
                eps->tag_len = 1;
                if (!(edmask&ED_AMA))
                {
                    fix_pcr(amp,pcoff);
                    amp->mode = E_PCR;
                    amp->eamode = Ea_PCR;
                    eps->tag = 'I';  /* displacements are signed words */
                }
                else
                {
                    amp->mode = E_ABS;
                    if (!eps->force_long && (eps->force_short || (   eps->ptr == 1
                                                                  && exp->expr_code == EXPR_VALUE
                                                                  && !eps->forward_reference
                                                                  && exp->expr_value >= -32678
                                                                  && exp->expr_value < 32768
                                                                 )
                                             )
                        )
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
                return 1;
            }
            if (eps->paren_cnt != 1 || *inp_ptr != ',')
            { /* if not exactly 1 unmatched '(' and stop... */
                bad_token(cp,"Invalid address mode syntax"); /*...on comma, then syntax is nfg */
                return 0;
            }
        }
        if (eps->register_reference)
        {    /* the displacement is a register */
            inp_ptr = cp;          /* backup and prepare to rescan */
            exp->expr_value = 0;       /* set the displacement to 0 */
            eps->register_reference = 0;   /* not a register */
            eps->ptr = 1;
        }
        amp->parts |= EAPARTS_BASED; /* say we've got a base displacement */
    }
    if ((amp->parts&EAPARTS_BASED) != 0)
    {
        amp->state = EASTATE_br;      /* next get the base register */
        ++amp->num_exprs;         /* move expression ptr */
        eps = amp->exp[amp->num_exprs]; /* point to next expression stack */
        exp = eps->stack;
    }
    ++inp_ptr;       /* eat whatever we stopped on */
    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; 
    cp = inp_ptr;        /* remember where we started */
    while (amp->state != EASTATE_done)
    { /* until the whole argument is processed */
        long treg;
        switch (amp->state)
        {
        case EASTATE_bd: { /* get the base displacement */
                es = get_term(amp,0,eps,"Invalid base displacement syntax");
                if (es < 0) return 0;   /* didn't like the expression */
                if (*inp_ptr != ',')
                {  /* there'd better be more stuff */
                    bad_token(cp,"Expected to find a base displacement here");
                    return 0;
                }
                if (es == 0)
                {  /* if there was a null expression */
                    exp->expr_code = EXPR_VALUE;
                    exp->expr_value = 0;
                    eps->ptr = 1;        /* pretend 1 term with value of 0 */
                }
                amp->parts |= EAPARTS_BASED; /* say we've got a base displacement */
                amp->state = EASTATE_br;    /* next, pickup base register */
                ++amp->num_exprs;       /* move expression ptr */
                eps = amp->exp[amp->num_exprs]; /* point to next expression stack */
                exp = eps->stack;
                ++inp_ptr;          /* eat the comma */
                continue;
            }
        case EASTATE_br: {     /* the only thing we'll allow next is a br */       
                es = get_term(amp,GTERM_D|GTERM_A|GTERM_PC,eps,"Base register can only be A or PC");
                if (es < 0) return 0;   /* he didn't like the expression */
                if (es > 0)
                {
                    treg = exp->expr_value&~(FORCE_WORD|FORCE_LONG);
                    if (treg < REG_An)
                    { /* if he used a D reg as the base register */
                        goto got_indexreg;    /* pretend it was an index reg */
                    }
                    amp->base_reg = treg;
                    amp->parts |= EAPARTS_BASER; /* say we found a BASE register */
                    if (eps->register_scale != 0)
                    {
                        bad_token(tkn_ptr,"Scale factor on a BASE register is ignored");
                    }
                }
                eps->ptr = 0;       /* zap the expression */
                if ((amflg&AMFLG_bracket) != 0 && *inp_ptr == ']')
                {
                    amflg |= AMFLG_postindx; /* signal a postindex address mode */
                    ++inp_ptr;       /* eat the bracket */
                    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                    cp = inp_ptr;        /* remember start of next term */
                    amflg &= ~AMFLG_bracket; /* brackets not allowed anymore */
                }
                if (*inp_ptr == ')')
                {  /* if this is all there is */
                    ++inp_ptr;       /* eat the paren */
                    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                    cp = inp_ptr;        /* remember start of next term */
                    amp->state = EASTATE_done; /* we're done getting parts */
                    continue;
                }
                if (*inp_ptr != ',')
                {  /* else next thing better be a comma */
                    bad_token(inp_ptr,"Expected an index register expression here");
                    return 0;
                }
                ++inp_ptr;          /* eat the comma */
                while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                cp = inp_ptr;       /* remember start of next term */
                amp->state = EASTATE_ir;    /* next go look for an index register */
                continue;
            }
        case EASTATE_ir: {
                es = get_term(amp,GTERM_D|GTERM_A,eps,"Index can only be an A or D register");
                if (es < 0) return 0;   /* he didn't like the expression */
                if (es > 0)
                {
                    treg = exp->expr_value&~(FORCE_WORD|FORCE_LONG);
                    got_indexreg:
                    amp->index_reg = exp->expr_value;
                    amp->parts |= EAPARTS_INDXR; /* say we found a BASE register */
                    amp->index_scale = eps->register_scale;
                }
                eps->ptr = 0;       /* zap the expression */
                if ((amflg&AMFLG_bracket) != 0 && *inp_ptr == ']')
                {
                    amflg |= AMFLG_preindx; /* signal a preindex address mode */
                    ++inp_ptr;       /* eat the bracket */
                    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                    cp = inp_ptr;        /* remember start of next term */
                }
                if (*inp_ptr == ')')
                {  /* if this is all there is */
                    ++inp_ptr;       /* eat the paren */
                    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                    cp = inp_ptr;        /* remember start of next term */
                    amp->state = EASTATE_done; /* we're done getting parts */
                    continue;
                }
                if (*inp_ptr != ',')
                {  /* else next thing better be a comma */
                    bad_token(inp_ptr,"Expected an index register expression here");
                    return 0;
                }
                ++inp_ptr;          /* eat the comma */
                while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                cp = inp_ptr;       /* remember start of next term */
                amp->state = EASTATE_od;    /* next go look for an index register */
                continue;
            }
        case EASTATE_od: {
                es = get_term(amp,0,eps,"Invalid index displacement syntax");
                if (es < 0) return 0;
                if (*inp_ptr != ')')
                {  /* there'd better not be more stuff */
                    bad_token(cp,"Expected to find close paren here");
                    return 0;
                }
                ++inp_ptr;          /* eat the close paren */
                while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
                if (es > 0)
                {       /* if there was a non-null expression */
                    amp->parts |= EAPARTS_INDXD; /* say we've got an outer displacement */
                    ++amp->num_exprs;    /* move expression ptr */
                }
                amp->state = EASTATE_done;  /* were done */
                continue;
            }
        }             /* --switch (state) */
    }                /* --while (!done) */
    fixup_ea(amp,amflg,valid,pcoff);
    return 1;
}

static int get_twoea( int bwl, unsigned long valid, int pcoff)
{
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
    {
        if (get_oneea(&source,bwl,valid,pcoff) > 0)
        {
            if (*inp_ptr == ',')
            {
                ++inp_ptr;
            }
            else
            {
                if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
                {
                    show_bad_token(inp_ptr,"Expected a comma here, one assumed",MSG_WARN);
                }
                else
                {
                    bad_token(inp_ptr,"Expected a second operand");
                    return 0;
                }
            }
            return get_oneea(&dest,bwl,valid,pcoff);
        }
        return 0;
    }
    bad_token((char *)0,"Instruction requires 2 operands");
    return 0;
}


static void bad_sourceam( void )
{
    EXP_stk *eps;
    EXPR_struct *exp;
    int i;
    bad_token((char *)0,"Invalid address mode for first operand");
    source.mode = E_Dn;
    source.eamode = Ea_Dn;
    source.base_reg = 0;
    source.index_reg = 0;
    for (i=0;i<2;++i)
    {
        eps = source.exp[i];
        eps->ptr = 0;
        exp = eps->stack;
        exp->expr_code = EXPR_VALUE;
        exp->expr_value = 0;
        eps->tag_len = 1;
        eps->tag = 'W';
    }
    f1_eatit();
    return;
}

static void bad_destam( void )
{
    EXP_stk *eps;
    EXPR_struct *exp;
    int i;
    bad_token((char *)0,"Invalid address mode for second operand");
    dest.mode = E_Dn;
    dest.eamode = Ea_Dn;
    dest.base_reg = dest.index_reg = 0;
    for (i=0;i<2;++i)
    {
        eps = dest.exp[i];
        eps->ptr = 0;
        exp = eps->stack;
        exp->expr_code = EXPR_VALUE;
        exp->expr_value = 0;
        eps->tag_len = 1;
        eps->tag = 'W';
    }
    f1_eatit();
    return;
}

static int immediate_mode( unsigned short opcode, int bwl)
{
    if ((source.mode&E_IMM) == 0) bad_sourceam();
    EXP0SP->expr_value = opcode | (bwl<<6) | dest.eamode | (dest.base_reg&7);
    EXP0.ptr = 1;
    source.exp[0]->tag = ea_to_tag[bwl&3];
    if (source.exp[0]->ptr < 1) source.exp[0]->ptr = 1;
    return 1;
}

/*****************************************************************************
        type0
    Type 0.
    Instructions:
    ABCD,CMPM,SBCD,ADDX,SUBX
*****************************************************************************/
int type0( int inst, int bwl)
{
    int rm;
    static unsigned short optabl0[] = {
        0xc100,     /* ABCD */
        0xb108,     /* CMPM */
        0x8100,     /* SBCD */
        0xd100,     /* ADDX */
        0x9100};    /* SUBX */

    if (get_twoea(bwl,E_ANY,2) == 0)
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
    EXP0SP->expr_value = optabl0[inst-1]|rm|(dest.base_reg<<9)|source.base_reg;
    return 1;
}

/******************************************************************************
        type1
    type 1
    instructions:
    ADD,SUB,ADDA,SUBA,ADDI,SUBI,ADDQ,SUBQ
******************************************************************************/
int type1( int inst, int bwl)
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
    if (get_twoea(bwl,E_ANY,2) == 0)
    {
        EXP0SP->expr_value = optabl1[inst];
        EXP0.ptr = 1;
        return 0;
    }
    eps = source.exp[0];
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
            if (   !(eps->force_short|eps->force_long)
                && eps->ptr == 1
                && eptr->expr_code == EXPR_VALUE
                && !eps->forward_reference
                && (eptr->expr_value > 0 && eptr->expr_value <= 8)
               )
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
                    if ((dest.mode&~E_AMA) != 0 || (dest.mode&E_AMA) == 0)
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
                EXP0SP->expr_value = optabl1[inst] | (bwl<<6);
                if ((bwl&4) != 0 || bwl == 3)
                {
                    EXP0SP->expr_value |= (source.base_reg<<9) | (dest.eamode+dest.base_reg);
                }
                else
                {
                    EXP0SP->expr_value |= (dest.base_reg<<9) | (source.eamode+source.base_reg);
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
            EXP0SP->expr_value = optabl1[inst] | (bwl<<6) | (dest.base_reg<<9) | source.eamode | source.base_reg;
            EXP0.ptr = 1;
            return 1;
        }
    case 2:         /* addi */
    case 6: {       /* subi */
            if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
            immediate_mode(optabl1[inst],bwl);
            return 1;
        }
    case 3:         /* addq */
    case 7: {       /* subq */
            if ((source.mode&E_IMM) == 0) bad_sourceam();
            if ((dest.mode&(E_AMA|E_Dn|E_An)) == 0) bad_destam();
            typeF(optabl1[inst],&source,bwl,&dest);
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

    if (get_twoea(bwl,E_ANY,2) == 0)
    {
        EXP0SP->expr_value = optabl2[inst];
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
                if ((dest.mode&~E_AMA) != 0 || (dest.mode&E_AMA) == 0)
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
            EXP0SP->expr_value = optabl2[inst] | (bwl<<6);
            if ((bwl&4) != 0 || bwl == 3)
            {
                EXP0SP->expr_value |= (source.base_reg<<9) | (dest.eamode+dest.base_reg);
            }
            else
            {
                EXP0SP->expr_value |= (dest.base_reg<<9) | (source.eamode+source.base_reg);
            }
            EXP0.ptr = 1;
            return 1;
        }
    }
    if ((source.mode&E_IMM) == 0)
    {     /* has to be immediate mode */
        bad_sourceam();
        source.exp[0]->ptr = 1;         /* make it immediate */
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
            source.exp[0]->tag = 'b';
        }
        else
        {
            if (bwl != 1) show_bad_token((char *)0,"Only word operations performed on SR",MSG_WARN);
            source.exp[0]->tag = 'W';
        }       
        EXP0SP->expr_value = optabl2s[inst];
        EXP0.ptr = 1;
        return 1;
    }               /* -- if (special registers) */
    if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
    EXP0SP->expr_value = optabl2[inst] | (bwl<<6) | (dest.eamode+dest.base_reg);
    EXP0.ptr = 1;
    return 1;
}

/******************************************************************************
        type3
    type 3
    Instructions
    ASL,ASR,LSL,LSR,ROL,ROR,ROXL,ROXR
******************************************************************************/
int type3( int inst, int bwl)
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
    if (get_oneea(&source,bwl,E_ANY,2) == 0)
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
            get_oneea(&dest,bwl,E_ANY,2);
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
            EXP0SP->expr_value = optabl3r[inst]|(1<<9)|(bwl<<6)|source.base_reg;
        }
        else
        {
            if (bwl != 1)
            {
                show_bad_token((char *)0,"Only words can be shifted with this address mode",MSG_WARN);
            }
            EXP0SP->expr_value = optabl3m[inst] | source.eamode | source.base_reg;
        }
        EXP0.ptr = 1;
        return 1;
    }               /* -- one operand mode */
    if ((dest.mode&E_Dn) == 0) bad_destam();    /* dest must be Dn */
    if ((source.mode&E_IMM) != 0)
    { /* source immediate? */
        typeF(optabl3r[inst],&source,bwl,&dest);    /* do 'quick' format */
        return 1;
    }
    if ((source.mode&E_Dn) == 0) bad_sourceam(); /* else source must be Dn */
    EXP0SP->expr_value = optabl3r[inst] | (source.base_reg<<9) | (bwl<<6) |
                         0x20 | dest.base_reg;
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
int type4( int inst, int bwl)
{
    int t,edmaskSave;
    EXP_stk *eps;
    EXPR_struct *expr;
    long tv;
    static unsigned short optabl4[] = { 0,
        0x6400,0x6500,0x6700,0x6c00,
        0x6e00,0x6200,0x6f00,0x6300,
        0x6d00,0x6b00,0x6600,0x6a00,
        0x6800,0x6900,0x6000,0x6100};

    edmaskSave = edmask;
    edmask |= ED_AMA;        /* default to ABS on branch */
    t=get_oneea(&source,bwl,E_ANY,2);
    edmask = edmaskSave;
    if (t == 0 || (source.mode&E_ABS) == 0)
    {
        if (t == 0)
        {
            bad_token((char *)0,"Instruction requires an operand");
        }
        else
        {
            bad_token((char *)0,"Illegal address mode for branch instructions");
        }
        source.exp[0]->ptr = 0;
        EXP0SP->expr_value = optabl4[inst] | 0xFE;
        EXP0.ptr = 1;
        return 1;
    }
    eps = source.exp[0];
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
    if ( eps->ptr == 1 && expr->expr_code == EXPR_VALUE && !eps->forward_reference )
    {
        int cndb=0,cndw;
        tv = expr->expr_value;
        cndw = (tv >= -32768 && tv < 32768);
        if (cndw) cndb = (tv != 0 && tv != -1 && tv >= -128 && tv < 128);
        if (bwl == 3 || cndb)
        {
            if (!cndb)
            {
                long toofar;
                toofar = expr->expr_value;
                if (toofar > 0)
                {
                    toofar -= 127;
                }
                else
                {
                    toofar = -toofar - 128;
                }
                sprintf(emsg,"Branch offset 0x%lX byte(s) out of range",toofar);
                bad_token((char *)0,emsg);
                EXP0SP->expr_value = optabl4[inst] | 0xFE;
            }
            else
            {
                EXP0SP->expr_value = optabl4[inst] | (tv&0xFF);
            }
            eps->ptr = 0;
            source.mode = source.eamode = 0;
            return 1;
        }
        if (bwl == 1 || cndw)
        {
            EXP0SP->expr_value = optabl4[inst];
            eps->tag = 'I';
            if (!cndw)
            {
                long toofar;
                toofar = expr->expr_value;
                if (toofar > 0)
                {
                    toofar -= 32767;
                }
                else
                {
                    toofar = -toofar - 32768;
                }
                sprintf(emsg,"Branch offset 0x%lX byte(s) out of range",toofar);
                bad_token((char *)0,emsg);
                expr->expr_value = -4;
            }
        }
        else
        {
            EXP0SP->expr_value = optabl4[inst] | 0xFF;
            eps->tag = 'L';
        }
    }
    else
    {
        EXP0.ptr = 1;
        EXP0.tag = "WWWb"[bwl];
        eps->tag = "IILz"[bwl];  /* set extended displacement size */
        if (bwl == 3)
        {      /* forced byte mode? */
            EXP_stk *ep2;
            EXPR_struct *exp2;
            ep2 = source.exp[1];
            exp2 = ep2->stack;
            memcpy(exp2,expr,eps->ptr*sizeof(EXPR_struct));
            exp2 = ep2->stack + eps->ptr;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = 0;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_PICK;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = 0;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_EQ<<8);
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_XCHG;
            exp2->expr_code = EXPR_VALUE;
            (exp2++)->expr_value = -1;
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_EQ<<8);
            exp2->expr_code = EXPR_OPER;
            (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_OR<<8);
            ep2->ptr = exp2 - ep2->stack;
            sprintf(emsg,"Bxx.S with branch displacement of 0 or -1 is illegal on line %d in %s",
                    current_fnd->fn_line,current_fnd->fn_name_only);
            ep2->tag = 0;
            write_to_tmp(TMP_TEST,0,ep2,0);
            write_to_tmp(TMP_ASTNG,strlen(emsg)+1,emsg,1);
            ep2->ptr = 0;     /* don't use this expression any more */
            EXP0SP->expr_value = optabl4[inst]>>8;    /* byte */
        }
        else if (bwl == 2)
        {
            EXP0SP->expr_value = optabl4[inst] | 0xFF;    /* long */
        }
        else
        {
            EXP0SP->expr_value = optabl4[inst];       /* short */
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

    if (get_twoea(bwl,E_ANY,2) == 0)
    {
        EXP0SP->expr_value = optabl5r[inst];
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
            bad_token((char *)0,"Instruction only allows long operation on D regs");
        }
        EXP0SP->expr_value = optabl5r[inst] | (source.base_reg<<9) | 
                             dest.eamode | dest.base_reg;
    }
    else
    {            /* if not use immediate form */
        if ((dest.mode&E_Dn) == 0 && bwl == 2)
        {    /* xxx.L command? */
            bad_token((char *)0,"Instruction only allows byte operation with this address mode");
        }
        EXP0SP->expr_value = optabl5m[inst] | (dest.eamode + dest.base_reg);
        source.exp[0]->tag = 'b';   /* byte in low bits of a word */
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
int type6( int inst, int bwl, EA *tsea, EA *tdea) 
{
    static unsigned short optabl6[] = { 0,
        0x4e71,
        0x4e70,
        0x4e73,
        0x4e77,
        0x4e75,
        0x4e76};

    EXP0SP->expr_value = optabl6[inst];
    EXP0.ptr = 1;
    return 1;
}

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
int type7( int inst, int bwl) 
{
    static unsigned short optabl7[] = { 0,
        0x4840,0x4ec0,0x4e80,0x4800,
        0x4400,0x4000,0x4600,0x4200,
        0x54c0,0x55c0,0x57c0,0x51c0,
        0x5cc0,0x5ec0,0x52c0,0x5fc0,
        0x53c0,0x5dc0,0x5bc0,0x56c0,
        0x5ac0,0x50c0,0x58c0,0x59c0,
        0x4ac0,0x4a00};

    if (get_oneea(&dest,bwl,E_ANY,2) == 0)
    {
        bad_token((char *)0,"Instruction requires 1 operand");
        if (inst < 4)
        {
            dest.exp[0]->ptr = 1;
            dest.mode = E_ABS;
            dest.eamode = Ea_ABSL;
            dest.exp[0]->ptr = 1;
            dest.exp[0]->tag = 'L';
        }
        else
        {
            dest.mode = E_Dn;
            dest.eamode = Ea_Dn;
            dest.base_reg = 0;
        }
    }
    if (inst < 4)
    {
        if ((dest.mode&(E_ATAn|E_DSP|E_NDX|E_ABS|E_PCR|E_PCN)) == 0)
        {
            bad_destam();
            dest.exp[0]->ptr = 1;
            dest.mode = E_ABS;
            dest.eamode = Ea_ABSL;
            dest.exp[0]->ptr = 1;
            dest.exp[0]->tag = 'L';
        }
    }
    else
    {
        if (inst < 26)
        {
            if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
        }
        else if (inst == 26)
        {
            if ((dest.mode&(E_ALL-E_IMM)) == 0) bad_destam();
        }
        else if (inst == 27)
        {
            if ((dest.mode&(E_An|E_Dn)) == 0) bad_destam();
        }
    }
    EXP0SP->expr_value = optabl7[inst] | (bwl<<6) | dest.eamode | dest.base_reg;
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
int type8( int inst, int bwl)
{

    if (get_twoea(bwl,E_ANY,2) == 0)
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
                source.exp[0]->stack->expr_code = EXPR_VALUE;
                source.exp[0]->stack->expr_value = 0;
                source.exp[0]->ptr = 1;
            }
            if ((dest.mode&(E_AMA|E_Dn|E_PCR|E_PCN)) == 0) bad_destam();
            immediate_mode(0x0c00,bwl);
            return 1;
        }
    }
    if ((dest.mode&(E_Dn|E_An)) == 0)
    {
        if ((source.mode&E_ANI) != 0 && (dest.mode&E_ANI) != 0)
        {
            EXP0SP->expr_value = 0xb108 | (dest.base_reg<<9) | (bwl<<6) | source.base_reg;
            EXP0.ptr = 1;
            return 1;
        }
        if ((source.mode&E_IMM) == 0)
        {
            bad_sourceam();
            source.mode = E_IMM;
            source.eamode = Ea_IMM;
            source.exp[0]->stack->expr_code = EXPR_VALUE;
            source.exp[0]->stack->expr_value = 0;
            source.exp[0]->ptr = 1;
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
        EXP0SP->expr_value = 0xb000 | (dest.base_reg<<9) | (bwl<<6) | source.eamode | source.base_reg;
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
int type9( int inst, int bwl)
{
    static unsigned short optabl9[] = { 0,
        0x54c8,0x55c8,0x57c8,0x51c8,0x5cc8,
        0x5ec8,0x52c8,0x5fc8,0x53c8,
        0x5dc8,0x5bc8,0x56c8,0x5ac8,0x50c8,
        0x58c8,0x59c8};
    EXP_stk *eps;
    EXPR_struct *expr;

    EXP0SP->expr_value = optabl9[inst];
    if ((get_twoea(bwl,E_ANY,2) == 0) || dest.mode != E_ABS)
    {
        bad_token((char *)0,"Illegal address mode for branch instructions");
        dest.exp[0]->ptr = 1;
        dest.exp[0]->stack->expr_value = -2;
        dest.exp[0]->tag = 'I';
        return 1;
    }
    if (source.mode != E_Dn)
    {
        bad_token((char *)0,"Source operand must be a D register");
        source.mode = E_Dn;
        source.base_reg = 0;
        source.exp[0]->ptr = 0;
    }
    EXP0SP->expr_value |= source.base_reg;
    eps = dest.exp[0];
    fix_pcr(&dest,2);
    eps->tag = 'I';
    expr = eps->stack;
    if ( eps->ptr == 1 && expr->expr_code == EXPR_VALUE && !eps->forward_reference )
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
int type10( int inst, int bwl) 
{
    static unsigned short optabl10[] = {
        0x81c0,     /* (0) DIVS.W */
        0x80c0,     /* (1) DIVU.W */
        0xc1c0,     /* (2) MULS.W */
        0xc0c0,     /* (3) MULU.W */
        0x4c40,     /* (4) DIVS.L */
        0x4c40,     /* (5) DIVSL.L */
        0x4c40,     /* (6) DIVU.L */
        0x4c40,     /* (7) DIVUL.L */
        0x4c00,     /* (8) MULS.L */
        0x4c00,     /* (9) MULU.L */
        0x4100};    /* (10) CHK */
    EXP_stk *eps;
    EXPR_struct *exp;
    int siz=4;
    if (inst < 4 || inst == 10) siz = 2;
    eps = &EXP0;
    exp = eps->stack;
    exp->expr_value = 0x4E71;
    if (get_twoea(bwl,E_ANY,siz) == 0) return 0;
    if ((source.mode&(E_ALL-E_An)) == 0) bad_sourceam();
    if ((dest.mode&E_Dn) == 0) bad_destam();
    dest.exp[0]->ptr = 0;
    exp->expr_value = optabl10[inst] | source.eamode | source.base_reg | ((inst==10)?(bwl<<7):0);
    if (inst < 4 || inst == 10)
    {
        exp->expr_value |= (dest.base_reg<<9);
        return 1;
    }
    eps->tag = 'L';     /* switch to long mode */
    exp->expr_value = (exp->expr_value << 16) | dest.base_reg;
    if (*inp_ptr == ':')
    {  /* if there's an optional register */
        ++inp_ptr;
        while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
        if (get_term(&dest,GTERM_D,dest.exp[0],"Mul/Div instructions can only have an optional D register") != 1)
        {
            EXP0SP->expr_value = 0x4E714E71;
            return 1;
        }
        dest.base_reg = dest.exp[0]->stack->expr_value;
        if (inst != 5) exp->expr_value |= 0x0400;
    }
    exp->expr_value |= (dest.base_reg<<12);
    if (inst == 4 || inst == 5 || inst == 8) exp->expr_value |= 0x0800;
    return 1;
}

/******************************************************************************
        type11
    type 11
    Instructions:
    EXG
******************************************************************************/
/*ARGSUSED*/
int type11( int inst, int bwl) 
{
    int omode,temp;
    unsigned short opcode;
    EXP0SP->expr_value = 0xC100;
    if (get_twoea(bwl,E_ANY,2) == 0) return 0;
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
            temp = source.base_reg;
            source.base_reg = dest.base_reg;
            dest.base_reg = temp;
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
    opcode |= dest.base_reg << 9;
    opcode |= source.base_reg;
    EXP0SP->expr_value = opcode;
    return 1;
}


/******************************************************************************
        type12
    type 12
    Instructions:
    EXT,SWAP,UNLK,RTM
******************************************************************************/
/*ARGSUSED*/
int type12( int inst, int bwl) 
{
    static unsigned short optabl12[] = { 0,
        0x4800,     /* ext */
        0x4840,     /* swap */
        0x4e58,     /* unlk */
        0x4e70};    /* rtm */
    unsigned short opcode,sam;

    opcode = optabl12[inst];
    get_oneea(&source,bwl,E_ANY,2);
    if (inst < 3)
    {
        sam = E_Dn;
    }
    else if (inst < 4)
    {
        sam = E_An;
    }
    else
    {
        sam = E_An|E_Dn;
    }
    if ((source.mode&sam) == 0) bad_sourceam();
    if (inst == 1) opcode |= bwl<<6;
    opcode |= source.exp[0]->stack->expr_value;
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
int type13( int inst, int bwl)
{
    EXP0SP->expr_value = 0x41c0;
    if (get_twoea(bwl,E_ANY,2) == 0) return 0;
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
    EXP0SP->expr_value |= (dest.base_reg<<9) | source.eamode | source.base_reg;
    return 1;
}

/******************************************************************************
        type14
    type 14
    Instructions:
    LINK
******************************************************************************/
/*ARGSUSED*/
int type14( int inst, int bwl) 
{
    get_twoea(bwl,E_ANY,2);
    if ((source.mode&E_An) == 0) bad_sourceam();
    if ((dest.mode&E_IMM) == 0)
    {
        bad_destam();
        dest.mode = E_IMM;
        dest.eamode = Ea_IMM;
        dest.exp[0]->stack->expr_code = EXPR_VALUE;
        dest.exp[0]->stack->expr_value = 0;
        dest.exp[0]->ptr = 1;
    }
    dest.exp[0]->tag = 'I';
    EXP0SP->expr_value = 0x4e50 | source.base_reg;
    return 1;
}

/******************************************************************************
        type15
    type 15
    Instructions:
        MOVE MOVEA MOVEQ MOVES
******************************************************************************/
/*ARGSUSED*/
int type15( int inst, int bwl) 
{
    unsigned short opcode;
    static short stran[] = {1,3,2};

    if (get_twoea(bwl,E_ANY,2) == 0)
    {
        EXP0SP->expr_value = 0x1000;
        return 1;
    }
    if (inst != 0)
    {            /* if not generic move */
        switch (inst)
        {
        case 1:           /* movea */
            if (dest.mode != E_An)
            {   /* movea can only have A reg as dest*/
                bad_destam();       /*  must have An for dest */
                dest.mode = E_An;
                dest.eamode = Ea_An;
                dest.exp[0]->ptr = 0;
                dest.base_reg = 0;
            }
            break;
        case 2:           /* moveq */
            if (source.mode != E_IMM) bad_sourceam();  /* source must be immediate */
            if (dest.mode != E_Dn) bad_destam();
            EXP0SP->expr_value = 0x70 | (dest.base_reg<<(9-8));
            EXP0.tag = 'b';
            source.exp[0]->tag = 'b';
            return 0;
        case 3:           /* moves */
            if ((source.mode&(E_Dn|E_An)) != 0)
            {
                if ((dest.mode&E_AMA) == 0) bad_destam();
                EXP0SP->expr_value = 0x0E000800 | (bwl<<22) |
                                     ((dest.eamode | dest.base_reg)<<16) |
                                     (source.reg<<12);
            }
            else
            {
                int tst = E_Dn|E_An;
                if ((source.mode&E_AMA) == 0)
                {
                    bad_sourceam();
                    tst |= E_AMA;
                }
                if ((dest.mode&tst) == 0) bad_destam();
                EXP0SP->expr_value = 0x0E000000 | (bwl<<22) |
                                     ((source.eamode | source.base_reg)<<16) |
                                     (dest.reg<<12);
            }
            EXP0.tag = 'L';
            return 0;
        case 4:           /* movec */
            movec_spec:
            if ((source.mode&(E_Dn|E_An)) != 0)
            {
                if ((dest.mode&(E_SPCL|E_USP)) == 0 || dest.movec_xlate == 0xFFF) bad_destam();
                EXP0SP->expr_value = 0x4E7B0000 | (source.reg<<12) | dest.movec_xlate;
            }
            else
            {
                int tst = E_Dn|E_An;
                if ((source.mode&(E_SPCL|E_USP)) == 0 || source.movec_xlate == 0xFFF)
                {
                    bad_sourceam();
                    tst |= E_SPCL|E_USP;
                }
                if ((dest.mode&tst) == 0) bad_destam();
                EXP0SP->expr_value = 0x4E7A0000 | (dest.reg<<12) | source.movec_xlate;
            }
            EXP0.tag = 'L';
            return 0;
        }
    }
    if ((dest.mode&~E_ALL) != 0 || (source.mode&~E_ALL) != 0)
    {
        if ((dest.mode&E_SPCL) != 0 || ((source.mode&E_SPCL) != 0 && bwl == 2)) goto movec_spec;
        if ((source.mode&~E_ALL) != 0)
        {
            if ((source.mode&E_USP) != 0)
            {
                if ((dest.mode&E_An) == 0) bad_destam();
                opcode = 0x4e68 | dest.base_reg;
            }
            else
            {
                if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
                if (bwl != 1)
                {
                    if (bwl != 3) show_bad_token((char *)0,"Only word mode moves allowed from CCR/SR",MSG_WARN);
                    bwl = 1;
                }
                if (source.mode == E_CCR)
                {
                    opcode = 0x42c0 | dest.eamode | dest.base_reg;
                }
                else if (source.mode == E_SR)
                {
                    opcode = 0x40c0 | dest.eamode | dest.base_reg;
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
            opcode = 0x4e60 | source.base_reg;
        }
        else
        {
            if ((source.mode&(E_ALL-E_An)) == 0) bad_sourceam();
            if (bwl != 1)
            {
                if (bwl != 3) show_bad_token((char *)0,"Only word mode moves allowed to CCR/SR",MSG_WARN);
                bwl = 1;
            }
            if (dest.mode == E_CCR)
            {
                opcode = 0x44c0 | source.eamode | source.base_reg;
            }
            else if (dest.mode == E_SR)
            {
                opcode = 0x46c0 | source.eamode | source.base_reg;
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
        if (bwl == 3) bwl = 2;      /* default move size is long */
        EXP0SP->expr_value = 0x2040 | (stran[bwl]<<12) | (dest.base_reg<<9) | source.eamode | source.base_reg;
        return 1;
    }
    if ((dest.mode&(E_AMA|E_Dn)) == 0) bad_destam();
    if (dest.mode == E_Dn && source.mode == E_IMM && bwl == 2)
    {
        EXP_stk *eps;
        EXPR_struct *exp;
        eps = source.exp[0];
        exp = eps->stack;
        if (   !(eps->force_short|eps->force_long)
            && eps->ptr == 1
            && exp->expr_code == EXPR_VALUE
            && !eps->forward_reference
            && (exp->expr_value >= -128 && exp->expr_value < 128)
           )
        {
            EXP0SP->expr_value = 0x7000 | (dest.base_reg<<9) | (exp->expr_value&255);
            eps->ptr = 0;
            return 1;
        }
    }
    if (bwl == 3) bwl = 2;      /* default move size is long */
    opcode = ((dest.eamode<<3)&0700) | (((dest.eamode|dest.base_reg)<<9)&07000); 
    EXP0SP->expr_value = (stran[bwl]<<12) | opcode | source.eamode | source.base_reg;
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
int type16( int inst, int bwl) 
{
    int dr;
    unsigned short reg_mask;
    EA *mask,*eadd;

    if (get_twoea(bwl,E_ANY,2) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if ((source.mode&(E_RLST|E_An|E_Dn)) == 0)
    {
        if ((dest.mode&(E_RLST|E_An|E_Dn)) == 0)
        {
            bad_destam();
            dest.exp[0]->ptr = 1;
            dest.exp[0]->stack->expr_code = EXPR_VALUE;
            dest.exp[0]->stack->expr_value = 0;
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
    if ((mask->mode&E_An) != 0)
    {
        reg_mask = 1<<(mask->base_reg+8);
    }
    else if ((mask->mode&E_Dn) != 0)
    {
        reg_mask = 1<<(mask->base_reg);
    }
    else
    {
        reg_mask = mask->exp[0]->stack->expr_value;
    }
    if (dest.mode == E_DAN)
    { /* for -(An) destination mode reverse register mask */
        reg_mask = (flip[(reg_mask >> 12) & 0xF]) |
                   (flip[(reg_mask >> 8) & 0xF] << 4) |
                   (flip[(reg_mask >> 4) & 0xF] << 8) |
                   (flip[reg_mask & 0xF] << 12);
    }
    EXP0SP->expr_value = 0x4880 | dr | (bwl == 2 ? 0x40 : 0) | eadd->eamode | eadd->base_reg;
    mask->exp[0]->psuedo_value = mask->exp[0]->stack->expr_value = reg_mask;
    mask->exp[0]->ptr = 1;
    mask->exp[0]->tag = 'W';
    mask->exp[0]->tag_len = 1;
    mask->eamode = Ea_IMM;  /* fool output routine */
    if (dr != 0)
    {
        EA temp;
        temp = source;
        source = dest;
        dest =  temp;
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
int type17( int inst, int bwl) 
{
    unsigned short opcode;

    if (get_twoea(bwl,E_DSP|E_Dn,2) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if (source.mode == E_Dn)
    {
        if (dest.mode != E_DSP)
        {
            bad_destam();
            dest.exp[0]->ptr = 1;
            dest.exp[0]->stack->expr_code = EXPR_VALUE;
            dest.exp[0]->stack->expr_value = 0;
        }
        opcode = 8 | (source.base_reg<<9) | ((bwl == 1) ? (6<<6):(7<<6)) | dest.base_reg;
    }
    else
    {
        if (source.mode != E_DSP)
        {
            bad_sourceam();
            source.exp[0]->ptr = 1;
        }
        if (dest.mode != E_Dn) bad_destam();
        opcode = 8 | (dest.base_reg<<9) | ((bwl == 1) ? (4<<6):(5<<6)) | source.base_reg;
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
int type18( int inst, int bwl) 
{

    EXP0SP->expr_value = 0x4e72;
    if (get_oneea(&source,bwl,E_ANY,2) == 0)
    {
        source.exp[0]->ptr = 1;
        return 1;
    }
    if (source.mode != E_IMM)
    {
        bad_sourceam();
        source.mode = E_IMM;
        source.eamode = Ea_IMM;
        source.exp[0]->stack->expr_code = EXPR_VALUE;
        source.exp[0]->stack->expr_value = 0;
        source.exp[0]->ptr = 1;
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
int type19( int inst, int bwl) 
{
    EXP_stk *eps;
    EXPR_struct *exp;

    if (get_oneea(&source,bwl,E_ANY,2) == 0)
    {
        return 1;
    }
    if (source.mode != E_IMM)
    {
        bad_sourceam();
        source.mode = E_IMM;
        source.eamode = Ea_IMM;
        source.exp[0]->stack->expr_code = EXPR_VALUE;
        source.exp[0]->stack->expr_value = 0;
        source.exp[0]->ptr = 1;
    }
    eps = source.exp[0];
    exp = eps->stack;
    if ( eps->ptr == 1 && exp->expr_code == EXPR_VALUE && !eps->forward_reference )
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
        sprintf(emsg,"%s:%d: TRAP vector out of range",
                current_fnd->fn_buff,current_fnd->fn_line);
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
        type20 
    type 20 instructions are the bit field operators. The have the form:
     _______________________
    | opcode        | ea    |
     -----------------------
    |dest|d0|offset|dw|width|
     -----------------------
    Instructions:
       (without dest): BFTST, BFCHG, BFCLR, BFSET
       (with dest):    BFEXTU, BFEXTS, BFFFO, BFINS
******************************************************************************/

int type20(int inst, int bwl) 
{
    EXP_stk *eps;
    EXPR_struct *exp;
    char *cp;
    unsigned long opcode;
    long tv;
    static unsigned short optabl20[] = {
        0xE8C0,     /* BFTST */
        0xE9C0,     /* BFEXTU */
        0xEAC0,     /* BFCHG */
        0xEBC0,     /* BFEXTS */
        0xECC0,     /* BFCLR */
        0xEDC0,     /* BFFFO */
        0xEEC0,     /* BFSET */
        0xEFC0};    /* BFINS */

    static unsigned long optabl20a[] = {
        E_ALL-E_An-E_ANI-E_DAN-E_IMM,   /* BFTST */
        E_ALL-E_An-E_ANI-E_DAN-E_IMM,   /* BFEXTU */
        (E_AMA-E_ANI-E_DAN)|E_Dn,    /* BFCHG */
        E_ALL-E_An-E_ANI-E_DAN-E_IMM,   /* BFEXTS */
        (E_AMA-E_ANI-E_DAN)|E_Dn,    /* BFCLR */
        E_ALL-E_An-E_ANI-E_DAN-E_IMM,   /* BFFFO */
        (E_AMA-E_ANI-E_DAN)|E_Dn,    /* BFSET */
        (E_AMA-E_ANI-E_DAN)|E_Dn};   /* BFINS */

    EXP0SP->expr_value = 0x4E714E71; /* in case of error */
    EXP0.tag = 'L';      /* this instruction is 32 bits */
    if (inst == 7)
    {
        EA temp;
        if (get_twoea(bwl,E_ANY,4) == 0) return 1;
        if (source.mode != E_Dn) bad_sourceam();
        if ((dest.mode&optabl20a[inst]) == 0) bad_destam();
        source.exp[0]->ptr = 0;       /* make sure there's no expression */
        temp = source;            /* switch source and dest */
        source = dest;
        dest = temp;
    }
    else
    {
        if (get_oneea(&source,bwl,E_ANY,4) == 0)
        {
            EXP0SP->expr_value = 0x4e71;
            return 1;
        }
        if ((optabl20a[inst]&source.mode) == 0)
        {
            bad_sourceam();
            f1_eatit();
            return 0;
        }
    }
    opcode = (optabl20[inst] | source.eamode | source.base_reg)<<16;
    if (inst == 7) opcode |= dest.base_reg<<12;
    eps = dest.exp[0];       /* point to a temp */
    exp = eps->stack;
    if (*inp_ptr == '{')
    {   /* we have a field width spec */
        ++inp_ptr;        /* eat the leading bracket */
        cp = inp_ptr;
        if (get_token() == EOL)
        { /* invalid syntax */
            bad_token(inp_ptr,"Premature end of line");
            return 1;
        }
        if (exprs(0,eps) != 1)
        {  /* invalid expression */
            bad_token(cp,"Invalid bit field offset syntax");
            return 1;
        }
        else
        {
            tv = exp->expr_value;
            if (eps->register_reference)
            {
                if (tv >= REG_An)
                {
                    bad_token(cp,"Only a D register is allowed as a bit field offset");
                    tv = 0;
                }
                opcode |= 0x0800 | (tv<<6);
            }
            else
            {
                if (tv <0 || tv > 31)
                {
                    bad_token(cp,"Bit field offset value out of range. S/B 0-31");
                    tv = 0;
                }
                opcode |= (tv<<6);
            }
        }
        if (*inp_ptr == ':')
        {    /* if we have a width field */
            long tv;
            ++inp_ptr; 
            cp = inp_ptr;
            if (get_token() == EOL)
            {  /* invalid syntax */
                bad_token(inp_ptr,"Premature end of line");
                return 1;
            }
            if (exprs(0,eps) != 1)
            {   /* invalid expression */
                bad_token(cp,"Invalid bit field width syntax");
                return 1;
            }
            tv = exp->expr_value;
            if (eps->register_reference)
            {
                if (tv >= REG_An)
                {
                    bad_token(cp,"Only a D register is allowed as a bit field width");
                    tv = 0;
                }
                opcode |= 0x0020 | tv;
            }
            else
            {
                if (tv <1 || tv > 32)
                {
                    bad_token(cp,"Bit field offset value out of range. S/B 1-32");
                    tv = 0;
                }
                opcode |= tv&31;
            }
        }
        if (*inp_ptr != '}')
        {
            bad_token(inp_ptr,"Expected to find a '}' here");
            return 1;
        }
        ++inp_ptr;        /* eat the closing brace */
        eps->ptr = 0;     /* eat the expression */
    }
    if ((inst&1) != 0 && inst != 7)
    {    /* we need a dest register */
        while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
        if (*inp_ptr != ',')
        {
            bad_token(inp_ptr,"Expected a comma here");
            return 1;
        }
        ++inp_ptr;
        if (get_term(&dest,GTERM_D,eps,"Bitfield instructions can only have D register as dest") != 1)
        {
            return 1;
        }
        opcode |= exp->expr_value << 12;
    }
    EXP0SP->expr_value = opcode;
    return 1;
}

/******************************************************************************
        type21
    type 21 instruction is CALLM. It has the form:
     _______________________
    | opcode        | ea    |
     -----------------------
    |      0    | arg count |
     -----------------------
    Instructions:
    CALLM
******************************************************************************/

int type21(int inst, int bwl) 
{
    EXP_stk *eps;
    EXPR_struct *exp;

    if (get_twoea(bwl,E_ANY,4) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    eps = source.exp[0];
    exp = eps->stack;
    if ((source.mode&E_IMM) == 0)
    {
        bad_sourceam();
        eps->ptr = 1;
        exp->expr_value = 0;
        exp->expr_code = EXPR_VALUE;
    }
    eps->tag = 'b';
    if ((dest.mode&(E_AMA-E_ANI-E_DAN)) == 0)
    {
        bad_destam();
        eps->ptr = 0;
        EXP0SP->expr_value = 0x4E714E71;  /* in case of error */
        EXP0.tag = 'L';       /* make this instruction 32 bits */
        return 1;
    }
    EXP0SP->expr_value = 0x06C0 | dest.eamode | dest.base_reg;
    return 1;
}

/******************************************************************************
        type22
    type 22 instructions are the compare and update instructions
     _______________________
    | opcode | size |  ea   |
     -----------------------	CAS format
    |   0  | Du  | Dc       |
     -----------------------
     -----------------------
    | opcode | size |   0   |
     -----------------------	CAS2 format
    | Rn1 | 0  | Du1  | Dc1 |
     -----------------------
    | Rn2 | 0  | Du2  | Dc2 |
     -----------------------
    Instructions:
       CAS and CAS2
******************************************************************************/

int type22(int inst, int bwl) 
{
    EXP_stk *eps;
    EXPR_struct *exp;
    if (inst == 0)
    {     /* if we're doing CAS */
        if (get_twoea(bwl,E_ANY,4) == 0)
        {
            EXP0SP->expr_value = 0x4e71;
            return 1;
        }
        if (source.mode != E_Dn || dest.mode != E_Dn)
        {
            bad_token((char *)0,"Both compare and update operands must be D registers");
            EXP0SP->expr_value = 0x4e71;
            source.exp[0]->ptr = source.exp[1]->ptr = 0;
            dest.exp[0]->ptr = dest.exp[1]->ptr = 0;
            f1_eatit();
            return 0;
        }
        EXP0SP->expr_value = 0x08c00000 | (bwl<<25) | ((dest.base_reg&7)<<6) | (source.base_reg&7);
        EXP0.tag = 'L';
        dest.exp[0]->ptr = 0;
        dest.mode = 0;
        source.exp[0]->ptr = 0;
        if (*inp_ptr == ',')
        {
            ++inp_ptr;
            while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
            get_oneea(&dest,bwl,E_AMA,4);  /* get an operand */
        }
        if ((dest.mode&E_AMA) == 0)
        {
            bad_token((char *)0,"Invalid address mode for destination operand");
        }
        EXP0SP->expr_value |= (dest.eamode | (dest.base_reg&7))<<16;
        return 1;
    }
    else
    {
        int i;
        static struct
        {
            unsigned long am;
            int val;
            char *msg;
        } cas2tabl[6] =  {{E_Dn,0,"Compare1 operand must be a D register"},
            {E_Dn,0,"Compare2 operand must be a D register"},
            {E_Dn,0,"Update1 operand must be a D register"},
            {E_Dn,0,"Update2 operand must be a D register"},
            {E_ATAn,0,"Destination operand must be a register indirect"},
            {E_ATAn,0,"Destination operand must be a register indirect"}};

        EXP0SP->expr_value = 0x4E71;
        eps = source.exp[0];
        exp = eps->stack;
        for (i=0;i<6;++i)
        {
            if (get_term(&source,cas2tabl[i].am,eps,cas2tabl[i].msg) <= 0)
            {
                f1_eatit();
                return 0;
            }
            cas2tabl[i].val = exp->expr_value;
            if (i < 5)
            {
                if (i&1)
                {
                    if (*inp_ptr != ',')
                    {
                        bad_token(inp_ptr,"Expected a comma here");
                        return 1;
                    }
                }
                else
                {
                    if (*inp_ptr != ':')
                    {
                        bad_token(inp_ptr,"Expected a colon here");
                        return 1;
                    }
                }          
                ++inp_ptr;
                while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
            }
        }
        EXP0SP->expr_value = 0x08FC | (bwl<<9);
        eps->ptr = 1;
        exp->expr_code = EXPR_VALUE;
        exp->expr_value = (cas2tabl[4].val<<28) | (cas2tabl[2].val<<22) | (cas2tabl[0].val<<16) | 
                          (cas2tabl[5].val<<12) | (cas2tabl[3].val<<6) | cas2tabl[1].val;
        eps->psuedo_value = exp->expr_value;
        eps->tag = 'L';
        source.eamode = Ea_IMM;
    }
    return 1;
}

/******************************************************************************
        type23
    type 23 instructions are the compare limit and check limit
     _______________________
    | opcode | size |  ea   |
     -----------------------	CHK2 format
    | Rn  | 0x?00	    |
     -----------------------

    Instructions:
       CMP2 and CHK2
******************************************************************************/

int type23(int inst, int bwl) 
{
    if (get_twoea(bwl,E_ANY,4) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if ((source.mode&(E_ALL-E_Dn-E_An-E_ANI-E_DAN-E_IMM)) == 0)
    {
        bad_sourceam();
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if ((dest.mode&(E_Dn|E_An)) == 0)
    {
        bad_destam();
        EXP0SP->expr_value = 0x4e71;
        source.exp[0]->ptr = source.exp[1]->ptr = 0;
        return 1;
    }
    EXP0.tag = 'L';      /* make this instruction 32 bits */
    EXP0SP->expr_value = ((0x00c0 | (bwl<<9) | source.eamode | source.base_reg)<<16) |
                         (dest.exp[0]->stack->expr_value<<12) | (inst<<11);
    return 1;
}

/******************************************************************************
        type24
    type 24 instructions are the pack and unpack instructions
     _______________________
    | 0x8000 | Rn1 |  Rn2   |
     -----------------------	Pack format
    |  Immediate value	    |
     -----------------------

    Instructions:
       PACK and UNPK
******************************************************************************/

int type24(int inst, int bwl) 
{
    EXP_stk *eps;
    EXPR_struct *exp;
    unsigned short optabl24[] = { 0x8140,0x8180};
    if (get_twoea(bwl,E_ANY,4) == 0)
    {
        EXP0SP->expr_value = 0x4e71;
        return 1;
    }
    if ((source.mode&(E_Dn|E_DAN)) == 0)
    {
        bad_sourceam();
        EXP0SP->expr_value = 0x4e71;
        return f1_eatit();
    }
    if ((dest.mode&(E_Dn|E_DAN)) == 0)
    {
        bad_destam();
        EXP0SP->expr_value = 0x4e71;
        return f1_eatit();
    }
    if (source.mode != dest.mode)
    {
        bad_token((char *)0,"Both source and dest address modes must be the same");
        EXP0SP->expr_value = 0x4e71;
        return f1_eatit();
    }
    EXP0SP->expr_value = optabl24[inst] | ((dest.base_reg&7)<<9) |
                         ((source.mode == E_DAN) ? 0x8:0) | (source.base_reg&7);
    eps = source.exp[0];
    exp = eps->stack;
    eps->ptr = 1;
    exp->expr_code = EXPR_VALUE;     /* assume a value of 0 */
    exp->expr_value = 0;         /* assume a value of 0 */
    source.eamode = Ea_IMM;
    source.mode = E_IMM;
    dest.exp[0]->ptr = 0;
    if (*inp_ptr == ',')
    {
        ++inp_ptr;
        while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
        get_oneea(&source,1,E_IMM,2); /* get an operand */
        if (source.mode != E_IMM)
        {
            bad_token((char *)0,"Expected an immediate operand for adjustment value");
            eps->ptr = 1;
            exp->expr_code = EXPR_VALUE;   /* assume a value of 0 */
            exp->expr_value = 0;       /* assume a value of 0 */
            source.eamode = Ea_IMM;
            source.mode = E_IMM;
        }
    }
    eps->tag = 'W';
    return 1;
}

/******************************************************************************
        type25
    type 25 instructions are the trap on condition
     _______________________
    | 0x5xFx | cond | bwl   |
     -----------------------	Trapcc format
    |  Immediate value	    |
     -----------------------

    Instructions:
       TRAPcc where cc= t,f,hi,ls,cc,cs,ne,eq,	(0-7)
            vc,vs,pl,mi,ge,lt,gt,le (8-F)
******************************************************************************/

int type25(int inst, int bwl) 
{
    EXP0SP->expr_value = 0x50F8 | (inst<<8) | bwl;
    if (bwl < 4)
    {
        int t;
        t = get_oneea(&source,bwl,E_IMM,2);
        if (t == 0 || source.mode != E_IMM)
        {
            bad_sourceam();
            EXP0SP->expr_value = 0x4e71;
            return 1;
        }
        source.exp[0]->tag = (bwl == 2) ? 'W':'L';
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
    eps = tsea->exp[0];
    exp = eps->stack;
    if ( eps->ptr == 1 && exp->expr_code == EXPR_VALUE && !eps->forward_reference )
    {
        if (exp->expr_value < 1 || exp->expr_value > 8)
        {
            show_bad_token((char *)0,"Quick immediate value out of range",MSG_ERROR);
        }
        EXP0SP->expr_code = EXPR_VALUE;
        EXP0SP->expr_value = opcode | (bwl<<6) | 
                             ((exp->expr_value&7)<<9) | tdea->eamode | tdea->base_reg;
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
        sprintf(emsg,"%s:%d: Quick immediate value out of range",
                current_fnd->fn_buff,current_fnd->fn_line); /* and message */
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
                              tdea->eamode | tdea->base_reg;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_OR;  /* and or in the opcode */
        eps->ptr += 6;          /* say there's 6 more items on stack */
        eps->tag = 'W';         /* opcode is a word */
        eps->tag_len = 1;       /* length of 1 */
        EXP0.ptr = 0;           /* no opcode stack */
    }
    tsea->mode = tsea->eamode = 0;  /* no mode to output */
    return 1;
}

int op_ntype(Opcode *opc)
{
    f1_eatit();
    return 0;
}
