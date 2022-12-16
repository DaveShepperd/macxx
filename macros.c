/*
    macros.c - Part of macxx, a cross assembler family for various micro-processors
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

    02-06-2022	- Changed added support for HLLxxF  - Tim Giddens

******************************************************************************/
#include "token.h"
#include "pst_tokens.h"
#include "listctrl.h"
#include "memmgt.h"
#include "exproper.h"
#include "utils.h"

unsigned char *macro_pool;   /* pointer to free space for macro expansion */
int macro_pool_size;    /* amount of space left in macro pool */
unsigned long macro_pool_used,peak_macro_pool_used;

int macro_level;    /* nest level of macro calls */
int macro_nesting;  /* .ne. if macro called a .INCLUDE file */

Mcall_struct *marg_head;

#ifdef DUMP_MACRO
void dump_macro( Macargs *ma, Opcode *mac )
{
    int c,i;
    char *d,*s,**sp;
    if (mac != 0)
    {
        sprintf(emsg,"\tDump of macro %s\n",mac->op_name);
        puts_lis(emsg,1);
        sprintf(emsg,"\t\t Opcode entry (%08lX):\n\t\t\top_margs: %08lX\n\t\t\top_class: %04X\n",
                (long)mac,(long)mac->op_margs,mac->op_class);
        puts_lis(emsg,3);
    }
    else
    {
        puts_lis("\tDump of .IRP macro\n",1);
    }
    sprintf(emsg,"\t\t Dummy argument list (%08lX) %d arguments:\n\t\t\tMacro body at %08lX\n",
            (long)ma,(long)ma->mac_numargs, (long)ma->mac_body);
    puts_lis(emsg,2);
    for (i=0;i<ma->mac_numargs;++i)
    {
        sp = ma->mac_keywrd+i;
        s = *sp;
        sprintf(emsg,"\t\t\tArg %d (%08lX)->(%08lX)-> %s\n",
                i,(long)sp,(long)s,s);
        puts_lis(emsg,1);
    }
    if (ma->mac_body != 0)
    {
        puts_lis("\t\tMacro body:\n",1);
        s = ma->mac_body;
        while (1)
        {
            d = emsg;
            *d++ = '\t';
            *d++ = '\t';
            *d++ = '\t';
            while (c = (unsigned char)*s++)
            {
                if ((c & 0x80) != 0)
                {
                    if (c == MAC_LINK)
                    {
                        ADJ_ALIGN(s)
                        s = *((char **)s);
                        sprintf(d,"{LINK to %08lX}", (long)s);
                    }
                    else if (c == MAC_EOM)
                    {
                        strcpy(d,"{EOM}\n");
                        d += strlen(d);
                        break;
                    }
                    else
                    {
                        sprintf(d,"{%d}",c&0x7F);
                    }
                    d += strlen(d);
                }
                else
                {
                    *d++ = c;
                }
            }
            *d = 0;        /* terminate the string */
            puts_lis(emsg,1);
            if (c == MAC_EOM) break;
        }
    }
    return;
}
#endif

void free_macbody( unsigned char *link )
{
#if !defined(SUN)
    unsigned char c,*s;   
    int *ip;
    s = (unsigned char *)link;
    ip = (int *)s;
    --ip;
    while (1)
    {
        c = *s++;
        if (c == MAC_LINK)
        {
            ADJ_ALIGN(s)
            free_macbody(*((unsigned char **)s));
            break;
        }
        if (c == MAC_EOM) break;
    }
    macro_pool_used -= *ip;
    MEM_free(link);
#endif
    return;
}

static int scan_margs( int numarg, char **args, char *aptr)
{
    int num = 0;
    for (num = 0; num < numarg; ++num)
    {
        if (strcmp(aptr,*args++) == 0) return(num|-128);
    }
    return 0;
}

int macro_to_mem( Macargs *ma, char *name)
{
    int nest = 0,tkncnt;
    char *src;
    unsigned char *dst;
    list_stats.include_level = include_level;
    while (1)
    {          /* until .ENDM/.ENDR found */
        int mpsize;
        if (get_text() == EOF) return EOF;
        list_stats.line_no = current_fnd->fn_line;
        src = inp_str;        /* point to beginning of string */
        mpsize = inp_len+1+1+ALIGNMENT+sizeof(char *);
        if (macro_pool_size < mpsize)
        {
            unsigned char **nmp;
            if (macro_pool != 0)
            {
                *macro_pool++ = MAC_LINK;
                ADJ_ALIGN(macro_pool)
            }
            nmp = (unsigned char **)macro_pool;
            macro_pool_size = 128;
            if (macro_pool_size < mpsize) macro_pool_size = mpsize;
            macro_pool = (unsigned char *)MEM_alloc(macro_pool_size);
            if (nmp != 0)
                *nmp = macro_pool;
            if (ma->mac_body == 0)
                ma->mac_body = macro_pool;
            macro_pool_used += macro_pool_size;
            if (macro_pool_used > peak_macro_pool_used)
            {
                peak_macro_pool_used = macro_pool_used;
            }
        }
        dst = macro_pool;
        tkncnt = 0;
        while (1)
        {       /* for each token on a line */
            unsigned char *d, *aptr;
            int c;
            int argnum;
            while ((cttbl[(int)*src]&CT_WS) != 0)
                *dst++ = *src++;
            aptr = d = dst;    /* remember start of token */
            c = *dst++ = *src++;   /* pass the first char of string */
            if (cttbl[c]&CT_EOL)
            {
                --src;      /* stop on EOL character */
                break;      /* and quit */
            }
            if (c == '\'')
            {   /* start with a "'"? */
                aptr = dst;
                c = *dst++ = *src++;  /* pickup the next char */
                if (c == '\'')
                {    /* followed by another one? */
                    d = aptr;    /* remember it */
                    aptr = dst;  /* remember start of token */
                    c = *dst++ = *src++;
                }
            }
            if (cttbl[c]&CT_ALP)
            { /* first char an ALP? */
                char *tpp;
                tpp = token_pool;
                if ((edmask&ED_LC) != 0)
                {  /* if case sensitive */
                    *tpp++ = c;      /* don't upcase it */
                    while (cttbl[c = *src]&(CT_ALP|CT_NUM))
                    {
                        ++src;        /* pass remaining ALP|NUM's */
                        *dst++ = c;
                        *tpp++ = c;
                    }
                }
                else
                {            /* else not case sensitive */
                    *tpp++ = _toupper(c);    /* upcase it */
                    while (cttbl[c = *src]&(CT_ALP|CT_NUM))
                    {
                        ++src;        /* yep, pass remaining ALP|NUM's */
                        *dst++ = c;
                        *tpp++ = _toupper(c);
                    }
                }
                *tpp = *dst = 0;        /* terminate the strings */
                if (tkncnt == 0 && (cttbl[c]&(CT_WS|CT_EOL|CT_SMC)) != 0 &&
                    aptr == d)
                {
                    Opcode *mptr;    /* yep */
                    *(token_pool+max_opcode_length) = 0;
                    mptr = opcode_lookup(token_pool,0);
                    if (  (mptr != 0) &&
                          ((mptr->op_class&(OP_CLASS_OPC|OP_CLASS_MAC)) == 0) && 
                          (mptr->op_class&DFLMAC))
                    {
                        if (mptr->op_class&DFLMEND)
                        {
                            --nest;
                            if (nest < 0)
                            {
                                while ((cttbl[(int)*src]&CT_WS) != 0) ++src;
                                tpp = token_pool;
                                while ((cttbl[c = *src]&(CT_ALP|CT_NUM)) != 0)
                                {
                                    ++src;
                                    *tpp++ = _toupper(c);
                                }
                                *tpp = 0;
                                if (tpp-token_pool > 0)
                                {
                                    if (name != 0 && strcmp(token_pool,name) != 0)
                                    {
                                        sprintf(emsg,"Name on .ENDM (%s) doesn't match macro name (%s)"
                                                ,token_pool,name);
                                        bad_token((char *)0,emsg);
                                    }
                                }   /* -- name on .ENDM */
                                inp_ptr = src;
                                dst = aptr;
                                for (d=dst-1; d >= macro_pool && (cttbl[c = *d]&CT_WS) != 0;--d);
                                if (d >= macro_pool)
                                {
                                    dst = d+1;
                                    if (c != 0 && c != '\n')
                                    {
                                        *dst++ = '\n';
                                        *dst++ = 0;
                                    }
                                }
                                else
                                {
                                    dst = macro_pool;
                                }
                                ma->mac_end = dst;
                                *dst++ = MAC_EOM;
                                *dst++ = 0;
                                return EOL;
                            }      /* -- nest level < 0 */
                        }
                        else
                        {  /* +- MACRO end directive */
                            ++nest;    /* bump nest level */
                        }     /* -- MACRO end directive */
                    }        /* -- MACRO directive */
                }           /* -- arg 0 terminated on WS */
                if (ma->mac_numargs > 0)
                {
                    argnum = scan_margs(ma->mac_numargs,ma->mac_keywrd,token_pool);
                    if (argnum < 0)
                    {
                        *d++ = argnum;    /* stuff in the argument number */
                        dst = d;      /* backup the pointer */
                        if (*src == '\'')
                        {   /* end with a "'"? */
                            ++src;     /* yep, then eat it */
                        }
                    }
                }
            }
            else
            {           /* +- First char is ALP */
                unsigned short oct,ct;  /* first char is NOT ALP */
                oct = cttbl[c];     /* get the flags of the first char */
                while (1)
                {          /* pass all chars until next ALP or EOL */
                    ct = cttbl[c = *src];
                    if (c == '\'') break;    /* stop on tics */
                    if ((ct&CT_EOL) != 0) break; /* stop at EOL */
                    if ((ct&CT_ALP) != 0)
                    {  /* found an ALP */
                        if ((oct&(CT_ALP|CT_NUM)) == 0)
                        { /* if previous char not a NUM */
                            break;
                        }
                    }
                    else
                    {
                        oct = ct;     /* remember previous char */
                    }
                    *dst++ = c;      /* pass current char */
                    ++src;           /* and advance */
                }
            }          /* -- First char is ALP */
            c = *src;      /* pickup the next source char */
            if (tkncnt == 0)
            {
                if (c == ':')
                {
                    *dst++ = c;
                    ++src;       /* skip over the : */
                    if (*src == ':')
                    {   /* a global? */
                        *dst++ = c;
                        ++src;    /* skip over the : */
                    }
                    --tkncnt;    /* doesn't count if label */
                }           /* -- char is ':' */
            }          /* -- tkncnt == 0 */
            ++tkncnt;      /* count the token */
        }             /* -- while all arguments */
        inp_ptr = src;        /* move the pointer */
        if (show_line != 0) line_to_listing();
        *dst++ = 0;       /* insert a terminator */
        macro_pool_size -= dst-macro_pool;
        macro_pool = dst;
    }                /* -- for all lines until .ENDM */
}

int op_macro( void )      /* define a macro */
{
    char **key_pool,**def_pool;
    unsigned char *gs_flag;
    unsigned char *mac_pool;
    Opcode *mac;
    int size,tt;
    unsigned long old_edmask;
    Macargs *ma;

    comma_expected = 2;      /* comma is optional */
    old_edmask = edmask;
    edmask &= ~(ED_LC|ED_DOL);
    tt = get_token();
    edmask = old_edmask;
    if (tt != TOKEN_strng)
    {
        bad_token(tkn_ptr,"Macro name must be a string beginning with an alpha");
        f1_eatit();
        return 0;
    }
    *(token_pool+max_opcode_length) = 0;
    mac = opcode_lookup(token_pool,2);
    if (mac == 0)
    {
        bad_token(tkn_ptr,"Unable to add macro name to PST");
        f1_eatit();
        return 0;
    }
    if (new_opcode != 0)
    {
        if (mac->op_name == token_pool)
        {
            token_pool += ++token_value;   /* save the macro name if new */
            token_pool_size -= token_value;
        }
    }
    if (mac->op_class == OP_CLASS_MAC)
    {
        ma = mac->op_margs;
        if (ma != 0)
        {            /* discard any old definitions */
            if (ma->mac_body != 0)
                free_macbody(ma->mac_body);
            MEM_free(ma);
        }
    }
    mac->op_class = OP_CLASS_MAC;    /* signal it's a macro now */
    size = (inp_len-(inp_ptr-inp_str)+3)&-4/2;   /* get length of remaining input */
    if (size > 126) size = 126;      /* maximum of 126 args */
    tt = size*(2+            /* room for keywrds and defaults */
               2*sizeof(char *)+      /* room for ptrs to keywrds */
               /*   and defaults */
               1)+                /* room for gsflags */
         sizeof(Macargs);   /* room for macargs struct */
    mac_pool = (unsigned char *)MEM_alloc(tt);        /* pickup some memory */
    ma = mac->op_margs = (Macargs *)mac_pool;
    mac_pool += sizeof(Macargs);
    key_pool = (char **)mac_pool;    /* get space for keyword arg */
    def_pool = key_pool+size;        /* get space for default arg */
    gs_flag = (unsigned char *)(def_pool+size);   /* point to GS flag area */
    mac_pool = gs_flag+size;     /* point to area to copy args */

    ma->mac_keywrd = key_pool;
    ma->mac_default = def_pool;
    ma->mac_gsflag = gs_flag;
    ma->mac_numargs = 0;                 /* assume 0 arguments */

    while (1)
    {                  /* first get the arguments */
        int flg;
        int c;
        flg = 0;
        while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
        if (*inp_ptr == ',')
        {
            ++inp_ptr;         /* eat intervening commas */
            while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip white space */
        }
        if (*inp_ptr == macro_arg_gensym)
        { /* if first char is a '?', gensym */
            flg = 1;
            ++inp_ptr;         /* and eat it */
        }
        if ((tt=get_token()) == EOL) break;   /* pickup rest of arg */
        if (tt != TOKEN_strng)
        {
            bad_token(tkn_ptr,"Illegal macro argument");
            continue;
        }
        *gs_flag++ = flg;
        strcpy((char *)mac_pool,token_pool);
        *key_pool++ = (char *)mac_pool;
        mac_pool += token_value+1;
        if (*inp_ptr == '=')
        {
            int nest = 0;
            char beg = macro_arg_open,end = macro_arg_close;
            ++inp_ptr;         /* eat the '=' */
            while ((cttbl[(int)*inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip over white space */
            *def_pool++ = (char *)mac_pool;
            if (*inp_ptr == macro_arg_escape) beg = end = *++inp_ptr;
            if (*inp_ptr == beg)
            {     /* delimited string? */
                ++inp_ptr;          /* yep, eat the beginning char */
                while (1)
                {
                    if ((cttbl[c = *inp_ptr]&CT_EOL) != 0) break;
                    if (c == end)
                    {
                        --nest;
                        if (nest < 0) break;
                    }
                    else if (c == beg)
                    {
                        ++nest;
                    }
                    *mac_pool++ = c;
                    ++inp_ptr;
                }
            }
            else
            {
                while (1)
                {
                    c = *inp_ptr;
                    if (cttbl[c] & (CT_EOL|CT_SMC|CT_COM|CT_WS)) break;
                    *mac_pool++ = c;
                    ++inp_ptr;
                }
            }
            *mac_pool++ = 0;
        }
        else
        {
            *def_pool++ = 0;   /* no default for this argument */
        }
        ma->mac_numargs += 1;
        continue;         /* next argument */
    }                /* -- while all arguments */
    if (show_line != 0) line_to_listing();
    macro_pool_size = 0;
    macro_pool = 0;
#ifdef DUMP_MACRO
    puts_lis("\t\tBefore the macro_to_mem() call\n",1);
    dump_macro(ma,mac);
#endif
    if (macro_to_mem(ma,mac->op_name) == EOF)
    {   /* process the macro body */
        inp_str[0] = '\n';    /* display a blank line */
        inp_str[1] = 0;
        sprintf(emsg,"No terminating .ENDM for macro definition: %s",mac->op_name);
        bad_token((char *)0,emsg);
        return EOF;
    }
#ifdef DUMP_MACRO
    puts_lis("\t\tAfter the macro_to_mem() call\n",1);
    dump_macro(ma,mac);      /* display the macro */
#endif
    return 0;
}

#ifndef MAC_PP
static void setup_mebstats( void )
{
    meb_stats.getting_stuff = 1;
    meb_stats.list_ptr = LLIST_OPC;
    meb_stats.f1_flag = 0;
	meb_stats.f2_flag = 0;
    meb_stats.has_stuff = 1;
    meb_stats.line_no = list_stats.line_no;
    meb_stats.include_level = list_stats.include_level;
    meb_stats.pc_flag = 0;
	list_source.optTextBuf[0] = 0;
	list_source.reqNewLine = 0;
	list_source.srcPosition = limitSrcPosition(list_source.srcPositionQued);
	/* detab the whole line */
	deTab(inp_str, 8, 0, 0, meb_stats.listBuffer + list_source.srcPosition, sizeof(meb_stats.listBuffer) - list_source.srcPosition);
    return;
}
#endif

int macro_call(Opcode *opc)
{
    Macargs *ma;
    char **keywrd,**defalt,**argptr,*args;
    unsigned char *gensym;
    Mcall_struct *mac_pool;
    int argcnt,arglen,argMask;       /* assume no args */
    char **args_area;

    if ((ma = opc->op_margs) == 0)
    {
        bad_token(tkn_ptr,"Undefined macro call. Fatal internal error.");
        return 0;
    }
    if (macro_level == 0)
    {
        show_line = list_mc;
#ifndef MAC_PP
        if (list_meb && !list_mes) setup_mebstats();
#endif
        list_mes = show_line;
    }
    else
    {
        show_line = list_mes & list_mc;
    }
    ++macro_level;       /* bump macro level */
    ++macro_nesting;     /* keep 2 copies */
    arglen = inp_len-(tkn_ptr-inp_str)+1; /* length of input args */
    argcnt = ma->mac_numargs;
    mac_pool = (Mcall_struct *)MEM_alloc((int)sizeof(Mcall_struct)+
                                         arglen+     /* + size of args */
                                         argcnt*((int)sizeof(char *)+    /* + room for ptrs to args */
                                                 7));        /* plus room for generated symbols */
    mac_pool->marg_next = marg_head; /* save ptr to previous list head */
    marg_head = mac_pool;    /* this one becomes the top */
    marg_head->marg_ptr = ma->mac_body;  /* point to beginning of macro text */
    marg_head->marg_top = ma->mac_body;  /* point to beginning of macro text */
    marg_head->marg_end = ma->mac_end;   /* point to EOM */
    keywrd = (char **)(marg_head+1); /* point to start of keyword args */
    marg_head->marg_flag = 0;    /* ordinary macro */
    marg_head->marg_count = 0;   /* loop count is 0 on ordinary macro */
    marg_head->marg_icount = 0;  /* loop count is 0 on ordinary macro */
    marg_head->marg_args = keywrd;   /* ptr to arguments */
    marg_head->marg_cndlvl = condit_level;
    marg_head->marg_cndnst = condit_nest;
    marg_head->marg_cndwrd = condit_word;
    marg_head->marg_cndpol = condit_polarity;
    condit_level = 0;
    condit_nest = 0;
    condit_word = 0;
    condit_polarity = 0;
    args = (char *)(keywrd+ma->mac_numargs); /* point to argument area */
    args_area = argptr = (char **)MEM_alloc(argcnt*(int)sizeof(char *));
	argMask = (CT_EOL|CT_SMC);
    for (argcnt = 0; argcnt < ma->mac_numargs; ++argcnt)
    {
        int c,beg,end;
        beg = macro_arg_open;
        end = macro_arg_close;
        *argptr = 0;      /* assume no argument */
        while (isspace((int)*inp_ptr)) ++inp_ptr; /* skip over white space */
        c = *inp_ptr;     /* pick up character */
        if ((cttbl[c]&argMask) != 0)
        {
            if (c == ',') ++inp_ptr; /* eat the comma */
            goto term_arg;     /* go terminate the argument */
        }
		argMask = (CT_EOL|CT_COM|CT_SMC);
        *argptr = args;       /* point to argument */
        if (c == macro_arg_genval)
        {  /* special value to ascii? */
            char *ep;
            int unpack_radix;
            ++inp_ptr;     /* yep, eat the escape char */
            unpack_radix = 0;
            if (*inp_ptr == macro_arg_genval)
            {
                c = *(inp_ptr+1);   /* get requested radix */
                c = _toupper(c);    /* upcase the char */
                if (c == 'O') unpack_radix = 8;
                if (c == 'D') unpack_radix = 10;
                if (c == 'H') unpack_radix = 16;
                if (c == 'X') unpack_radix = 16;
                if (unpack_radix != 0)
                {
                    inp_ptr += 2;    /* eat both items */
                }
            }
            if (unpack_radix == 0) unpack_radix = current_radix;
            get_token();       /* setup the variables */
            ep = tkn_ptr;      /* start at current input */
            while ((cttbl[(int)*ep]&(CT_EOL|CT_WS|CT_COM)) == 0) ++ep; /* find end */
            if ((cttbl[(int)*ep]&CT_WS) != 0)
            { /* if white space then */
                char sv1,sv2;
                sv1 = *ep;      /* save the ws char */
                *ep++ = '\n';   /* replace with nl */
                sv2 = *ep;      /* save the following char too */
                *ep = 0;        /* and replace it with a 0 */
                exprs(0,&EXP0); /* pickup an absolute expression */
                *ep = sv2;      /* restore chars */
                *--ep = sv1;
            }
            else
            {
                exprs(0,&EXP0); /* pickup an absolute expression */
            }
            if (unpack_radix < 10)
            {
                sprintf(args,"%lo",EXP0SP->expr_value);
            }
            else if (unpack_radix < 16)
            {
                sprintf(args,"%ld",EXP0SP->expr_value);
            }
            else
            {
                unsigned long tv;
                tv = EXP0SP->expr_value;
                while (tv > 15) tv >>= 4;
                if (tv > 9)
                {
                    sprintf(args,"0%lX",EXP0SP->expr_value);
                }
                else
                {
                    sprintf(args,"%lX",EXP0SP->expr_value);
                }
            }
            args += strlen(args);  /* position to end of string */
            goto term_arg;     /* and terminate the arg */
        }
        if (c == macro_arg_escape)
        {  /* argument escape? */
            ++inp_ptr;     /* yep, eat it */
            c = end = beg = *inp_ptr; /* and set the new delimiters */
        }
        if (c == beg)
        {       /* special delimiter character */
            int nest = 0;
            ++inp_ptr;     /* eat it */
            while (1)
            {        /* copy string */
                c = *inp_ptr;   /* pickup next char */
                if ((cttbl[c]&CT_EOL) != 0) goto term_arg;
                if (c == end)
                { /* match end delimiter? */
                    --nest;      /* yep, de-nest */
                    if (nest < 0)
                    {  /* end of term? */
                        ++inp_ptr;    /* yep, eat the terminator character */
                        break;    /* and terminate the argument */
                    }
                }
                else if (c == beg)
                {
                    ++nest;      /* bump the nest level */
                }
                *args++ = c;    /* pass the character */
                ++inp_ptr;
            }
        }
        else
        {          /* not escaped */
            int not_kw = 1;
            char *tpp;
            tpp = token_pool;  /* point to temp space for upcased token */
            while (1)
            {        /* copy string */
                c = *inp_ptr;   /* pickup next char */
                if ((cttbl[c]&(CT_EOL|CT_WS|CT_COM|CT_SMC)) != 0)
					break;
                *tpp++ = _toupper(c);   /* upcase the keyword */
                if (not_kw && c == '=')
                {   /* asking for keyword parameter? */
                    not_kw = 0;      /* signal already through here */
                    *(tpp-1) = 0;        /* null terminate the string */
                    c = scan_margs(ma->mac_numargs,ma->mac_keywrd,token_pool);
                    if ((c&0x80) != 0)
                    {
                        ++inp_ptr;        /* eat the = */
                        args = *argptr;   /* reset the pointers */
                        tpp = token_pool;
                        *(keywrd+(c&0x7F)) = args;
                        *argptr = 0;      /* no regular argument */
                        continue;     /* and keep going */
                    }
                    *tpp = '=';      /* put the char back */
                    c = *inp_ptr;        /* and restore c */
                }
                *args++ = c;    /* pass the character */
                ++inp_ptr;
            }
        }  
term_arg:
        *args++ = 0;      /* null terminate the string */
        ++argptr;         /* skip to next arg */
        while ((cttbl[c = *inp_ptr]&CT_WS) != 0) ++inp_ptr; /* skip over trailing white space */
        if (c == ',') ++inp_ptr;  /* eat trailing comma */
        if ((cttbl[c]&CT_EOL) != 0) break;
    }
    argptr = args_area;      /* backup to beginning of dummy arg area */
    keywrd = (char **)(marg_head+1); /* point to start of keyword args */
    defalt = ma->mac_default;    /* point to default list */
    gensym = ma->mac_gsflag; /* point to generated symbol flag */
    for (argcnt = 0; argcnt < ma->mac_numargs; ++argcnt)
    {
        if (*keywrd == 0)
        {   /* user pass a keyword argument? */
            if (*argptr == 0)
            {    /* nope, user pass positional argument? */
                if (*gensym != 0)
                { /* generated symbol */
                    *keywrd = args;  /* point to argument */
                    sprintf(args,"%ld",autogen_lsb);
                    ++autogen_lsb;   /* bump generated symbol number */
                    args += strlen(args);
                    *args++ = '$';   /* make it a local symbol */
                    *args++ = 0; /* null terminate it */
                }
                else
                {
                    if (*defalt != 0)
                    {
                        *keywrd = *defalt;    /* nope, use the default */
                    }
                    else
                    {
                        *keywrd = "";     /* else nothing */
                    }
                }
            }
            else
            {
                *keywrd = *argptr;  /* else use the next positional argument */
            }
        }
        if (strlen(*keywrd) != 0) marg_head->marg_icount += 1;
        ++argptr;         /* bump to next argumnet pointer */
        ++keywrd;         /* advance to next argument */
        ++defalt;         /* and its cooresponding default */
        ++gensym;         /* next generated symbol flag */
    }
    MEM_free((char *)args_area);     /* free the temp arg list */
    return 1;
}

int op_irp( void )            /* .IRP macro */
{
    char **key_pool,**def_pool,*dst;
    unsigned char *gs_flag;
    char *macro_name;
    unsigned char *mac_pool;
    int size,tt;
    Mcall_struct *marg;
    Macargs *ma;

    tt = get_token();
    if (tt != TOKEN_strng)
    {
        bad_token(tkn_ptr,"IRP dummy argument must be a string beginning with an alpha");
    }
    size = (inp_len-(inp_ptr-inp_str)+3)&-4/2;   /* get length of remaining input */
    if (size > 126) size = 126;      /* maximum of 126 args */
#define IRP_NAME ".IRP %s,<...>"
    tt = size*(2+            /* room for keywrds and defaults */
               2*sizeof(char *)+      /* room for ptrs to keywrds */
               /*   and defaults */
               1)+                /* room for gsflags */
         sizeof(Mcall_struct)+      /* room for mcall struct */
         sizeof(Macargs)+       /* room for macargs struct */
         2*token_value+sizeof(IRP_NAME); /* room for macro name and argument name */
    mac_pool = MEM_alloc(tt);        /* pickup some memory */
    marg = (Mcall_struct *)mac_pool; /* point to argument area */
    ma = (Macargs *)(marg+1);        /* pointer to dummy macargs */
    key_pool = (char **)(ma+1);      /* get space for arg ptrs */
    def_pool = key_pool+size;        /* get space for default arg */
    gs_flag = (unsigned char *)(def_pool+size);   /* point to GS flag area */
    mac_pool = gs_flag+size;     /* point to next free area */
    macro_name = (char *)mac_pool;       /* point to area for macro name */
    sprintf(macro_name,IRP_NAME,token_pool); /* create macro name */
    mac_pool += strlen(macro_name)+1;

    ma->mac_keywrd = key_pool;
    ma->mac_default = def_pool;
    ma->mac_gsflag = gs_flag;
    ma->mac_numargs = 1;         /* .IRP has only 1 replaceable argument */
    *key_pool++ = (char *)mac_pool;      /* pointer to dummy argument */
    strcpy((char *)mac_pool,token_pool);     /* copy in dummy argument name */
    mac_pool += token_value+1;       /* move pointer */
    marg->marg_args = key_pool;      /* pointer to array of ptrs to args */

#if 0
    tt = size*(1+            /* room for arguments */
               sizeof(char *))+       /* room for ptrs to arguments */
         2*sizeof(char *)+      /* room for 2 extra pointers */
         sizeof(Mcall_struct)+      /* room for mcall struct */
         sizeof(Macargs)+       /* room for dummy macargs struct */
         token_value+12;        /* room for error message */
    mac_pool = MEM_alloc(tt);        /* pickup some memory */
    marg = (Mcall_struct *)mac_pool; /* point to argument area */
    ma = (Macargs *)(marg+1);    /* pointer to dummy macargs */
    key_pool = (char **)(ma+1);      /* get space for arg ptrs */
    macro_name = (char *)(key_pool+size+1); /* point to area for macro name */
    sprintf(macro_name,".IRP %s,<...>",token_pool);  /* create macro name */
    mac_pool = macro_name+12+token_value; /* move pointer */
    ma->mac_keywrd = key_pool;       /* pointer to pointer to dummy arg */
    *key_pool++ = mac_pool;      /* pointer to dummy argument */
    strcpy(mac_pool,token_pool);     /* copy in dummy argument name */
    mac_pool += token_value+1;       /* move pointer */
    marg->marg_args = key_pool;      /* pointer to array of ptrs to args */
    ma->mac_numargs = 1;         /* .IRP has only 1 replaceable argument */
#endif

    while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
    if (*inp_ptr != ',')
    {       /* next thing must be a comma */
        show_bad_token(inp_ptr,"Expected a comma here. One is assumed", MSG_WARN);
    }
    else
    {
        ++inp_ptr;            /* eat the comma */
        while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
    }
    if (*inp_ptr != macro_arg_open)
    {        /* next thing must be open brace */
        show_bad_token(inp_ptr,"Expected a macro_arg_open here. One is assumed", MSG_WARN);
    }
    else
    {
        ++inp_ptr;            /* eat opening bracket */
    }
    dst = (char *)mac_pool;          /* point to start of free space */
    *key_pool++ = dst;           /* space for replacement argument ptr */
    while (1)
    {              /* for all arguments */
        int nest;
        int c,beg,end;
        c = *inp_ptr;
        if (c == '\n' || c == 0)
        {
            bad_token(inp_ptr,"Expected a macro_arg_close here. One is assumed.");
            break;
        }
        beg = macro_arg_open;
        end = macro_arg_close;
        while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
        if (*inp_ptr == '^')
        {        /* arg delim escape? */
            c = *++inp_ptr;        /* yep, eat it and get the next*/
            if (c == '\n' || c == 0)
            {
                --inp_ptr;
                bad_token(inp_ptr,"Premature end of line");
                break;
            }
            beg = end = c;         /* pass the delimiter */
        }
        *key_pool++ = dst;        /* point to place to deposit string */
        nest = 0;             /* nest the search */
        if (*inp_ptr == beg)
        {        /* starting with a bracket */
            ++inp_ptr;         /* eat the starting char */
            while (1)
            {            /* for all chars in argument */
                c = *inp_ptr;       /* pickup next char */
                if (cttbl[c]&CT_EOL) break; /* break if EOL */
                ++inp_ptr;          /* eat the char */
                if (c == end)
                {     /* find an end? */
                    --nest;          /* yep */
                    if (nest < 0)
                    {      /* all of them? */
                        break;        /* yep, we're done then */
                    }
                }
                else if (c == beg)
                {
                    ++nest;          /* bump nest level */
                }
                *dst++ = c;         /* pass the char */
            }
        }
        else
        {
            while (1)
            {            /* for all chars in argument */
                c = *inp_ptr;       /* pickup next char */
                if ((cttbl[c]&(CT_EOL|CT_WS|CT_COM)) != 0) break;   /* break if EOL */
                if (c == macro_arg_close)
                {     /* trailing bracket */
                    --nest;          
                    if (nest < 0) break; /* found end of string */
                }
                else
                {
                    if (c == macro_arg_open) ++nest; /* count inner brackets */
                }
                inp_ptr++;          /* its ok, so eat it */
                *dst++ = c;         /* pass it */
            }
        }
        *dst++ = 0;           /* terminate the string */
        while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
        if (*inp_ptr == macro_arg_close)
        {        /* terminate on closing brace? */
            ++inp_ptr;         /* yep, eat it */
            break;             /* and we're done */
        }
        if (*inp_ptr == ',') ++inp_ptr;   /* skip optional comma */
    }                    /* -- while all arguments */
    marg->marg_icount = key_pool-marg->marg_args-1;
    f1_eol();                /* should be a EOL here */
    if (show_line != 0) line_to_listing();   /* display it */
    macro_pool = 0;
    macro_pool_size = 0;         /* start with fresh memory */
#ifdef DUMP_MACRO
    puts_lis("\t\tBefore the .irp macro_to_mem() call\n",1);
    dump_macro(ma,(Opcode *)0);
#endif
    if (macro_to_mem(ma,macro_name) == EOF)
    {   /* process the macro body */
        inp_str[0] = '\n';    /* display a blank line */
        inp_str[1] = 0;
        sprintf(emsg,"No terminating .ENDR for directive: %s",macro_name);
        bad_token((char *)0,emsg);
        return EOF;
    }
    ++macro_level;       /* bump macro level */
    ++macro_nesting;     /* keep 2 copies */
    marg->marg_next = marg_head; /* save ptr to previous list head */
    marg_head = marg;        /* this one becomes the top */
    marg_head->marg_ptr = ma->mac_body;  /* point to beginning of macro text */
    marg_head->marg_top = ma->mac_body;  /* point to beginning of macro text */
    marg_head->marg_end = ma->mac_end;   /* point to EOM */
    marg_head->marg_flag = 2;    /* .IRP type macro */
    marg_head->marg_count = 0;   /* loop count is 0 for first argument */
    marg_head->marg_cndlvl = condit_level;
    marg_head->marg_cndnst = condit_nest;
    marg_head->marg_cndwrd = condit_word;
    marg_head->marg_cndpol = condit_polarity;
    condit_level = 0;
    condit_nest = 0;
    condit_word = 0;
    condit_polarity = 0;
#ifdef DUMP_MACRO
    puts_lis("\t\tAfter the .irp macro_to_mem() call\n",1);
    dump_macro(ma,(Opcode *)0);  /* display the macro */
#endif
    if (macro_level == 1)
    {
#ifndef MAC_PP
        if (list_meb && !list_mes) setup_mebstats();
#endif
        list_mes = 1;     /* make sure the .ENDR shows up */
    }
    return 0;            /* continue with macro next */
}

int op_irpc( void )           /* .IRPC macro */
{
    char **key_pool,**def_pool,*dst,*first_targ;
    unsigned char *gs_flag;
    char *macro_name;
    unsigned char *mac_pool;
    int c,size,tt,nest;
    Mcall_struct *marg;
    Macargs *ma;
	int noOpenBrace;

    tt = get_token();
    if (tt != TOKEN_strng)
    {
        bad_token(tkn_ptr,"IRPC dummy argument must be a string beginning with an alpha");
    }
    size = (inp_len-(inp_ptr-inp_str));  /* get length of remaining input */
#define IRPC_NAME ".IRPC %s,<...>"
    tt = size*(2+            /* room for keywrds and defaults */
               2*sizeof(char *)+      /* room for ptrs to keywrds */
               /*   and defaults */
               1)+                /* room for gsflags */
         sizeof(Mcall_struct)+      /* room for mcall struct */
         sizeof(Macargs)+       /* room for macargs struct */
         2*token_value+sizeof(IRPC_NAME); /* room for macro name and argument name */
    mac_pool = MEM_alloc(tt);        /* pickup some memory */
    marg = (Mcall_struct *)mac_pool; /* point to argument area */
    ma = (Macargs *)(marg+1);        /* pointer to dummy macargs */
    key_pool = (char **)(ma+1);      /* get space for arg ptrs */
    def_pool = key_pool+size;        /* get space for default arg */
    gs_flag = (unsigned char *)(def_pool+size);   /* point to GS flag area */
    mac_pool = gs_flag+size;     /* point to next free area */
    macro_name = (char *)mac_pool;       /* point to area for macro name */
    sprintf(macro_name,IRPC_NAME,token_pool);    /* create macro name */
    mac_pool += strlen(macro_name)+1;

    ma->mac_keywrd = key_pool;
    ma->mac_default = def_pool;
    ma->mac_gsflag = gs_flag;
    ma->mac_numargs = 1;         /* .IRPC has only 1 replaceable argument */
    *key_pool++ = (char *)mac_pool;      /* pointer to dummy argument */
    strcpy((char *)mac_pool,token_pool);     /* copy in dummy argument name */
    mac_pool += token_value+1;       /* move pointer */
    marg->marg_args = key_pool;      /* pointer to array of ptrs to args */

#if 0
    tt =  size+              /* room for arg string */
          2*sizeof(char *)+      /* room for 2 ptrs to strings */
          sizeof(char *)+        /* room for 1 extra 0 pointer */
          sizeof(Mcall_struct)+  /* room for mcall struct */
          sizeof(Macargs)+   /* room for dummy macargs struct */
          token_value+1+         /* room for the dummy argument */
          token_value+13;        /* room for macro name */
    mac_pool = MEM_alloc(tt);        /* pickup some memory */
    marg = (Mcall_struct *)mac_pool; /* point to argument area */
    ma = (Macargs *)(marg+1);    /* pointer to dummy macargs */
    key_pool = (char **)(ma+1);      /* get space for arg ptrs */
    macro_name = (char *)(key_pool+3);   /* point to area for macro name */
    sprintf(macro_name,".IRPC %s,<...>",token_pool); /* create macro name */
    mac_pool = macro_name+14+token_value; /* move pointer */
    ma->mac_keywrd = key_pool;       /* pointer to pointer to dummy arg */
    *key_pool++ = mac_pool;      /* pointer to dummy argument */
    strcpy(mac_pool,token_pool);     /* copy in dummy argument name */
    mac_pool += token_value+1;       /* move pointer */
    marg->marg_args = key_pool;      /* pointer to array of ptrs to args */
    ma->mac_numargs = 1;         /* .IRPC has only 1 replaceable argument */
#endif

    while (isspace(*inp_ptr))
		++inp_ptr; /* skip over white space */
    if (*inp_ptr != ',')
    {       /* next thing must be a comma */
        show_bad_token(inp_ptr,"Expected a comma here. One is assumed", MSG_WARN);
    }
    else
    {
        ++inp_ptr;            /* eat the comma */
        while (isspace(*inp_ptr)) ++inp_ptr; /* skip over white space */
    }
	while (isspace(*inp_ptr))
		++inp_ptr; /* skip over white space */
    if (*inp_ptr != macro_arg_open)
    {        /* next thing must be open brace */
/*        bad_token(inp_ptr,"Expected an macro_arg_open here. One is assumed"); */
		noOpenBrace = 1;
    }
    else
    {
        ++inp_ptr;            /* eat opening bracket */
		noOpenBrace = 0;
    }
    dst = (char *)mac_pool;          /* point to start of free space */
    nest = 0;                /* assume no nesting */
    *key_pool++ = dst;           /* point to place to deposit string */
    first_targ = dst++;          /* place to deposit first char */
    *dst++ = 0;              /* and terminate it */
    *key_pool = dst;         /* record placement of real arg */
    while (1)
    {              /* for chars in string */
        c = *inp_ptr;         /* pickup char */
        if ((cttbl[c]&CT_EOL) != 0)
			break; /* stop if hit EOL */
        ++inp_ptr;            /* eat the char */
		if ( !noOpenBrace )
		{
			if ( c == macro_arg_close )
			{           /* see if EOL */
				--nest;
				if (nest < 0)
					break;       /* end of string */
			}
			if (c == macro_arg_open)
				++nest;      /* rollup nest level */
		}
		else
		{
			if ( (cttbl[c]&(CT_EOL|CT_WS|CT_SMC)) )
				 break;
		}
        *dst++ = c;           /* pass the character */
    }
    *dst = 0;                /* terminate the string */
    marg->marg_icount = dst- *key_pool;  /* argument count */
    dst = *key_pool;         /* point back to beginning of string */
    *first_targ = *dst;          /* record the first char */
    f1_eol();                /* should be a EOL here */
    if (show_line != 0)
		line_to_listing();   /* display it */
    macro_pool = 0;
    macro_pool_size = 0;         /* start with fresh memory */
#ifdef DUMP_MACRO
    puts_lis("\t\tBefore the .irpc macro_to_mem() call\n",1);
    dump_macro(ma,(Opcode *)0);
#endif
    if (macro_to_mem(ma,macro_name) == EOF)
    {   /* process the macro body */
        inp_str[0] = '\n';    /* display a blank line */
        inp_str[1] = 0;
        sprintf(emsg,"No terminating .ENDR for directive: %s",macro_name);
        bad_token((char *)0,emsg);
        return EOF;
    }
    ++macro_level;       /* bump macro level */
    ++macro_nesting;     /* keep 2 copies */
    marg->marg_next = marg_head; /* save ptr to previous list head */
    marg_head = marg;        /* this one becomes the top */
    marg_head->marg_ptr = ma->mac_body;  /* point to beginning of macro text */
    marg_head->marg_top = ma->mac_body;  /* point to beginning of macro text */
    marg_head->marg_end = ma->mac_end;   /* point to EOM */
    marg_head->marg_flag = 3;    /* .IRPC type macro */
    marg_head->marg_count = 0;   /* loop count is 0 for first argument */
    marg_head->marg_cndlvl = condit_level;
    marg_head->marg_cndnst = condit_nest;
    marg_head->marg_cndwrd = condit_word;
    marg_head->marg_cndpol = condit_polarity;
    condit_level = 0;
    condit_nest = 0;
    condit_word = 0;
    condit_polarity = 0;
#ifdef DUMP_MACRO
    puts_lis("\t\tAfter the .irpc macro_to_mem() call\n",1);
    dump_macro(ma,(Opcode *)0);
#endif
    if (macro_level == 1)
    {
#ifndef MAC_PP
        if (list_meb && !list_mes) setup_mebstats();
#endif
        list_mes = 1;     /* make sure the .ENDR shows up */
    }
    return 0;            /* continue with macro next */
}

int op_rept( void )           /* .REPT macro */
{
    char *macro_name;
    unsigned char *mac_pool;
    int tt;
    Mcall_struct *marg;
    Macargs *ma;
    long rept_count;

    tt = get_token();
    exprs(1,&EXP0);
    if (EXP0.ptr != 1 || EXP0SP->expr_code != EXPR_VALUE)
    {
        bad_token((char *)0,"Expression must resolve to absolute number. Assumed 0");
        EXP0SP->expr_value = 0;
        EXP0SP->expr_code = EXPR_VALUE;
    }
#define REPT_NAME ".REPT %ld"
    tt =  sizeof(Mcall_struct)+  /* room for mcall struct */
          sizeof(Macargs)+   /* room for dummy macargs struct */
          sizeof(REPT_NAME)+7;   /* room for macro name+ASCII repeat count+null */
    mac_pool = MEM_alloc(tt);        /* pickup some memory */
    marg = (Mcall_struct *)mac_pool; /* point to argument area */
    ma = (Macargs *)(marg+1);    /* pointer to dummy macargs */
    macro_name = (char *)(ma+1);     /* point to area for macro name */
    rept_count = EXP0SP->expr_value;
    f1_eol();                /* should be a EOL here */
    if (rept_count >=  32768 ||
        rept_count <  -32768 )
    {
        bad_token((char *)0,"Repeat count must be between -32768 and 32,767");
        if (rept_count < 0)
        {
            rept_count = -32768;
        }
        else
        {
            rept_count = 32767;
        }
    }
    sprintf(macro_name,REPT_NAME,rept_count);
    ma->mac_numargs = 0;         /* .REPT has 0 replaceable args */
	dump_hex4((long)rept_count & 0xFFFF, list_stats.listBuffer + LLIST_RPT);
#ifndef MAC_PP
    list_stats.listBuffer[LLIST_RPT-1] = '(';
    list_stats.listBuffer[LLIST_RPT+4] = ')';
#endif
    if (show_line != 0) line_to_listing();   /* display it */
    macro_pool_size = 0;
    macro_pool = 0;
    if (macro_to_mem(ma,macro_name) == EOF)
    {   /* process the macro body */
        inp_str[0] = '\n';        /* display a blank line */
        inp_str[1] = 0;
        sprintf(emsg,"No terminating .ENDR for directive: %s",macro_name);
        bad_token((char *)0,emsg);
        return EOF;
    }
    if (rept_count > 0)
    {
        ++macro_level;            /* bump macro level */
        ++macro_nesting;      /* keep 2 copies */
        marg->marg_next = marg_head;  /* save ptr to previous list head */
        marg_head = marg;         /* this one becomes the top */
        marg_head->marg_ptr = ma->mac_body;   /* point to beginning of macro text */
        marg_head->marg_top = ma->mac_body;   /* point to beginning of macro text */
        marg_head->marg_end = ma->mac_end;    /* point to EOM */
        marg_head->marg_flag = 1;         /* .REPT type macro */
        marg_head->marg_count = rept_count;   /* loop count */
        marg_head->marg_icount = rept_count;  /* loop count */
        marg_head->marg_cndlvl = condit_level;
        marg_head->marg_cndnst = condit_nest;
        marg_head->marg_cndwrd = condit_word;
        marg_head->marg_cndpol = condit_polarity;
        condit_level = 0;
        condit_nest = 0;
        condit_word = 0;
        condit_polarity = 0;
#ifndef MAC_PP
        if (macro_level == 1 && list_meb && !list_mes) setup_mebstats();
#endif
    }
    else
    {
        free_macbody(ma->mac_body);   /* free the area */
        MEM_free((char *)marg);       /* and free the local space */
    }
    if (macro_level == 1) list_mes = 1;  /* make sure the .ENDR shows up */
    return 0;                /* continue with macro next */
}

void mexit_common( int depth)
{
    if (depth == 0 && marg_head->marg_flag != 0)
    { /* .irp or .rept block */
        marg_head->marg_ptr = marg_head->marg_end;
    }
    else
    {
        if (depth > macro_level) depth = macro_level;
        macro_level -= depth;
        macro_nesting -= depth;
        while (depth > 0)
        {
            Mcall_struct *nxt;
            nxt = marg_head->marg_next;
            if (marg_head->marg_flag != 0) free_macbody(marg_head->marg_top);
            condit_level = marg_head->marg_cndlvl;
            condit_nest = marg_head->marg_cndnst;
            condit_polarity = marg_head->marg_cndpol;
            condit_word = marg_head->marg_cndwrd;
            MEM_free((char *)marg_head);       /* free up macro call memory */
            marg_head = nxt;           /* and setup caller's frame */
            --depth;
            if (marg_head == 0) break;
        }
    }
#ifndef MAC_PP
    if (macro_level == 0 && meb_stats.getting_stuff)
    {
        if (meb_stats.has_stuff || meb_stats.list_ptr != LLIST_OPC)
            display_line(&meb_stats);
        else
            clear_list(&meb_stats);
        meb_stats.getting_stuff = 0;
        clear_list(&list_stats);
    }
#endif
    return;
}

int op_mexit( void )
{
    int cnt;
    if (macro_level <= 0)
    {
        if (current_fnd->fn_next && current_fnd->fn_next->macro_level)
        {
            bad_token((char *)0,"Not legal in a .INCLUDE file called from a macro");
        }
        else
        {
            bad_token((char *)0,"Not inside a macro or rept block");
        }
        f1_eatit();
        return 0;
    }
    if ((cttbl[(int)*inp_ptr]&(CT_EOL|CT_SMC)) != 0)
    {
        if (marg_head->marg_flag != 0)
        {
            cnt = 0;
        }
        else
        {
            cnt = 1;
        }
    }
    else
    {
        get_token();      /* pickup next token */
        exprs(0,&EXP0);
        cnt = EXP0SP->expr_value;
    }
    mexit_common(cnt);
    return 0;
}

int op_rexit( void )
{
    if (macro_level <= 0)
    {
        bad_token((char *)0,"Not inside a rept block");
    }
    else
    {
        if (marg_head->marg_flag != 0)
        {
            mexit_common(0);
        }
        else
        {
            mexit_common(1);
        }
    }
    return 0;
}

int op_endm( void )
{
    bad_token((char *)0,"Not inside a macro definition or rept block");
    return 0;
}

int op_nchr( void )
{
    int cnt=0;
    SS_struct *sym_ptr;
    if (get_token() != TOKEN_strng)
    {
        bad_token(tkn_ptr,"Expected a symbol name here");
        f1_eatit();
        return 0;
    }
    *(token_pool+max_symbol_length) = 0;
    sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);  /* get a symbol */
    if (sym_ptr->flg_label)
    {
        bad_token(tkn_ptr,"Symbol name already used as a label");
        f1_eatit();
        return 0;
    }
    if (sym_ptr->flg_segment)
    {
        bad_token(tkn_ptr,"Symbol name already used as a segment name");
        f1_eatit();
        return 0;
    }
    sym_ptr->flg_ref = 1;
    if (*inp_ptr == ',') ++inp_ptr;
    while (cttbl[(int)*inp_ptr]&CT_WS) ++inp_ptr;
    if (*inp_ptr == macro_arg_open)
    {
        int nest = 1;
        ++inp_ptr;
        while ((cttbl[(int)*inp_ptr]&CT_EOL) == 0)
        {
            if (*inp_ptr == macro_arg_close)
            {
                --nest;
                if (nest <= 0)
                {
                    ++inp_ptr;
                    break;
                }
            }
            else if (*inp_ptr == macro_arg_open)
            {
                ++nest;
            }
            ++cnt;
            ++inp_ptr;
        }
    }
    else
    {
        while ((cttbl[(int)*inp_ptr]&(CT_SMC|CT_EOL)) == 0)
        {
            ++cnt;
            ++inp_ptr;
        }
    }
    sym_ptr->ss_value = cnt;
    sym_ptr->ss_exprs = 0;
    sym_ptr->flg_exprs = 0;
    sym_ptr->flg_abs = 1;
    sym_ptr->flg_defined = 1;
    list_stats.pf_value = cnt;
    list_stats.f2_flag = 1;
    return 0;
}

int op_narg( void )
{
    int cnt = 0;
    SS_struct *sym_ptr;
    if (get_token() != TOKEN_strng)
    {
        bad_token(tkn_ptr,"Expected a symbol name here");
        f1_eatit();
        return 0;
    }
    *(token_pool+max_symbol_length) = 0;
    sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);  /* get a symbol */
    if (sym_ptr->flg_label)
    {
        bad_token(tkn_ptr,"Symbol name already used as a label");
        f1_eatit();
        return 0;
    }
    if (sym_ptr->flg_segment)
    {
        bad_token(tkn_ptr,"Symbol already used as a segment name");
        f1_eatit();
        return 0;
    }
    if (macro_level > 0)
    {
        cnt = marg_head->marg_icount;
    }
    sym_ptr->ss_value = cnt;
    sym_ptr->ss_exprs = 0;
    sym_ptr->flg_exprs = 0;
    sym_ptr->flg_abs = 1;
    sym_ptr->flg_defined = 1;
    sym_ptr->flg_ref = 1;
    list_stats.pf_value = cnt;
    list_stats.f2_flag = 1;
    return 0;
}
