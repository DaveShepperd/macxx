/*
    endlin.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "outx.h"
#include "listctrl.h"
#include "memmgt.h"
#include <stdlib.h>         /* Get proto for abort() */

#if !defined(M_UNIX)
    #define MISER (options[QUAL_MISER]) 
    #define NOT_MISER (!options[QUAL_MISER]) 
#else
    #define MISER (0) 
    #define NOT_MISER (1) 
#endif

static char out_buf[72];    /* place to hold tmp data */
SEG_struct *out_seg;    /* segment in which data is placed */
long out_pc;            /* expected PC of data */
static char *out_indx = out_buf; /* pointer to next spot in out_buf */
static int out_remaining;   /* # of bytes remaining in buffer */
static short tmp_length;
static TMP_struct rtmp;
TMP_struct *tmp_next=0,*tmp_top=0,*tmp_pool=0,*tmp_ptr=0;
static int tmp_pool_size;
long tmp_pool_used;
EXPR_struct tmp_org;
EXP_stk tmp_org_exp;

#if defined(DEBUG)
static struct
{
    int typ;
    char *name;
} tmp_types[] = {
    {TMP_NUM,"TMP_NUM"},
    {TMP_NNUM,"TMP_NNUM"},
    {TMP_SIZE,"TMP_SIZE"},
    {TMP_ZERO,"TMP_ZERO"},
    {TMP_B8,"TMP_B8"},
    {TMP_B16,"TMP_B16"},
    {TMP_B32,"TMP_B32"},
    {TMP_EOF,"TMP_EOF"},
    {TMP_EXPR,"TMP_EXPR"},
    {TMP_ASTNG,"TMP_ASTNG"},
    {TMP_TEST,"TMP_TEST"},
    {TMP_BOFF,"TMP_BOFF"},
    {TMP_OOR,"TMP_OOR"},
    {TMP_FILE,"TMP_FILE"},
    {TMP_ORG,"TMP_ORG"},
    {TMP_START,"TMP_START"},
    {TMP_BSTNG,"TMP_BSTNG"},
    {TMP_SCOPE,"TMP_SCOPE"},
    {TMP_STAB,"TMP_STAB"},
    {TMP_LINK,"TMP_LINK"},
    {EXPR_RELMD,"EXPR_RELMD"},
    {EXPR_SYM,"EXPR_SYM"},
    {EXPR_VALUE,"EXPR_VALUE"},
    {EXPR_OPER,"EXPR_OPER"},
    {EXPR_L,"EXPR_L"},
    {EXPR_B,"EXPR_B"},
    {EXPR_TAG,"EXPR_TAG"},
    {EXPR_IDENT,"EXPR_IDENT"},
    {EXPR_SEG,"EXPR_SEG"},
    {EXPR_LINK,"EXPR_LINK"},
    {0,0}};

static char *sho_type(int typ)
{
    int i;
    for (i=0;tmp_types[i].name;++i)
    {
        if (tmp_types[i].typ == typ) return tmp_types[i].name;
    }
    return "undefined";
}
#endif

static char *sqz_it( char *src, int typ, int cnt, int siz)
{
#if !defined(M_UNIX)
    union
    {
        char *b8;
        short *b16;
        long *b32;
        FN_struct **fnp;
        long l;
    } sqz;
    register long value;
    register int itz;

#ifdef DEBUG
    fprintf(stderr,"SQZ'ing typ %s(0x%0X) having %d items with a size of %d from %08X\n",
            sho_type(typ),typ,cnt,siz,src);
#endif
    sqz.b8 = ((char *)tmp_pool)+1;   /* leave room for type code */
    ADJ_ALIGN(sqz.b8)
    *sqz.b16++ = current_fnd->fn_line;   /* insert the current line number */
    switch (typ)
    {           /* and test for type */ 
    case TMP_FILE: {
            *sqz.fnp++ = *(FN_struct **)src;
            break;
        }
    case TMP_SCOPE: {
            *sqz.b16++ = *src;
            break;
        }
    case TMP_EOF: break;      /* this is easy */
    case TMP_ASTNG:           /* these are strings */
    case TMP_BSTNG: {
            itz = cnt*siz;
            if ((value = itz) != 0)
            {
                if (itz < 0) value = -itz;
                if (value < 128)
                {
                    *sqz.b8++ = value;
                    typ |= TMP_B8;
                }
                else
                {
                    ADJ_ALIGN(sqz.b8)
                    *sqz.b16++ = value;
                    typ |= TMP_B16;
                }
            }
            memcpy(sqz.b8,src,itz);    /* move the items */
            sqz.b8 += itz;     /* adjust the pointer */
            break;         /* done */
        }
    case TMP_EXPR:
    case TMP_ORG:
    case TMP_TEST:
    case TMP_BOFF:
    case TMP_OOR:
    case TMP_START: {
            register EXPR_struct *texp;
            register EXP_stk *eps;
            eps = (EXP_stk *)src;
            *sqz.b8++ = eps->ptr;  /* stack pointer */
            *sqz.b8++ = (eps->base_page_reference<<2) |
                        (eps->forward_reference<1) |
                        (eps->register_reference);
            ADJ_ALIGN(sqz.b8)
            *sqz.b16++ = eps->tag;
            if (eps->tag != 0) *sqz.b16++ = eps->tag_len;
            texp = eps->stack;
            while (cnt--)
            {    /* do all the elements */
#if defined(DEBUG)
                fprintf(stderr,"\tsqzing item %s(0x%0X) at %08lX\n",sho_type(texp->expr_code),texp->expr_code,sqz.b8);
#endif
                switch (texp->expr_code)
                {
                case EXPR_OPER: {
                        *sqz.b8++ = EXPR_OPER | TMP_NNUM;
                        *sqz.b8++ = (char)texp->expr_value;
                        if ((texp->expr_value&255) == EXPROPER_TST)
                        {
                            *sqz.b8++ = texp->expr_value >> 8;
                        }
                        break;
                    }
                case EXPR_VALUE: {
                        unsigned long val;
                        if (texp->expr_value >= 0)
                        {
                            val = texp->expr_value;
                        }
                        else
                        {
                            val = -texp->expr_value;
                        }
                        if (val < 64l)
                        {
                            *sqz.b8++ = (char)(texp->expr_value & TMP_NUM);
                            break;
                        }
                        if (val < 128l)
                        {
                            *sqz.b8++ = EXPR_VALUE|TMP_B8|TMP_NNUM;
                            *sqz.b8++ = (char)texp->expr_value;
                            break;
                        }
                        if (val < 32768l)
                        {
                            *sqz.b8++ = EXPR_VALUE|TMP_B16|TMP_NNUM;
                            ADJ_ALIGN(sqz.b8)
                            *sqz.b16++ = (short)texp->expr_value;
                            break;
                        }
                        *sqz.b8++ = EXPR_VALUE|TMP_B32|TMP_NNUM;
                        ADJ_ALIGN(sqz.b8)
                        *sqz.b32++ = texp->expr_value;
                        break;
                    }
                case EXPR_IDENT: {
                        *sqz.b8++ = texp->expr_code|TMP_NNUM|TMP_B32;
                        ADJ_ALIGN(sqz.b8)
                        *sqz.b32++ = texp->expr_value;
                        break;
                    }
                case EXPR_LINK: {
                        *sqz.b8++ = texp->expr_code|TMP_NNUM|TMP_B32;
                        ADJ_ALIGN(sqz.b8)
                        *sqz.b32++ = (unsigned long)texp->expr_seg;
                        break;
                    }
                case EXPR_L:
                case EXPR_B:
                case EXPR_SEG:
                case EXPR_SYM: {
                        int t = TMP_B32|TMP_NNUM;
                        if (texp->expr_expr != (EXP_stk *)0)
                        {
                            t = TMP_B32|TMP_NNUM|EXPR_RELMD;
                        }
                        t |= texp->expr_code;
                        *sqz.b8++ = t;
                        ADJ_ALIGN(sqz.b8)
                        *sqz.b32++ = texp->expr_value;
                        if ((t & EXPR_RELMD) != 0)
                        {
                            ADJ_ALIGN(sqz.b8)
                            *sqz.b32++ = texp->expr_count;
                        }     /* -- RELMD */
                    }        /* -- case */
                default: break;
                }           /* -- switch expr_code */
                ++texp;
            }          /* -- while expr */
        }             /* -- case TMP_EXPR */
    }                /* -- switch TMP_TYPE */
    tmp_pool->tf_type = typ; /* stuff in the typ code */
#ifdef DEBUG
    fprintf(stderr,"\tSquoze from %08X to %08X\n",tmp_pool,sqz.b8);
#endif
    return(sqz.b8);
#else
    fprintf(stderr,"\tSqzit not configured\n");
    abort();
    return 0;
#endif
}

static char *unsqz_it( char *src )
{
#if !defined(M_UNIX)
    union
    {
        char *b8;
        short *b16;
        long *b32;
        unsigned char *u8;
        unsigned short *u16;
        unsigned long *u32;
        TMP_struct *tsp;
        SS_struct **symp;
        FN_struct **fnp;
        long l;
    } sqz;
    int typ,i;

#ifdef DEBUG
    fprintf(stderr,"UNSQZ'ing from %08X\n",src);
#endif
    tmp_ptr = &rtmp;         /* point to local tmp_struct */
    rtmp.tf_length = 0;          /* assume a length of 0 */
    sqz.b8 = src;            /* setup the squeezer */
    rtmp.tf_type = (typ = *sqz.b8++) & ~TMP_SIZE;
    ADJ_ALIGN(sqz.b8)
    rtmp.line_no = *sqz.b16++;       /* pickup the line number */
#if defined(DEBUG)
    fprintf(stderr,"\ttype %s(0x%0X) from line %d\n",sho_type(rtmp.tf_type),rtmp.tf_type,rtmp.line_no);
#endif
    switch (rtmp.tf_type)
    {      /* and test for type */ 
    default: {
            sprintf(emsg,"Illegal 'sqz' code of %02X",rtmp.tf_type);
            err_msg(MSG_FATAL,emsg);
            EXIT_FALSE;
        }
    case TMP_FILE: {          /* change of file */
            current_fnd = *sqz.fnp++;
            break;
        }
    case TMP_SCOPE: {         /* change of scope */
            current_scope = *sqz.b16++;
            current_procblk = current_scope&SCOPE_PROC;
            current_scopblk = current_scope&~SCOPE_PROC;
            break;
        }
    case TMP_EOF: {
#ifdef DEBUG
            fprintf(stderr,"\tUNSQUOZE eof message at location %08X\n",
                    src);
#endif
            break;
        }
    case TMP_ASTNG:           /* these are strings */
    case TMP_BSTNG: {
            typ &= TMP_SIZE;
            switch (typ)
            {
            case TMP_ZERO: break;
            case TMP_B8: {
                    rtmp.tf_length = *sqz.b8++;
                    break;
                }
            case TMP_B16: {
                    ADJ_ALIGN(sqz.b8)
                    rtmp.tf_length = *sqz.b16++;
                    break;
                }
            case TMP_B32: {
                    ADJ_ALIGN(sqz.b8)
                    rtmp.tf_length = *sqz.b32++;
                    break;
                }
            }
            tmp_pool = sqz.tsp;
#ifdef DEBUG
            fprintf(stderr,"\tUNSQUOZE %d byte string. type = %0X, from %08X to %08X\n",
                    rtmp.tf_length,rtmp.tf_type,src,sqz.b8+rtmp.tf_length);
#endif
            sqz.b8 += rtmp.tf_length;
            break;
        }
    case TMP_EXPR:
    case TMP_ORG:
    case TMP_BOFF:
    case TMP_OOR:
    case TMP_TEST:
    case TMP_START: {
            register EXPR_struct *texp;
            int cnt;
            EXP0.ptr = *sqz.u8++;
            cnt = *sqz.b8++;
            EXP0.base_page_reference = (cnt>>2) & 1;
            EXP0.forward_reference = (cnt>1) & 1;
            EXP0.register_reference = cnt &1;
            ADJ_ALIGN(sqz.b8)
            if ((EXP0.tag = *sqz.u16++) != 0) EXP0.tag_len = *sqz.u16++;
            texp = EXP0.stack;     /* point to expression stack 0 */
            cnt = EXP0.ptr;
#if defined(DEBUG)
            fprintf(stderr,"\tunsqzing %d items into exprs stack from %08lX\n",cnt,sqz.b8);
#endif
            for (;cnt;--cnt,++texp)
            {  /* do all the elements */
                int relmod;
                i = *sqz.b8++ & 0xFF;   /* pickup the type code */
#if defined(DEBUG)
                fprintf(stderr,"\t\tunsqzing item %d: %s(0x%0X) at %08lX\n",cnt,sho_type(i),i,sqz.b8);
#endif
                if ((i & TMP_NNUM) == 0)
                {
                    texp->expr_code = EXPR_VALUE;    /* type is abs value */
                    if (i & (TMP_NNUM/2)) i |= -TMP_NNUM;    /* sign extend */
                    texp->expr_value = i;
                    continue;
                }
                texp->expr_code = i & ~(TMP_SIZE|EXPR_RELMD|TMP_NNUM);
                texp->expr_seg = (SEG_struct *)0;
                relmod = i & EXPR_RELMD;
                i &= TMP_SIZE;
                switch (texp->expr_code)
                {
                case EXPR_OPER: {
                        texp->expr_value = *sqz.b8++; /* get the operator character */
                        if ((texp->expr_value&255) == EXPROPER_TST)
                        {
                            texp->expr_value |= *sqz.b8++ << 8;
                        }
                        continue;
                    }
                case EXPR_IDENT:
                case EXPR_VALUE: {
                        switch (i)
                        {
                        case TMP_ZERO: {
                                texp->expr_value = 0;
                                continue;
                            }
                        case TMP_B8: {
                                texp->expr_value = *sqz.b8++;
                                continue;
                            }
                        case TMP_B16: {
                                ADJ_ALIGN(sqz.b8)
                                texp->expr_value = *sqz.b16++;
                                continue;
                            }
                        case TMP_B32: {
                                ADJ_ALIGN(sqz.b8)
                                texp->expr_value = *sqz.b32++;
                                continue;
                            }
                        }
                    }
                case EXPR_L:
                case EXPR_B:
                case EXPR_SEG:
                case EXPR_SYM: {
                        ADJ_ALIGN(sqz.b8)
                        texp->expr_value = *sqz.b32++;
                        if (relmod)
                        {
                            ADJ_ALIGN(sqz.b8)
                            texp->expr_sym = *sqz.symp++;
                        }
                        continue;
                    }        /* -- case */
                case EXPR_LINK: {
                        ADJ_ALIGN(sqz.b8)
                        texp->expr_expr = (EXP_stk *)(*sqz.b32++);
                        texp->expr_value = 0;
                        continue;
                    }        /* -- case */
                default: continue; /* this keeps gcc happy */
                }           /* -- switch expr_code */
            }          /* -- for() expr */
        }             /* -- case TMP_EXPR */
    }                /* -- switch TMP_TYPE */
#ifdef DEBUG
    fprintf(stderr,"\tUNSQUOZE %d items into exprs_stack[0] from %08X to %08X\n",
            exprs_stack[0].ptr,src,sqz.b8);
#endif
    current_fnd->fn_line = rtmp.line_no;
    return(sqz.b8);
#else
    fprintf(stderr,"\nUnsqzit not configured\n");
    abort();
    return 0;
#endif
}

/********************************************************************
 * Write a bunch of data to the temp file
 */
void write_to_tmp(int typ, int itm_cnt, void *itm_ptr, int itm_siz)
/*
 * At entry:
 *	typ - TMP_xxx value id'ing the block data
 *	itm_cnt - number of items to write
 *	itm_ptr - pointer to items to write (or tag character)
 *	itm_siz - size in bytes of each item (or tag number)
 * At exit:
 *	data written to temp file (exits to VMS if error)
 */
{
    union
    {
        void *v;
        char *c;
        long *l;
        TMP_struct *t;
        EXP_stk *txp;
        long lng;
    } src,dst,tmp;       /* mem pointers */
    int itz;
    int class = 0;

    if (!output_files[OUT_FN_OBJ].fn_present) return; /* nuthin' to do if no output */
    if (tmp_pool_size == 0)
    {
        tmp_pool_size = max_token*8;      /* get some memory */
        tmp_pool_used += max_token*8;
        tmp_pool = (TMP_struct *)MEM_alloc(tmp_pool_size);
        tmp_top = tmp_pool;       /* remember where the top starts */
    }
    src.v = itm_ptr;
    tmp.t = tmp_pool;
    ADJ_ALIGN(tmp.c)
    if (typ == TMP_EXPR || typ == TMP_START || typ == TMP_ORG ||
        typ == TMP_TEST || typ == TMP_BOFF || typ == TMP_OOR) class = 1;
        if (class != 0)
        {
            itm_cnt = src.txp->ptr;
            itm_siz = sizeof(EXPR_struct);
            itz = itm_cnt*sizeof(EXPR_struct) + sizeof(EXP_stk);
        }
        else
        {
            itz = itm_cnt*itm_siz;
        }
    if (tmp_fp == 0 )
    {
        int tsiz;
        tsiz = 2*sizeof(TMP_struct) + itz;
        if (tmp_pool_size < tsiz)
        {
            if (tsiz < max_token*8) tsiz = max_token*8;
            tmp.t->tf_type = TMP_LINK; /* link to a new area */
            tmp.t->tf_link = (TMP_struct *)MEM_alloc(tsiz);
            tmp_pool_used += tsiz;
            tmp_pool_size = tsiz;
            tmp_pool = tmp.t->tf_link;
            tmp.t = tmp_pool;
        }
        if NOT_MISER
        {
            dst.t = tmp.t+1;
            tmp.t->tf_type = typ;      /* set the type */
            tmp.t->line_no = current_fnd->fn_line;
            if (class != 0)
            {
                tmp.t->tf_length = itz+sizeof(TMP_struct);
                dst.txp->ptr = src.txp->ptr;
                dst.txp->tag = src.txp->tag;
                dst.txp->tag_len = src.txp->tag_len;
                dst.txp->base_page_reference = src.txp->base_page_reference;
                dst.txp->forward_reference = src.txp->forward_reference;
                dst.txp->register_reference = src.txp->register_reference;
                dst.txp->stack = (EXPR_struct *)(dst.txp+1);
                ++dst.txp;
                src.c = (char *)src.txp->stack;
                itz -= sizeof(EXP_stk);
            }
            else
            {
                tmp.t->tf_length = itm_cnt; /* set the item count */
            }
            if (itz != 0) memcpy(dst.c,src.c,itz);
            dst.c += itz;
            ADJ_ALIGN(dst.c)
        }
        else
        {
            dst.c = sqz_it((char *)itm_ptr,typ,itm_cnt,itm_siz);
            ADJ_ALIGN(dst.c)
        }
        tmp_pool_size -= dst.c - tmp.c;
        tmp_pool = dst.t;             /* update pointer */
    }
    else
    {
        int t;
        dst.c = sqz_it((char *)itm_ptr,typ,itm_cnt,itm_siz);
        tmp_length = dst.c - (char *)tmp_top;
        t = fwrite(&tmp_length,sizeof(tmp_length),1,tmp_fp);
        if (t != 1)
        {
            sprintf(emsg,"%%%s-F-FATAL, Tried to fwrite %d bytes (1 elem) to tmp_file, wrote %d.\n\t",
                    macxx_name,sizeof(tmp_length),t*sizeof(tmp_length));
            perror(emsg);
            EXIT_FALSE;
        }
        t = fwrite(tmp_top,(int)tmp_length,1,tmp_fp);
        if (t != 1)
        {
            sprintf(emsg,"%%%s-F-FATAL, Tried to fwrite %d bytes (1 elem) to tmp_file, wrote %d.\n\t",
                    macxx_name,tmp_length,t*tmp_length);
            perror(emsg);
            EXIT_FALSE;
        }
    }
    return;
}

/**********************************************************************
 * Read a bunch of data from tmp file
 */
int read_from_tmp( void )
/*
 * At entry:
 * At exit:
 */
{
    TMP_struct *ts;
    char *tmps;
    int code;

    if (tmp_fp != 0)
    {
        int t;
        t = fread(&tmp_length,sizeof(tmp_length),1,tmp_fp);
        if (t != 1)
        {
            sprintf(emsg,"%%%s-F-FATAL, Tried to fread %d bytes (1 elem) from tmp_file, actually read %d.\n\t",
                    macxx_name,sizeof(tmp_length),t*sizeof(tmp_length));
            perror(emsg);
            EXIT_FALSE;
        }
        t = fread(tmp_top,(int)tmp_length,1,tmp_fp);
        tmp_next = (TMP_struct *)unsqz_it( (char *)tmp_top ); /* unpack the text */
        return( rtmp.tf_type );
    }
    else
    {
#if ALIGNMENT
        tmps = (char *)tmp_next;
        ADJ_ALIGN(tmps)
        ts = (TMP_struct *)tmps;
#else
        ts = tmp_next;  /* point to next tmp element */
#endif
        if (ts->tf_type == TMP_LINK)
        {
            int ferr;
            ts = tmp_next = (TMP_struct *)ts->tf_length;
            ferr = MEM_free((char *)tmp_top);
#if !defined(SUN)
            if (ferr)
            {    /* give back the memory */
                sprintf(emsg,"Error (%08X) free'ing %d bytes at %08lX from tmp_pool",
                        ferr, max_token*8, (long)tmp_top);
                err_msg(MSG_WARN,emsg);
            }
#endif
            tmp_top = ts;          /* point to next top */
        }
        tmp_ptr = ts;             /* point to tmp pointer */
        if MISER
        {
#if ALIGNMENT
            tmps = unsqz_it(tmp_ptr);  /* unpack the text */
            ADJ_ALIGN(tmps)
            tmp_next = (TMP_struct *)tmps;
#else
            tmp_next = (TMP_struct *)unsqz_it( (char*)tmp_ptr); /* unpack the text */
#endif
            return(rtmp.tf_type);
        }
        else
        {
            ++ts;
            tmps = (char *)ts;
            tmp_pool = ts;
            code = tmp_ptr->tf_type;
            switch (code)
            {
            case TMP_FILE: {
                    union
                    {
                        unsigned long lng;
                        char *cp;
                        FN_struct **fnp;
                        TMP_struct *tp;
                    } src;
                    src.tp = tmp_ptr+1;
                    current_fnd = *src.fnp++;
                    tmp_next = src.tp;
                    break;
                }
            case TMP_SCOPE: {
                    union
                    {
                        unsigned short *scope;
                        TMP_struct *tp;
                    } src;
                    src.tp = tmp_ptr+1;
                    current_scope = *src.scope++;
                    current_procblk = current_scope&SCOPE_PROC;
                    current_scopblk = current_scope&~SCOPE_PROC;
                    tmp_next = src.tp;
                    break;
                }
            case TMP_START:
            case TMP_EXPR:
            case TMP_BOFF:
            case TMP_OOR:
            case TMP_TEST:
            case TMP_ORG: {
                    union
                    {
                        EXP_stk *eps;
                        TMP_struct *tp;
                        unsigned char *cp;
                        unsigned long lng;
                    } src;
                    int l;
                    src.tp = tmp_ptr+1;
                    EXP0.ptr = src.eps->ptr;
                    EXP0.tag = src.eps->tag;
                    EXP0.tag_len = src.eps->tag_len;
                    EXP0.register_reference = src.eps->register_reference;
                    EXP0.base_page_reference = src.eps->base_page_reference;
                    EXP0.forward_reference = src.eps->forward_reference;
                    l = src.eps->ptr*sizeof(EXPR_struct);
                    memcpy(EXP0.stack,src.eps->stack,l);
                    src.eps += 1;
                    src.cp += l;
                    ADJ_ALIGN(src.cp)
                    tmp_next = src.tp;
                    break;
                }
            case TMP_BSTNG:
            case TMP_ASTNG: {
                    tmps += tmp_ptr->tf_length;
                    ADJ_ALIGN(tmps)
                    tmp_next = (TMP_struct *)tmps;
                    break;
                }               /* -- case */
            }              /* -- switch */
            current_fnd->fn_line = tmp_ptr->line_no;
        }                 /* -- if miser */
    }                    /* -- if fp */
    return(code);
}

void rewind_tmp()
{
    if (tmp_fp)
    {
#ifdef VMS
        if (fseek(tmp_fp,0,0))
        {   /* rewind temp file */
            perror("Unable to rewind tmp file");
            EXIT_FALSE;
        }
#else
        fclose(tmp_fp);       /* close the temp file */
        if ((tmp_fp = fopen(output_files[OUT_FN_TMP].fn_buff,"rb")) == 0)
        {
            sprintf(emsg,"%%%s-F-FATAL, Unable to open %s for input\n\t",
                    macxx_name,output_files[OUT_FN_TMP].fn_buff);
            perror (emsg);
            EXIT_FALSE;
        }
#endif
    }
    else
    {
        tmp_next = tmp_top;
    }
    return;
}

/**********************************************************************
 * Display truncation error
 */
void trunc_err( long mask, long tv)
/* 
 * At entry:
 *	mask - mask to compare against
 *	tv - requested value to write
 *	current_pc - current location counter
 */
{
    char *s,*terr;
    int tsiz,sev;
    long toofar=0;

    if (options[QUAL_OCTAL])
    {
        if (mask > 255)
        {
            s = "Truncation 0%06lo bytes offset from segment {%s}.\n\tDesired: 0%010lo, output a: 0%06lo";
        }
        else
        {
            s = "Truncation 0%06lo bytes offset from segment {%s}.\n\tDesired: 0%010lo, output a: 0%03lo";
        }
    }
    else
    {
        if (mask > 255 && mask != 65534)
        {
            s = "Truncation 0x%04lX bytes offset from segment {%s}.\n\tDesired: 0x%08lX, output a: 0x%04lX";
        }
        else
        {
            int range = 127;
            if (mask == 254 || mask == 65534)
            {
                toofar = tv;
                if (toofar == 0)
                {
                    s = "Branch offset of 0x%02lX is illegal at 0x%04lX bytes from segment {%s}";
                }
                else
                {
                    if (mask == 65534) range = 65535;
                    if (toofar > 0)
                    {
                        toofar -= range;
                    }
                    else
                    {
                        toofar = -toofar-(range+1);
                    }
                    s = "Branch offset 0x%02lX byte(s) out of range at 0x%04lX bytes from segment {%s}";
                }
            }
            else
            {
                s = "Truncation 0x%04lX bytes offset from segment {%s}\n\tDesired: 0x%08lX, written: 0x%02lX";
            }      
        }
    }
    if (pass > 1)
    {
        char *fm,*em="\n\tAt line %d in file %s";
        fm = MEM_alloc(strlen(s)+strlen(em)+1);
        strcpy(fm,s);
        strcat(fm,em);
        s = fm;
    }
    tsiz = strlen(s)+strlen(current_section->seg_string)+40;
    if (pass > 1) tsiz += strlen(current_fnd->fn_name_only);
    terr = MEM_alloc(tsiz);
    if (mask == 254 || mask == 65534)
    {
        if (pass > 1)
        {
            sprintf(terr,s,toofar,current_offset,current_section->seg_string,current_fnd->fn_line,current_fnd->fn_name_only);
        }
        else
        {
            sprintf(terr,s,toofar,current_offset,current_section->seg_string);
        }
        sev = MSG_ERROR;      /* branch fail is ERROR */
    }
    else
    {
        if (pass > 1)
        {
            sprintf(terr,s,current_offset,current_section->seg_string,tv,tv&mask,current_fnd->fn_line,current_fnd->fn_name_only);
        }
        else
        {
            sprintf(terr,s,current_offset,current_section->seg_string,tv,tv&mask);
        }
        sev = MSG_WARN;       /* others are light weight errors */
    }
    if (pass > 1)
    {
        err_msg(sev,terr);
    }
    else
    {
        show_bad_token((char *)0,terr,sev);
    }
    MEM_free(terr);
    return;
}

#define FLUSH_OUTBUF if (out_indx - out_buf != 0) {\
      int dbg;\
      dbg = options[QUAL_DEBUG];\
      options[QUAL_DEBUG] = 0;\
      write_to_tmp(TMP_BSTNG,(int)(out_indx-out_buf),out_buf,1);\
      options[QUAL_DEBUG] = dbg;\
      out_indx = out_buf;\
      out_remaining = sizeof(out_buf);\
      }

void move_pc( void )
{
    FLUSH_OUTBUF;
#if defined(PC_DEBUG)
    fprintf(stderr,"ENDLIN: Moving PC from \"%s\"+%08lX to \"%s\"+%08lX\n",
            out_seg?out_seg->seg_string:"",out_pc,current_section->seg_string,current_pc);
#endif
    tmp_org.expr_value = current_pc;
    tmp_org.expr_seg = current_section;
    write_to_tmp(TMP_ORG,0,&tmp_org_exp,0);
    out_seg = current_section;
    return;
}
/*****************************************************************************
* End of source line processing.
*/
int endlin( void )
/*
 * At entry:
 *
 * At exit:
 *	returns true;
 */
{
    FLUSH_OUTBUF;
    out_pc = current_offset;
    return 1;
}   

int p1o_any( EXP_stk *eps )
{
    switch (eps->tag)
    {
    case 'b':
    case 'c':
    case 's':
    case 'z':
    case 'Z':
        return(p1o_byte(eps));
    case 'w':
    case 'W':
    case 'i':
    case 'I':
    case 'u':
    case 'U':
    case 'n':
    case 'N':
    case 'y':
    case 'Y':
        return(p1o_word(eps));
    case 'l':
    case 'L':
    case 'j':
    case 'J':
        return(p1o_long(eps));
    case 'X':
    case 'x':
        return(p1o_var(eps));
    }
    return 1;
}

int p1o_byte( EXP_stk *eps )
{
    register long tv;
    EXPR_struct *exp_ptr;
    int j,pflg;

    if (out_seg != current_section || out_pc != current_offset)
    {
        move_pc();
    }
    j = eps->ptr;
    if ( j <= 0) return j;
    exp_ptr = eps->stack;
    if ( j == 1 && exp_ptr->expr_code == EXPR_VALUE && eps->tag_len <= 1)
    {
        tv = exp_ptr->expr_value;
        if ((tv > 255) || (tv < -128)) trunc_err(255L,tv);
        if (out_remaining <= 0) FLUSH_OUTBUF;
        --out_remaining;
        *out_indx++ = (unsigned char)tv;
        pflg = 0;
    }
    else
    {         /* -+if 1 absolute term */
        if (j == 1 && exp_ptr->expr_code == EXPR_SEG)
        {
            pflg = '\'';
        }
        else
        {
            pflg = 'x';
        }
        FLUSH_OUTBUF;
        write_to_tmp(TMP_EXPR,0,eps,0);
        tv = eps->psuedo_value;      
    }                /* --if 1 absolute term */
    if (list_bin) n_to_list(macxx_nibbles_byte,tv,pflg);
    if (eps->tag_len <= 1)
    {
        current_offset += macxx_mau_byte; /* update current location */
    }
    else
    {
        current_offset += macxx_mau_byte*eps->tag_len;    /* update current location */
    }
    out_pc = current_offset; /* what to expect next time */
#if defined(PC_DEBUG)
    fprintf(stderr,"ENDLIN: Just output %d bytes. Current offset = %08lX\n",
            eps->tag_len,current_offset);
#endif
#ifndef MAC_PP
    meb_stats.expected_pc = out_pc;
#endif
    ++binary_output_flag;
    return 0;
}               /* --for all expressions */

int p1o_word( EXP_stk *eps )
{
    register long tv,lv;
    EXPR_struct *exp_ptr;
    int j,tag,swapem,pflg;

    if (out_seg != current_section || out_pc != current_offset)
    {
        move_pc();
    }
    j = eps->ptr;
    if ( j <= 0) return 0;
    tag = eps->tag;
    exp_ptr = eps->stack;
    swapem = tag == 'W' || tag == 'I' || tag == 'U';
    if ( j == 1 && eps->stack->expr_code == EXPR_VALUE && eps->tag_len <= 1)
    {
        tv = exp_ptr->expr_value;
        if ((tv > 65535) || (tv < -65536)) trunc_err(65535L,tv);
        if (swapem)
        {
            tv = ((tv&255)<<8) | ((tv>>8)&255);
        }
        if (out_remaining < 2) FLUSH_OUTBUF;
        out_remaining -= 2;
        *out_indx++ = (char)tv;
        *out_indx++ = (char)(tv>>8);
        pflg = 0;
    }
    else
    {         /* -+if 1 absolute term */
        FLUSH_OUTBUF;
        write_to_tmp(TMP_EXPR,0,eps,0);
        if (j == 1 && exp_ptr->expr_code == EXPR_SEG)
        {
            pflg = '\'';
        }
        else
        {
            pflg = 'x';
        }
    }                /* --if 1 absolute term */
    if (list_bin)
    {
        lv = eps->psuedo_value;
        if (list_cod && swapem == 0)
        {
            lv = ((lv&255)<<8) | ((lv>>8)&255);
        }
        n_to_list(macxx_nibbles_word,lv,pflg);
    }
    if (eps->tag_len <= 1)
    {
        current_offset += macxx_mau_word; /* update current location */
    }
    else
    {
        current_offset += macxx_mau_word*eps->tag_len;    /* update current location */
    }
    out_pc = current_offset;
#if defined(PC_DEBUG)
    fprintf(stderr,"ENDLIN: Just output %d words. Current offset = %08lX\n",
            eps->tag_len,current_offset);
#endif
#ifndef MAC_PP
    meb_stats.expected_pc = out_pc;
#endif
    ++binary_output_flag;
    return 1;
}   

int p1o_long( EXP_stk *eps )
{
    register long tv,lv;
    EXPR_struct *exp_ptr;
    int j,tag,pflg;

    if (out_seg != current_section || out_pc != current_offset)
    {
        move_pc();
    }
    j = eps->ptr;
    if ( j == 0) return 0;
    exp_ptr = eps->stack;
    tv = exp_ptr->expr_value;
    tag = eps->tag;
    if ( j == 1 && exp_ptr->expr_code == EXPR_VALUE && eps->tag_len <= 1)
    {
        unsigned char l0,l1,l2,l3;
        switch (tag)
        {
        default:
        case 'L':          /* big endian */
            l3 = (unsigned char)tv;
            l2 = (unsigned char)(tv>>8);
            l1 = (unsigned char)(tv>>16);
            l0 = (unsigned char)(tv>>24);
            break;
        case 'l':          /* little endian */
            l0 = (unsigned char)tv;
            l1 = (unsigned char)(tv>>8);
            l2 = (unsigned char)(tv>>16);
            l3 = (unsigned char)(tv>>24);
            break;
        case 'j':          /* high byte of low word first */
            l1 = (unsigned char)tv;
            l0 = (unsigned char)(tv>>8);
            l3 = (unsigned char)(tv>>16);
            l2 = (unsigned char)(tv>>24);
            break;
        case 'J':          /* low byte of high word first */
            l2 = (unsigned char)tv;
            l3 = (unsigned char)(tv>>8);
            l0 = (unsigned char)(tv>>16);
            l1 = (unsigned char)(tv>>24);
            break;
        }
        if (out_remaining < 4) FLUSH_OUTBUF;
        out_remaining -= 4;
        *out_indx++ = l0;
        *out_indx++ = l1;
        *out_indx++ = l2;
        *out_indx++ = l3;
        pflg = 0;
    }
    else
    {         /* -+if 1 absolute term */
        FLUSH_OUTBUF;
        write_to_tmp(TMP_EXPR,0,eps,0);
        if (j == 1 && exp_ptr->expr_code == EXPR_SEG)
        {
            pflg = '\'';
        }
        else
        {
            pflg = 'x';
        }
    }                /* --if 1 absolute term */
    if (list_bin)
    {
        lv = eps->psuedo_value;
        if (list_cod)
        {
            unsigned char l0,l1,l2,l3;
            l0 = (unsigned char)lv;
            l1 = (unsigned char)(lv>>8);
            l2 = (unsigned char)(lv>>16);
            l3 = (unsigned char)(lv>>24);
            switch (tag)
            {     /* adjust the display of the values */
            case 'l':       /* little endian bytes */
                lv = (l0<<24) | (l1<<16) | (l2<<8) | l3;
                break;
            case 'J':       /* display high byte of low word first */
                lv = (l2<<24) | (l3<<16) | (l0<<8) | l1;
                break;
            case 'j':       /* display low byte of high word first */
                lv = (l1<<24) | (l0<<16) | (l3<<8) | l2;
            case 'L':       /* big endian, don't change anything */
                break;
            }
        }
        n_to_list(macxx_nibbles_long,lv,pflg);
    }
    if (eps->tag_len <= 1)
    {
        current_offset += macxx_mau_long; /* update current location */
    }
    else
    {
        current_offset += macxx_mau_long*eps->tag_len;    /* update current location */
    }
    out_pc = current_offset;
#if defined(PC_DEBUG)
    fprintf(stderr,"ENDLIN: Just output %d longs. Current offset = %08lX\n",
            eps->tag_len,current_offset);
#endif
#ifndef MAC_PP
    meb_stats.expected_pc = out_pc;
#endif
    ++binary_output_flag;
    return 1;
}   

int p1o_var( EXP_stk *eps )
{
    register long tv,lv;
    EXPR_struct *exp_ptr;
    int j,tag,pflg,bytes;
    unsigned long bits;
    long uBits;
    
    if (out_seg != current_section || out_pc != current_offset)
    {
        move_pc();
    }
    j = eps->ptr;
    if ( j == 0) return 0;
    exp_ptr = eps->stack;
    tv = exp_ptr->expr_value;
    tag = eps->tag;
    bytes = ((int)eps->tag_len+macxx_mau-1)/macxx_mau;
    bits = (1<<eps->tag_len)-1;
    uBits = bits;
    if (bytes < 1 || bytes > 4)
    {
        sprintf(emsg,"Internal error - storing illegal bitfield of size %d",eps->tag_len);
        bad_token((char *)0,emsg);
        if (bytes < 1) bytes = 1;
        else if (bytes > 4) bytes = 4;
    }
    if ( j == 1 && exp_ptr->expr_code == EXPR_VALUE)
    {
#if 0
        printf("Checking for truncation. tv=0x%08lX, bits=0x%08lX, bits/2=0x%08lX, -(bits/2+1)=0x%08lX, tv >= %d, tv <= %d\n",
               tv, uBits, uBits/2, -(uBits/2+1),
               tv > uBits/2,
               tv < -(uBits/2+1));
#endif
        if ( tv > uBits/2 || tv < -(uBits/2+1) )
			trunc_err(uBits,tv);
        tv &= bits;
        if (out_remaining < bytes) FLUSH_OUTBUF;
        out_remaining -= bytes;
        if (tag == 'X')
        {
            for (j=eps->tag_len-macxx_mau;j>0;j -= macxx_mau) *out_indx++ = (unsigned char)(tv>>j);
            *out_indx++ = (unsigned char)tv;
        }
        else
        {
            for (j=0;j < (int)eps->tag_len;j += macxx_mau) *out_indx++ = (unsigned char)(tv>>j);
        }
        pflg = 0;
    }
    else
    {         /* -+if 1 absolute term */
        FLUSH_OUTBUF;
        write_to_tmp(TMP_EXPR,0,eps,0);
        if (j == 1 && exp_ptr->expr_code == EXPR_SEG)
        {
            pflg = '\'';
        }
        else
        {
            pflg = 'x';
        }
    }                /* --if 1 absolute term */
    if (list_bin)
    {
        if (list_cod && tag == 'x')
        {
            lv = 0;
            for (j=eps->tag_len-8;j>0;j -= 8) lv = (lv << 8) | ((tv>>j)&0xff);
            lv = (lv << 8) | (tv&0xff);
        }
        else
        {
            lv = eps->psuedo_value;
        }
        n_to_list(macxx_nibbles_byte*bytes,lv,pflg);
    }
    current_offset += macxx_mau_byte*bytes;  /* update current location */
    out_pc = current_offset;
#if defined(PC_DEBUG)
    fprintf(stderr,"ENDLIN: Just output %d bits. Current offset = %08lX\n",
            eps->tag_len,current_offset);
#endif
#ifndef MAC_PP
    meb_stats.expected_pc = out_pc;
#endif
    ++binary_output_flag;
    return 1;
}   
