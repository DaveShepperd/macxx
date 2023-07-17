/*
    outx.c - Part of macxx, a cross assembler family for various micro-processors
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
/* 			OUTX
*	A collection of output routines for producing extended TEKHEX and
*	VLDA output.
*	The symbol and object output routines each gather their own type
*	of output until either the column outx_width would be exceeded, a type-
*	specific break occurs (e.g. change of address in object output),
*	or a call is made to flush...(). Symbol and object lines may be
*	intermingled, but only at the line level. These are fairly special
*	purpose routines, intended to be used with CHKFOR, a combination
*	formatter and checksum generator.
*/
#include "outx.h"
#include <errno.h>

#ifdef VMS
    #define NORMAL 1
    #define ERROR 0x10000004
#else
    #define NORMAL 0
    #define ERROR 1
#endif
#define	DEBUG 9

/*
            DATA
*/
FILE   *outxabs_fp;
FILE   *outxsym_fp;
int    outx_lineno;
int    outx_width=78;   /* object width */
int    outx_swidth=78;  /* symbol width */
int    outx_debug;
int new_identifier=1;        /* new identifiers */

/* line buffers for Ext-Tek output */
char    *eline;         /* pass2 builds expressions here */
static  char    *sline;
static  char    *oline;

/* pointers into lines and checksums for lines */
static       char   *symp=0,*op=0,*maxop=0;
static  unsigned int    symcs,objcs;
static  unsigned long   addr;
static  unsigned char   varcs;
static  unsigned char   namcs;

char hexdig[] = "0123456789ABCDEF";

static VLDA_abs *vlda_oline;
static VLDA_sym *vlda_sym;  /* pointer to line area */
static VLDA_seg *vlda_seg;  /* pointer to segment definition */
static struct my_desc *vid;

void outx_init( void )
{
    eline = MEM_alloc(MAX_LINE);
    sline = MEM_alloc(MAX_LINE);
    oline = MEM_alloc(MAX_LINE);
    vlda_sym = (VLDA_sym *)sline;
    vlda_seg = (VLDA_seg *)sline;
    vlda_oline = (VLDA_abs *)oline;
    vid = (struct my_desc *)oline;
    return;
}

static char *get_symid(SS_struct *sym_ptr,char *s)
{
    if (sym_ptr->flg_ident)
    {
        sprintf(s," %%%d",sym_ptr->ss_ident);
    }
    else
    {
        sym_ptr->ss_ident = new_identifier++;
        sym_ptr->flg_ident = 1;
#if 0
        if (!sym_ptr->flg_global)
        {
            sprintf(s," {.L%05d}%%%d",sym_ptr->ss_ident,sym_ptr->ss_ident);
        }
        else
        {
#endif
            sprintf(s," {%s}%%%d",sym_ptr->ss_string,sym_ptr->ss_ident);
#if 0
        }
#endif
    }
    while (*s++);
    --s;
    return(s);
}

static short rsize;

static char *do_outexp_vlda(EXP_stk *eps,char *s,int *len)
{
    int k;
    SS_struct *sym_ptr;
    EXPR_struct *exp;
    union vexp ve;

    k = eps->ptr;        /* the length */
    exp = eps->stack;        /* the stack */
    ve.vexp_chp = s;     /* point to output array */
    for (;k > 0; --k,++exp)
    {
        switch (exp->expr_code)
        {
        case EXPR_L: {     /* segment length.. */
                *ve.vexp_type++ = VLDA_EXPR_L;
                *ve.vexp_ident++ = exp->expr_count;
                continue;
            }
        case EXPR_B: {     /* segment base */
                *ve.vexp_type++ = VLDA_EXPR_B; /* insert type code */
                *ve.vexp_ident++ = exp->expr_count;
                continue;
            }
        case EXPR_SEG: {   /* segment */
                unsigned long cmp;
                if (exp->expr_seg->seg_ident > 255)
                {
                    *ve.vexp_type++ = VLDA_EXPR_SYM;
                    *ve.vexp_ident++ = exp->expr_seg->seg_ident;
                }
                else
                {
                    *ve.vexp_type++ = VLDA_EXPR_CSYM;
                    *ve.vexp_byte++ = exp->expr_seg->seg_ident;
                }
                exp->expr_value += exp->expr_seg->rel_offset;
                cmp = (exp->expr_value >= 0) ? exp->expr_value : -exp->expr_value;
                if (cmp == 0)
                {
                    continue;
                }
                else if (cmp < 128)
                {
                    *ve.vexp_type++ = VLDA_EXPR_CVALUE;
                    *ve.vexp_byte++ = exp->expr_value;
                }
                else if (cmp < 32768)
                {
                    *ve.vexp_type++ = VLDA_EXPR_WVALUE;
                    *ve.vexp_word++ = exp->expr_value;
                }
                else
                {
                    *ve.vexp_type++ = VLDA_EXPR_VALUE;
                    *ve.vexp_long++ = exp->expr_value;
                }
                *ve.vexp_type++ = VLDA_EXPR_OPER;
                *ve.vexp_oper++ = '+';
                *len += 2;      /* added 2 terms to expression */
                continue;
            }
        case EXPR_SYM: {   /* symbol */
                sym_ptr = exp->expr_sym;
                if (!sym_ptr->flg_defined || sym_ptr->flg_exprs)
                {
                    if (!sym_ptr->flg_ident)
                    {
                        sym_ptr->ss_ident = new_identifier++;
                        sym_ptr->flg_ident = 1;
                    }
                    if (sym_ptr->ss_ident > 255)
                    {
                        *ve.vexp_type++ = VLDA_EXPR_SYM;
                        *ve.vexp_ident++ = sym_ptr->ss_ident;
                    }
                    else
                    {
                        *ve.vexp_type++ = VLDA_EXPR_CSYM;
                        *ve.vexp_byte++ = sym_ptr->ss_ident;
                    }
                    continue;
                }
                exp->expr_value = sym_ptr->ss_value;
            }          /* fall through to EXPR_VALUE */
        case EXPR_VALUE: { /* constant */
                unsigned long cmp;
                cmp = (exp->expr_value >= 0) ? exp->expr_value : -exp->expr_value;
                if (cmp == 0)
                {
                    *ve.vexp_type++ = VLDA_EXPR_0VALUE;
                    continue;
                }
                else if (cmp < 128)
                {
                    *ve.vexp_type++ = VLDA_EXPR_CVALUE;
                    *ve.vexp_byte++ = exp->expr_value;
                }
                else if (cmp < 32768)
                {
                    *ve.vexp_type++ = VLDA_EXPR_WVALUE;
                    *ve.vexp_word++ = exp->expr_value;
                }
                else
                {
                    *ve.vexp_type++ = VLDA_EXPR_VALUE;
                    *ve.vexp_long++ = exp->expr_value;
                }
                continue;
            }
        case EXPR_OPER: {  /* operator */
                *ve.vexp_type++ = VLDA_EXPR_OPER;
                *ve.vexp_oper++ = exp->expr_value;
                if ((exp->expr_value&255) == EXPROPER_TST)
                {
                    *ve.vexp_oper++ = exp->expr_value >> 8;
                }
                continue;
            }
        case EXPR_LINK: {  /* link to another expression */
                EXP_stk *ep;
                int ll;
                ep = exp->expr_expr;
                ll = ep->ptr;
                ve.vexp_chp = do_outexp_vlda(ep,ve.vexp_chp,&ll);
                *len += ll-1;   /* account for new items, but take out link */
                continue;
            }
        default: {
                sprintf(emsg,"OUTX (do_expr_vlda)-Undefined expression code: %d",exp->expr_code);
                err_msg(MSG_ERROR,emsg);
                continue;
            }          /* --default */
        }             /* --switch */
    }                /* --for */
    if (eps->tag != 0)
    {
        unsigned long cmp;
        cmp = eps->tag_len;
        if (cmp < 2)
        {
            *ve.vexp_type++ = VLDA_EXPR_1TAG;  /* insert tag code */
            *ve.vexp_type++ = eps->tag;        /* insert tag code */
        }
        else if (cmp < 128)
        {
            *ve.vexp_type++ = VLDA_EXPR_CTAG;  /* insert tag code */
            *ve.vexp_type++ = eps->tag;    /* insert tag code */
            *ve.vexp_byte++ = eps->tag_len;
        }
        else if (cmp < 32768)
        {
            *ve.vexp_type++ = VLDA_EXPR_WTAG;  /* insert tag code */
            *ve.vexp_type++ = eps->tag;    /* insert tag code */
            *ve.vexp_word++ = eps->tag_len;
        }
        else
        {
            *ve.vexp_type++ = VLDA_EXPR_TAG;   /* insert tag code */
            *ve.vexp_type++ = eps->tag;    /* insert tag code */
            *ve.vexp_long++ = eps->tag_len;
        }
    }
    return(ve.vexp_chp); /* exit */
}

static char *do_outexp_ol(EXP_stk *eps,char *s)
{
    int k;
    SS_struct *sym_ptr;
    EXPR_struct *exp;

    k = eps->ptr;        /* the length */
    exp = eps->stack;        /* the stack */
    *s = 0;          /* null terminate the dst string */
    for (;k > 0; --k,++exp)
    {
        switch (exp->expr_code)
        {
        char lb;
        case EXPR_B:
            lb = 'B';
            goto lb_common1;
        case EXPR_L:
            lb = 'L';
            lb_common1: sprintf(s," %c %%%d",lb,exp->expr_seg->seg_ident);
            goto lb_common2;
        case EXPR_SEG:
            exp->expr_value += exp->expr_seg->rel_offset;
            if (exp->expr_value != 0)
            {
                sprintf(s," %%%d %ld +",exp->expr_seg->seg_ident,exp->expr_value);
            }
            else
            {
                sprintf(s," %%%d",exp->expr_seg->seg_ident);
            }
            lb_common2: while (*s++);
            --s;
            continue;
        case EXPR_SYM: {   /* symbol or segment */
                sym_ptr = exp->expr_sym;
                if (!sym_ptr->flg_defined || sym_ptr->flg_exprs)
                {
/* printf("Doing a getsymid on \"%s\". ident=%d\n",sym_ptr->ss_string,sym_ptr->ss_ident); */
                    s = get_symid(sym_ptr,s);
                    continue;
                }
                exp->expr_value = sym_ptr->ss_value;
            }
        case EXPR_VALUE: {
                sprintf(s," %ld",exp->expr_value);
                while (*s++);
                --s;
                continue;
            }
        case EXPR_OPER: {
                *s++ = ' '; /* a space */
                *s++ = exp->expr_value;
                if ((exp->expr_value&255) == EXPROPER_TST)
                {
                    *s++ = exp->expr_value >> 8;
                }
                *s = 0;     /* move the null */
                continue;
            }
        case EXPR_LINK: {  /* link to another expression */
                s = do_outexp_ol(exp->expr_expr,s);
                *s = 0;
                continue;
            }
        default: {
                sprintf(emsg,"OUTX (do_exp_ol)-Undefined expression code: %d",exp->expr_code);
                err_msg(MSG_ERROR,emsg);
                continue;
            }          /* --default */
        }             /* --switch */
    }                /* --for */
    if (eps->tag)
    {
        if (eps->tag_len == 1 || eps->tag_len == 0)
        {
            *s++ = ':';        /* follow with tag code */
            *s++ = eps->tag;   /* and tag */
        }
        else
        {
            sprintf(s,":%c %d",eps->tag,eps->tag_len); /* follow with tag and length */
            while (*s++);
            --s;           /* backup to NULL */
        }
    }
    return(s);
}

static int formvar(unsigned long num, char *where );

char *outexp(EXP_stk *eps, char *s, char *wrt, FILE *fp )
{
    int len, evenodd;

    flushobj();              /* flush the buffer */
    if (options[QUAL_BINARY])
    {
        union vexp ve;
        union vexp adj_len;
        adj_len.vexp_chp = ve.vexp_chp = s;   /* point to output array */
        ++ve.vexp_len;                /* skip the length field */
        len = eps->ptr;
        ve.vexp_chp = do_outexp_vlda(eps,ve.vexp_chp,&len);
        *adj_len.vexp_len = len + ((eps->tag != 0)?1:0); /* expression size (in items) */
        if (wrt==0) return(ve.vexp_chp);     /* exit */
        rsize = ve.vexp_chp-wrt;
#ifndef VMS
        fwrite(&rsize,sizeof(short),1,fp);
        evenodd = rsize&1;
#else
        evenodd = 0;
#endif
        fwrite(wrt,(int)rsize+evenodd,1,fp); /* write it */
        return 0;         /* exit */
    }
    else
    {         /* --vlda */
        *s = 0;           /* null terminate the dst string */
        len = eps->ptr;
        s = do_outexp_ol(eps,s);
        if (wrt)
        {
            *s++ = '\n';
            *s = 0;
            fputs(wrt,fp);
        }
    }                /* --vlda */
    return(s);
}

char *outxfer(EXP_stk *eps, FILE *fp)
{
    char *s;

    eps->tag = 0;        /* make sure there's no tag */
    if (options[QUAL_BINARY])
    {
        char *kluge;
        int evenodd;
        union vexp ve;
        ve.vexp_chp = kluge = eline;     /* point to output array */
        *ve.vexp_type++ = VLDA_XFER;  /* set the type */
        s = outexp(eps,ve.vexp_chp,(char *)0,(FILE *)0);
        rsize = s-kluge;
#ifndef VMS
        fwrite(&rsize,sizeof(short),1,fp);
        evenodd = rsize&1;
#else
        evenodd = 0;
#endif
        fwrite(eline,(int)rsize+evenodd,1,fp); /* write it */
        return 0;         /* exit */
    }
    else
    {         /* --vlda */
        strcpy(eline,".start");
        s = eline+strlen(eline);
        s = outexp(eps,s,(char *)0,(FILE *)0);
        *s++ = '\n';
        *s = 0;
        fputs(eline,fp);
    }                /* --vlda */
    return(0);
}

/*			outbstr(from,len)
*	Output a byte string, accumulating the checksums for the output record
*	and both the even and odd ROMs. 
*/
int outbstr(char *from, int len )
{
    register unsigned char *rop,*fop;
    unsigned char c;
    int  k,limit,toeol=0;
    fop = (unsigned char *)from;
    if (!options[QUAL_BINARY])
    {
        len += len;       /* double the input length if ASCII output */
    }
    else
    {
        toeol = oline + outx_width - op;
        if (len >= toeol-2) flushobj();
    }
    while (len > 0)
    {
        if ( op != 0 )
        {
            toeol = oline + outx_width - op;
            if (toeol  < 2 )
            {
                flushobj();
            }
        }
        if ( op == 0 )
        {      /* After flushing, or when first... */
            switch (output_mode)
            {     /* dispatch on output mode */
            case OUTPUT_HEX: {
                    strcpy(oline,"%ll6kk");  /* ...started, set up the buffer */
                    op = oline+6;        /* with header and address */
                    op += formvar(addr,op);
                    objcs = varcs + 6;
                    break;           /* out of switch */
                }
            case OUTPUT_OL: {
                    op = oline;      /* point to start of buffer */
                    *op++ = '\'';        /* start with a single quote */
                    break;
                }
            case OUTPUT_OBJ:
                vlda_oline->vlda_type = VLDA_TXT;
                vlda_oline->vlda_addr = addr; /* load address */
                op = oline + sizeof(VLDA_abs);
                break;
            case OUTPUT_VLDA: {
                    vlda_oline->vlda_type = VLDA_ABS;
                    vlda_oline->vlda_addr = addr; /* load address */
                    op = oline + sizeof(VLDA_abs);
                    break;
                }
            }
            toeol = oline + outx_width - op;
            maxop = op;
        }
        limit = (toeol < len) ? toeol : len ;
        if ((limit &= ~1) == 0) limit = 1;
        if (outx_debug > 2)
        {
            fprintf(stderr,"Loading %d bytes at addr %08lx in rcd %08lX\n",
                    limit,addr,vlda_oline->vlda_addr);
        }
        rop = (unsigned char *)op;
        len -= limit;
        if (!options[QUAL_BINARY])
        {
            addr += limit/2;       /* update address */
            for (  ; limit > 0 ; limit -= 2)
            {  /* first transfer the high nibble */
                k = *fop++;
                c = k >> 4;
                objcs += c;
                *rop++ = hexdig[c];
                c = k & 0xF;        /* now transfer the low one */
                objcs += c;
                *rop++ = hexdig[c];
            }              /* end for */    
        }
        else
        {
            addr += limit;
            for (  ; limit > 0 ; --limit) *rop++ = *fop++;
        }
        op = (char *)rop;
        if (maxop < op) maxop = op;
    }        /* end while */
    return 0;
}

/*			outorg(exp_ptr)
*	Sets the origin for following outbstr calls. If address
*	supplied is different from current address (in addr), flushes object
*	buffer.
*/

void outorg(EXP_stk *eps)
{
    EXPR_struct *exp;
    unsigned long taddr,laddr,address;       /* address of end of txt + 1 */
    char *s;
    eps->tag = 0;            /* make sure there's no tag */
    exp = eps->stack;            /* get ptr to stack */
    address = exp->expr_value;       /* assume value */
    if (macxx_bytes_mau != 1)
    {
        exp += eps->ptr;          /* point to end of stack */
        exp->expr_value = macxx_bytes_mau; /* put on value */
        (exp++)->expr_code = EXPR_VALUE;
        exp->expr_value = '*';        /* put on multiplier */
        (exp++)->expr_code = EXPR_OPER;
        eps->ptr += 2;
        exp = eps->stack;
        address *= macxx_bytes_mau;
    }
    switch (output_mode)
    {       /* how to encode it */
    case OUTPUT_HEX: {        /* absolute tekhex mode */
            if ( address == addr) return;  /* nothing to do */
            flushobj();            /* flush out tekhex files */
            break;
        }  
    case OUTPUT_OL: {         /* relative .OL output */
            flushobj();            /* flush the buffer */
            if (eps->ptr < 1)
            {
                err_msg(MSG_ERROR,"OUTX (outorg)-Too few terms in expression setting .org");
                break;
            }
            strcpy(oline,".org");
            s = oline + 4;
            s = outexp(eps,s,(char *)0,(FILE *)0);
            if (*(s-1) == '+')
            {
                --s;
            }
            else
            {
                *s++ = ' ';
                *s++ = '0';
            }
            *s++ = '\n';
            *s = 0;
            fputs(oline,outxabs_fp);
            break;
        }
    case OUTPUT_VLDA: {       /* absolute VLDA mode */
            if ( address == addr) return;  /* nothing to do */
            if ( op != 0 )
            {       /* if there's something in the buffer */
                taddr = maxop - oline + sizeof(VLDA_abs) +
                        (laddr=vlda_oline->vlda_addr);
                if ( address > taddr ||     /* if off the end of the txt rcd */
                     address < laddr)
                {    /* or he's backpatching too far */
                    flushobj();      /* flush out the current txt record */
                }
                else
                {
                    op = oline + address - laddr + sizeof(VLDA_abs);
                    /* otherwise, just backup into txt */
                }
            }
            break;             /* fall out of switch */
        }                 /* -- case 0,1 */
    case OUTPUT_OBJ: {        /* relative VLDA output */
            flushobj();            /* flush the buffer */
            vlda_oline->vlda_type = VLDA_ORG;
            outexp(eps,(char *)&(vlda_oline->vlda_addr),oline,outxabs_fp);
            break;
        }
    }
    if (outx_debug > 2) fprintf(stderr,"Changing ORG from %08lX to %08lX\n",addr,address);
    addr = address;          /* set the new address */
    return;
}

/*			outbyt(num,where)
*	"Output" a byte as two hex digits in buffer at where. return the
*	checksum.
*/

int outbyt(unsigned int    num, char    *where)
{
    int chk;
    chk = (num >> 4) &0xF;
    *where++ = hexdig[chk];
    *where = hexdig[num &= 0xF];
    return(chk+num);
}


/*			formvar(num,where)
*	Format a variable length number in buffer pointed to by where,
*	returning the string length (number length + 1 for count digit).
*	This preliminary is required to check if the number will fit on
*	the current line. The variable varcs will contain the contribution
*	of this string to the checksum.
*/
static int formvar(unsigned long num, char *where )
{
    int count;
    register char *vp;
    register char c;
    unsigned long msk;
    vp = where;

/*	First, predict where the string will end    */

    for (count = 0,msk = ~0 ; msk & num ; msk <<= 4) ++count;
    if ( count == 0 ) ++count;
    vp = &where[count+1];
    *vp = 0;
    varcs = 0;

/*	Now, output characters from lsd to msd    */
    do
    {
        c = num & 0xF;
        varcs += c;
        *--vp = hexdig[(int)c];
    } while ( (num >>= 4) != 0);
    c = (count &= 0xF);
    varcs += c;
    *--vp = hexdig[(int)c];
    if ( vp != where )
    {
        fprintf(stderr, "OUTX:formvar- bad estimate, %lX != %lX\n", (unsigned long)vp, (unsigned long)where);
    }
    return(count+1);
}

/*			formsym(name,type,address,where)
*	Format a symbol entry in buffer at where, returning the string length
*	(name length + 2 for type and count digit + length of address from
*	formvar(). This preliminary is required to check if the symbol will
*	fit on the current line.
*/
static int formsym(char *name, int type, unsigned long address, char *where)
{
    int i;
    char *np,c;
    namcs = type;
    *where = type + '0';
    np = &where[2];
    for ( i = 0 ; i < 16 ; ++i)
    {
        if ( (c = name[i]) == 0 ) break;
        *np++ = c;
#ifndef isdigit
        if ( c >= '0' && c <= '9' ) c -= '0';
        else if ( c >= 'A' && c <= 'Z' ) c -= ('A' - 10);
        else if ( c >= 'a' && c <= 'z' ) c -= ('a' - 40);
#else
        if ( isdigit(c) ) c -= '0';
        else if ( isupper(c) ) c -= ('A' - 10);
        else if ( islower(c) ) c -= ('a' - 40);
#endif
        else if ( c == '.' )  c = 38;
        else if ( c >= '_' )  c = 39;
        namcs += c;
    }
    if ( i == 0 )
    {
        fprintf(stderr,"OUTX:formsym- line %d: Bad symbol %s\n",outx_lineno,name);
        return(0);
    }
    namcs += ( i &= 0xF );
    where[1] = hexdig[i];
    i = formvar(address,np);
    namcs += varcs;
    np[i] = 0;
    return((np-where) + i);
}

static char *sline_ptr;

int outsym(SS_struct *sym_ptr,int mode)
{
    int len;
    char save1,save2,*ssp;

/*	If the first time through, set up the head of a block, and init
 *	checksum and length
 */
    if ( symp == 0 )
    {
        sline_ptr = sline;
        if (mode == OUTPUT_VLDA) *sline_ptr++ = VLDA_TPR;
        strcpy(sline_ptr,"%ll3kk2S_");    /* length will overwrite ll, checksum kk */
        symcs = 0x48;
        symp = sline_ptr+9;
    }                    /* --symp == 0 */
    len = formsym(sym_ptr->ss_string,1,(unsigned long)sym_ptr->ss_value,symp);
    if ( symp - sline + len >= outx_swidth)
    {

/*	We can't fit the current symbol on the line, so we stop here and output
*	the line, then reset to initial conditions plus new symbol.
*/
        symcs += outbyt((unsigned int)(symp - (sline_ptr+1)),sline_ptr+1);
        outbyt(symcs,sline_ptr+4);
        ssp = symp;
        save1 = *ssp++;
        save2 = *ssp;
        if (mode == OUTPUT_VLDA)
        {
            char *kluge = sline;
            int evenodd;
            *symp = '\r';
            *ssp = '\n';
#ifndef VMS
            rsize = ssp-kluge+1;
            fwrite(&rsize,sizeof(short),1,outxsym_fp);
            evenodd = (int)(ssp-sline+1)&1;
#else
            evenodd = 0;
#endif
            fwrite(sline,(int)(ssp-sline+1)+evenodd,1,outxsym_fp); /* write a transparent rcd */
        }
        else
        {
            *symp = '\n';
            *ssp = 0;
            fputs(sline,outxsym_fp);
        }
        *symp = save1;        /* replace our NULL (repair the patch)*/    
        *ssp = save2;
        strcpy(sline_ptr+9,symp);
        symcs = 0x48 + namcs;
        symp = sline_ptr + 9 + len;
    }
    else
    {         /* --symp < outx_swidth */
        symcs += namcs;
        symp += len;
    }                /* --symp < outx_swidth */
    return 0;            /* done */
}

void outsym_def(SS_struct *sym_ptr,int mode)
{
    register char *s,*name=sym_ptr->ss_string;

    switch (mode)
    {
    case OUTPUT_VLDA:     /* vlda absolute */
    case OUTPUT_HEX: {    /* absolute output (tekhex/non-relative) */
            outsym(sym_ptr,mode);
            return;        /* easy out */
        }
    case OUTPUT_OL: {     /* relative output (.OL format) */
            if (!sym_ptr->flg_defined)
            {
                strcpy(sline,".ext ");
            }
            else
            {
                strcpy(sline,sym_ptr->flg_global ? ".defg" : ".defl");
            }
            s = get_symid(sym_ptr,sline+5);    /* get an ASCII ID */
            if (!sym_ptr->flg_defined)
            {
                strcpy(s,"\n");
            }
            else
            {
                if (sym_ptr->flg_abs)
                {
                    sprintf(s," %ld\n",sym_ptr->ss_value);
                }
                else
                {
                    if (sym_ptr->flg_exprs)
                    {
/* printf("Doing %d term expression defining \"%s\"\n",sym_ptr->ss_exprs->ptr,sym_ptr->ss_string); */
                        s = outexp(sym_ptr->ss_exprs,s,(char *)0,(FILE *)0);
                        *s++ = '\n';
                        *s = 0;
                    }
                    else
                    {
                        sprintf(s," %%%d %ld +\n",sym_ptr->ss_seg->seg_ident,sym_ptr->ss_value);
                    }
                }
            }
            fputs(sline,outxsym_fp);
            return;
        }
    case OUTPUT_OBJ: {    /* vlda relative */
            char *kluge = sline;
            int flg, evenodd;
            vlda_sym->vsym_rectyp = VLDA_GSD; /* signal its a GSD record */
            flg = VSYM_SYM;        /* it's a symbol */
            if (sym_ptr->flg_defined) flg |= VSYM_DEF; /* symbols may be defined */
            if (sym_ptr->flg_exprs) flg |= VSYM_EXP;
            if (sym_ptr->flg_abs) flg |= VSYM_ABS; /* may-be abs */
            if (!sym_ptr->flg_global) flg |= VSYM_LCL; /* may be local */
            vlda_sym->vsym_flags = flg;
            if (!sym_ptr->flg_ident)
            {
                sym_ptr->ss_ident = new_identifier++;
                sym_ptr->flg_ident = 1;
            }
            vlda_sym->vsym_ident = sym_ptr->ss_ident;
            vlda_sym->vsym_value = sym_ptr->ss_value;
            s = sline + sizeof(VLDA_sym);
            if (flg&VSYM_DEF)
            {
                if (flg&VSYM_EXP)
                {
                    vlda_sym->vsym_eoff = s - kluge;
                    s = outexp(sym_ptr->ss_exprs,s,(char *)0,(FILE *)0);
                }
                else
                {
                    if ((flg&VSYM_ABS) == 0)
                    {
                        union vexp ve;
                        unsigned long cmp;
                        vlda_sym->vsym_flags |= VSYM_EXP;
                        vlda_sym->vsym_eoff = s - kluge;
                        ve.vexp_chp = s;
                        cmp = (sym_ptr->ss_value >= 0) ? sym_ptr->ss_value : -sym_ptr->ss_value;
                        *ve.vexp_len++ = (cmp != 0) ? 3 : 1;
                        if (sym_ptr->ss_seg->seg_ident > 255)
                        {
                            *ve.vexp_type++ = VLDA_EXPR_SYM;
                            *ve.vexp_ident++ = sym_ptr->ss_seg->seg_ident;
                        }
                        else
                        {
                            *ve.vexp_type++ = VLDA_EXPR_CSYM;
                            *ve.vexp_byte++ = sym_ptr->ss_seg->seg_ident;
                        }
                        if (cmp != 0)
                        {
                            if (cmp < 128)
                            {
                                *ve.vexp_type++ = VLDA_EXPR_CVALUE;
                                *ve.vexp_byte++ = sym_ptr->ss_value;
                            }
                            else if (cmp < 32768)
                            {
                                *ve.vexp_type++ = VLDA_EXPR_WVALUE;
                                *ve.vexp_word++ = sym_ptr->ss_value;
                            }
                            else
                            {
                                *ve.vexp_type++ = VLDA_EXPR_VALUE;
                                *ve.vexp_long++ = sym_ptr->ss_value;
                            }
                            *ve.vexp_type++ = VLDA_EXPR_OPER;
                            *ve.vexp_oper++ = '+';
                        }
                        s = ve.vexp_chp;
                    }
                }
            }
            vlda_sym->vsym_noff = s-kluge;
            while ((*s++ = *name++) != 0); /* copy in the symbol name string */
#ifndef VMS
            rsize = s-kluge;
            fwrite(&rsize,sizeof(short),1,outxsym_fp);
            evenodd = rsize&1;
#else
            evenodd = 0;
#endif
            fwrite(sline,(int)(s-sline)+evenodd,1,outxsym_fp); /* write it */
            return;        /* done */
        }             /* --case 2,3 */
    }                /* --switch */
    return;          /* done */
}

void outseg_def(SEG_struct *seg_ptr)
{
    register char *s,*name=seg_ptr->seg_string;

    switch (output_mode)
    {
    case OUTPUT_VLDA:     /* absolute VLDA output */
    case OUTPUT_HEX: {    /* absolute output (tekhex/non-relative) */
/*	 outsym(seg_ptr,output_mode); */
            return;        /* easy out */
        }
    case OUTPUT_OL: {     /* relative output (.OL format) */
            strcpy(sline,".seg");
            s = sline+4;
            sprintf(s," {%s}%%%d",seg_ptr->seg_string,seg_ptr->seg_ident);
            while (*s++);
            --s;
            if (seg_ptr->seg_dalign != 0)
            {
                sprintf(s," %d %d {",
                        seg_ptr->seg_salign,seg_ptr->seg_dalign);
            }
            else
            {
                sprintf(s," %d %c {",
                        seg_ptr->seg_salign,seg_ptr->flg_ovr?'c':'u');
            }
            while (*s++);
            --s;
            if (seg_ptr->flg_abs)  *s++ = 'a';
            if (seg_ptr->flg_based) *s++ = 'b';
            if (seg_ptr->flg_ovr)  *s++ = 'c';
            if (seg_ptr->flg_data) *s++ = 'd';
            if (seg_ptr->flg_literal) *s++ = 'l';
            if (seg_ptr->flg_noout) *s++ = 'o';
            if (seg_ptr->flg_ro)   *s++ = 'r';
            if (!seg_ptr->flg_reference) *s++ = 'u';
            if (seg_ptr->flg_zero) *s++ = 'z';
            *s++ = '}';
            *s++ = '\n';
            *s++ = 0;
            fputs(sline,outxsym_fp);
            fprintf(outxsym_fp,".len %%%d %ld\n",
                    seg_ptr->seg_ident,seg_ptr->seg_len);
            if (seg_ptr->flg_based)
            {
                fprintf(outxsym_fp,".abs %%%d %ld\n",
                        seg_ptr->seg_ident,seg_ptr->seg_base);
            }              /* --based */
            return;            /* done */
        }
    case OUTPUT_OBJ: {        /* vlda relative */
            int flg, evenodd;
            char *kluge = sline;
            vlda_seg->vseg_rectyp=VLDA_GSD; /* signal its a GSD record */
            flg = VSEG_DEF;        /* VSEG_SYM=0, and VSEG_DEF=1 */
            if (seg_ptr->flg_based) flg |= VSEG_BASED; /* may be based */
            if (seg_ptr->flg_zero) flg |= VSEG_BPAGE; /* may be bsect */
            if (seg_ptr->flg_abs | seg_ptr->flg_based) flg |= VSEG_ABS; /* may-be abs */
            if (seg_ptr->flg_ovr) flg |= VSEG_OVR; /* may be overlaid */
            if (seg_ptr->flg_data) flg |= VSEG_DATA; /* segment is data/code */
            if (seg_ptr->flg_ro) flg |= VSEG_RO; /* segment is read/only */
            if (seg_ptr->flg_reference) flg |= VSEG_REFERENCE; /* segment is referenced */
            if (seg_ptr->flg_literal) flg |= VSEG_LITERAL; /* segment is a literal pool */
            vlda_seg->vseg_flags = flg;
            vlda_seg->vseg_ident = seg_ptr->seg_ident;
            vlda_seg->vseg_base = seg_ptr->seg_base; /* if based */
            vlda_seg->vseg_offset = 0; /* no offsets in assembly */
            vlda_seg->vseg_salign = seg_ptr->seg_salign; /* copy alignments */
            vlda_seg->vseg_dalign = seg_ptr->seg_dalign;
            vlda_seg->vseg_maxlen = seg_ptr->seg_maxlen;
            vlda_seg->vseg_noff = sizeof(VLDA_seg);
            s = sline + sizeof(VLDA_seg);
            while ( (*s++ = *name++) );    /* copy in the segment name string */
#ifndef VMS
            rsize = s-kluge;
            fwrite(&rsize,sizeof(short),1,outxsym_fp);
            evenodd = rsize&1;
#else
            evenodd = 0;
#endif
            fwrite(sline,(int)(s-sline)+evenodd,1,outxsym_fp); /* write it */
            ((VLDA_slen *)vlda_seg)->vslen_rectyp=VLDA_SLEN; /* signal its a GSD record */
            ((VLDA_slen *)vlda_seg)->vslen_ident = seg_ptr->seg_ident;
            ((VLDA_slen *)vlda_seg)->vslen_len = seg_ptr->seg_len; /* copy the length */
#ifndef VMS
            rsize = sizeof(VLDA_slen);
            fwrite(&rsize,sizeof(short),1,outxsym_fp);
            evenodd = rsize&1;
#endif
            fwrite(sline,sizeof(VLDA_slen)+evenodd,1,outxsym_fp); /* write it */
        }             /* --case 2,3 */
    }                /* --switch */
    return;          /* done */
}

int flushobj( void )
{
    if (op != 0 )
    {      /* ok to flush? */
        switch (output_mode)
        {
        case OUTPUT_OL: {  /* ol format */
                strcpy(op,"'\n");   /* terminate the line with '\n */
                fputs(oline,outxabs_fp);
                break;
            }
        case OUTPUT_HEX: {
                *op++ = '\n';   /* terminate with a nl. */
                *op = 0;        /* then a null */
                objcs += outbyt((unsigned int)(op - (oline + 2)),oline + 1);
                outbyt(objcs,oline + 4);
                fputs(oline,outxabs_fp);
                break;
            }
        case OUTPUT_VLDA:
        case OUTPUT_OBJ: {
                int evenodd;
#ifndef VMS
                char *kluge = oline;
                rsize = maxop-kluge;
                fwrite(&rsize,sizeof(short),1,outxabs_fp);
                evenodd = rsize&1;
#else
                evenodd = 0;
#endif
                if (outx_debug > 2) printf("fwriting %d bytes to object file\n",maxop-oline);
                fwrite(oline,(int)(maxop-oline)+evenodd,1,outxabs_fp);
                break;
            }
        }
        op = 0;
    }
    return 0;
}

#if 0
static int flushsym(mode)
int mode;
{
    int i=2;
    if (symp)
    {          /* ok to flush? */
        if (mode == OUTPUT_VLDA)
        {
            *symp++ = '\r';
            i++;
        }
        *symp++ = '\n';       /* terminate line with new-line */
        *symp = 0;
        symcs += outbyt((unsigned int)(symp - (sline_ptr+i)),sline_ptr+1);
        outbyt(symcs,sline_ptr+4);
        switch (mode)
        {
        case OUTPUT_VLDA: {
                int evenodd;
#ifndef VMS
                char *kluge = sline;
                rsize = symp-kluge;
                fwrite(&rsize,sizeof(short),1,outxsym_fp);
                evenodd = rsize&1;
#else
                evenodd = 0;
#endif
                fwrite(sline,(int)(symp-sline)+evenodd,1,outxsym_fp); /* write it */
                break;
            }
        case OUTPUT_HEX: {
                fputs(sline,outxsym_fp);
                break;
            }
        }
        symp = 0;
    }
    return 0;
}
#endif

/*	termobj(traddr)
*	Outputs terminating record (including transfer address- traddr) to
*	output.
*/

void termobj(long traddr)
{
    if ( op ) flushobj();
    if (output_mode == OUTPUT_HEX)
    {
        strcpy(oline,"%ll8kk");
        op = oline + 6;
        op += formvar((unsigned long)traddr,op);
        objcs = varcs + 8;
        if ( ( oline + outx_width - op ) < 2 )
        {
            fprintf(stderr,"OUTX:termobj- Width (=%d) too small\n",outx_width);
        }
        *op++ = '\n';
        *op = 0;
        objcs += outbyt((unsigned int)(op - (oline+2)),oline+1);
        outbyt(objcs,oline+4);
        fputs(oline,outxabs_fp);
    }
    op = 0;
    return;
}

static int major_version,minor_version;

void outid(FILE *fp,int mode)
{
    int w;
    char *s,err_msgs[20];
    VLDA_id *vldaid = (VLDA_id *)oline;

    major_version = VLDA_MAJOR;
    minor_version = VLDA_MINOR;
    switch (mode)
    {
    case OUTPUT_HEX:
    case OUTPUT_VLDA: {
            return;            /* nothing to do on abs output */
        }
    case OUTPUT_OL: {
            w = 0;
            if (error_count[MSG_FATAL]+error_count[MSG_ERROR] != 0) w |= 2;
            if (error_count[MSG_WARN] != 0) w |= 1;
            switch (w)
            {
            case 1:
                sprintf(err_msgs," wrn=%d",error_count[MSG_WARN]);
                break;
            case 2:
                sprintf(err_msgs," err=%d",error_count[MSG_ERROR]+error_count[MSG_FATAL]);
                break;
            case 3:
                sprintf(err_msgs," err=%d, wrn=%d",error_count[MSG_ERROR]+error_count[MSG_FATAL],error_count[MSG_WARN]);
                break;
            default:
                err_msgs[0] = 0;
            }
            fprintf(fp,".id \"translator\" \"%s %s%s\"\n",
                    macxx_name, macxx_version, err_msgs);
            fprintf(fp,".id \"mod\" \"%s\"\n",
                    output_files[OUT_FN_OBJ].fn_name_only);
            vid->md_len = 30;
#ifdef VMS
            vid->md_type = vid->md_class = 0;
#endif
            fprintf(fp,".id \"date\" %s\n",ascii_date);
            fprintf(fp,".id \"target\" \"%s\"\n",macxx_target);
#if 0
            fprintf(fp,".id \"maxtoken\" %d\n", max_token);
#endif
            return;
        }
    case OUTPUT_OBJ: {
            int evenodd;
            char *kluge = oline;
            vldaid->vid_rectyp = VLDA_ID;
            vldaid->vid_siz = sizeof(VLDA_id);
            vldaid->vid_maj = major_version;
            vldaid->vid_min = minor_version;
            vldaid->vid_symsiz = sizeof(VLDA_sym);
            vldaid->vid_segsiz = sizeof(VLDA_seg);
            vldaid->vid_image = sizeof(VLDA_id);
            vldaid->vid_errors = (error_count[MSG_ERROR] < 256) ? error_count[MSG_ERROR] : 255;
            vldaid->vid_warns = (error_count[MSG_WARN] < 256) ? error_count[MSG_WARN] : 255;
#if 0
            vldaid->vid_maxtoken = max_token;
#endif
            s = oline + sizeof(VLDA_id);
            sprintf(s,"\"%s %s\"",macxx_name,macxx_version);
            while (*s++);
            vldaid->vid_target = s - kluge;
            strcpy(s,macxx_target);
            while (*s++);
            vldaid->vid_time = s - kluge;
            strcpy(s,ascii_date);
            while (*s++);
#ifndef VMS
            rsize = s-kluge;
            fwrite(&rsize,sizeof(short),1,fp);
            evenodd = rsize&1;
#else
            evenodd = 0;
#endif
            fwrite(oline,(int)(s-oline)+evenodd,1,fp);
            return;
        }
    }
}

static int tstexpcommon(char *asc, int alen, EXP_stk *eps, char *olstr, int vlda_cmd)
{
    char *s;   
#ifndef VMS
    int rsize;
#endif
    switch (output_mode)
    {
    case OUTPUT_HEX:
    case OUTPUT_VLDA:
        break;
    case OUTPUT_OBJ: {
            int evenodd;
            VLDA_test *vt;
            flushobj();
            vt = (VLDA_test *)eline;
            vt->vtest_rectyp = vlda_cmd;
            vt->vtest_eoff = sizeof(VLDA_test);
            s = eline+vt->vtest_eoff;
            s = outexp(eps,s,(char *)0,(FILE *)0);
            vt->vtest_soff = s-(char *)vt;
            strncpy(s,asc,alen);
            s += alen;
            *s++ = 0;
#ifndef VMS
            rsize = s-(char *)vt;
            fwrite(&rsize,sizeof(short),1,outxabs_fp);
            evenodd = rsize&1;
#else
            evenodd = 0;
#endif
            fwrite(eline,(int)(s-eline)+evenodd,1,outxabs_fp); /* write it */
            break;
        }
    case OUTPUT_OL: {
            strcpy(eline, olstr);
            strncat(eline, asc, alen);
            strcat(eline, "\"");
            s = outexp(eps,eline+strlen(eline),(char *)0,(FILE *)0);
            *s++ = '\n';
            *s = 0;
            fputs(eline,outxabs_fp);
            break;
        }
    }
    return 1;
}

int outtstexp(char *asc, int alen, EXP_stk *eps)
{
    return tstexpcommon(asc, alen, eps, ".test \"", VLDA_TEST);
}

int outboffexp(char *asc, int alen, EXP_stk *eps)
{
    return tstexpcommon(asc, alen, eps, ".bofftest \"", VLDA_BOFF);
}

int outoorexp(char *asc, int alen, EXP_stk *eps)
{
    return tstexpcommon(asc, alen, eps, ".oortest \"", VLDA_OOR);
}

int out_dbgfname(FILE *fp, FN_struct *odfp)
{
    if (output_mode == OUTPUT_OL)
    {
        fprintf(fp,".dbgod \"%s\" %s\n",
                odfp->fn_buff,odfp->fn_version);
    }
    else if (output_mode == OUTPUT_OBJ)
    {
        int evenodd;
        VLDA_dbgdfile *vp;
        char *s;
        vp = (VLDA_dbgdfile *)eline;
        vp->type = VLDA_DBGDFILE;
        vp->name = sizeof(VLDA_dbgdfile);
        s = eline+sizeof(VLDA_dbgdfile);
        strcpy(s,odfp->fn_buff);
        s += strlen(s);
        ++s;
        vp->version = s-(char *)vp;
        strcpy(s,odfp->fn_version);
        s += strlen(s);
        rsize = s-(char *)vp+1;
#ifndef VMS
        fwrite(&rsize,sizeof(short),1,fp);
        evenodd = rsize&1;
#else
        evenodd = 0;
#endif
        fwrite(eline,(int)rsize+evenodd,1,fp); /* write it */
    }
    return 0;
}
