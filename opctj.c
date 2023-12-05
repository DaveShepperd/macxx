/*
    opctj.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "tjop_class.h"

#define STACK_DEBUG 0
#define OP_DEBUG 2
#define DEBUG 1

#define SCC_BIT  0x00200000
#define SRC_POS  5
#define DST_POS  0
#define MAXREG   31
#define DISP_MASK (-MIN_DISP | MAX_DISP)
#define DEFNAM(name,numb) {"name",name,numb},

/* next four variables are for compatability with V7.4 Macxx */
unsigned short rel_salign = 3; /* default alignment for rel segments */
unsigned short rel_dalign = 1; /* def algnmnt for data in rel segments */
unsigned short abs_salign = 3; /* default alignment for abs segments */
unsigned short abs_dalign = 1; /* def algnmnt for data in abs segments */

char macxx_name[] = "mactj";
char *macxx_target = "TOM"; /* "1357785-001"; */
char *macxx_descrip = "Cross assembler for the Tom and Jerry.";

unsigned short macxx_rel_salign = 3; /* default alignment for rel segments */
unsigned short macxx_rel_dalign = 1; /* def algnmnt for data in rel segments */
unsigned short macxx_abs_salign = 3; /* default alignment for abs segments */
unsigned short macxx_abs_dalign = 1; /* def algnmnt for data in abs segments */
unsigned short macxx_min_dalign = 1;

char macxx_mau = 8;         /* # of bits/minimum addressable unit */
char macxx_bytes_mau = 1;       /* number of bytes/mau */
char macxx_mau_byte = 1;        /* number of mau's in a byte */
char macxx_mau_word = 2;        /* number of mau's in a word */
char macxx_mau_long = 4;        /* number of mau's in a long */
char macxx_nibbles_byte = 2;        /* For the listing output routines */
char macxx_nibbles_word = 4;
char macxx_nibbles_long = 8;

unsigned long macxx_edm_default = ED_LC|ED_GBL|ED_M68|ED_DOL; /* default edmask */
/* default list mask */
unsigned long macxx_lm_default = ~(LIST_ME|LIST_MEB|LIST_MES|LIST_LD);
int current_radix = 10;     /* default the radix to decimal */
char expr_open = '(';       /* char that opens an expression */
char expr_close = ')';      /* char that closes an expression */
char expr_escape = '^';     /* char that escapes an expression term */
char macro_arg_open = '<';  /* char that opens a macro argument */
char macro_arg_close = '>'; /* char that closes a macro argument */
char macro_arg_escape = '^';    /* char that escapes a macro argument */
char macro_arg_gensym = '?';    /* char indicating generated symbol for macro */
char macro_arg_genval = '\\';   /* char indicating generated value for macro */
int max_opcode_length = 16; /* significant length of opcodes and */
int max_symbol_length = 16; /*  symbols */

/* End of processor specific stuff */

static unsigned short which_stack;  /* from whence to finally output */

static int merge_stacks(int first, int second);

enum opr
{
    OPR_EXP=1,   /* simple expression */
    OPR_IMM, /* immediate value */
    OPR_REG, /* simple register */
    OPR_IREG,    /* indirect register */
    OPR_IIREG14, /* indirect register 14 indexed imm */
    OPR_IIREG15, /* indirect register 15 indexed imm */
    OPR_IRREG14, /* indirect register 14 indexed reg */
    OPR_IRREG15  /* indirect register 15 indexed reg */
};

#define AMOK_NONE	0x00		/* no special address modes allowed */
#define AMOK_IMM	0x01		/* immediate value is ok */
#define AMOK_IREG	0x02		/* indirect register value is ok */
#define AMOK_IIREG	0x04		/* indirect indexed value is ok */
#define AMOK_REG	0x08		/* register value only */
#define AMOK_ANY	0x0F		/* any address modes allowed */
#define REG_MIN		0x00		/* minimum register number */
#define REG_MAX		0x1F		/* Maximum register number */
#define IMM_MIN		0x00		/* Minimum immediate value */
#define IMM_MAX		0x1F		/* Maximum immediate value */
#define BR_MIN		(-16)		/* Minimum branch displacement */
#define BR_MAX		(15)		/* Maximum branch displacement */
#define CMP_MIN		(-16)		/* Minimum compare immediate value */
#define CMP_MAX		(15)		/* Maximum compare immediate value */
#define TRUE		1
#define FALSE		0

typedef struct
{
    char *ccode;         /* condition code name */
    int value;           /* value */
} Ccode;

typedef struct
{
    int size;            /* total # of structs in cc pool */
    int remain;          /* remaining structs in cc pool */
    Ccode *ptr;          /* pointer to next available entry */
    Ccode *tbl;          /* ptr to array of condition codes */
} CcodeCtrl;

static CcodeCtrl ccodectrl;
static Ccode cclist[] = {
    { "A",  0x00},
    { "CS", 0x08},
    { "CC", 0x04},
    { "EQ", 0x02},
    { "LT", 0x08},
    { "MI", 0x18},
    { "NE", 0x01},
    { "PL", 0x14},
    { 0, 0}
};

typedef struct
{
    SEG_struct *section;     /* section */
    unsigned long offset;    /* offset */
    unsigned short opcode;   /* opcode */
    int class;           /* opcode class */
    int len;         /* length of instruction */
#if 0
    int src;         /* source register */
    int src_isreg;       /* source is a register */
    int dst;         /* dest register */
    int dst_isreg;       /* destination is a register */
    int cycle;           /* # of ticks for this instruction */
    int flags;           /* flags */
#endif
} OpStatus;

static OpStatus prev_opcode, this_opcode;

static int find_cc(Ccode **rv) { /* look for condition code in cctable */
    unsigned long oedmask = edmask;
    Ccode *ans;      

    *rv = 0;         /* assume error */
    edmask &= ~ED_LC;        /* clear LC flag */
    get_token();         /* pickup the cc token in uppercase */
    edmask = oedmask;        /* reset the mask */
    if (token_type != TOKEN_strng)
    { /* it better be a string */
        bad_token(tkn_ptr, "Expected a condition code string");
        f1_eatit();
        return -1;        /* fatal error */
    }
    ans = ccodectrl.tbl;     /* point to condition code table */
    while (ans->ccode)
    {     /* look through the whole array */
        if (strcmp(ans->ccode, token_pool) == 0)
        {
            *rv = ans;
            return 1;      /* success */
        }
        ++ans;
    }
    return 0;            /* not in there */
}

int op_ccdef(Ccode *cc) {
    int ii;
    EXP_stk *ep = exprs_stack;
    EXPR_struct *exp = ep->stack;
    char *oip;

    ii = find_cc(&cc);       /* look for condition code */
    if (ii < 0) return 0;    /* syntax error */
    if (ii == 0)
    {       /* creating a new one */
        if (ccodectrl.remain <= 1)
        { /* if no room to put new code */
            ii = ccodectrl.size/2; /* make it 1/2 again as big */
            ccodectrl.size += ii;
            ccodectrl.remain += ii;
            ii = ccodectrl.ptr - ccodectrl.tbl;
            ccodectrl.tbl = (Ccode *)MEM_realloc((char *)ccodectrl.tbl, ccodectrl.size*sizeof(Ccode));
            ccodectrl.ptr = ccodectrl.tbl + ii;
        }
        cc = ccodectrl.ptr;   /* point to next available entry */
        ccodectrl.ptr += 1;   /* add 1 */
        ccodectrl.remain -= 1;    /* take 1 from total */
    }
    cc->ccode = token_pool;
    token_pool += token_value+1; /* keep token */
    cc->value = 31;      /* make the cc a "never" condition in case of error */
    comma_expected = 1;      /* next thing has to be a comma */
    get_token();         /* prime the pump */
    oip = tkn_ptr;
    if (exprs(1, &EXP0) < 0)
    {   /* pickup expression */
        bad_token(tkn_ptr, "Invalid expression");
        return 0;
    }
    ep->ptr = compress_expr(ep);
    if (ep->ptr != 1 || exp->expr_code != EXPR_VALUE || exp->expr_value < 0 || exp->expr_value > 31)
    {
        bad_token(oip, "Expression must resolve to an absolute value >= 0 and <= 31");
    }
    else
    {
        cc->value = exp->expr_value;
    }
    return 0;
}

int ust_init( void ) {        /* User defined symbol table initialization */
    int ii,jj,upc;
    char *regname;
    SS_struct *ptr;

    ccodectrl.size = sizeof(cclist)/sizeof(Ccode);
    ccodectrl.remain = 1;
    ccodectrl.tbl = (Ccode *)MEM_alloc(sizeof(cclist));
    ccodectrl.ptr = ccodectrl.tbl + (ccodectrl.size-1);
    memcpy(ccodectrl.tbl, cclist, sizeof(cclist));
#if 0
    default_long_tag[0] = 'j';
    default_long_tag[1] = 'J';
#endif
    quoted_ascii_strings = TRUE;

    regname = MEM_alloc(4*32*2); /* room for all the register names in both upper and lowercase */
    for (upc='R',jj=0; jj < 2; ++jj,upc = 'r')
    {
        for (ii=0; ii<=32; ++ii)
        {
            if (ii < 32)
            {
                sprintf(regname,"%c%d", upc, ii);
            }
            else
            {
                if (jj == 0)
                {
                    strcpy(regname,"PC");
                }
                else
                {
                    strcpy(regname,"pc");
                }
            }
            if ((ptr = sym_lookup(regname,SYM_INSERT_IF_NOT_FOUND)) == 0)
            {
                sprintf(emsg,"Unable to insert symbol %s into symbol table",
                        regname);
                bad_token((char *)0,emsg);
                continue;
            }
            ptr->flg_defined = 1;      /* defined */
            ptr->flg_ref = 1;          /* referenced */
            ptr->flg_macLocal = 0;        /* not local */
			ptr->flg_gasLocal = 0;
            ptr->flg_global = 0;       /* not global */
            ptr->flg_label = 1;        /* can't redefine it */
			ptr->flg_fixed_addr = 1;
            ptr->ss_fnd = 0;           /* no associated file */
            ptr->flg_register = 1;     /* it's a register */
            ptr->flg_abs = 1;          /* it's not relocatible */
            ptr->flg_exprs = 0;        /* no expression with it */
            ptr->ss_seg = 0;           /* not an offset from a segment */
            ptr->ss_value = ii;        /* value */
            regname += strlen(regname)+1;  /* move pointer */
        }
    }
    return 1;
}

typedef struct
{
    EXP_stk *ep;     /* expression stack containing operand */
    EXP_stk *tmp;    /* ptr to expression stack to use as temp */
    char *ptr;       /* ptr to start of operand string */
    int reg;     /* t/f if expression must be a register */
    int min;     /* minimum value allowed */
    int max;     /* maximum value allowed */
    int scale;       /* amount to shift right */
    int align;       /* alignment mask */
    int sts;     /* status flag for function use */
} AM_details;

/* check_operand:
 * Checks the validity of an operand and prepares check code to deposit
 * into object file if appropritate.
 * At entry:
 *	amd is pointer to struct (described above) containing arguments:
 *	ep - pointer to expression having operand to check
 *	tmp - pointer to expression to use to build check expression if required
 *	ptr - pointer to start of operand string (for error messages)
 *	reg - true if operand must be a register
 *	min - minimum value allowed of operand
 *	max - maxiumum value allowed of operand
 * At exit:
 *	tmp stack may have been filled with an expression
 *	returns TRUE if operand ok, else returns FALSE
 */

static int check_operand(AM_details *amd) {
    EXP_stk *ep, *tmp;
    EXPR_struct *exp, *tmpexp;

    ep = amd->ep;
    exp = ep->stack;
    tmp = amd->tmp;
    tmpexp = tmp->stack;
    tmp->ptr = 0;            /* assume no check expression */
    amd->sts = FALSE;            /* assume nfg */
    if (amd->reg && !ep->register_reference)
    {   /* if has to be a register and isn't */
        bad_token(amd->ptr, "Expected a register expression");
        ep->ptr = 0;          /* no operand */
        return FALSE;         /* nfg */
    }
#if 0
    if (!ep->register_reference && amd->oneto32)
    {   /* immediate value relative to 1? */
        exp = ep->stack+ep->ptr;  /* yep, subtract one */
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = amd->oneto32;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_ADD; /* adjust immediate value */
        ep->ptr += 2;         /* add 2  terms to expression */
        ep->ptr = compress_expr(ep);  /* collapse the expression */
        exp = ep->stack;          /* reset exp */
    }
#endif
    if (ep->ptr == 1 && exp->expr_code == EXPR_VALUE)
    {  /* if expression is absolute */
        if (exp->expr_value < amd->min || exp->expr_value > amd->max)
        {
            bad_token(tkn_ptr, "Operand value out of range");
            ep->ptr = 0;
            return FALSE;
        }
        if ((amd->align&exp->expr_value) != 0)
        {
            bad_token(tkn_ptr, "Operand value is misaligned");
            ep->ptr = 0;
            return FALSE;
        }
    }
    else
    {
        memcpy(tmpexp, (char *)exp, ep->ptr*sizeof(EXPR_struct));
        tmpexp += ep->ptr;
        if (amd->align)
        {
            tmpexp->expr_code = EXPR_VALUE;
            (tmpexp++)->expr_value = 0;        /* pick's argument; 0=top of stack */
            tmpexp->expr_code = EXPR_OPER;
            (tmpexp++)->expr_value = EXPROPER_PICK;/* dup top of stack */
            tmpexp->expr_code = EXPR_VALUE;
            (tmpexp++)->expr_value = amd->align;   /* check for out of range + */
            tmpexp->expr_code = EXPR_OPER;
            (tmpexp++)->expr_value = EXPROPER_AND;
            tmpexp->expr_code = EXPR_OPER;
            (tmpexp++)->expr_value = EXPROPER_XCHG;/* save answer, get tos */
        }
        tmpexp->expr_code = EXPR_VALUE;
        (tmpexp++)->expr_value = 0;       /* pick's argument; 0=top of stack */
        tmpexp->expr_code = EXPR_OPER;
        (tmpexp++)->expr_value = EXPROPER_PICK;   /* dup top of stack */
        tmpexp->expr_code = EXPR_VALUE;
        (tmpexp++)->expr_value = amd->max;    /* check for out of range + */
        tmpexp->expr_code = EXPR_OPER;
        (tmpexp++)->expr_value = EXPROPER_TST | (EXPROPER_TST_GT<<8);
        tmpexp->expr_code = EXPR_OPER;
        (tmpexp++)->expr_value = EXPROPER_XCHG;   /* save answer, get tos */
        tmpexp->expr_code = EXPR_VALUE;
        (tmpexp++)->expr_value = amd->min;    /* check for out of range - */
        tmpexp->expr_code = EXPR_OPER;
        (tmpexp++)->expr_value = EXPROPER_TST | (EXPROPER_TST_LT<<8);
        tmpexp->expr_code = EXPR_OPER;
        (tmpexp++)->expr_value = EXPROPER_TST | (EXPROPER_TST_OR<<8);
        if (amd->align)
        {
            tmpexp->expr_code = EXPR_OPER;
            (tmpexp++)->expr_value = EXPROPER_TST | (EXPROPER_TST_OR<<8);
        }
        tmp->ptr = tmpexp - tmp->stack;
    }
    if (amd->scale)
    {
        exp = ep->stack+ep->ptr;      /* yep, shift right by scale */
        exp->expr_code = EXPR_VALUE;
        (exp++)->expr_value = amd->scale;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = EXPROPER_SHR; /* shift right */
        ep->ptr += 2;         /* add 2  terms to expression */
        ep->ptr = compress_expr(ep);  /* collapse the expression */
        exp = ep->stack;          /* reset exp */
    }
    amd->sts = TRUE;
    return TRUE;
}

static int
get_oper(AM_details *amd, int amode) {
    EXP_stk *ep;
#if 0
    EXPR_struct *exp;
#endif
    SS_struct *ss;
    int tmpam = OPR_EXP;    /* assume a simple expression */

    ep = amd->ep;
#if 0
    exp = ep->stack;        /* point to expression stack */
#endif
    get_token();        /* pickup the next token */
    if (*tkn_ptr == '#')
    {  /* he's getting an immediate value */
        if ((amode&AMOK_IMM) == 0)
        {
            bad_token(inp_ptr, "Illegal address mode");
            ep->ptr = 0;
            return -1;
        }
        amd->ptr = tkn_ptr;
        get_token();     /* prime the pump */
        if (exprs(1, ep) < 1)
        {
            bad_token(amd->ptr, "Expression expected");
            return -1;
        }
        return OPR_IMM;      /* operand is an immediate */
    }
    amd->ptr = tkn_ptr;
    if ((amode&(AMOK_IREG|AMOK_IIREG|AMOK_REG)) != 0 && *tkn_ptr == '(')
    {  /* see if we possibly have an '(R14+' syntax */
        get_token();     /* pickup the next item */
        if (  (amode&AMOK_IIREG) != 0 &&     /* if we can have indexed register direct */
              token_type == TOKEN_strng)
        {  /* and the next token is a string */
            ss = sym_lookup(token_pool, SYM_DO_NOTHING);   /* see if it is a symbol */
            if (ss && ss->flg_register && /* if it is a register symbol and... */
                (ss->ss_value == 14 || ss->ss_value == 15) && /* it is R14 or R15 */
                *inp_ptr == '+')
            {  /* and it is followed by a '+') */
                tmpam = OPR_IIREG14 + (ss->ss_value&1);    /* assume (R1[45]+n) */
                ++inp_ptr;             /* eat the '+' */
                get_token();           /* prime the pump */
            }
        }
        if (exprs(1, ep) < 1)
        {
            char *msg;
            if (token_type == EOL)
            {      /* if we stopped on EOL */
                msg = "Operand missing";
            }
            else
            {
                msg = "Invalid operand expression";
            }
            bad_token(amd->ptr, msg);
            ep->ptr = 0;      /* just pop the expression */
            return -1;
        }
        if (*inp_ptr != ')')
        {   /* stop on closing paren? */
            bad_token(inp_ptr, "Expected a closing paren");
            ep->ptr = 0;      /* just pop the expression */
            return -1;        /* operand is nfg */
        }
        get_token();     /* get the closing paren */
        if (*inp_ptr == ',' || (cttbl[(int)*inp_ptr]&(CT_SMC|CT_EOL)) != 0)
        {
            ep->ptr = compress_expr(ep);  /* convert it if necessary */
            if (ep->register_reference)
            {
                if (tmpam == OPR_EXP)
                {
                    tmpam = OPR_IREG; /* this is a simple register indirect */
                }
                else
                {
                    tmpam += 2; /* bump the operand type to OPR_IRREG1[45] */
                }
            }
            return tmpam;     /* done */
        }
        inp_ptr = amd->ptr;  /* it's none of the above so reset the pointer */
        get_token();     /* prime the pump */
    }
    if (exprs(1, ep) < 1)
    { /* eval the expression */
        char *msg;
        if (token_type == EOL)
        {     /* if we stopped on EOL */
            msg = "Operand missing";
        }
        else
        {
            msg = "Invalid operand expression";
        }
        bad_token(amd->ptr, msg);
        ep->ptr = 0;     /* just pop the expression */
        return -1;
    }
    ep->ptr = compress_expr(ep); /* convert it if necessary */
    if (ep->register_reference) tmpam = OPR_REG;
    return tmpam;
}

static int get_operir(AM_details *amd) {
    int amode;
    EXP_stk *ep;
    EXPR_struct *exp;
    amd->ptr = inp_ptr;      /* record start of operand */
    amd->reg = TRUE;     /* source must be some sort of register */
    amode = get_oper(amd, AMOK_IREG|AMOK_IIREG);
    if (amode < 0) return 0;
    if (amode < OPR_IREG || amode > OPR_IRREG15)
    {
        bad_token(amd->ptr, "Expected register indirect or indirect indexed");
        return FALSE;
    }
    if (amode == OPR_IREG || amode == OPR_IRREG14 || amode == OPR_IRREG15)
    { /* if register indirect */
        amd->min = MIN_REG;
        amd->max = MAX_REG;
        amd->reg = TRUE;  /* has to be a register (we know it is) */
    }
    else
    {
        amd->min = 1;     /* register indirect immediate value has to be 1-32 */
        amd->max = 32;
        amd->reg = FALSE;
    }
    amd->scale = amd->align = 0; /* no scale or alignment */
    if (!check_operand(amd)) return FALSE;
    if (amode == OPR_IIREG14 || amode == OPR_IIREG15)
    {
        ep = amd->ep;
        exp = ep->stack;
        if (   ep->ptr == 1
			&& exp->expr_code == EXPR_VALUE
			&& exp->expr_value == 0
		   )
        {
            exp->expr_value = (amode==OPR_IIREG14) ? 14 : 15;
            amode = OPR_IREG;      /* convert opcode to simple indirect */
        }
    }
    return amode;
}

static int get_operr(AM_details *amd) {
    int amode;
    amd->ptr = inp_ptr;      /* record start of operand */
    amd->reg = TRUE;     /* source must be some sort of register */
    amode = get_oper(amd, AMOK_REG);
    if (amode < 0) return 0;
    if (amode != OPR_REG)
    {
        bad_token(amd->ptr, "Expected register expression");
        return FALSE;
    }
    amd->min = MIN_REG;  /* minimum register number */
    amd->max = MAX_REG;  /* maximum register number */
    amd->scale = amd->align = 0; /* no scale or alignment */
    if (!check_operand(amd)) return FALSE;
    return amode;
}

static unsigned short ldcodes[] = {
    OPC_LOADIR<<OPC_CODE,
    OPC_LOADR14n<<OPC_CODE,
    OPC_LOADR15n<<OPC_CODE,
    OPC_LOADR14R<<OPC_CODE,
    OPC_LOADR15R<<OPC_CODE
};

static unsigned short stcodes[] = {
    OPC_STOREIR<<OPC_CODE,
    OPC_STORER14n<<OPC_CODE,
    OPC_STORER15n<<OPC_CODE,
    OPC_STORER14R<<OPC_CODE,
    OPC_STORER15R<<OPC_CODE
};

static unsigned short do_ldst_op(int which) {
    AM_details amdsrc, amddst;
    unsigned short opcode=0xFFFF;
    int selam;

    if (which)
    {
        amdsrc.ep = exprs_stack+2; /* source expression goes into 2 */
        amdsrc.tmp = exprs_stack+3; /* check expression goes into 3 */
        if (get_operr(&amdsrc) == 0) return opcode;   /* ST opcode */
    }
    else
    {
        amdsrc.ep = exprs_stack;  /* source expression goes into 0 */
        amdsrc.tmp = exprs_stack+1;   /* check expression goes into 1 */
        if ((selam = get_operir(&amdsrc)) == 0) return opcode; /* LD opcode */
    }
    comma_expected = TRUE;   /* expect a comma between operands */
    if (which)
    {
        amddst.ep = exprs_stack+0; /* dest expression goes into 0 */
        amddst.tmp = exprs_stack+1; /* check expression goes into 1 */
        if ((selam = get_operir(&amddst)) == 0) return opcode;
    }
    else
    {
        amddst.ep = exprs_stack+2;    /* dest expression goes into 2 */
        amddst.tmp = exprs_stack+3;   /* check expression goes into 3 */
        if (get_operr(&amddst) == 0) return opcode;
    }
    selam -= OPR_IREG;           /* compute table index */
    if (which)
    {
        opcode = stcodes[selam];      /* get STORE opcode */
    }
    else
    {
        opcode = ldcodes[selam];      /* get LOAD opcodes */
    }
    return opcode;
}

static unsigned short do_move_op( void ) {
    unsigned short opcode = 0xFFFF;
    AM_details amsrc, amdst;   
    EXP_stk *ep;
    EXPR_struct *exp;
    int amode;

    ep = exprs_stack;
    exp = ep->stack;
    amsrc.ptr = inp_ptr;     /* record start of operand */
    amsrc.reg = FALSE;       /* source need not be register */
    amsrc.ep = ep;       /* source goes into expr 0 */
    amsrc.tmp = ep+1;        /* check expression goes into expr 1 */
    amode = get_oper(&amsrc, AMOK_REG|AMOK_IMM);
    if (amode < 0) return opcode;
    if (amode != OPR_REG && amode != OPR_IMM)
    {
        bad_token(amsrc.ptr, "Expected register or immediate expression");
        return opcode;
    }
    amdst.ptr = tkn_ptr;
    amdst.reg = TRUE;        /* always have to move into a register */
    amdst.ep = ep+2;
    amdst.tmp = ep+3;
    amsrc.min = amdst.min = 0;
    amsrc.max = amdst.max = 31;
    amsrc.scale = amdst.scale = 0;   /* no scale or alignment */
    amsrc.align = amdst.align = 0;
    comma_expected = TRUE;   /* expect a comma between operands */
    if (!get_operr(&amdst)) return opcode;
    if (!check_operand(&amdst)) return opcode;
    if (ep->ptr == 1 && exp->expr_code == EXPR_VALUE)
    {
        if (amode == OPR_REG)
        {
            if (exp->expr_value == 32)
            {
                opcode = OPC_MOVEPCR<<OPC_CODE;
            }
            else
            {
                goto src_is_reg;
            }
        }
        else
        {
            if (exp->expr_value >= 0 && exp->expr_value <= 31)
            {
                opcode = OPC_MOVEQ<<OPC_CODE;
            }
            else
            {
                opcode = OPC_MOVEI<<OPC_CODE;
            }
        }
    }
    else
    {
        if (amode == OPR_REG)
        {
            src_is_reg:      
            amsrc.reg = TRUE;
            if (!check_operand(&amsrc)) return opcode;
            opcode = OPC_MOVERR<<OPC_CODE;
        }
        else
        {
            opcode = OPC_MOVEI<<OPC_CODE;
        }
    }
    return opcode;
}

static unsigned short do_single_op(Opcode *opc) {   /* single operand processing */
    unsigned short opcode = 0xFFFF;
    AM_details amdst;
    EXP_stk *ep;
#if 0
    EXPR_struct *exp;
#endif

    if ((opc->op_amode&AM_ANYDST) == 0) return opc->op_value;    /* zero op instruction */
    ep = exprs_stack+2;
#if 0
    exp = ep->stack;
#endif
    amdst.ptr = inp_ptr;     /* record start of operand */
    amdst.reg = TRUE;        /* dest has to be register */
    amdst.ep = ep;       /* dest goes into expr 2 */
    amdst.tmp = ep+1;        /* check expression goes into expr 3 */

    if (get_operr(&amdst))
    {
        opcode = opc->op_value;
    }
    return opcode;
}

static unsigned short do_rr_op(Opcode *opc, int which) {
    AM_details amdsrc, amddst;
    unsigned short opcode=0xFFFF;
    int selam;

    if (!which)
    {
        amdsrc.ep = exprs_stack;      /* source expression goes into 0 */
        amdsrc.tmp = exprs_stack+1;   /* source check expression goes into 1 */
        amddst.ep = exprs_stack+2;    /* dest expression goes into 2 */
        amddst.tmp = exprs_stack+3;   /* dest check expression goes into 3 */
    }
    else
    {
        amddst.ep = exprs_stack+0;    /* dest expression goes into 0 */
        amddst.tmp = exprs_stack+1;   /* dest check expression goes into 1 */
        amdsrc.ep = exprs_stack+2;    /* source expression goes into 2 */
        amdsrc.tmp = exprs_stack+3;   /* source check expression goes into 3 */
    }
    if ((opc->op_amode&AM_SIREG) != 0)
    {
        selam = get_operir(&amdsrc);
        if (selam != OPR_IREG)
        {
            bad_token(amdsrc.ptr, "Expected register indirect");
            return opcode;
        }
    }
    else
    {
        if (!get_operr(&amdsrc)) return opcode;
    }
    comma_expected = TRUE;   /* expect a comma between operands */
    if ((opc->op_amode&AM_DIREG) != 0)
    {
        selam = get_operir(&amddst);
        if (selam != OPR_IREG)
        {
            bad_token(amddst.ptr, "Expected register indirect");
            return opcode;
        }
    }
    else
    {
        if (!get_operr(&amddst)) return opcode;
    }
    opcode = opc->op_value;
    return opcode;
}

static unsigned short do_nr_op(Opcode *opc) {
    AM_details amdsrc, amddst;
    unsigned short opcode=0xFFFF;
    EXP_stk *ep;
    EXPR_struct *exp;
    int selam;

    ep = amdsrc.ep = exprs_stack; /* source expression goes into 0 */
    amdsrc.tmp = exprs_stack+1;  /* check expression goes into 1 */
    amdsrc.reg = FALSE;      /* source actually cannot be a register */
    selam = get_oper(&amdsrc, AMOK_IMM); /* immediate it the only one allowed */
    if (ep->register_reference || (selam != OPR_IMM && selam != OPR_EXP))
    {
        bad_token(amdsrc.ptr, "Expected a non-register expression");
        return opcode;
    }
    selam = opc->op_amode&(AM_IMM32|AM_SHIFT);
    if (selam != AM_IMM32)
    { /* if we need to range check the source */
        if ((selam&AM_SHIFT) != 0)
        {
            exp = ep->stack+ep->ptr;/* point to top of expression */
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_NEG;    /* negate the number */
            exp->expr_code = EXPR_VALUE;
            (exp++)->expr_value = 32;      /* compute 32-n */
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_ADD;    /* compute 32-n */
            ep->ptr += 3;      /* added 3 terms to expression */
            selam = 0;     /* pretend we're supposed to check 0-31 */
        }
        if (selam == 0)
        {     /* immediate value is 0-31 */
            amdsrc.min = 0;
            amdsrc.max = 31;
        }
        else if (selam == AM_IMM1)
        {    /* immediate value is 1-32 */
            amdsrc.min = 1;
            amdsrc.max = 32;
        }
        else if (selam == AM_IMM2)
        {    /* immediate value is -16 to +15 */
            amdsrc.min = -16;
            amdsrc.max = 15;
        }
        amdsrc.scale = amdsrc.align = 0;  /* no scale or alignment */
        if (!check_operand(&amdsrc)) return opcode;   /* operand nfg */
    }
    amddst.ep = exprs_stack+2;   /* dest expression goes into 2 */
    amddst.tmp = exprs_stack+3;  /* check expression goes into 3 */
    comma_expected = TRUE;   /* expect a comma between operands */
    if ((opc->op_amode&AM_DIREG) != 0)
    {
        selam = get_operir(&amddst);
        if (selam != OPR_IREG)
        {
            bad_token(amddst.ptr, "Expected register indirect");
            return opcode;
        }
    }
    else
    {
        if (!get_operr(&amddst)) return opcode;
    }
    opcode = opc->op_value;
    return opcode;
}

static unsigned short do_jmp_op(Opcode *opc, int which) {
    AM_details amddst;
    unsigned short opcode=0xFFFF;
    EXP_stk *ep;
    EXPR_struct *exp;
    Ccode *cc;
    int ii;

    ii = find_cc(&cc);
    if (ii < 0) return opcode;   /* syntax error */
    if (ii == 0)
    {       /* not in the table */
        bad_token(tkn_ptr, "Undefined condition code");
        return opcode;
    }
    ep = amddst.ep = exprs_stack+0; /* dest expression goes into 0 */
    amddst.tmp = exprs_stack+1;  /* check expression goes into 1 */
    comma_expected = TRUE;   /* expect a comma between operands */
    if (which == 0)
    {        /* type 0 is JUMP */
        ii = get_operir(&amddst);
        if (ii != OPR_IREG)
        {
            bad_token(amddst.ptr, "Expected register indirect");
            return opcode;
        }
    }
    else
    {         /* type 1 is JR */
        ii = get_oper(&amddst, 0);    /* get a regular expression */
        if (ii != OPR_EXP)
        {  /* it better be a simple expression */
            bad_token(amddst.ptr, "Expected a non-register expression");
            return opcode;
        }
        exp = ep->ptr + ep->stack;
        exp->expr_code = EXPR_SEG;
        exp->expr_value = current_offset + BR_OFF;
        (exp++)->expr_seg = current_section;
        exp->expr_code = EXPR_OPER;
        (exp++)->expr_value = '-';
        ep->ptr += 2;
        ep->ptr = compress_expr(ep);
        amddst.align = 1;     /* supposed to be even */
        amddst.scale = 1;     /* shift it right one bit */
        amddst.min = -32;     /* branch offset is -32<=n<=30 */
        amddst.max = 30;
        amddst.reg = FALSE;
        if (!check_operand(&amddst)) return opcode;
    }
    opcode = opc->op_value | cc->value;
    return opcode;
}

#if DEBUG & OP_DEBUG
static char *class_names[] = {
    "JR", "JUMP", "LD", "ST", "MOVE", "UNDEF", "UNDEF", "UNDEF"
};
#endif

#if DEBUG & STACK_DEBUG
static void dump_stacks();
#endif

void do_opcode( Opcode *opc )
{
    int ii, err_cnt = error_count[MSG_ERROR], ill_tst=FALSE;
    int opclass;
    EXPR_struct *exp;
    EXP_stk *ep;
    unsigned short opcode;

    which_stack = 0;
    ep = exprs_stack;
    for (ii=0; ii<4; ++ep,++ii)
    {
        ep->ptr = 0;         /* clear all stacks */
        ep->tag = 0;
        ep->tag_len = 1;
    }
    this_opcode.section = current_section;
    this_opcode.offset = current_offset;
    this_opcode.class = opc->op_class;
    this_opcode.len = 2;        /* assume instruction is 2 bytes in length */
    if (prev_opcode.section == current_section &&
        prev_opcode.offset+prev_opcode.len == current_offset)
    {
        ill_tst = TRUE;
    }
    opcode = opc->op_value;     /* start with the basic opcode */
#if DEBUG & OP_DEBUG
    fprintf(stderr,"got class %s, value: %04X %s",
            class_names[opc->op_class&7], opc->op_value, inp_ptr);
#endif
    opclass = opc->op_class & 15;
    switch (opclass)
    {          /* switch on opcode type (class) */
    case OPCL_LD :          /* Load register: LD (Rs), Rd */
        opcode = do_ldst_op(0);
        break;
    case OPCL_ST :          /* Store register: ST Rs, (Rd) */
        opcode = do_ldst_op(1);
        break;
    case OPCL_MOVE :        /* Move into register: MOVE Rs, Rd */
        opcode = do_move_op();
        break;
    case OPCL_R:
        opcode = do_single_op(opc); /* do single operand processing */
        break;
    case OPCL_RR:
        opcode = do_rr_op(opc, 0);  /* do double register operand processing */
        break;
    case OPCL_BRR:
        opcode = do_rr_op(opc, 1);  /* do double register operand processing */
        break;
    case OPCL_nR:
        opcode = do_nr_op(opc); /* do immediate,register operand processing */
        break;
    case OPCL_JMP :         /* jump indirect: JUMP cc, (Rd) */
        opcode = do_jmp_op(opc, 0); /* jump processing */
        break;
    case OPCL_JR:
        opcode = do_jmp_op(opc, 1); /* jr processing */
        break;
    default:
        bad_token(inp_ptr, "Opcode not implemented yet");
        f1_eatit();         /* eat the line */
        opcode = 0xFFFF;        /* make it illegal */
        break;
    }                    /* -- switch(class) */

    which_stack = 2;         /* assume to use stack 2 */
    if (opcode == 0xFFFF || err_cnt != error_count[MSG_ERROR])
    { /* if any errors detected... */
        f1_eatit();           /* ...eat to EOL */
        opcode = OPC_NOP<<OPC_CODE;   /* change opcode to NOP */
        ep = exprs_stack;
        (ep++)->ptr = 0;          /* zap stack 0 */
        (ep++)->ptr = 0;          /* and stack 1 */
        exp = ep->stack;          /* jam opcode into stack 2 */
        exp->expr_code = EXPR_VALUE;
        exp->expr_value = opcode;
        (ep++)->ptr = 1;          /* one term */
        ep->ptr = 0;         /* zap stack 3 */
    }
    else
    {
#if DEBUG & STACK_DEBUG
        fprintf(stderr,"Before doing anything:\n");
        dump_stacks();
#endif
        ep = exprs_stack+1;       /* point to stack 1 */
        if (ep->ptr || (ep+2)->ptr)
        { /* if there are test expressions */
            sprintf(emsg, "%s:%d", current_fnd->fn_name_only, current_fnd->fn_line);
            if (ep->ptr)
            {
                int tag;
                if (opclass == OPCL_JMP || opclass == OPCL_JR)
                {
                    tag = TMP_BOFF;
                }
                else
                {
                    tag = TMP_OOR;
                }
                ep->tag = 0;
                write_to_tmp(tag, 0, (char *)ep, 0);
                write_to_tmp(TMP_ASTNG, strlen(emsg)+1, emsg, 1);
                ep->ptr = 0;        /* done with stack 1 */
            }
            ep += 2;
            if (ep->ptr)
            {
                ep->tag = 0;
                write_to_tmp(TMP_OOR, 0, (char *)ep, 0);
                write_to_tmp(TMP_ASTNG, strlen(emsg)+1, emsg, 1);
                ep->ptr = 0;        /* done with stack 3 */
            }
        }
        ep = exprs_stack+2;       /* point to stack 2 */
        exp = ep->stack + ep->ptr;    /* point to end of dst expression */
        if (ep->ptr)
        {            /* if there's a destination */
            exp->expr_code = EXPR_VALUE;   /* AND it with 0x1F so it appears ok in listing */
            (exp++)->expr_value = 31;
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_AND;
            exp->expr_code = EXPR_VALUE;
            (exp++)->expr_value = opcode;  /* OR in opcode to dst expression */
            exp->expr_code = EXPR_OPER;
            (exp++)->expr_value = EXPROPER_OR;
            ep->ptr += 4;          /* add 4 terms to dst */
        }
        else
        {
            exp->expr_code = EXPR_VALUE;
            exp->expr_value = opcode;  /* just ram in the opcode */
            ep->ptr = 1;           /* only 1 term */
        }
        if (opcode != (OPC_MOVEI<<OPC_CODE))
        {    /* if not a move immediate */
            ep = exprs_stack;      /* point to stack 0 */
            exp = ep->stack;
            if (ep->ptr != 0)
            {        /* if there's a source operand */
                exp = ep->stack + ep->ptr;
                exp->expr_code = EXPR_VALUE;    /* AND it with 0x1F to truncate values 1-32 ... */
                (exp++)->expr_value = 31;       /* ... and negative values */
                exp->expr_code = EXPR_OPER;
                (exp++)->expr_value = EXPROPER_AND;
                exp->expr_code = EXPR_VALUE;
                (exp++)->expr_value = OPC_SRC;
                exp->expr_code = EXPR_OPER;
                (exp++)->expr_value = EXPROPER_SHL; /* shift the source operand 5 bits left */
                ep->ptr += 4;           /* add 4 terms to expression */
                which_stack = merge_stacks(0, 2);   /* merge source and destination stacks */
            }
        }
    }

#if DEBUG & STACK_DEBUG
    fprintf(stderr,"After adding opcode, which_stack: %d\n", which_stack);
    dump_stacks();
#endif

    exprs_stack[which_stack].ptr = compress_expr(exprs_stack+which_stack);

#if DEBUG & STACK_DEBUG
    fprintf(stderr,"After compress(), which_stack: %d\n",which_stack);
    dump_stacks();
#endif
    exprs_stack[which_stack].tag = ( edmask & ED_M68 ) ? 'U' : 'u';
    if (list_bin) compress_expr_psuedo(exprs_stack+which_stack);
    p1o_word(exprs_stack+which_stack);   /* output inst */
    exprs_stack[which_stack].ptr = 0;    /* done with expression */
    if (opcode == (OPC_MOVEI<<OPC_CODE))
    { /* movei is special */
        ep = exprs_stack;         /* operand is in stack 0 */
        exp = ep->stack;
        if (ep->ptr == 0)
        {       /* if there's no source operand, add one */
            exp = ep->stack;
            exp->expr_code = EXPR_VALUE;
            exp->expr_value = 0;
            ep->ptr = 1;
        }
        ep->tag = 'j';            /* high byte of low word first */
        p1o_long(ep);         /* output source operand */
        ep->ptr = 0;          /* nothing in stack anymore */
        this_opcode.len = 6;      /* this is a 6 byte instruction */
        if (ill_tst)
        {
            unsigned short pop = (prev_opcode.opcode>>OPC_CODE);
            if (pop == OPC_JR || pop == OPC_JMP)
            {
                bad_token((char *)0,"Cannot follow a JR or JUMP with a MOVEI");
            }
        }
    }
    else if (ill_tst)
    {
        unsigned short pop = prev_opcode.opcode>>OPC_CODE;
        unsigned short top = opcode>>OPC_CODE;
        int pclass = prev_opcode.class & 15;
        int tclass = opc->op_class & 15;
        int tjmp = tclass == OPCL_JR || tclass == OPCL_JMP;
        int pjmp = pclass == OPCL_JR || pclass == OPCL_JMP;
        if (tjmp && pjmp)
        {
            bad_token((char *)0, "Cannot follow a JR or JUMP with another JR or JUMP");
        }
        else if (top == OPC_MOVEPCR && pjmp )
        {
            bad_token((char *)0, "Cannot follow a JR or JUMP with a MOVE PC");
        }
        else if (pop == OPC_IMULTN && top != OPC_IMACN)
        {
            bad_token((char *)0, "Cannot follow a IMULTN with anything other than IMACN");
        }
        else if (pop == OPC_IMACN && (top != OPC_IMACN && top != OPC_RESMAC))
        {
            bad_token((char *)0, "Cannot follow an IMACN with anything other than another IMACN or RESMAC");
        }
        else if (top == OPC_MMULT && (pclass == OPCL_LD || pclass == OPCL_ST))
        {
            bad_token((char *)0, "Cannot follow a LOAD or STORE with an MMULT");
        }
    }
    this_opcode.opcode = opcode;     /* remember this opcode */
    prev_opcode = this_opcode;
}                   /* -- do_opcode() */

/* 		merge_stacks()
*	merges two expression stacks, by copying the shorter to the longer
*	and adding an 'or' operator. Returns the number of the resulting stack
*	and sets its stack ptr correctly. There is a slight preference for
*	returning first.
*/
static int merge_stacks(int first, int second) {
    short flen,slen;
    int final;
    EXP_stk *ep1,*ep2;
    EXPR_struct *esp1,*esp2;

    ep1 = &exprs_stack[first];
    ep2 = &exprs_stack[second];
    esp1 = ep1->stack;
    esp2 = ep2->stack;

    /* if either is empty, return the other... */
    if ( (slen = ep2->ptr) == 0) return first;
    if ( (flen = ep1->ptr) == 0) return second;

    if ( flen < slen )
    {
        /* first is shorter, copy it to second */
        memcpy((char *)(esp2+ep2->ptr),(char *)esp1,
               flen * sizeof(EXPR_struct));
        final = second;
        ep1->ptr = 0;        /* first stack is now empty */
        ep1 = ep2;           /* set final pointer to second stack */
        esp1 = esp2;
    }
    else
    {
        /* second is shorter, copy it to first */
        memcpy((char *)(esp1+ep1->ptr),(char *)esp2,
               slen * sizeof(EXPR_struct));
        final = first;
        ep2->ptr = 0;        /* second stack is now empty */
    }
    flen += slen;            /* my own constant subexpression elimination */
    esp1 = ep1->stack + flen;    /* compute new end of stack */
    ep1->ptr = flen+1;       /* set new size of final stack */
    esp1->expr_code = EXPR_OPER; /* glue on an "or" to merge the items */
    esp1->expr_value = EXPROPER_OR;
    return final;
}

#if STACK_DEBUG
void dump_stack( int stacknum )
{
    int tag,last;
    EXP_stk *eps;
    EXPR_struct *curr,*top;

    eps = &exprs_stack[stacknum];
    tag = eps->tag;

    fprintf(stderr,"\nStack %d Tag '%c' (0x%02X) depth %d\n",
            stacknum,
            (isgraph(tag) ? tag : '.'),
            tag,
            last = eps->ptr);

    curr = eps->stack;
    top = eps->stack+last;

    for ( ; curr < top ; ++curr )
    {
        switch (tag = curr->expr_code)
        {
        case EXPR_VALUE :
            fprintf(stderr," 0x%X\n",curr->expr_value);
            break;
        case EXPR_OPER :
            tag = curr->expr_value;
            fprintf(stderr," %c (0x%X)\n",(isgraph(tag) ? tag : '.'),tag);
            break;
        case EXPR_SEG :
            fprintf(stderr," %s + 0x%X\n",
                    (curr->expt.expt_seg)->seg_string,curr->expr_value);
            break;
        case EXPR_SYM :
            fprintf(stderr,"`%s'\n",
                    (curr->expt.expt_sym)->ss_string);
            break;
        default :
            fprintf(stderr,"?? code: 0x%X value: 0x%X\n",tag,curr->expr_value);
        }   /* end case */
    }       /* end for */
}       /* end routine dump_stack */

static void
dump_stacks()
{
    int i;
    for ( i = 0 ; i < EXPR_MAXSTACKS ; ++i )
    {
        if ( exprs_stack[i].ptr ) dump_stack(i);
    }
}
#endif

long op_ntype(Opcode *opc)
{
    bad_token(inp_ptr, "Function not supported at this time");
    f1_eatit();
    return -1;
}

#if !defined(VMS)
    #include <stdlib.h>
#endif

int op_float(Opcode *opc) {
#if defined(VMS) || !defined(INCLUDE_FLOAT)
    bad_token(inp_ptr, "Pseudo op not supported on this system at this time");
    f1_eatit();
    return -1;
#else
    double d;
    union
    {
        float f;
        long l;
    } u;
    char *otp=0;
    EXP_stk *ep;
    EXPR_struct *exp;

    list_stats.pc = current_offset;
    list_stats.pc_flag = 1;
    ep = exprs_stack;
    exp = ep->stack;
    ep->tag = 'j';
    ep->tag_len = 1;
    if ((cttbl[(int)*inp_ptr]&(CT_SMC|CT_EOL)) != 0)
    {
        exp->expr_code = EXPR_VALUE;
        u.f = 0.0;
        exp->expr_value = ep->psuedo_value = u.l;
        ep->ptr = 1;
        p1o_long(ep);     /* null arg means insert a 0 */
        return 1;
    }
    while (1)
    {           /* for all items on line */
        char c;
        if (*inp_ptr == ',')
        {    /* if the next item is a comma */
            while (c = *++inp_ptr,isspace(c)); /* skip over white space */
            u.f = 0.0;     /* get a IEEE 0 */
        }
        else
        {
            get_token();
            if (token_type == EOL) break;
            d = strtod(tkn_ptr, &otp);
            if (isspace(*otp)) ++otp;  /* eat trailing white space */
            if (*otp == ',')
            {     /* if we stopped on a comma */
                while (c = *++otp,isspace(c)); /* eat comman and skip over white space */
            }
            else if ((cttbl[(int)*otp]&(CT_SMC|CT_EOL)) == 0)
            {
                bad_token(otp, "Invalid floating point number syntax");
                f1_eatit();
                break;
            }
            inp_ptr = otp;
            u.f = d;
        }
        exp->expr_code = EXPR_VALUE;
        exp->expr_value = ep->psuedo_value = u.l;
        ep->ptr = 1;
        p1o_long(ep);     /* output element 0 */
    }
    return 1;
#endif
}

#define MAC_TJ
#include "opcommon.h"
