/*
    m11types.c - Part of macxx, a cross assembler family for various micro-processors
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
#include <stdio.h>
#include <ctype.h>
#include "token.h"
#include "pdp11.h"
#include "exproper.h"
#include "listctrl.h"
#include "pst_tokens.h"

#define AMFLG_LEADMINUS (1) /* Found leading - */
#define AMFLG_INDIRECT  (2) /* Found leading @ */
#define AMFLG_DISPL     (4) /* found displacement */
#define AMFLG_TRAILPLUS (8) /* found a trailing + */
#define AMFLG_RESTART   (16) /* reset tkn_ptr for rescan */
#define AMFLG_IMMEDIATE	(32) /* Found # */

/* Expect a (reg) or (reg)+ syntax or if justReg non-zero, expect just a register expression */

static int get_regea(int amflg, EA *amp, int justReg)
{
    char *tmp;
    int treg;

    tmp = inp_ptr;      /* incase of rescan */
    if (get_token() == EOL)
        return 0;
    exprs(1,&EXP3);     /* pickup an expression */
    treg = EXP3SP->expr_value;
    if (EXP3.ptr != 1 || !EXP3.register_reference
        || EXP3SP->expr_code != EXPR_VALUE)
    {
        EXP3.ptr = 0;
        return 0;       /* and signal it wasn't a register */
    }
    if (treg > 7 )
    {
        bad_token(tkn_ptr,"Register value can only be 0 - 7");
        treg = 0;
    }
	if ( justReg )
	{
		EXP3.ptr = 0;   /* clear residual from EXP3 */
		amp->eamode |= treg;
		return 1;
	}
	if ( *inp_ptr != ')' )
    {
        bad_token(inp_ptr,"Expected a ')' here");
    }
    else
    {
        ++inp_ptr;      /* eat closing paren */
        while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
            ++inp_ptr; 
        if ( !(amflg&(AMFLG_DISPL|AMFLG_LEADMINUS)) )
        {
            if ( *inp_ptr == '+' )
            {
                ++inp_ptr;
                while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
                    ++inp_ptr; 
                if ( !(cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC|CT_COM)) )
                    return 0;
                amflg |= AMFLG_TRAILPLUS;
            }
        }
    }
    switch ((amflg&15))
    {
    case 0:                              /* (r) */
        amp->mode = E_IREG;
        amp->eamode = Ea_IREG;
        break;
    case AMFLG_TRAILPLUS:                /* (r)+ */
        amp->mode = E_IREGAI;
        amp->eamode = Ea_IREGAI;
        break;
    case AMFLG_INDIRECT|AMFLG_TRAILPLUS: /* @(r)+ */
        amp->mode = E_IIREGAI;
        amp->eamode = Ea_IIREGAI;
        break;
    case AMFLG_LEADMINUS:                /* -(r) */
        amp->mode = E_ADIREG;
        amp->eamode = Ea_ADIREG;
        break;
    case AMFLG_INDIRECT|AMFLG_LEADMINUS: /* @-(r) */
        amp->mode = E_IADIREG;
        amp->eamode = Ea_IADIREG;
        break;
    case AMFLG_DISPL:                    /* X(r) */
        amp->mode = E_DSP;
        amp->eamode = Ea_DSP;
        break;
    case AMFLG_INDIRECT|AMFLG_DISPL:     /* @X(r) */
        amp->mode = E_IDSP;
        amp->eamode = Ea_IDSP;
        break;
    default:
        bad_token(tmp, "Undefined address mode");
        treg = 0;
        break;
    }
    EXP3.ptr = 0;   /* clear residual from EXP3 */
    amp->eamode |= treg;
    return 1;       /* done */
}

static int chkOpenParen(void)
{
	while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
		++inp_ptr; 
	if (*inp_ptr == '(')
	{
		++inp_ptr;      /* eat the opening paren and skip following whitespace */
		while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
			++inp_ptr; 
		return 1;
	}
	return 0;
}

static int get_oneea( EA *amp, int pc_offset, int commaExpected )
{
    EXP_stk *eps;
    EXPR_struct *exp;
    int amflg;
    char *cp;

    amp->mode = E_NUL;      /* assume no mode found */
    amp->eamode = 0;
    eps = amp->exp;
    eps->tag = 'w';     /* assume word mode extra bytes */
    eps->tag_len = 1;    
    eps->forward_reference = 0;
    eps->register_reference = 0;
    eps->register_mask = 0;
    eps->psuedo_value = 0;
    exp = eps->stack;
    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
        ++inp_ptr; 
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
	{
        return 0;
	}
	if ( commaExpected )
	{
		if (*inp_ptr == ',')
		{
			++inp_ptr;
			while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
				++inp_ptr; 
			if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
				return 0;
		}
		else
			show_bad_token(inp_ptr,"Expected a comma here, one assumed",MSG_WARN);
	}
	amflg = 0;          /* no special flags present */
    if ( *inp_ptr == '@' )  /* Indirect? */
    {
        ++inp_ptr;      /* yep, eat it */
        amflg |= AMFLG_INDIRECT;    /* signal found indirect */
    }
    if (*inp_ptr == '#')
    {  /* immediate? */
        ++inp_ptr;      /* yep, eat the char */
		amflg |= AMFLG_IMMEDIATE;
	    get_token();        /* prime the pump */
        cp = inp_ptr;
        exprs(1,eps);       /* pickup the expression */
        if ( eps->register_reference )
        {
            bad_token(cp,"No such thing as immediate register");
            amflg = 0;
        }
        if ( (amflg&AMFLG_INDIRECT) )
        {
            amp->mode = E_ABS;
            amp->eamode = Ea_ABS;
        }
        else
        {
            amp->mode = E_IMM;
            amp->eamode = Ea_IMM;
        }
        eps->tag = 'w';
        return 1;
    }
    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
        ++inp_ptr; 
    cp = inp_ptr;       /* remember where we started */
    if (*cp == '-')
    {       /* a leading minus sign? */
        amflg |= AMFLG_LEADMINUS; /* say there was a leading minus */
        ++inp_ptr;        /* and eat it */
    }
    while ((cttbl[(int)*inp_ptr]&CT_WS) != 0)
        ++inp_ptr; 
    do
    {
		if ( chkOpenParen() )
        {
            if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC|CT_COM)) != 0)
            {
                bad_token(inp_ptr,"Expected a register expression here, found EOL");
                return 0;
            }
            if ( get_regea(amflg,amp,0) )
                break;
        }
		/* It wasn't a register expression so start over */
        amflg &= ~AMFLG_LEADMINUS;  /* This doesn't count anymore */
        inp_ptr = cp;       /* nope, move pointer back */
        if (get_token() == EOL)
        {
            bad_token((char *)0,"Expected an operand, found EOL");
            return 0;
        }
        exprs(1,eps);       /* evaluate register or displacement expression */
        if (*inp_ptr == '(')  /* stop on open paren? */
        {
            if ( eps->register_reference )
            {
                bad_token(inp_ptr,"Invalid address expression");
                break;
            }
            amflg |= AMFLG_DISPL;
            ++inp_ptr;      /* eat the '(' */
            cp = inp_ptr;
            if (get_regea(amflg,amp,0) == 0)  /* get index register */
            {
                bad_token(cp,"Expected register expression here");
            }
            break;
        }
        if ( eps->register_reference )
        {
            if ( eps->ptr != 1 || exp->expr_code != EXPR_VALUE )
            {
                bad_token(cp,"Complex register expressions not supported");
                exp->expr_value = 0;
            }
            if ( exp->expr_value > 7 )
            {
                bad_token(cp,"Register value out of range");
                exp->expr_value = 0;
            }
            amp->mode = E_REG;
            amp->eamode = Ea_REG | exp->expr_value;
            eps->ptr = 0;  /* be sure the expression is gone */
            eps->tag_len = 0;
            break;
        }
        eps->tag_len = 1;
        eps->tag = 'w';  /* displacements are signed words, but ignore that for now */
        if (!(edmask&ED_AMA))
        {
            if ( (amflg&AMFLG_INDIRECT) )
            {
                amp->mode = E_IPCR;
                amp->eamode = Ea_IPCR;
            }
            else
            {
                amp->mode = E_PCR;
                amp->eamode = Ea_PCR;
            }
            exp = eps->stack + eps->ptr;
            exp->expr_code = EXPR_SEG;
            exp->expr_value = pc_offset;
            (exp++)->expr_seg = current_section;
            exp->expr_code = EXPR_OPER;
            exp->expr_value = '-';
            eps->ptr += 2;
            break;
        }
        amp->mode = E_ABS;
        amp->eamode = Ea_ABS;
    } while ( 0 );
    return 1;
}

#if 0
static void bad_sourceam(void)
{
    bad_token((char *)0,"Invalid address mode for first operand");
    source.mode = E_REG;
    source.eamode = Ea_REG;
    source.exp->ptr = 0;
    source.exp->stack->expr_code = EXPR_VALUE;
    source.exp->stack->expr_value = 0;
    return;
}
#endif
static void bad_destam(void)
{
    bad_token((char *)0,"Invalid address mode for second operand");
    dest.mode = E_REG;
    dest.eamode = Ea_REG;
    dest.exp->ptr = 0;
    dest.exp->stack->expr_code = EXPR_VALUE;
    dest.exp->stack->expr_value = 0;
    return;
}

static int validatePCR( int dstmode, int regok, int immok )
{
	if ( (edmask&ED_CPU) )
	{
		if ( (!immok && (dstmode == 017 || dstmode == 027)) ||
			 dstmode == 047 || dstmode == 057 )
			show_bad_token(NULL,"Instruction may behave differently on various processor types",MSG_WARN);
		if ( !regok && !(dstmode&070) )
			bad_destam();
	}
    return 0;
}

/* Class 0 = no operands */
/* HALT,WAIT,RTI,BPT,IOT,RESET,RTT,NOP,CLC,CLV,CLZ,CCC,SEC,SEV,SEZ,SEN,SCC */
int type0( int baseval, int amode )
{
    return 0;
}

/* Class 1 = dst only, can be expression */
/* JMP,SWAB,CLR,CLRB,COM,COMB,INC,INCB,DEC,DECB,NEG,NEGB,ADC,ADCB,SBC,SBCB,TST,TSTB,
 * ROR,RORB,ROL,ROLB,ASR,ASRB,ASL,ASLB,MFPI,MTPI,MFPD,MTPD,SXT,MTPS,MFPS */
int type1( int baseval, int amode )
{
    if ( get_oneea(&dest, current_offset+4, 0 ) )
    {
        validatePCR(dest.eamode, baseval != 0100, 0 );
    }
    return 0;
}

/* Class 2 = one operand can only be register, fits in dst slot */
/* RTS,FADD,FSUB,FMUL,FDIV */
int type2( int baseval, int amode )
{
    EA *amp;
    EXP_stk *eps;
    EXPR_struct *exp;

    amp = &dest;
    amp->eamode = 0;
    eps = amp->exp;
    eps->forward_reference = 0;
    eps->register_reference = 0;
    exp = eps->stack;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
    {
        if ( baseval == 0200 )
            amp->eamode = 7;        /* Default to PC on blank RTS instruction */
        else
            bad_token(NULL,"Expected a register operand");
        return 0;
    }
    get_token();
    exprs(1,eps);
    if (eps->ptr != 1 || !eps->register_reference
        || exp->expr_code != EXPR_VALUE || (exp->expr_value & ~7) )
    {
        bad_token(tkn_ptr,"Operand can only be a register with value between 0 - 7");
    }
    else
    {
        amp->eamode = exp->expr_value;
    }
    eps->ptr = 0;
    return 0;
}

/* Class 3 = dst only, can only be number from 0-7 */
/* SPL */
int type3( int baseval, int amode )
{
    EA *amp;
    EXP_stk *eps;
    EXPR_struct *exp;

    amp = &dest;
    amp->eamode = 0;
    eps = amp->exp;
    eps->forward_reference = 0;
    eps->register_reference = 0;
    exp = eps->stack;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
    {
        bad_token(NULL,"Expected a numeric operand");
        return 0;
    }
    get_token();
    exprs(1,eps);
    if (eps->ptr != 1 || exp->expr_code != EXPR_VALUE || (exp->expr_value & ~7) )
    {
        bad_token(tkn_ptr,"Operand value can only be between 0 - 7");
    }
    else
    {
        amp->eamode = exp->expr_value;
    }
    eps->ptr = 0;
    return 0;
}

/* Class 4 = dst only, can only be number from 0-63 */
/* MARK */
int type4( int baseval, int amode )
{
    EA *amp;
    EXP_stk *eps;
    EXPR_struct *exp;

    amp = &dest;
    amp->eamode = 0;
    eps = amp->exp;
    eps->forward_reference = 0;
    eps->register_reference = 0;
    exp = eps->stack;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
    {
        bad_token(NULL,"Expected a numeric operand");
        return 0;
    }
    get_token();
    exprs(1,eps);
    if (eps->ptr != 1 || exp->expr_code != EXPR_VALUE || (exp->expr_value & ~63) )
    {
        bad_token(tkn_ptr,"Operand value can only be between 0 - 63");
    }
    else
    {
        amp->eamode = exp->expr_value;
    }
    eps->ptr = 0;
    return 0;
}

/* Class 5 = Branch */
int type5( int baseval, int amode )
{
    EA *amp;
    EXP_stk *eps, *ep2;
    EXPR_struct *exp, *exp2;

    amp = &dest;
    amp->mode = 0;
    amp->eamode = 0;
    eps = exprs_stack;      /* Use stack 0 for branch displacement */
    eps->forward_reference = 0;
    eps->register_reference = 0;
    exp = eps->stack;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
    {
        bad_token(NULL,"Expected an operand");
        return 0;
    }
    get_token();
    exprs(1,eps);
    exp = eps->stack + eps->ptr;
    exp->expr_code = EXPR_SEG;
    exp->expr_value = current_offset+2;
    (exp++)->expr_seg = current_section;
    exp->expr_code = EXPR_OPER;
    (exp++)->expr_value = '-';
    eps->ptr += 2;
    eps->ptr = compress_expr(eps);

    if ( eps->ptr != 1 || eps->stack->expr_code != EXPR_VALUE )
    {
        ep2 = source.exp;
        exp2 = ep2->stack;
        memcpy(exp2,eps->stack,eps->ptr*sizeof(EXPR_struct));
        exp2 = ep2->stack + eps->ptr;
        /* Dup the branch displacement (top item on stack) */
        exp2->expr_code = EXPR_VALUE;
        (exp2++)->expr_value = 0;
        exp2->expr_code = EXPR_OPER;
        (exp2++)->expr_value = EXPROPER_PICK;
		/* Leaves stack: [0]=disp, [1]=disp */
		exp2->expr_code = EXPR_VALUE;
		(exp2++)->expr_value = 1;
		exp2->expr_code = EXPR_OPER;
		(exp2++)->expr_value = EXPROPER_AND;
		/* Leaves stack: [0]=disp, [1]=(disp & 1) */
		exp2->expr_code = EXPR_OPER;
		(exp2++)->expr_value = EXPROPER_XCHG;
		/* Leaves stack: [0]=(disp&1), [1]=disp */
		/* Dup the branch displacement (top item on stack) */
		exp2->expr_code = EXPR_VALUE;
		(exp2++)->expr_value = 0;
		exp2->expr_code = EXPR_OPER;
		(exp2++)->expr_value = EXPROPER_PICK;
		/* Leaves stack: [0]=(disp&1), [1]=disp, [2]=disp */
        /* Check it against -512 (leaves a 0 or 1 on stack) */
        exp2->expr_code = EXPR_VALUE;
        (exp2++)->expr_value = -256;
        exp2->expr_code = EXPR_OPER;
        (exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_LT<<8);
		/* Leaves stack: [0]=(disp&1), [1]=disp, [2]=(disp < -256) */
		exp2->expr_code = EXPR_OPER;
		(exp2++)->expr_value = EXPROPER_XCHG;
		/* Leaves stack: [0]=(disp&1), [1]=(disp < -256), [2]=disp */
		exp2->expr_code = EXPR_VALUE;
		(exp2++)->expr_value = 254;
		exp2->expr_code = EXPR_OPER;
		(exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_GT<<8);
		/* Leaves stack: [0]=(disp&1), [1]=(disp < -256), [2]=(disp > 254) */
        exp2->expr_code = EXPR_OPER;
        (exp2++)->expr_value = EXPROPER_OR;
		/* Leaves stack: [0]=(disp&1), [1]=(disp < -256) | (disp > 254) */
		exp2->expr_code = EXPR_OPER;
		(exp2++)->expr_value = EXPROPER_OR;
		/* Leaves stack: [0]=(disp&1) | (disp < -256) | (disp > 254) */
		ep2->ptr = exp2-ep2->stack;
        ep2->tag = 0;
		sprintf(emsg,"%s:%d", current_fnd->fn_name_only,current_fnd->fn_line);
        write_to_tmp(TMP_BOFF,0,ep2,0);
        write_to_tmp(TMP_ASTNG,strlen(emsg)+1,emsg,1);
        ep2->ptr = 0;       /* don't use this expression any more */
    }
	else
	{
		/* we're here because: */
		/* eps->ptr == 1 and eps->stack->expr_code == EXPR_VALUE */
		long value = eps->stack[0].expr_value;
		if ( (value&1) || value > 254 || value < -256 )
		{
			char err[128];
			char *sign="";
			long absValue = value;
			if ( value < 0 )
			{
				absValue = -value;
				sign = "-";
			}
			snprintf(err,sizeof(err),"Branch offset of %ld. (%s0%lo) is odd or out of range. Must be -256. <= x <= 254. (-0400 <= x <= 0376)", value, sign, absValue&0xFFFF);
			show_bad_token(NULL,err,MSG_WARN);
			eps->stack[0].expr_value = -2;
		}
	}
	exp = eps->stack + eps->ptr;
    exp->expr_code = EXPR_VALUE;
    (exp++)->expr_value = 1;
    exp->expr_code = EXPR_OPER;
    (exp++)->expr_value = EXPROPER_SHR;
    exp->expr_code = EXPR_VALUE;
    (exp++)->expr_value = 255;
    exp->expr_code = EXPR_OPER;
    (exp++)->expr_value = EXPROPER_AND;
    exp->expr_code = EXPR_VALUE;
    (exp++)->expr_value = baseval;
    exp->expr_code = EXPR_OPER;
    (exp++)->expr_value = EXPROPER_OR;
    eps->ptr += 6;
    return 0;
}

/* Class 6 = src must be register, dst can be expression */
/* JSR */
int type6( int baseval, int amode )
{
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
    {
        int offs;
        offs = current_offset + 4;
        if ( get_oneea(&source, offs, 0 ) )
        {
            if ( !(source.eamode&070) && source.exp->ptr == 0 )
            {
                if ( source.eamode == 076 )
                    show_bad_token(NULL,"Instruction may behave differently on various processors",MSG_WARN);
				if ( get_oneea(&dest, offs, 1) )
					return validatePCR(dest.eamode, 0, 0 );
            }
            else                            /* source is not register type */
            {
                dest.eamode = source.eamode;    /* switch source and dest */
                dest.mode = source.mode;
                source.eamode = 07;         /* default source register to PC */
                source.mode = E_REG;
                /* The source expression, if any, will simply follow and there is no dest expression */
                return validatePCR(dest.eamode, 0, 0);
            }
        }
    }
    bad_token(inp_ptr,"JSR requires displacement or register,displacment operands");
    return 0;
}

/* Class 7 = src can be expression, dst can be expression */
/* MOV,MOVB,CMP,CMPB,BIT,BITB,BIC,BICB,BIS,BISB,ADD,SUB */
int type7( int baseval, int amode )
{
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
    {
        int offs;
        offs = current_offset + 4;
        if ( get_oneea(&source, offs, 0 ) )
        {
            int seam = source.eamode;
            if ( (seam&070) == 060 || (seam&070) == 070 
                 || seam == 027 || seam == 037 )
                offs += 2;
			if ( get_oneea(&dest, offs, 1) )
			{
				validatePCR(seam, 1, 1 );
				return validatePCR(dest.eamode, 1, 0 );
			}
        }
    }
    bad_token(inp_ptr,"Instruction requires two operands");
    return 0;
}

/* Class 8 = src can be expression, dst can only be register (s is swapped with d) */
/* MUL, DIV, ASH, ASHC */
int type8( int baseval, int amode )
{
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
    {
        int offs, chk=0;
        offs = current_offset + 4;
        if ( get_oneea(&source, offs, 0 ) )
        {
			if ( get_oneea(&dest, offs, 1) )
				chk = 1;
            if ( chk )
            {
                int sav;
                if ( (dest.eamode&070) || dest.exp->ptr || !dest.exp->register_reference )
                {
                    bad_token(NULL,"Destination operand must be a register");
                    dest.eamode = 0;
                    dest.mode = 0;
                    dest.exp->ptr = 0;
                }
                sav = dest.eamode;
                dest.eamode = source.eamode;
                dest.mode = source.mode;
                source.eamode = sav;
                source.mode = E_REG;
                return validatePCR(dest.eamode, 1, 1 );
            }
        }
    }
    bad_token(inp_ptr,"Instruction requires two operands");
    return 0;
}

/* Class 9 = src is register, dst is branch addr */
/* SOB */
int type9( int baseval, int amode )
{
	EA *amp;
	EXP_stk *eps, *ep2;
	EXPR_struct *exp, *exp2;
	if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
	{
		amp = &source;
		amp->mode = 0;
		amp->eamode = 0;
		if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
		{
			bad_token(NULL,"Expected an operand");
			return 0;
		}
		if ( get_regea(0, amp, 1) )
		{
			amp = &dest;
			amp->mode = 0;
			amp->eamode = 0;
			eps = exprs_stack;      /* Use stack 0 for branch displacement */
			eps->forward_reference = 0;
			eps->register_reference = 0;
			exp = eps->stack;
			comma_expected = 1;
			get_token();
			exprs(1,eps);
			exp = eps->stack + eps->ptr;
			exp->expr_code = EXPR_SEG;
			exp->expr_value = current_offset+2;
			(exp++)->expr_seg = current_section;
			exp->expr_code = EXPR_OPER;
			(exp++)->expr_value = '-';
			eps->ptr += 2;
			eps->ptr = compress_expr(eps);
			if ( eps->ptr != 1 || eps->stack->expr_code != EXPR_VALUE )
			{
				ep2 = source.exp;
				exp2 = ep2->stack;
				memcpy(exp2,eps->stack,eps->ptr*sizeof(EXPR_struct));
				exp2 = ep2->stack + eps->ptr;
				/* Dup the branch displacement (top item on stack) */
				exp2->expr_code = EXPR_VALUE;
				(exp2++)->expr_value = 0;
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_PICK;
				/* Leaves stack: [0]=disp, [1]=disp */
				exp2->expr_code = EXPR_VALUE;
				(exp2++)->expr_value = 1;
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_AND;
				/* Leaves stack: [0]=disp, [1]=(disp & 1) */
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_XCHG;
				/* Leaves stack: [0]=(disp&1), [1]=disp */
				exp2->expr_code = EXPR_VALUE;
				(exp2++)->expr_value = 0;
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_PICK;
				/* Leaves stack: [0]=(disp&1), [1]=disp, [2]=disp */
				exp2->expr_code = EXPR_VALUE;
				(exp2++)->expr_value = -256;
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_LT<<8);
				/* Leaves stack: [0]=(disp&1), [1]=disp, [2]=(disp < -256) */
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_XCHG;
				/* Leaves stack: [0]=(disp&1), [1]=(disp < -256), [2]=disp */
				exp2->expr_code = EXPR_VALUE;
				(exp2++)->expr_value = -2;
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_TST | (EXPROPER_TST_GT<<8);
				/* Leaves stack: [0]=(disp&1), [1]=(disp < -256), [2]=(disp > -2) */
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_OR;
				/* Leaves stack: [0]=(disp&1), [1]=(disp < -256) | (disp > -2) */
				exp2->expr_code = EXPR_OPER;
				(exp2++)->expr_value = EXPROPER_OR;
				/* Leaves stack: [0]=(disp&1) | (disp < -256) | (disp > -2) */
				ep2->ptr = exp2-ep2->stack;
				ep2->tag = 0;
				sprintf(emsg,"%s:%d",
						current_fnd->fn_name_only,current_fnd->fn_line);
				write_to_tmp(TMP_BOFF,0,ep2,0);
				write_to_tmp(TMP_ASTNG,strlen(emsg)+1,emsg,1);
				ep2->ptr = 0;       /* don't use this expression any more */
			}
			else
			{
				/* we're here because: */
				/* eps->ptr == 1 and eps->stack->expr_code == EXPR_VALUE */
				long value = eps->stack[0].expr_value;
				if ( (value&1) || value > -2 || value < -256 )
				{
					char err[128];
					const char *sign="";
					long absValue;
					if ( (absValue=value) < 0 )
					{
						absValue = -value;
						sign = "-";
					}
					snprintf(err, sizeof(err), "Branch offset of %ld. (%s0%lo) is odd or out of range. Must be even and -256. <= x <= -2. (-0400 <= x <= -02)", value, sign, absValue);
					show_bad_token(NULL,err,MSG_WARN);
					eps->stack[0].expr_value = -2;
				}
			}
			exp = eps->stack + eps->ptr;
			exp->expr_code = EXPR_OPER;
			(exp++)->expr_value = EXPROPER_NEG;
			exp->expr_code = EXPR_VALUE;
			(exp++)->expr_value = 1;
			exp->expr_code = EXPR_OPER;
			(exp++)->expr_value = EXPROPER_SHR;
			exp->expr_code = EXPR_VALUE;
			(exp++)->expr_value = 63;
			exp->expr_code = EXPR_OPER;
			(exp++)->expr_value = EXPROPER_AND;
			exp->expr_code = EXPR_VALUE;
			(exp++)->expr_value = baseval|(source.eamode<<6);
			exp->expr_code = EXPR_OPER;
			(exp++)->expr_value = EXPROPER_OR;
			eps->ptr += 7;
			dest.eamode = 0;
			eps->ptr = compress_expr(eps);
		}
		return 0;
	}
	bad_token(inp_ptr,"Instruction requires two operands");
    return 0;
}

/* Class 10 = dst only, can only be number from 0-255 */
/* EMT, TRAP */
int type10( int baseval, int amode )
{
    EA *amp;
    EXP_stk *eps;
    EXPR_struct *exp;

    amp = &dest;
    amp->eamode = 0;
    eps = amp->exp;
    eps->forward_reference = 0;
    eps->register_reference = 0;
    exp = eps->stack;
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
    {
        bad_token(NULL,"Expected a numeric operand");
        return 0;
    }
    get_token();
    exprs(1,eps);
    if (eps->ptr != 1 || exp->expr_code != EXPR_VALUE || (exp->expr_value & ~255) )
    {
        bad_token(tkn_ptr,"Operand value can only be between 0 - 255");
    }
    else
    {
        amp->eamode = exp->expr_value;
    }
    eps->ptr = 0;
    return 0;
}

/* Class 11 = src must be register, dst can be expression */
/* XOR */
int type11( int baseval, int amode )
{
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) == 0)
    {
        int offs;
        offs = current_offset + 4;
        if ( get_oneea(&source, offs, 0 ) )
        {
            if ( (source.eamode&070) || source.exp->ptr || !source.exp->register_reference )
            {
                bad_token(NULL,"Source operand must be a register");
                source.eamode = 0;
                source.mode = 0;
                source.exp->ptr = 0;
            }
			if ( get_oneea(&dest, offs, 1) )
			{
				return validatePCR(dest.eamode, 1, 0 );
			}
        }
    }
    bad_token(inp_ptr,"Instruction requires two operands");
    return 0;
}

/* Class 12 = dst only, can be expression; could be immediate */
/* MTPI,MTPD,MTPS */
int type12( int baseval, int amode )
{
    if ( get_oneea(&dest, current_offset+4, 0 ) )
    {
        validatePCR(dest.eamode, 1, 1 );
    }
    return 0;
}

