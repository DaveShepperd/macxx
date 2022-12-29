/*
    pass1.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "token.h"		/* define compile time constants */
#include "pst_tokens.h"
#include "listctrl.h"
#include "strsub.h"
#include "memmgt.h"
#include "exproper.h"
#include <strings.h>

#if DEBUG_TXT_INP
    #define MDEBUG(x) do { printf x; fflush(stdout); } while (0)
#else
    #define MDEBUG(x) do { ; } while (0)
#endif

int lis_line;           /* lines remaining on list page */
SEG_struct *current_section;
int include_level;      /* inclusion level */
struct str_sub *string_macros;  /* ptr to array of structs for string subs */
int strings_substituted;    /* flag indicating string substitution occured */
unsigned long current_lsb=1;    /* current local symbol block number */
unsigned long next_lsb=2;   /* next available local symbol block number */
unsigned long autogen_lsb=65000; /* autolabel for macro processing */
char emsg[ERRMSG_SIZE];         /* space to build error messages */
char *inp_str;          /* place to hold input text */
int   inp_str_size;     /* size of inp_str */
char *presub_str;       /* place to hold input before string substitution */
char *inp_ptr;          /* pointer to next place to get token */
int inp_len;            /* length of input string */
int token_type;         /* decoded token type */
unsigned char token_minus;  /* minus sign prefixed */
char cur_char;          /* first character of token */
char *tkn_ptr;          /* pointer to first character of token */
long token_value;       /* value of token */
int next_type;          /* next token character type */
char *token_pool;       /* pointer to free token memory */
int token_pool_size;        /* size of remaining free token memory */
char *token_pool_base;      /* base pointer to token pool (token_pool variable moves) */
int max_token=MAX_TOKEN;    /* length of maximum token in file */
SEG_struct *seg_pool;       /* pointer to free segment space */
SEG_struct **seg_list,**subseg_list;    /* room for some segments */
int seg_list_index;     /* index to next available segment entry in seg_list */
int seg_list_size;      /* max size of seg list */
int subseg_list_index,subseg_list_size; /* index and size of subseg list */
int seg_pool_size;      /* elements remaining in segment pool */
int no_white_space_allowed;
int squawk_syms;

#if defined(MAC68K) || defined(MAC682K)
int white_space_section;
int dotwcontext;
#endif

extern Mcall_struct *marg_head;
extern char **cmd_assems;
extern int cmd_assems_index;
extern void mexit_common( int depth);

int comma_expected;     /* 0 - comma not expected */
/* 1 - comma expected and required */
/* 2 - comma optional */

#ifdef lint
void *memset(char *ptr, char fill, int size)
{
    return;
}
void *memcpy(char *dst, char *src, int siz)
{
    return;
}
#endif
#ifndef MAC_PP
/******************************************************************
 * Pick up memory for special segment block storage
 */
SEG_struct *get_seg_mem(SS_struct **sym_ptr, char *name)
/* At entry:
 * 	sym_ptr - pointer to segment block to extend
 * At exit:
 *	seg_pool is updated to point to new free space
 *	seg_pool_size is updated to reflect the size of seg_spec_pool
 *	seg_list, seg_list_size and seg_list_index are updated
 */
{
    SEG_struct *seg_ptr;
    if (--seg_pool_size < 0)
    {
        int t = sizeof(SEG_struct)*32;
        sym_pool_used += t;
        seg_pool = (SEG_struct *)MEM_alloc(t);
        seg_pool_size = 31;
    }
    if (seg_list_index >= seg_list_size)
    {
        seg_list_size += 8;
        seg_list = (SEG_struct **)MEM_realloc((char *)seg_list,
                                              (unsigned int)(seg_list_size*(sizeof(SEG_struct *))));
    }
    *(seg_list+seg_list_index) = seg_ptr = seg_pool++;
    seg_ptr->seg_index = seg_list_index++;
    seg_ptr->seg_string = name;
    seg_ptr->seg_ident = new_identifier++;
    *sym_ptr = get_symbol_block(1);
    (*sym_ptr)->ss_string = name;
    (*sym_ptr)->ss_seg = seg_ptr;
    (*sym_ptr)->flg_segment = 1;
    (*sym_ptr)->flg_ref = 1;
    if (name == token_pool)
    {
        token_pool += token_value+1;
    }
    return(seg_ptr);
}
/******************************************************************/

/******************************************************************
 * Pick up memory for special subsegment block storage
 */
SEG_struct *get_subseg_mem( void )
/* At entry:
 * At exit:
 *	seg_pool is updated to point to new free space
 *	seg_pool_size is updated to reflect the size of seg_spec_pool
 *      subsection list is updated
 */
{
    SEG_struct *seg_ptr;
    if (--seg_pool_size < 0)
    {
        int t = sizeof(SEG_struct)*32;
        sym_pool_used += t;
        seg_pool = (SEG_struct *)MEM_alloc(t);
        seg_pool_size = 31;
    }
    if (subseg_list_index >= subseg_list_size)
    {
        subseg_list_size += 8;
        subseg_list = (SEG_struct **)MEM_realloc((char *)subseg_list,
                                                 (unsigned int)(subseg_list_size*(sizeof(SEG_struct *))));
    }
    *(subseg_list+subseg_list_index) = seg_ptr = seg_pool++;
    ++subseg_list_index;
    return seg_ptr;
}
/******************************************************************/
#endif

unsigned long record_count;
int get_text_assems;

char *get_token_pool(int amt, int flag) 
{
    token_pool_size = max_token*8;
    if (amt > token_pool_size)
        token_pool_size = 2*amt;
    if (!flag)
    {
        token_pool_base = token_pool = MEM_alloc(token_pool_size);
    }
    else
    {
        if (token_pool == token_pool_base)
        {
            token_pool_base = token_pool = MEM_realloc(token_pool_base, token_pool_size);
        }
        else
        {
            char *otp = token_pool;
            token_pool_base = token_pool = MEM_alloc(token_pool_size);
            memcpy(token_pool, otp, amt);
        }
    }
    misc_pool_used += token_pool_size;
    return token_pool;
}

/******************************************************************
 * Pick up a line of text from the input file
 */
int get_text( void )
/*
 * At entry:
 * 	No requirements. 
 * At exit:
 * 	inp_str is loaded with next input line.
 *	if EOF detected, routine returns with EOF else
 *	routine returns TRUE
 */
{
    char *strPtr;
    if (token_pool_size <= MAX_TOKEN) get_token_pool(max_token, 1);
    if (inp_str_size < MAX_LINE)
    {   /* make sure there's room */
        if (inp_str) MEM_free(inp_str);
        inp_str_size += MAX_LINE;
        inp_str = MEM_alloc(inp_str_size);
        if (presub_str)
        {
            MEM_free(presub_str);
            presub_str = NULL;
        }
    }
    inp_ptr = inp_str;       /* reset the pointer */
#if defined(MAC68K) || defined(MAC682K)
    white_space_section = 0;
    no_white_space_allowed = 0;
#endif
    if ( get_text_assems < cmd_assems_index )
    {
		if ( cmd_assems )
        {
			if ( (strPtr=cmd_assems[get_text_assems]) )
			{
				strcpy(inp_str, strPtr);
				inp_len = strlen(inp_str)+1;
				strPtr = inp_str+inp_len-1;
				*strPtr++ = '\n';
				*strPtr = 0;
	#if !defined(MAC_PP)
				if (options[QUAL_DEBUG]) dbg_line(0);
	#endif
				++get_text_assems;
				return 1;
			}
			if ( pass )
			{
				MEM_free((char *)cmd_assems);
				cmd_assems = NULL;
				cmd_assems_index = 0;
			}
        }
		get_text_assems = cmd_assems_index;
    }
gt_loop:
	strPtr = inp_str;
	inp_ptr = inp_str;
    if (macro_level == 0)
    {
        ++current_fnd->fn_line;   /* increment the source line # */
        inp_len = 0;      /* no length so far */
        while (1)
        {
            inp_str[inp_str_size-3] = 0;   /* make sure the line is null terminated */
            if (!fgets(strPtr, inp_str_size-3-inp_len, current_fnd->fn_file))
            {
                *strPtr = '\0';
                if (!feof(current_fnd->fn_file))
                {
                    MDEBUG(("Got an input error. So far, line is:\n%s\n", inp_str));
                    perror(current_fnd->fn_buff);    /* error on input */
                    EXIT_FALSE;
                }
                if (inp_len == 0)
                {
                    MDEBUG(("EOF on input"));
#if !defined(MAC_PP)
                    if (options[QUAL_DEBUG]) dbg_line(1);    /* mark eof */
#endif
                    return EOF;      /* simple EOF on input */
                }
            }
			if ( (edmask&ED_CR) )
			{
				while ( *strPtr )
					*strPtr++ &= 0x7f;       /* zap off any 8th bit */
			}
			else
			{
				char cc;
				while ( (cc = *strPtr) )
				{
					cc &= 0x7f;			/* zap off any 8th bit */
					if ( cc == '\r' )
					{
						*strPtr++ = '\n';
						*strPtr = 0;
						break;
					}
					*strPtr++ = cc;
				}
			}
            if (strPtr[-1] == '\n')
				break;				  /* if we terminated on a \n, we're done */
            if (feof(current_fnd->fn_file))
				break;				 /* terminated on an EOF */
            if (presub_str)
            {      /* we're going to realloc, so toss this one */
                MEM_free(presub_str);
                presub_str = NULL ;
            }
            inp_len = strPtr-inp_str;       /* compute the length so far */
            if (inp_len+MAX_LINE > 32767 || inp_len+MAX_LINE < 0) break;
            inp_str_size += MAX_LINE;  /* make buffer bigger */
            inp_str = MEM_realloc(inp_str, inp_str_size); /* move the buffer */
            strPtr = inp_str+inp_len;       /* move pointer */
        }
#if !defined(MAC_PP)
        if (options[QUAL_DEBUG]) dbg_line(0);
#endif
    }
    else
    {
        unsigned char *src;
        src = marg_head->marg_ptr;    /* point to macro text */
        MDEBUG(("Reading macro\n"));
        while (1)
        {
            int cc;
            cc = *src++;
            if ((cc & 0x80) != 0)
            {
                int t;
                char *ts;
                if (cc == MAC_LINK)
                {    /* end of text area? */
                    ADJ_ALIGN(src)
                    src = *(unsigned char **)src; /* link to next area */
                    MDEBUG(("\tLinked to %p\n", src));
                    continue;        /* and continue */
                }
                else if (cc == MAC_EOM)
                {  /* end of macro */
                    cc = marg_head->marg_flag;
                    if (cc == 1)
                    {        /* .rept type */
                        --marg_head->marg_count;  /* decrement the loop cnt */
                        MDEBUG(("\tEOM on .REPT macro. count now %d\n", marg_head->marg_count));
                        if (marg_head->marg_count != 0)
                        {
                            marg_head->marg_ptr = marg_head->marg_top;
                            goto gt_loop;      /* retry the get */
                        }
                    }
                    else if (cc == 2)
                    { /* .IRP */
                        char **ap,**ap1;
                        ap = marg_head->marg_args;
                        ++marg_head->marg_count;
                        MDEBUG(("\tEOM on .IRP macro. arg count now %d\n", marg_head->marg_count));
                        ap1 = ap+1+marg_head->marg_count;
                        if (*ap1 != 0)
                        {  /* not end of args */
                            *ap = *ap1;    /* reset the 0th arg ptr */
                            marg_head->marg_ptr = marg_head->marg_top; /* rewind */
                            goto gt_loop;  /* and continue */
                        }
                    }
                    else if (cc == 3)
                    { /* .irpc */
                        char **ap,**ap1,chr;
                        ap = marg_head->marg_args;
                        ap1 = ap+1;
                        ++marg_head->marg_count;
                        MDEBUG(("\tEOM on .IRPC macro. arg count now %d\n", marg_head->marg_count));
                        chr = *(*ap1+marg_head->marg_count);
                        if (chr != 0)
                        {   /* not end of args */
                            **ap = chr;    /* replace character */
                            marg_head->marg_ptr = marg_head->marg_top; /* rewind */
                            goto gt_loop;  /* and continue */
                        }
                    }
                    MDEBUG(("\tEOM on macro. Performing .mexit\n"));
                    mexit_common(1);     /* zap 1 level of macro */
                    goto gt_loop;        /* and continue */
                }               /* -- MAC_EOM */
                t = cc&0x7F;         /* get index to argument */
                ts = *(marg_head->marg_args+t); /* point to arg ptr */
                MDEBUG(("\tArgument %d substitution. Placing %s\n", t, ts));
                while ( (*strPtr++ = *ts++) )
					;    /* copy in argument */
                --strPtr;            /* backup over the null */
                continue;
            }
            *strPtr++ = cc;
            if (cc == 0)
            {
                --strPtr;       /* leave pointing to NULL */
                break;          /* done */
            }
        } 
        marg_head->marg_ptr = src;
    }
    MDEBUG(("Before string substitutions: %s\n", inp_str));
    strings_substituted = 0;
    if (string_macros != 0)
    {
        char *src, *scmp;
        char *dst;
        struct str_sub *sub;
        int cc, inp_str_resized=0;

        if (presub_str == NULL)
        {
            presub_str = MEM_alloc(inp_str_size);
        }
        src = inp_str;            /* exchange buffers */
        dst = inp_str = presub_str;
        presub_str = src;
        while ((cc = *src++) != 0)
        {
            char *dp, *tp, *tpend;
            int tic;
            if (cc != '\'' && (cttbl[cc]&CT_ALP) == 0)
            {
                *dst++ = cc;
                continue;
            }
            dp = dst;
            if (cc == '\'')
            {
                if ((cttbl[(int)*src]&CT_ALP) == 0)
                {
                    *dst++ = cc;
                    continue;
                }
                *dp++ = cc;      /* pass the tic in case token doesn't match */
                cc = *src++;     /* and pickup the next char */
                tic = 1;        /* signal token began with a tic */
            }
            else
            {
                tic = 0;        /* token didn't start with a tic */
            }
            tp = token_pool;
            tpend = tp+token_pool_size;
            if ((edmask&ED_LC) == 0)
            {
                *tp++ = _toupper(cc);    /* upcase the first char */
                *dp++ = cc;      /* incase its not a token */
                cc = *src;       /* pickup second char of token */
                while ((cttbl[(int)cc] & (CT_ALP|CT_NUM)) != 0)
                { /* while its a ALP_NUM */
                    *tp++ = _toupper(cc);
                    *dp++ = cc;
                    if (tp >= tpend)
                    {
                        int ii;
                        ii = tp-token_pool;
                        tp = get_token_pool(ii, 1) + ii;
                        tpend = token_pool + token_pool_size;
                    }
                    cc = *++src;
                }
                *tp = 0;
                scmp = token_pool;
                token_value = tp-token_pool;
            }
            else
            {
                *dp++ = cc;      /* incase its not a token */
                cc = *src;       /* pickup second char of token */
                while ((cttbl[cc] & (CT_ALP|CT_NUM)) != 0)
                { /* while its a ALP_NUM */
                    *dp++ = cc;
                    cc = *++src;
                }
                *dp = 0;
                token_value = dp-dst;
                scmp = dst;
            }
            sub = string_macros;
            for (; sub->name != 0;)
            {
                if (sub->name_len == -1)
                {
                    sub = (struct str_sub *)sub->name;
                    continue;
                }
                if (sub->name_len == token_value)
                {
                    if (strncmp(scmp,sub->name,(int)token_value) == 0)
                    {
                        if (dst+token_value > inp_str+inp_str_size)
                        {
                            int ii;
                            ii = dst-inp_str;
                            inp_str_size += inp_str_size/2;
                            inp_str = MEM_realloc(inp_str, inp_str_size);
                            dst = inp_str + ii;
                            inp_str_resized = 1;
                        }
                        strcpy(dst,sub->value);
                        strings_substituted = 1;
                        dst += sub->value_len;
                        if (tic != 0 && *src == '\'') ++src;
                        dp = dst;
                        break;
                    }
                }
                ++sub;
            }
            dst = dp;
            continue;
        }
        strPtr = dst;
        *strPtr = 0;
        if (inp_str_resized)
        {
            presub_str = MEM_realloc(presub_str, inp_str_size);    /* in case there's an error */
        }
    }
    inp_len = strPtr-inp_str;
    record_count++;
    if (*(strPtr-1) != '\n')
    {    /* last thing on line a \n? */
        if (*(strPtr-1) == '\r')
        { /* a cr is ok, though */
            *(strPtr-1) = '\n';     /* but replace it with a \n */
        }
        else
        {
            *strPtr++ = '\n';       /* nope, so add one */
            *strPtr   = '\0';       /* and terminate it too */
            if (inp_len >= inp_str_size-3)
				bad_token((char *)0,"Line too long; Truncated");
        }
    }
    if (inp_len > 2 && *(strPtr-2) == '\r')
		*(strPtr-2) = ' ';		 /* eat extra \r's */
    if (inp_str[0] == '\f')
    {
        inp_str[0] = ' ';		/* replace formfeed with space */
        lis_line = 0;
    }
    MDEBUG(("After string substitutions: %s\n", inp_str));
    inp_ptr = inp_str;       /* in case inp_str has moved */
    return(1);
}
/******************************************************************/

char lis_title[LIS_TITLE_LEN], lis_subtitle[LIS_TITLE_LEN];

void puts_titles(void)
{
	fputs(lis_title,lis_fp);      /* write title line */
	 if (!lis_subtitle[0])
	 {
		 fputs("\n\n",lis_fp);      /* no subtitle yet */
	 }
	 else
	 {
		 fputs(lis_subtitle,lis_fp);    /* write subtitle line */
	 }
	 lis_line = LIS_LINES_PER_PAGE-3;          /* reset the line counter */
	 *lis_title = '\f';        /* make first char a FF */
}

/**************************************************************************
 * PUTS_LIS - put a string to LIS file
 */
void puts_lis(char *string, int lines )
/*
 * At entry:
 *	string - pointer to text to write to map file
 *	lines - number of lines in the text (0=don't know how many)
 */
{
    int i;
    char *s;
    if (lis_fp == 0)
		return;     /* easy out if no lis file */
    if (!lis_title[0])
    {
        snprintf(lis_title,sizeof(lis_title),"\r%-40s %s %s   %s\n",
                output_files[OUT_FN_OBJ].fn_name_only,
                macxx_name,macxx_version,ascii_date);
    }
    if ((i=lines) == 0)
    {
        if ((s = string) != 0 )			   /* point to string */
		{
			while (*s)
			{
				if (*s++ == '\n')
					i++;  /* count \n's in text */
			}
		}
    }
    if (i == 0 || i > lis_line)
    {    /* room on page? */
		puts_titles();
    }
    if (i != 0)
    {
        fputs(string,lis_fp);     /* write caller's text */
        lis_line -= i;            /* take lines from total */
    }
    return;              /* done */
}

/******************************************************************/
#if !defined(MAX_LINE_ERRORS)
#define MAX_LINE_ERRORS (4)
#endif
char *line_errors[MAX_LINE_ERRORS];
char line_errors_sev[MAX_LINE_ERRORS];
int line_errors_index;
static char *btmsg;
static int btmsg_siz;

/******************************************************************
 * Display bad string.
 */
void show_bad_token( char *ptr, char *msg, int sev )
/*
 * At entry:
 * 	ptr - points to character in error in inp_str
 *	msg - pointer to string that is the error message to display
 *	sev - message severity (one of MSG_ERROR, MSG_WARN, etc.)
 * At exit:
 *	inp_str is displayed, a line terminated with '^' is displayed
 *	under inp_str to indicate error position and the error message
 * 	is displayed under that. 
 */
{
    char *err_str, *s, *is=inp_str;
    int msgsiz,fnl;
    FILE *tfp;

	if ( !pass )
		return;

	if ( pass == 1 )
    {
        tfp = lis_fp;
        lis_fp = (FILE *)0;
    }
    else
    {
        tfp = (FILE *)0;
    }
    fnl = (strlen(current_fnd->fn_name_only)+7) & -8;
    msgsiz = strlen(msg)+fnl+16;
    if (strings_substituted == 0 || !presub_str)
    {
        msgsiz += inp_len;
    }
    else
    {
        msgsiz += strlen(presub_str);
    }
    s = err_str = MEM_alloc(msgsiz+2);
    if (ptr != 0 && strings_substituted == 0)
    {
        while (is < ptr)
        {
            *s++ = (*is++ != '\t') ? ' ' : '\t';
        }
        *s++ = '^';
        *s   = '\0';
        msgsiz += s-err_str+fnl+LLIST_SIZE;
        if (btmsg_siz < msgsiz)
        {
            if (btmsg != 0) MEM_free(btmsg);
            btmsg = MEM_alloc(btmsg_siz=msgsiz);
        }
        if (include_level > 0)
        {
            sprintf(btmsg,"%-*.*s-%-7d%%s%*s%s\n%%s%s\n",
                    fnl,fnl,current_fnd->fn_name_only,current_fnd->fn_line,
                    fnl+8," ",err_str,msg);
        }
        else
        {
            if (inp_len == 0)
            {
                sprintf(btmsg,"%s\n%%s%s\n",
                    err_str, msg );
            }
            else
            {
                sprintf(btmsg,"%5d   %%s        %s\n%%s%s\n",
                    current_fnd->fn_line, err_str, msg);
            }
        }
    }
    else
    {
        msgsiz += 10;
        if (btmsg_siz < msgsiz)
        {
            if (btmsg != 0) MEM_free(btmsg);
            btmsg = MEM_alloc(btmsg_siz=msgsiz);
        }
        if (include_level > 0)
        {
            sprintf(btmsg,"%-*.*s-%-7d%%s%%s%s\n",
                    fnl,fnl,current_fnd->fn_name_only,current_fnd->fn_line,
                    msg);
        }
        else
        {
            if (inp_len == 0)
            {
                sprintf(btmsg,"%%s%s\n",msg);
            }
            else
            {
                sprintf(btmsg,"%5d   %%s%%s%s\n",current_fnd->fn_line,msg);
            }
        }
    }
    if ( !options[QUAL_ABBREV] )
    {
        err_msg(sev|MSG_CTRL|MSG_PINPSTR,btmsg);
    }
    else
    {
        sprintf(btmsg,":%d - %s\n", (ptr > inp_str) ? ptr-inp_str : 0, msg );
        err_msg(sev, btmsg);
    }
    if (pass == 1) lis_fp = tfp;
    if (tfp && line_errors_index < MAX_LINE_ERRORS)
    {
        line_errors_sev[line_errors_index] = sev;
        if (ptr != (char *)0)
        {
            sprintf(btmsg,
                    "%*.0s%s\n%%s%s\n",LLIST_SIZE," ",err_str,msg);
            line_errors[line_errors_index] = btmsg;
            btmsg = (char *)0;
            btmsg_siz = 0;
        }
        else
        {
            sprintf(btmsg,"%%s%s\n",msg);
            line_errors[line_errors_index] = btmsg;
            btmsg = (char *)0;
            btmsg_siz = 0;
        }
        ++line_errors_index;
    }
    MEM_free(err_str);
    err_str = 0;
}

void bad_token( char *ptr, char *msg )
{
    show_bad_token(ptr,msg,MSG_ERROR);
}
/******************************************************************/


/******************************************************************
 * mklocal - make a local symbol name
 */
int mklocal(int cnt)
{
    register char *rs;
    register unsigned long lsb;

    lsb = current_lsb;
    rs = token_pool;
    while (lsb > 0)
    {
        *rs++ = hexdig[lsb&15];
        lsb >>= 4;
    }
    *rs++ = '_';
    strncpy(rs,tkn_ptr,cnt);
    rs += cnt;
    *rs = 0;
    token_type = TOKEN_local;
    return rs-token_pool;
}

/******************************************************************
 * get_token - gets the next term from the input file.
 */
int get_token( void )
/*
 * At entry:
 * 	current_fnd - pointer to file descriptor structure
 *	inp_ptr - points to next source byte
 *	inp_str - contains the source line
 * At exit:
 *	The token is copied into the token pool and the routine
 *	returns a code according to the type of token processed.
 *	The variable token_value is set to the length of string
 *	type tokens or the binary value of constant type tokens.
 *	TOKEN_strng	- symbol name (any a/n string actually)
 * 	TOKEN_numb	- a number
 * 	TOKEN_local	- a local symbol name
 * 	TOKEN_oper	- arithimetic operator (+,-,<,>,...)
 *	TOKEN_ascs	- an ascii string
 * 	EOL - newline
 * 	EOF - end of file
 */
{
    char c;
    unsigned short ct,cct;

    token_value = token_minus = 0;
    while (1)
    {          /* in order to allow continuations */
#if defined(MAC68K) || defined(MAC682K)
        if ( isspace(*inp_ptr) && options[QUAL_GRNHILL] )
        {
            ++white_space_section;
        }
#endif
        while (isspace(*inp_ptr))
            ++inp_ptr; /* skip over white space */
        c = *inp_ptr;
        tkn_ptr = inp_ptr++;  /* point to beginning of token */
        cur_char = c;     /* say what the token is */
        ct = cttbl[(int)c];       /* decode the character */
#if defined(MAC68K) || defined(MAC682K)
/*        printf("Into get_token(). ct=0x%02X, c='%c', inp_ptr=\"%s\"",
            ct, c, inp_ptr ); */
        if ( options[QUAL_GRNHILL] && (white_space_section > 2) && no_white_space_allowed )
        {
            ct = CT_SMC;
        }
#endif
        if (ct & (CT_EOL|CT_SMC) )
        {
            --inp_ptr;     /* stick on EOL char */
            token_type = EOL;  /* yep, signal same */
            next_type = EOL;   /* next one too */
            return(EOL);      /* and return EOL */
        }
        if (comma_expected != 0)
        {
            if (c == ',')
            {
                comma_expected = 0; /* clear comma expected */
                continue;       /* eat the expected or optional comma */
            }
            if (comma_expected != 2)
            {
                bad_token(tkn_ptr,"Expected comma here; one is assumed");
            }
            comma_expected = 0;
        }
        cct = cctbl[(int)c];
        if (c == '$' && (edmask&ED_DOL) != 0) cct = CC_NUM;
        switch (cct)
        {        /* act on the decoded data */
        case CC_PCX: {     /* printing character */
                if (c == '\\')
                {
                    if (*inp_ptr == '\n')
                    {  /* next thing a \n? */
#if defined(MAC68K) || defined(MAC682K)
                        int s_ws_sec, s_no_ws_a;
                        s_ws_sec = white_space_section;
                        s_no_ws_a = no_white_space_allowed;
                        white_space_section = 0;
                        no_white_space_allowed = 0;
#endif
                        if (get_text() == EOF)
                            return(EOF); /* get another line */
#if defined(MAC68K) || defined(MAC682K)
                        white_space_section = s_ws_sec;
                        no_white_space_allowed = s_no_ws_a;
#endif
                        continue;     /* and keep going */
                    }                /* else fall thru to oper */
                }
#ifndef MAC_PP
                if (condit_word >= 0)
                { /* if not in unsatisfied conditional */
                    if (c == ',')
                    {
                        if (comma_expected == 0)
                        {
                            bad_token(tkn_ptr,"Unexpected comma. Ignored.");
                        }
                        continue;     /* expected or optional comma skipped */
                    }
                }
#endif
                token_type = TOKEN_pcx;
                token_value = c;
                break;
            }
        case CC_OPER: {
                token_type = TOKEN_oper;    /* this is an operator */
                token_value = c;
                break;
            }
        case CC_NUM: {         /* number */
                long tmp;
                int tmp_radix = current_radix; /* copy of local radix */
                int min_radix = 0;      /* copy of minimum radix rqd */
                int i;

                token_type = TOKEN_numb;    /* signal we've got a number */
                if (c == '0' && (*inp_ptr == 'x' || *inp_ptr == 'X'))
                {
                    tkn_ptr += 2;        /* skip over the 0x */
                    tmp_radix = 16;      /* switch to hexidecimal */
                }
                else if (c == '$')
                {
                    ++tkn_ptr;       /* eat the dollar */
                    tmp_radix = 16;      /* switch to hex */
                }
                for (i=0;i<2;++i)
                {
                    tmp = 0;
                    inp_ptr = tkn_ptr-1;
                    while (1)
                    {
                        c = *++inp_ptr;   /* get char */
                        if (c < '0') break;   /* not a digit */
                        if (c > '9')
                        {    /* could be trouble */
                            if (c < 'A') break; /* end */
                            if (c <= 'F')
                            {
                                c -= 7;     /* adjust it */
                            }
                            else
                            {
                                if (c < 'a') break; /* end */
                                if (c > 'f') break; /* end */
                                c -= 'a'-'A'+7; /* adjust for lowercase */
                            }
                        }
                        c -= '0';     /* de-ascify the char */
                        if (c > min_radix) min_radix = c;
#if 0
                        if (c >= tmp_radix)
                        { /* too big to eat? */
                            break;     /* no point to continue */
                        }
#endif
                        tmp *= tmp_radix;
                        tmp += c;
                    }
                    if (c == '.' || c == '$')
                    {
                        if (c == '$')
                        {
                            if (min_radix >= 10)
                            {
                                bad_token(tkn_ptr,"Local symbols must be decimal");
                            }
                            else
                            {
                                tmp = mklocal(inp_ptr-tkn_ptr+1);
                            }
                            ++inp_ptr;     /* eat the $ */
                            break;
                        }
                        else    /* It's a period */
                        {
#if MAC68K
                            /* Motorola has this syntax where a .W or .L can suffix an expression */
                            if ( dotwcontext )
                            {
                                char wl;
                                wl = _toupper(inp_ptr[1]);
                                if ( wl == 'W' || wl == 'L' || wl == 'B' )
                                    break;          /* And will handle it during expression processing */
                            }
#endif
                            if (min_radix >= 10)
                            {
                                bad_token(tkn_ptr,"Not a decimal number");
                            }
                        }
                        if (min_radix >= 10 || tmp_radix == 10)
                        {
                            ++inp_ptr;     /* eat the . or $ */
                            break;
                        }
                        tmp_radix = 10;   /* set the radix to 10 */
                        continue;     /* and rescan */
                    }
                    if (min_radix >= tmp_radix)
                    {
                        if (tmp_radix < 8)
                        {
                            bad_token(tkn_ptr,"Not a binary number");
                        }
                        else
                        {
                            if (tmp_radix < 10)
                            {
                                bad_token(tkn_ptr,"Not an octal number");
                            }
                            else
                            {
                                bad_token(tkn_ptr,"Not a decimal number");
                            }
                        }
                        if (min_radix < 8) tmp_radix = 8;
                        if (min_radix >= 8 && min_radix < 10) tmp_radix = 10;
                        if (min_radix >= 10) tmp_radix = 16;
                        continue;
                    }
                    break;           /* else break */
                }
                token_value = tmp;      /* we got the number */
                break;          /* out of switch */
            }          /* -- case num */
        case CC_ALP:       /* alpha-numeric */
            if ((edmask&ED_DOTLCL) != 0)
            {  /* special local symbols? */
                if (*inp_ptr == '.')
                {       /* double dot? */
                    ++inp_ptr;    /* skip the double dots */
                    while (1)
                    {   /* skip to end of variable */
                        c = *inp_ptr++;
                        if ((cttbl[(int)c] & (CT_ALP|CT_NUM)) == 0) break;
                    }
                    --inp_ptr;    /* backup 1 */
                    token_value = mklocal(inp_ptr-tkn_ptr); /* convert into local symbol */
                    break;    /* done */
                }
            }
        default: {     /* everything else is a string */
                register char *rs = token_pool;
                char *tpend = token_pool+token_pool_size;
                if ((edmask&ED_LC) != 0)
                {  /* check ED_LC once at head of string */
                    *rs++ = c;   /* string gets moved as is */
                    while (1)
                    {
                        c = *inp_ptr++;
                        if ((cttbl[(int)c] & (CT_ALP|CT_NUM)) == 0)
                            break;
                        if (rs >= tpend)
                        {
                            int ii;
                            ii = rs-token_pool;
                            rs = get_token_pool(ii, 1) + ii;
                        }
                        *rs++ = c;
                    }
                }
                else
                {
                    *rs++ = _toupper(c); /* upcase the string */
                    while (1)
                    {
                        c = *inp_ptr++;
                        if ((cttbl[(int)c] & (CT_ALP|CT_NUM)) == 0) break;
                        if (rs >= tpend)
                        {
                            int ii;
                            ii = rs-token_pool;
                            rs = get_token_pool(ii, 1) + ii;
                        }
                        *rs++ = _toupper(c);
                    }
                }
#if defined(MAC68K) || defined(MAC682K)
                /* Motorola has this syntax where a .W, .L or .B can suffix a label or expression */
                if ( dotwcontext && rs > token_pool+2 )
                {
                    c = _toupper(rs[-1]);
                    if ( rs[-2] == '.' && (c == 'W' || c == 'L' || c == 'B') )
                    {
                        rs -= 2;
                        inp_ptr -= 2;
                    }
                }
#endif
                token_value = rs - token_pool;
                if (token_value > max_token)
                    max_token = token_value;
                --inp_ptr;      /* backup source pointer */
                *rs = 0;        /* null terminate the dest string */
                token_type = TOKEN_strng;
                break;
            }          /* -- case */
        }
#if defined(MAC68K) || defined(MAC682K)
        if ( !options[QUAL_GRNHILL] || !no_white_space_allowed )
#endif
            while (isspace(*inp_ptr))
                ++inp_ptr;  /* advance over spaces */
        return(token_type);
    }                /* -- while (1) token string */
}               /* -- get_token() */

/******************************************************************
 * Ignore the contents of a line
 */
int f1_eatit( void )
/*
 * At entry:
 * 	no requirements
 * At exit:
 *	next line has been placed in inp_str
 *  returns a 0
 */
{
    while ((cttbl[(int)*inp_ptr] & (CT_EOL|CT_SMC)) ==0) ++inp_ptr;
    return 0;
}

/*******************************************************************
 * Check that there's no more crap on the line
 */

void f1_eol( void )
/*
 * At entry:
 *	no requirements
 * At exit:
 *	ptr placed at EOL
 */
{
    unsigned short ct;
    while (cttbl[(int)*inp_ptr] & CT_WS) ++inp_ptr;
    ct = cttbl[(int)*inp_ptr];
#if defined(MAC68K) || defined(MAC682K)
    if ( options[QUAL_GRNHILL] && (no_white_space_allowed || white_space_section > 2) )
        ct = CT_SMC;
#endif
    if (!(ct & (CT_EOL|CT_SMC)))
    {
        show_bad_token(inp_ptr,"End of line or comment expected here",MSG_WARN);
    }
    f1_eatit();
    return;
}

#define DEFG_SYMBOL 1
#define DEFG_GLOBAL 2
#define DEFG_LOCAL  4
#define DEFG_LABEL  8
#define DEFG_STATIC 16
/*******************************************************************
 * Define local/global symbol
 */
int f1_defg(int flag)
/*
 * At entry:
 * 	flag - see definitions of DEFG_xxx above
 * At exit:
 *	returns true if successful
 * 	symbol definition written to sym_def_file
 *
 * Command syntax:
 *	symbol =[=] expression
 *	symbol :[:]	(special case expression is current_psect + offset)
 *	symbol :[:[=[=]]]
 */
{
    int i;
    SS_struct *ptr;
#ifndef MAC_PP
    if (token_value == 1 && *token_pool == '.')
    {
        if ((flag&~DEFG_SYMBOL) != 0)
        {
            bad_token(tkn_ptr," The name '.' is reserved and cannot be redefined");
            return 0;
        }
        f1_org();         /* move the PC  (ala .=nnn) */
        return 1;
    }
#endif
    if ((flag&DEFG_LOCAL) == 0)
        *(token_pool+max_symbol_length) = 0;
    if ( (flag&DEFG_STATIC) )
        flag &= ~DEFG_GLOBAL;
    if (current_scope != 0)
    {
        if ((flag&DEFG_GLOBAL) != 0)
        {    /* declaring a global in a proc block */
            int csave;
            csave = current_scope;     /* save the current scope */
            current_scope = current_procblk = current_scopblk = 0;
            ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);        /* add level 0 symbol/label */
            current_scope = csave;     /* restore */
            current_procblk = csave&SCOPE_PROC;
            current_scopblk = csave&~SCOPE_PROC;
            flag &= ~DEFG_GLOBAL;      /* don't set global bit */
        }
        else
        {
            ptr = do_symbol(SYM_INSERT_HIGHER_SCOPE);        /* add higher level symbol */
        }
    }
    else
    {
        ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);       /* add level 0 symbol/label */
    }
    if (ptr == 0)
    {
        bad_token(tkn_ptr,"Unable to insert symbol into symbol table");
        if ((flag & DEFG_LABEL) == 0) f1_eatit();
        return 0;
    }
#if !defined(MAC_PP)
#if !defined(NO_XREF)
    if (options[QUAL_CROSS])
    {
        if ((flag&(DEFG_LABEL|DEFG_SYMBOL)) != 0 && (flag&DEFG_LOCAL) == 0)
        {
            do_xref_symbol(ptr,1);
        }
    }
#endif
    if (find_segment(ptr->ss_string, seg_list, seg_list_index) != 0)
    {
        show_bad_token(tkn_ptr, "Name already in use as a PSECT name", MSG_WARN);
    }
#endif
    if ((flag&DEFG_LABEL) != 0)
    {    /* if defining a label... */
		if ( ptr->flg_defined )
        {       /* and it's already defined... */
			if ( !ptr->flg_pass0 )
			{
				if (    !ptr->flg_exprs
					 && ptr->ss_value == current_pc
					 && ptr->ss_seg == current_section
					)
				{
					/* Assigning a label to the same location so that's okay */
					return 1;
				}
				/* and it has been previously defined in pass 1 */
				if ( include_level > 0 )
				{
					sprintf(emsg,       /* then it's nfg */
							"Label multiply defined; previously defined at %s:%d",
							ptr->ss_fnd->fn_buff,ptr->ss_line);
				}
				else
				{
					sprintf(emsg,
							"Label multiply defined; previously defined at line %d",
							ptr->ss_line);
				}
				bad_token(tkn_ptr,emsg);
				return 0;
			}
			if (    ptr->flg_exprs
				 || ptr->ss_value != current_pc
				 || ptr->ss_seg != current_section
				)
			{
				sprintf(emsg,
						"Label defined with value %s:%04lX (exprs=%d) in pass 0 and %s:%04lX in pass 1 at %s:%d",
						ptr->ss_seg ? ptr->ss_seg->seg_string:"", ptr->ss_value, ptr->flg_exprs,
						current_section->seg_string, current_pc,
						ptr->ss_fnd->fn_buff, ptr->ss_line);
				bad_token(tkn_ptr,emsg);
				return 0;
			}

        }
        ptr->flg_label = 1;   /* make it a label */
    }
    else
    {         /* if defining a symbol... */
        if (ptr->flg_label)
        { /* and it's already a label... */
            if (include_level > 0)
            {
                sprintf(emsg,       /* then it's nfg */
                        "Symbol multiply defined; previously defined at line %d\n\t%s%s",
                        ptr->ss_line,"in .INCLUDEd file ",ptr->ss_fnd->fn_buff);
            }
            else
            {
                sprintf(emsg,       /* then it's nfg */
                        "Symbol multiply defined; previously defined at line %d",
                        ptr->ss_line);
            }
            bad_token(tkn_ptr,emsg);
            f1_eatit();
            return 0;
        }
        ptr->flg_label = 0;   /* else signal that it's a symbol */
    }
    ptr->flg_defined = 1;    /* signal symbol or label is defined */
	ptr->flg_pass0 = !pass;  /* and signal which pass it was defined in */
#if 0				/* definition doesn't set reference bit */
    ptr->flg_ref = 1;        /* signal symbol is referenced */
#endif
    ptr->flg_local = ((flag&DEFG_LOCAL) != 0);
    if (ptr->flg_local && (flag&DEFG_GLOBAL))
    {
        bad_token(tkn_ptr,"Local symbol cannot be made global");
        flag &= ~DEFG_GLOBAL;
    }
    if ( !ptr->flg_static )
        ptr->flg_global |= ((flag&DEFG_GLOBAL) != 0); /* sticky local/global flag */
    ptr->ss_fnd = current_fnd;   /* record the file that defined it */
    ptr->ss_line = current_fnd->fn_line; /* and the line # of same */
    if ((flag & DEFG_SYMBOL) != 0)
    { /* if symbol... */
        if (get_token() == EOL)
        {     /* pickup the next token */
            bad_token(tkn_ptr, "Didn't expect EOL here");
            return -1;
        }
#if defined(MAC68K) || defined(MAC682K)
        if ( options[QUAL_GRNHILL] )
        {
            white_space_section = 2;
            no_white_space_allowed = 1;
        }
#endif
        if (exprs(1,&EXP0) < 1)
		{
			return(-1); /* get a global expression */
		}
        i = EXP0.ptr;
        ptr->flg_register = EXP0.register_reference;
        ptr->flg_regmask = EXP0.register_mask;
        if (i == 1)
        {     /* if only 1 term... */
            if (EXP0SP->expr_code == EXPR_VALUE)
            {
                ptr->ss_value = EXP0SP->expr_value;
                ptr->flg_abs = 1;
                ptr->flg_exprs = 0;
                ptr->ss_seg = 0;    /* no expression involved */
                i =0;
            }
            if (EXP0SP->expr_code == EXPR_SEG)
            {
                ptr->ss_seg = EXP0SP->expr_seg;
                ptr->ss_value = EXP0SP->expr_value;
                ptr->flg_exprs = 0;
                if (ptr->ss_seg->flg_abs)
                {
                    ptr->ss_seg = 0;
                    ptr->flg_abs = 1;
                }
                i =0;
            }
        }
        if (i > 0)
        {          /* if 1 or more terms... */
            char *src,*dst;        /* mem pointers */
            EXP_stk *exp_ptr;
            int cnt;
            ptr->flg_exprs = 1;    /* signal that there's an expression */
            cnt = i*sizeof(EXPR_struct);
            sym_pool_used += cnt;
            exp_ptr = (EXP_stk *)MEM_alloc(cnt+(int)sizeof(EXP_stk));
            exp_ptr->ptr = i;      /* set the length */
            exp_ptr->stack = (EXPR_struct *)(exp_ptr+1); /* point to stack */
            ptr->ss_exprs = exp_ptr;   /* tell symbol where expression is */
            dst = (char *)exp_ptr->stack;  /* point to area to load expression */
            src = (char *)EXP0SP;      /* point to source expression */
            memcpy(dst,src,cnt);       /* copy all the data */
            return 1;          /* and quit */
        }
    }
    else
    {             /* if label... */
        ptr->ss_seg = current_section;    /* always defined as seg + offset */
        ptr->ss_value = current_pc;
        ptr->flg_exprs = 0;
        current_section->flg_reference = 1; /* signal that segment referenced */
    }
    return 1;
}


/*******************************************************************
 * Set program transfer address
 */
int op_end( void )
/*
 * At entry:
 * At exit:
 *	tmp file updated with start command
 *
 * Command syntax:
 *	.end [expression]
 */
{
#ifndef MAC_PP
    EXP0.ptr = 0;
    if (get_token() != EOL)
    {    /* pickup the token */
        if (exprs(1,&EXP0) < 1)
        {
            bad_token(tkn_ptr,"Invalid transfer address expression");
        }
    }
    if (EXP0.ptr < 1)
    {
        EXP0SP->expr_code = EXPR_VALUE;
        EXP0SP->expr_value = 1;
        EXP0.ptr = 1;
    }
    endlin();            /* flush the opcode buffer */
    write_to_tmp(TMP_START,0,&EXP0,0);
#else
    f1_eatit();
#endif
    return 2;
}

#ifndef MAC_PP
/*******************************************************************
 * Set origin (program PC).
 */
int f1_org( void )
/*
 * At entry:
 *	flag - not used
 * At exit:
 *	tmp file updated with origin command
 *
 * Command syntax:
 *	. = n
 */
{
    int type;
    get_token();         /* pickup next token */
    exprs(1,&EXP0);      /* evaluate the expression */
    type = EXP0SP->expr_code;
    if (EXP0.ptr != 1 || (type != EXPR_VALUE && type != EXPR_SEG))
    {
        bad_token((char *)0,"Expression must resolve to a single defined term");
        return 0;
    }
    if (current_section->seg_pc > current_section->seg_len)
    {
        current_section->seg_len = current_section->seg_pc;
    }
    if (type == EXPR_VALUE)
    {    /* setting to an absolute value */
        if (!current_section->flg_abs)
        {
            bad_token((char *)0,"Can't set PC to absolute value in relative section");
        }
    }
    else
    {
        if (current_section != EXP0SP->expr_seg)
        {
            bad_token((char *)0,"Can't move PC to different section with .=");
            return 0;
        }
    }
    current_offset = EXP0SP->expr_value;
    return 2;
}
#endif

/******************************************************************
 * Process symbol type token. This token has the syntax of:
 *	string
 */
SS_struct *do_symbol(SymInsertFlag_t flag)
/*
 * At entry:
 *	flag -
 *      0 if not to insert into symbol table.
 *		1 if to insert into symbol table if symbol not found
 *		2 if to insert into symbol table even if symbol found
 *		3 if to insert into symbol table at higher scope
 *      4 if to insert into symbol table as a static local
 * 	token_type must be set to type of TOKEN_strng.
 *	The string is made permanent in the token pool, the string (symbol) is added to
 *	the hashed symbol table and a symbol structure is picked up
 *	from free pool.
 * At exit:
 *	returns 0 if error else returns pointer to symbol block
 */
{
    SS_struct *sym_ptr;
    if (flag && token_value)
    {
        sym_ptr = sym_lookup(token_pool, flag);
    }
    else
    {
        sym_ptr = get_symbol_block(1);
        new_symbol = 1;
    }
	
    switch (new_symbol)
    {
    case SYM_ADD:           /* symbol is added */
    case SYM_ADD|SYM_DUP:   /* symbol added and is a duplicate */
    case SYM_FIRST|SYM_ADD: /* symbol is added, first in the hash table */
		{
            int cnt;
            sym_ptr->ss_fnd = current_fnd;
            if (new_symbol != 5)
            {
                cnt = strlen(token_pool)+1; /* compute length of string */
                token_pool += cnt;      /* move the pointer */
                token_pool_size -= cnt; /* and size */
            }
			sym_ptr->ss_scope = current_scope;
            if (token_type == TOKEN_local)
                sym_ptr->flg_local = 1;
            sym_ptr->ss_line = current_fnd->fn_line;   /* and the line # of same */
        }
		/* FALL through to no symbol case */
    case 0:
		{        /* no symbol added */
#if !defined(MAC_PP) && !defined(NO_XREF)
            if (options[QUAL_CROSS]) do_xref_symbol(sym_ptr,0);
#endif
            break;     /* done */
        }
    default: {
            sprintf(emsg,
                    "Internal error. {new_symbol} invalid (%d)\n\t%s%s%s%s",
                    new_symbol,"while processing {",sym_ptr->ss_string,
                    "} in file ",current_fnd->fn_buff);
            err_msg(MSG_FATAL,emsg);
        }
    }
    if ( flag == SYM_INSERT_LOCAL )
    {
        sym_ptr->flg_static = 1;
        sym_ptr->flg_global = 0;       /* Incase this was previously set */
    }
#if !defined(MAC_PP)
					if ( squawk_syms )
					{
						snprintf(emsg, sizeof(emsg), "do_symbol(): Found symbol '%s' at %p. next=%p, defined=%d, exprs=%d, segment=%d, more=%d, value=0x%lX, segPtr=%p",
								 sym_ptr->ss_string,
								 (void *)sym_ptr,
								 (void *)sym_ptr->ss_next,
								 sym_ptr->flg_defined,
								 sym_ptr->flg_exprs,
								 sym_ptr->flg_segment,
								 sym_ptr->flg_more,
								 sym_ptr->ss_value,
								 (void *)sym_ptr->ss_seg);
						show_bad_token(NULL,emsg, MSG_WARN);
					}
#endif
    return sym_ptr;
}

int expr_message_tag = 0;
int binary_output_flag;

#ifdef MAC_PP
char *inp_str_partial;
static int line_is_empty;

static void macpp_line( void ) {
    static FN_struct *old_fn;
    static int old_line_no;

    if (condit_word >= 0)
    {
        int need_line_no = 0;
        if ( old_fn != current_fnd ) need_line_no = 1;
        if ( (old_line_no != current_fnd->fn_line-1)) need_line_no = 1;
        if ( need_line_no && strlen(inp_str) > 1)
        {
            char *np;
            np = current_fnd->fn_name_only;
            if (np == 0 || strcmp(np,"") == 0) np = current_fnd->fn_buff;
            fprintf(obj_fp, "# %d \"%s\"\n", current_fnd->fn_line, np);
            old_line_no = current_fnd->fn_line;
            old_fn = current_fnd;
        }
        else
        {
            if (old_fn == current_fnd && old_line_no == (current_fnd->fn_line-1))
            {
                ++old_line_no;
            }
        }
    }
}
#endif

static void found_symbol( int gbl_flg, int tokt )
{
    int c;

    gbl_flg |= DEFG_SYMBOL;
    ++inp_ptr;          /* eat the = */
    c = *inp_ptr;       /* get the next char */
    if (condit_word < 0)
    {  /* unsatisfied conditional */
        if (list_cnd)
        {
			list_stats.listBuffer[LLIST_USC] = 'X';
#ifdef MAC_PP
            binary_output_flag = 1; /* in case we're in a macro */
#endif
        }
        f1_eatit();      /* eat the line */
        return;
    }
    if (c == '=')
    {
        ++inp_ptr;       /* eat the second = */
        gbl_flg |= DEFG_GLOBAL;  /* and set the global bit */
    }
    no_white_space_allowed = options[QUAL_GRNHILL] ? 1 : 0;
    if (tokt == TOKEN_local)
        gbl_flg |= DEFG_LOCAL;
#ifdef MAC_PP
    if ((gbl_flg&DEFG_GLOBAL) == 0)    /* not a double = */
    {
        binary_output_flag = 1;  /* in case we're in a macro */
        f1_eatit();      /* eat the line */
        if (obj_fp != 0)
        {
            if (options[QUAL_LINE]) macpp_line();
            fputs(inp_str_partial,obj_fp);
        }
        return;
    }
#endif
    gbl_flg = f1_defg(gbl_flg); /* define a symbol */
    list_stats.pf_value = EXP0.psuedo_value;
    if (macro_nesting > 0 && list_me)
    {
        show_line = 1;
    }
    if ((show_line&gbl_flg) != 0)
    {
        if (EXP0.ptr > 1)
        {
            list_stats.f1_flag = 'x'<<8;
        }
        else
        {
            if (EXP0SP->expr_code == EXPR_SEG)
            {
                list_stats.f1_flag = '\''<<8;
            }
            else
            {
                list_stats.f1_flag = 1;
            }
        }
    }
    f1_eol();           /* better be at eol */
}

/*******************************************************************
 * Main entry for PASS1 processing
 */
void pass1( int file_cnt)
/*
 * At entry:
 *	no requirements
 * At exit:
 *	lots of stuff done. Symbol tables built, temp files filled
 *	with text, etc.
 */
{
    int tkn_flg= 1,tokt;
    char c,save_c;
    Opcode *opc;
#if !defined(MAC_PP)
    SS_struct *sym_ptr;
#endif

    if (file_cnt == 0)
    {
#ifndef MAC_PP
		if ( !options[QUAL_2_PASS] )
		{
			current_section = get_seg_mem(&sym_ptr, ".ABS.");
			current_section->flg_abs = 1;
			current_section->flg_ovr = 1;
			current_section->flg_based = 1;
			current_section->seg_salign = macxx_abs_salign;
			current_section->seg_dalign = macxx_abs_dalign;
			sym_ptr = sym_lookup(current_section->seg_string, SYM_INSERT_IF_NOT_FOUND);
			sym_ptr->flg_global = 1;
			current_section = get_seg_mem(&sym_ptr, ".REL.");
			current_section->seg_salign = macxx_rel_salign;
			current_section->seg_dalign = macxx_rel_dalign;
			sym_ptr = sym_lookup(current_section->seg_string, SYM_INSERT_IF_NOT_FOUND);
			sym_ptr->flg_global = 1;
		}
		else
		{
			current_section = find_segment(".REL.",seg_list,seg_list_index);
		}
#endif
        opcinit();            /* seed the opcode table */
    }
    expr_message_tag = 1;    /* signal not to display messages */
    if (get_text() == EOF)
        return;
#if defined(MAC_PP)
    inp_str_partial = inp_str;
    line_is_empty = 1;
#endif
    show_line = (list_level >= 0) ? 1 : 0;
    list_stats.line_no = current_fnd->fn_line;
    list_stats.include_level = include_level;
#if 0
    current_fnd->fn_line = 0;
#endif
    while (1)
    {
        comma_expected = 0;   /* no commas expected */
        tokt = tkn_flg ? get_token() : token_type;
        tkn_flg = 1;          /* assume to get next token */
        if (tokt == EOF)
        {
#ifndef MAC_PP
            if (current_section->seg_pc > current_section->seg_len)
            {
                current_section->seg_len = current_section->seg_pc;
            }
#endif
            return;
        }
        if (tokt == EOL)
        {
#if defined(MAC_PP)
            if (obj_fp != 0 && 
                (inp_ptr != inp_str || *inp_ptr != ';') &&
                line_is_empty != 0 &&
                condit_word >= 0)
            {
                if (options[QUAL_LINE]) macpp_line();
                fputs(inp_str,obj_fp);
            }
#endif
            if (lis_fp != 0)
            {
                if (line_errors_index != 0) show_line = 1;
                if (show_line == 0)
                {
                    if (macro_nesting > 0 &&
#ifndef MAC_PP
                        list_meb &&
#else
                        list_mes && 
#endif
                        binary_output_flag != 0)
                    {
                        show_line = 1;
                    }
                }
                if (show_line != 0)
                {
                    line_to_listing();
                }
                else
                {
					memset(list_stats.listBuffer, ' ', LLIST_SIZE);
					list_stats.listBuffer[LLIST_SIZE] = 0;
                    list_stats.pc_flag = 0;
                    list_stats.f1_flag = 0;
                    list_stats.f2_flag = 0;
#ifndef MAC_PP
                    list_stats.list_ptr = LLIST_OPC;
#endif
                }
                lm_bits.list_mask = qued_lm_bits.list_mask;
            }
            if (get_text() == EOF)
            {
                token_type = EOF;
                tkn_flg = 0;
                continue;
            }
            binary_output_flag = 0;    /* signal no binary output */
            show_line = 0;         /* assume not to print */
            if (macro_level > 0)
            {
                list_stats.line_no = 0;
            }
            else
            {
                list_stats.line_no = current_fnd->fn_line;
            }
            list_stats.include_level = include_level;
            if (list_level > 0 || (list_level == 0 && (macro_nesting == 0 || list_me) && 
                                   (condit_word >= 0 || list_cnd)))
            {
                show_line = 1;
            }
#ifdef MAC_PP
            inp_str_partial = inp_str;
            line_is_empty = 1;     /* assume the line is empty */
#endif
            continue;
        }
#if defined(MAC_PP)
        line_is_empty = 0;        /* line is not empty */
#endif
#if defined(MAC68K) || defined(MAC682K)
        dotwcontext = 0;
/*        printf("Processing token=\"%s\", type=%d, rest of line: %s",
            token_pool, tokt, tkn_ptr ); */
#endif
        if (tokt == TOKEN_strng || tokt == TOKEN_local)
        {
            int gbl_flg = 0;       /* assume not global */
            c = *inp_ptr;          /* pickup next item */
            if (c == ':'
#if defined(MAC68K) || defined(MAC682K)
                || ( options[QUAL_GRNHILL] 
                     && !white_space_section
                     && tokt == TOKEN_strng
                     && (token_pool[0] == '.' && token_pool[1] == 'L')
                   )
#endif
               )
            {        /* it might be a label */
                gbl_flg |= DEFG_LABEL;  /* it's a label */
                if ( c == ':' )
                {
                    ++inp_ptr;          /* eat the : */
                    c = *inp_ptr;       /* get the next char */
                    if (c == ':')
                    {
                        ++inp_ptr;       /* eat the second : */
                        gbl_flg |= DEFG_GLOBAL;  /* and set the global bit */
                    }
                    c = *inp_ptr;       /* get the next char */
                }
#ifndef MAC_PP
                if (c == '=')
                {
                    gbl_flg &= ~DEFG_LABEL;  /* it's not a label */
                    found_symbol(gbl_flg, tokt); /* a := construct? */
                    continue;
                }
#endif
                if (condit_word < 0)
                {
#if defined(MAC68K) || defined(MAC682K)
                    white_space_section = 3;
                    no_white_space_allowed = 0;
#endif
                    continue;        /* in unsatisfied conditional */
                }
#ifdef MAC_PP
                if (obj_fp != 0)
                {
                    char save0;
                    save0 = *inp_ptr;
                    *inp_ptr = 0;
                    if (options[QUAL_LINE])
                    {
                        macpp_line();
                        fprintf(obj_fp, "%s\n", inp_str_partial);
                    }
                    else
                    {
                        fputs(inp_str_partial, obj_fp);
                    }
                    *inp_ptr = save0;
                    inp_str_partial = inp_ptr;
                }
#else
                if (tokt == TOKEN_local)
                {
                    gbl_flg |= DEFG_LOCAL;
                }
                else if ((ED_LSB & edmask) == 0)
                {
                    current_lsb = next_lsb++;
                    autogen_lsb=65000;   /* autolabel for macro processing */
                }
                f1_defg(gbl_flg);   /* define a label */
                list_stats.pc = current_offset;
                list_stats.pc_flag = 1;
#if defined(MAC68K)
                ++white_space_section;
#endif
#endif
                continue;       /* rest of line is ok */
            }
            if (c == '=')
            {
                found_symbol(gbl_flg, tokt);
                continue;
            }
#if defined(MAC68K) || defined(MAC682K)
            if ( !white_space_section )
            {
                char *einp = tkn_ptr + strlen(token_pool);
/*                printf("tkn_ptr = %p, inp_ptr = %p, einp=%p, token=%s\n",
                    tkn_ptr, inp_ptr, einp, token_pool);
*/                    
                if ( isspace(*einp) )
                {
                    ++white_space_section;
                    c = *inp_ptr;
                    if ( (cttbl[(int)c]&CT_ALP) ) /* Peek at next token */
                    {
                        if ( (!strncasecmp(inp_ptr,"EQU",3) || !strncasecmp(inp_ptr,"SET",3))
                             && (cttbl[(int)inp_ptr[3]]&(CT_WS|CT_SMC|CT_EOL))
                           )
                        {
/*							printf("Found EQU. tkn_ptr='%s'\n", tkn_ptr); */
							if ( strncasecmp(tkn_ptr,".define",7) || !(cttbl[(int)tkn_ptr[7]]&(CT_WS)) )
							{
								inp_ptr += 2;           /*  found_symbol adds one too */
								gbl_flg &= ~DEFG_LABEL;  /* it's not a label */
								found_symbol(gbl_flg, tokt);
								continue;
							}
                        }
                    }
                    if (    options[QUAL_GRNHILL]
						 && tokt == TOKEN_strng
						 && token_pool[0] != '.'
						 && (strncasecmp(tkn_ptr,".define",7) || !(cttbl[(int)tkn_ptr[7]]&(CT_WS)))
					   )
                    {
                        if (condit_word < 0)
                        {
                            white_space_section = 3;
                            no_white_space_allowed = 0;
                            continue;        /* in unsatisfied conditional */
                        }
                        gbl_flg |= DEFG_LABEL;  /* it's a label */
                        f1_defg(gbl_flg);   /* define a label */
                        list_stats.pc = current_offset;
                        list_stats.pc_flag = 1;
                        continue;
                    }
                }
            }
#endif
            save_c = *(token_pool+max_opcode_length);
            *(token_pool+max_opcode_length) = 0;
            if ((opc = opcode_lookup(token_pool,0)) != 0)
            {
                int old_nest = condit_nest;
                long old_condit = condit_word;
                if ((opc->op_class&DFLPST) != 0)
                {
                    Opcode *newopc = 0;
                    if (get_token() == TOKEN_strng)
                    {
                        newopc = opcode_lookup(token_pool,0);
                        if (newopc != 0 && (newopc->op_class&OP_CLASS_OPC) == 0)
                        {
                            if (newopc->op_next != 0 &&
                                strcmp(newopc->op_next->op_name,newopc->op_name) == 0)
                            {
                                opc = newopc->op_next;
                            }
                            else
                            {
                                newopc = 0;
                            }
                        }
                        else
                        {
                            opc = newopc;
                        }
                    }
                    if (newopc == (Opcode *)0)
                    {
                        bad_token(tkn_ptr,"No such opcode");
                        f1_eatit();
                        continue;
                    }
                }
                if ((opc->op_class&OP_CLASS_OPC) == 0 &&
                    (opc->op_class&DFLCND) != 0)
                {
                    if ((opc->op_class&DFLPARAM) != 0)
                    {
                        (*opc->op_func)(opc); /* do the conditional */
                    }
                    else
                    {
                        (*opc->op_func)();    /* do the conditional */
                    }
                    show_line = (line_errors_index != 0) ||
                                (list_cnd && ((macro_nesting == 0) || list_mes));
                    if (show_line)
                    {
                        if (condit_word < 0 && old_condit < 0)
                        {
							list_stats.listBuffer[LLIST_USC] = 'X';
                        }
                        else
                        {
                            if (old_nest >= condit_nest)
                            {
                                --old_nest;
                            }
                            if ((opc->op_class&(~DFLCND)) == 0)
                            {
                                sprintf(list_stats.listBuffer+LLIST_CND,"(%d)",old_nest);
                            }
                        }
#ifdef MAC_PP
                        binary_output_flag = list_cnd; /* in case we're in a macro */
#endif
                    }
                    continue;        /* and continue */
                }
                if (condit_word < 0)
                {
                    if ((list_mes == 0 && macro_nesting > 0) || list_cnd == 0)
						show_line = 0;
                    if (show_line != 0)
						list_stats.listBuffer[LLIST_USC] = 'X';
                    f1_eatit();      /* eat rest of line */
#ifdef MAC_PP
                    binary_output_flag = show_line;  /* in case we're in a macro */
#endif
                    continue;        /* and proceed */
                }
#ifndef MAC_PP
                list_stats.pc = current_offset;
#endif
                if (opc->op_class &OP_CLASS_OPC)
                {  /* if bit 15 set, it's an opcode */
#ifdef MAC_PP
                    if (obj_fp != 0)
                    {
                        if (options[QUAL_LINE]) macpp_line();
                        fputs(inp_str_partial,obj_fp);
                    }
                    binary_output_flag = 1;
                    f1_eatit();
#else
                    do_opcode(opc);
                    list_stats.pc_flag = 1;      /* signal to print the PC */
#endif
                }
                else
                {
                    if (opc->op_class & OP_CLASS_MAC)
                    {  /* if bit 14 set, it's a macro */
                        macro_call(opc);
                        f1_eatit();           /* eat rest of line */
#ifdef MAC_PP
                        binary_output_flag = 1;   /* in case we're in a macro */
#endif
                    }
                    else
                    {
                        int rv;
#if defined(MAC68K) || defined(MAC682K)
                        if ( options[QUAL_GRNHILL] )
                        {
                            no_white_space_allowed = 1;
                        }
#endif
                        if ((opc->op_class&DFLPARAM) != 0)
                        {
                            rv = (*opc->op_func)(opc); /* do the psuedo-op */
                        }
                        else
                        {
                            rv = (*opc->op_func)();    /* do the psuedo-op */
                        }
                        if ((opc->op_class&DFLIIF) != 0)
                        {
                            if (rv == 0)
                            {
                                if ((list_mes == 0 && macro_nesting > 0) ||
                                    list_cnd == 0) show_line = 0;
                                if (show_line != 0) list_stats.listBuffer[LLIST_USC] = 'X';
                                f1_eatit();     /* eat rest of line */
                            }
                            continue;
                        }
                        list_stats.pc_flag |= rv&1;
                        if ((rv&2) != 0)
                        {
                            list_stats.f2_flag = 1;
                            list_stats.pf_value = EXP0.psuedo_value;
                        }
#ifdef MAC_PP
                        if ((opc->op_class&DFLMAC) == 0)
                        {
                            binary_output_flag = 1;    /* in case we're in a macro */
                        }
#endif
                    }
                }
                f1_eol();
                continue;
            }              /* -- found opcode/psuedo-op/macro */
            *(token_pool+max_opcode_length) = save_c;
        }                 /* -- type is string */
        if (condit_word < 0)
        {
            if (list_cnd) list_stats.listBuffer[LLIST_USC] = 'X';
            f1_eatit();            /* eat rest of line */
#ifdef MAC_PP
            binary_output_flag = list_cnd; /* in case we're in a macro */
#endif
        }
        else
        {
            inp_ptr = tkn_ptr;     /* backup and */
            if ( *inp_ptr == '*' || *inp_ptr == '#' )
            {
#if defined(MAC_PP)
                binary_output_flag = 1; /* in case we're in a macro */
                if (obj_fp != 0)
                {
                    if (options[QUAL_LINE]) macpp_line();
                    fputs(inp_str_partial,obj_fp);
                }
#endif
                f1_eatit();         /* eat the line */
                continue;
            }
#ifndef MAC_PP
            if ( (edmask&(ED_WRD|ED_BYT)) )
            {       /* .word or .byte default enabled? */
				if ( (edmask&ED_BYT) )
					op_byte();          /* pretend its a .byte psuedo-op */
				else
					op_word();          /* pretend its a .word psuedo-op */
                list_stats.pc_flag = 1;
                f1_eol();           /* better be at eol */
            }
            else
            {
                bad_token(tkn_ptr,"Undefined opcode, macro or psuedo_op");
                f1_eatit();         /* eat the line */
            }
#else
            if (obj_fp != 0)
            {
                if (options[QUAL_LINE]) macpp_line();
                fputs(inp_str_partial,obj_fp);
            }
            binary_output_flag = 1;
            f1_eatit();            /* ignore line */
#endif
        }
        continue;
    }
}
