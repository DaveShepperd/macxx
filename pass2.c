/*
    pass2.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "memmgt.h"

long xfer_addr;
static int noout_flag;
#if 0
extern char *outexp();
extern char *outxfer();
extern int endlin(),rewind_tmp(),read_from_tmp();
extern int flushobj();
extern int outorg(),outtstexp();
                    extern SEG_struct *get_absseg();
                    extern void outboffexp(char *, int, EXP_stk *);
                    extern void outoorexp(char *, int, EXP_stk *);

#endif

#define LONG_LBF   'l'
#define LONG_HBF   'L'
#define WORD_LBF  'w'
#define WORD_HBF  'W'
#define SIGN_LBF  'i'
#define SIGN_HBF  'I'
#define USIGN_LBF 'u'
#define USIGN_HBF 'U'
#define VAR_LBF	  'x'
#define VAR_HBF	  'X'

#if 0
    #define FLIP_SENSE 0
    #define LONG_LBF   'L'
    #define LONG_HBF   'l'
    #define WORD_LBF  'W'
    #define WORD_HBF  'w'
    #define SIGN_LBF  'I'
    #define SIGN_HBF  'i'
    #define USIGN_LBF 'U'
    #define USIGN_HBF 'u'
    #define VAR_LBF	  'X'
    #define VAR_HBF	  'x'
#endif

#define LONG_LWF  'j'
#define LONG_HWF  'J'

int out_compexp(unsigned char *value, long tv, int tag, int taglen) {
    int flip = 0;
    switch (tag)
    {
    case VAR_LBF:
    case VAR_HBF: {
            long mask;
            mask = (1<<taglen) -1; /* compute mask */   
            if (tv&~mask)
            {
                if ( (edmask&ED_TRUNC) )
					trunc_err(mask,tv);
                tv &= mask;
            }
            if (tag == VAR_HBF)
            {
                int i;
                for (i=taglen-macxx_mau;i>0;i -= macxx_mau) *value++ = tv>>i;
            }
            else
            {
                int i;
                for (i=0; i < (taglen+(macxx_mau-1))/macxx_mau; ++i, tv = tv >> 8) *value++ = tv;
            }
            return(taglen+(macxx_mau-1))/macxx_mau;
        }
    case LONG_LWF: {      /* long, high byte of low word first */
            *value++ = tv>>8;  /* bits 15-8 */
            *value++ = tv;
            *value++ = tv>>24;
            *value++ = tv>>16;
#if 0
            unsigned char b0,b1,b2,b3; /* bytes in long */
            b0 = tv&0xFF;          /* pickup individual bytes */
            b1 = (tv>>8)&0xFF;
            b2 = (tv>>16)&0xFF;
            b3 = (tv>>24)&0xFF;
            tv = (b2<<24) | (b3<<16) | (b0<<8) | (b1);
            *value = tv;
#endif
            return sizeof(long);
        }
    case LONG_HWF: {      /* long, low byte of high word first */
            *value++ = tv>>16;
            *value++ = tv>>24;
            *value++ = tv;
            *value++ = tv>>8;
#if 0
            b0 = tv&0xFF;  /* pickup individual bytes */
            b1 = (tv>>8)&0xFF;
            b2 = (tv>>16)&0xFF;
            b3 = (tv>>24)&0xFF;
            tv = (b1<<24) | (b0<<16) | (b3<<8) | (b2);
            *value = tv;
#endif
            return sizeof(long);
        }
    case LONG_HBF: {      /* long, high byte first */
            *value++ = tv>>24;
            *value++ = tv>>16;
            *value++ = tv>>8;
            *value++ = tv;
#if 0
            char *vp;
            char b0,b1,b2,b3;  /* bytes in long */
            vp = (char *)value;    /* point to area */
            b0 = *vp++;        /* pickup individual bytes */
            b1 = *vp++;
            b2 = *vp++;
            b3 = *vp;
            *vp = b0;      /* and swap them */
            *--vp = b1;
            *--vp = b2;
            *--vp = b3;
#endif
        }             /* fall thru to 'l' */
    case LONG_LBF: {      /* long, low byte first */
            *value++ = tv;
            *value++ = tv>>8;
            *value++ = tv>>16;
            *value++ = tv>>24;
            return sizeof(long);
        }
    case WORD_HBF: {      /* word, high byte first  */
            flip = 1;      /* signal to flip it */
        }             /* fall thru to 'w' */
    case WORD_LBF: {   
            if ( (edmask&ED_TRUNC) && (tv > 65535 || tv < -65536) )
				trunc_err(65535L,tv);
            goto common_word;  /* jump to common word processing */
        }
    case SIGN_HBF: {
            flip = 1;
        }
    case SIGN_LBF: {
            if ( (edmask&ED_TRUNC) && (tv > 32767 || tv < -32768))
				trunc_err(65535L,tv);
            goto common_word;
        }
    case USIGN_HBF: {
            flip = 1;
        }
    case USIGN_LBF: {
            if ( (edmask&ED_TRUNC) && (tv &0xFFFF0000L))
				trunc_err(65535L,tv);
        }
        common_word:
#if 0
        {
            char *vp;
            char b0,b1;        /* bytes in long */
#ifndef BIG_ENDIAN
            if (flip == 1)
            {   /* flip the bytes */
                vp = (char *)value; /* point to area */
                b0 = *vp++;     /* pickup individual bytes */
                b1 = *vp;
                *vp = b0;       /* and swap them */
                *--vp = b1;
            }
#else
            vp = ((char *)value)+3;  /* point to area */
            b0 = *vp;      /* pickup individual bytes */
            b1 = *--vp;
            if (flip == 1)
            {
                *--vp = b1;     /* and swap them */
                *--vp = b0;
            }
            else
            {
                *--vp = b0;
                *--vp = b1;
            }
#endif
            return sizeof(short);
        }
#else
        if (flip)
        {
            *value++ = tv >> 8;
            *value++ = tv;
        }
        else
        {
            *value++ = tv;
            *value++ = tv >> 8;
        }
        return sizeof(short);
#endif
    case 'b': {
            if ( (edmask&ED_TRUNC) && (tv > 255 || tv < -128))
				trunc_err(255L,tv);
            *value = tv;
            return sizeof(char);
        }
    case 's': {
            if ( (edmask&ED_TRUNC) && (tv > 127 || tv < -128))
				trunc_err(255L,tv);
            *value = tv;
            return sizeof(char);
        }
    case 'c': {
            if ( (edmask&ED_TRUNC) && (tv & 0xFFFFFF00))
				trunc_err(255L,tv);
            *value = tv;
            return sizeof(char);
        }
    case 'y': {
            if ( tv > 32767L || tv < -32768L )
            {
                if ((edmask&ED_TRUNC))
					trunc_err(65534L,tv);
                tv = -3;
            }
            *value++ = tv;
            *value++ = tv>>8;
            return sizeof(short);
        }
    case 'Y': {
            if ( tv > 32767L || tv < -32768L )
            {
				if ( (edmask&ED_TRUNC) )
					trunc_err(65534L,tv);
                tv = -3;
            }
            *value++ = tv>>8;
            *value++ = tv;
            return sizeof(short);
        }
    case 'z': {
/*
     if ((tv == 0 || tv == -1) && macxx_name[3] == '6' && macxx_name[4] == '8') {
        sprintf(emsg,"Branch offset of 0 or -1 is illegal on line %d in file %s",
                current_fnd->fn_line,current_fnd->fn_name_only);
        err_msg(MSG_ERROR,emsg);
     }
*/
            if (tv > 127 || tv < -128)
            {
				if ( (edmask&ED_TRUNC) )
					trunc_err(254L, tv);
                tv = -2;
            }
            *value = tv;
            return sizeof(char);
        }
    }
    return 0;
}

/**********************************************************************
 * Pass2 - generate output
 */
int pass2( void )
/*
 * At entry:
 *	no requirements, called from mainline
 * At exit:
 *	output file written.
 */
{
    int i,r_flg=1;
    long lc;

    if ((outxabs_fp = obj_fp) == 0) return(1); /* no output required */
    endlin();            /* flush the opcode buffer */
    if (options[QUAL_DEBUG]) dbg_flush(1);   /* flush the debug buffers */
    write_to_tmp(TMP_EOF,0,0,0); /* make sure that there's an EOF in tmp */
    rewind_tmp();        /* rewind to beginning */
    while (1)
    {
        if (r_flg) read_from_tmp();  /* now read the whole thing in */
        r_flg = 1;        /* gotta read next time */
        switch (tmp_ptr->tf_type)
        { /* see what we gotta do */
        case TMP_EOF: {
                if (options[QUAL_DEBUG]) dbg_output(); /* add debug stuff to object file */
                termobj(xfer_addr);     /* terminate and flush the output buffer */
                if (!tmp_fp)
                {
                    int ferr;
                    ferr = MEM_free((char *)tmp_top);
#if !defined(SUN)
                    if (ferr)
                    {   /* give back the memory */
                        sprintf(emsg,"Error (%08X) free'ing %d bytes at %08lX from tmp_pool",
                                ferr, max_token*8, (unsigned long)tmp_top);
                        err_msg(MSG_WARN,emsg);
                    }
#endif
                    tmp_top = 0;
                }
                return(1);     /* done with pass2 */
            }
        case TMP_EXPR: {
                if (noout_flag != 0)
                {
                    break;
                }
                else
                {
                    EXP0.ptr = compress_expr(&EXP0);
                    if (EXP0.ptr <= 0)
                    {
                        char *s;
                        if (options[QUAL_OCTAL])
                        {
                            s = "\t(%06lo bytes offset from segment {%s})\n";
                        }
                        else
                        {
                            s = "\t(%04lX bytes offset from segment {%s})\n";
                        }
                        snprintf(emsg,ERRMSG_SIZE,s,current_offset,current_section->seg_string);
                        err_msg(MSG_ERROR|MSG_CTRL,emsg);
                    }
#ifdef PC_DEBUG
                    fprintf(stderr,"%s:%d: EXPR pc = %08lX, sec = {%s}, tag=%c:%d\n",
							current_fnd->fn_buff,
                            current_fnd->fn_line,current_offset,
                            current_section->seg_string,
                            EXP0.tag,EXP0.tag_len);
#endif
                }
                if (EXP0.tag != 0)
                {
                    if ((lc = EXP0.tag_len) == 0)
                    {
                        lc = EXP0.tag_len = 1;     /* get the count */
                    }
                    if ((EXP0.ptr == 1 && EXP0SP->expr_code == EXPR_VALUE) && 
                        (EXP0.tag_len == 1 || _toupper(EXP0.tag) == 'X'))
                    {
                        unsigned char tmpv[sizeof(long)];
                        i = out_compexp(tmpv, EXP0SP->expr_value, (int)EXP0.tag,EXP0.tag_len);
                        outbstr((char *)tmpv,i);
                    }
                    else
                    {
                        register char tag;
                        flushobj();
                        tag = EXP0.tag;
                        if (tag == 'y')
							EXP0.tag = tag = 'w';
						if (tag == 'Y')
							EXP0.tag = tag = 'W';
                        if (tag == 'z')
							EXP0.tag = tag = 's';
                        if (options[QUAL_BINARY])
                        {
                            union vexp ve;
                            ve.vexp_chp = eline;
                            *ve.vexp_type++ = VLDA_EXPR;
                            outexp(&EXP0,ve.vexp_chp,eline,obj_fp);
                        }
                        else
                        {
                            outexp(&EXP0,eline,eline+1,obj_fp);
                        }
                        tag = _toupper(tag);
                        if (tag == 'B' || tag == 'C' || tag == 'S')
                        {
                            i = macxx_mau_byte;
                        }
                        else
                        {
                            if (tag == 'L')
                            {
                                i = macxx_mau_long;
                            }
                            else if (tag == 'X')
                            {
                                i = EXP0.tag_len+(macxx_mau-1)/macxx_mau;
                            }
                            else
                            {
                                i = macxx_mau_word;
                            }
                        }
                    }
                    current_offset += lc*i;
                    break;           /* exit the TMP_xxx switch */
                }
                else
                {
                    if (options[QUAL_RELATIVE])
                    {
                        flushobj();
                        outexp(&EXP0,eline,eline,obj_fp);
                    }
                }
                break;
            }
        case TMP_BSTNG: 
        case TMP_ASTNG: {
                if (noout_flag == 0) outbstr((char *)tmp_pool,(int)tmp_ptr->tf_length);
#ifdef PC_DEBUG
                fprintf(stderr,"%s:%d: TXT, %ld bytes, pc = %08lX, sec = {%s}\n",
						current_fnd->fn_buff,
						current_fnd->fn_line,
                        tmp_ptr->tf_length,    
                        current_offset,
                        current_section->seg_string);
#endif
                current_offset += tmp_ptr->tf_length*macxx_mau_byte;
                break;
            }
        case TMP_ORG: {
                SEG_struct *seg_ptr;
/*	    EXP0.ptr = compress_expr(&EXP0); */
                if (EXP0.ptr <= 0)
                {
                    if (options[QUAL_OCTAL])
                    {
                        sprintf(emsg,"ORG may be incorrectly set to %010lo\n",
                                EXP0SP->expr_value);
                    }
                    else
                    {
                        sprintf(emsg,"ORG may be incorrectly set to %08lX\n",
                                EXP0SP->expr_value);
                    }
                    err_msg(MSG_CONT,emsg);
                }
                if ((seg_ptr = EXP0SP->expr_seg) == 0)
                {
                    EXP0SP->expr_code = EXPR_SEG;
                    EXP0SP->expr_seg = seg_ptr = get_absseg();
                }
                current_section = seg_ptr;
                current_offset = EXP0SP->expr_value+seg_ptr->rel_offset;
                noout_flag = seg_ptr->flg_noout;
                if (noout_flag == 0) outorg(&EXP0);
#ifdef PC_DEBUG
                fprintf(stderr,"%s:%d: ORG at newpc = %08lX, sec = {%s}\n",
						current_fnd->fn_buff,
                        current_fnd->fn_line,
						current_offset,
                        current_section->seg_string);
#endif
                break;
            }          /* -- case */
        case TMP_START: {
                EXP0.ptr = compress_expr(&EXP0);
                if (EXP0.ptr <= 0)
                {
                    if (options[QUAL_OCTAL])
                    {
                        sprintf(emsg,"XFER addr may be incorrectly set to %010lo\n",
                                EXP0SP->expr_value);
                    }
                    else
                    {
                        sprintf(emsg,"XFER addr may be incorrectly set to %08lX\n",
                                EXP0SP->expr_value);
                    }
                    err_msg(MSG_CONT,emsg);
                }
                if (options[QUAL_RELATIVE])
                {
                    outxfer(&EXP0,obj_fp);
                }
                else
                {
                    if (xfer_addr == 0 || xfer_addr == 1)
                    {
                        xfer_addr = EXP0SP->expr_value;
                    }
                }
                break;
            }          /* -- case */
        case TMP_OOR:
        case TMP_BOFF:
        case TMP_TEST: {
                EXP_stk *eps;
                EXPR_struct *expr;
                int otyp = tmp_ptr->tf_type;    /* save our type */
                eps = exprs_stack;
                expr = eps->stack;
                eps->ptr = compress_expr(eps);  /* evaluate the expression */
                read_from_tmp();    /* get the ascii message */
                if (tmp_ptr->tf_type != TMP_ASTNG)
                {
                    err_msg(MSG_ERROR,"Internal error processing .TEST, .BOFF ot .OOR directives");
                }
                else if (eps->ptr <= 0)
                {
                    sprintf(emsg,"Expression error while processing .TEST, .BOFF or .OOR (eps->ptr=%d): %s\n",
                            eps->ptr, (char *)tmp_pool);
                    err_msg(MSG_CONT,emsg);
                }
                else
                {
                    if (eps->ptr == 1 && expr->expr_code == EXPR_VALUE)
                    {
                        if (expr->expr_value != 0)
                        {
                            if (otyp == TMP_BOFF)
                            {
                                sprintf(emsg, "Branch offset out of range: %s", (char *)tmp_pool);
                            }
                            else if (otyp == TMP_OOR)
                            {
                                sprintf(emsg, "Operand out of range: %s", (char *)tmp_pool);
                            }
                            else
                            {
                                sprintf(emsg, ".TEST expression evaluated true: %s", (char *)tmp_pool);
                            }          
                            err_msg(MSG_ERROR, emsg);
                        }
                        break;
                    }
                    if (otyp == TMP_TEST)
                    {
                        outtstexp((char *)tmp_pool,(int)tmp_ptr->tf_length,&EXP0);
                    }
                    else if (otyp == TMP_BOFF)
                    {
                        outboffexp((char *)tmp_pool, (int)tmp_ptr->tf_length, &EXP0);
                    }
                    else
                    {
                        outoorexp((char *)tmp_pool, (int)tmp_ptr->tf_length, &EXP0);
                    }
                }
                break;
            }          /* -- case */
        case TMP_FILE: {
                continue;       /* FILE type is easy */
            }
        case TMP_SCOPE: {
                continue;       /* SCOPE type is easy */
            }
        default: {
                printf("%%MACXX-F-Undefined code byte found in tmp file: %d\n",
                       tmp_ptr->tf_type);
                EXIT_FALSE;
            }          /* -- case */
        }             /* -- switch */
    }                /* -- while */
}               /* -- pass2 */
