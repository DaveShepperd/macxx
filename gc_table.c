/*
    gc_table.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "gc_struct.h"
#include "memmgt.h"

#if defined(M_XENIX) || defined(sun) || defined(M_UNIX) || defined(unix)
    #define OPT_DELIM '-'		/* UNIX(tm) uses dashes as option delimiters */
#else
    #define OPT_DELIM '/'		/* everybody else uses slash */
#endif

#define QTBL(\
	noval,			/* t/f if no value is allowed */\
	optional,		/* t/f if value is optional */\
	number,			/* t/f is value must be a number */\
	output,			/* t/f if param is an output file */\
        string,			/* t/f if param is string (incl ws) */\
	negate,			/* t/f if param is negatible */\
	qual,			/* parameter mask */\
	name,			/* name of parameter */\
	index			/* output file index */\
) {\
noval,\
optional,\
number,\
output,\
string,\
negate, \
0, \
0, \
0, \
qual,\
name,\
index,\
0\
}

struct qual qual_tbl[] = {
#include "qual_tbl.h"
};
int max_qual = QUAL_MAX;

int gc_pass, gc_err;

char *do_option(char *str)
{
    char *s = str+1; /* eat the "/" character */
    char c,c1,loc[6],*lp=loc;    /* make room for local copy of string */
    int cnt,lc,neg=0;
    c = *s;
    c1 = *(s+1);
    if (_toupper(c) == 'N' && _toupper(c1) == 'O')
    {
        neg = 1;
        s += 2;
    }
    for (cnt = 0; cnt < sizeof(loc)-1; cnt++)
    {
        c = *s++;
        *lp++ = c = _toupper(c); /* convert the local copy to uppercase */
        if (c == 0 || c == OPT_DELIM || c == ' ' ||
            c == ',' || c == '=' || c == '\t')
        {
            --s;
            --lp;
            break;         /* out of for */
        }
    }        
    *lp = 0;         /* terminate our string */
    if (cnt == 0)
    {
        gc_err++;         /* signal an error occured */
        sprintf(emsg,"No qualifier present: {%s}",str);
        err_msg(MSG_FATAL,emsg);
    }
    else
    {
        for (lc = 0; lc < max_qual; lc++)
        {
            if (strncmp(qual_tbl[lc].name,loc,cnt) == 0) break;
#ifndef MAC_PP
            if (lc == QUAL_OUTPUT && strncmp("OBJECT",loc,cnt) == 0) break;
#endif
        }
        if (neg != 0)
        {
            if (lc < max_qual)
            {
                if (!qual_tbl[lc].negate)
                {
                    sprintf(emsg,"Option {%c%s} is not negatable.",OPT_DELIM,loc);
                    err_msg(MSG_ERROR,emsg);
                    gc_err++;
                }
                else
                {
                    qual_tbl[lc].negated = 1;
                }
            }
        }
        while (1)
        {
            c = *s++;      /* pickup char */
            if (c == 0 || c == OPT_DELIM || c == ' ' ||
                c == ',' || c == '\t')
            {
                --s;
                break;      /* out of while */
            }
            if (c == '=')
            {        /* value is a special case */
                if (lc < max_qual)
                {
                    if (qual_tbl[lc].negated)
                    {
                        sprintf(emsg,"No value allowed on negated option {%c%s}.",OPT_DELIM,loc);
                        err_msg(MSG_ERROR,emsg);
                        gc_err++;
                        continue;
                    }
                    if (!qual_tbl[lc].noval)
                    {
                        if (qual_tbl[lc].value == 0)
                        {
                            char *beg = s;
                            if (qual_tbl[lc].number)
                            {
                                while (isdigit(*s)) ++s;
                                if (s-beg == 0 || sscanf(beg,"%ld",(long *)&qual_tbl[lc].value) != 1)
                                {
                                    sprintf(emsg,"Value {%s} on {%c%s} must be number.",
                                            beg,OPT_DELIM,loc);
                                    err_msg(MSG_ERROR,emsg);
                                    gc_err++;
                                }
                            }
                            else
                            {
                                if (!qual_tbl[lc].string)
                                {
                                    while ((c = *s) && c != OPT_DELIM && c != ' ' &&
                                           c != ',' && c != '\t') ++s;
                                }
                                else
                                {
                                    s += strlen(s);
                                }
                                if ((qual_tbl[lc].value = (char *)MEM_alloc(s-beg+1)) == 0)
                                {
                                    perror("Out of memory while parsing command options");
                                    EXIT_FALSE;
                                }
                                strncpy(qual_tbl[lc].value,beg,s-beg);
                                qual_tbl[lc].value[s-beg] = 0;
                            }
                        }
                        else
                        {
                            sprintf(emsg,"Value already specified on option {%c%s}",
                                    OPT_DELIM,loc);
                            err_msg(MSG_WARN,emsg);
                        }
                    }
                    else
                    {
                        sprintf(emsg,"Value not allowed on option {%c%s}",
                                OPT_DELIM,loc);
                        err_msg(MSG_ERROR,emsg);
                        gc_err++;
                    }
                }
            }
        }     
        if (lc >= max_qual )
        {
            gc_err++;
            sprintf(emsg,"Option {%c%s} not defined",OPT_DELIM,loc);
            err_msg(MSG_FATAL,emsg);
        }
        else
        {
            qual_tbl[lc].present = 1;
            if (!qual_tbl[lc].negated)
            {
                options[qual_tbl[lc].qualif] = 1;
            }      /* -- if negated	*/
        }         /* -- if lc >= MAX	*/
    }            /* -- if cnt		*/
    return s;
}           /* -- do_option		*/
