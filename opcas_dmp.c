/*
    opcas.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "asop_class.h"
#include "opcas_dmp.h"

#if STACK_DEBUG
#define MAX_STR_LEN 200
void dump_stack(FILE *outf, EXP_stk *eps)
{
    int len,maxLen,tag,last;
    EXPR_struct *curr,*top;
	char str[MAX_STR_LEN];
    tag = eps->tag;
	const char *operType;
	
    fprintf(outf,"\nStack %ld Tag '%c' (0x%02X) depth %d\n",
            eps - exprs_stack,
            (isgraph(tag) ? tag : '.'),
            tag,
            last = eps->ptr);

    curr = eps->stack;
    top = eps->stack+last;
	len = 0;
	maxLen = MAX_STR_LEN-1;
    for ( ; curr < top && len < MAX_STR_LEN-1; ++curr )
    {
		maxLen -= len;
		if ( maxLen < 0 )	/* Keep the analyizer happy */
			maxLen = 0;
		switch (tag = curr->expr_code)
        {
        case EXPR_VALUE :
            len += snprintf(str+len,maxLen, " 0x%X",curr->expr_value);
            break;
		case EXPR_OPER :
			tag = curr->expr_value;
			operType = getOperType(tag);
			if ( !operType )
			{
				if ( (tag&0xFF) == '!' )
				{
					tag >>= 8;
					len += snprintf(str + len, maxLen, " !%c (0x%X)", (isgraph(tag) ? tag : '.'), tag);
				}
				else
					len += snprintf(str + len, maxLen, " %c (0x%X)", (isgraph(tag) ? tag : '.'), tag);
			}
			else
				len += snprintf(str+len, maxLen, " %s", operType);
			break;
        case EXPR_SEG :
            len += snprintf(str+len, maxLen," {seg}'%s' 0x%X ",
                    (curr->expt.expt_seg)->seg_string,curr->expr_value);
            break;
        case EXPR_SYM :
            len += snprintf(str+len, maxLen, " {sym}'%s' 0x%X ",
                    (curr->expt.expt_sym)->ss_string,curr->expr_value);
            break;
        default :
            len += snprintf(str+len, maxLen, " ?? (code: 0x%X value: 0x%X)", tag, curr->expr_value);
        }   /* end case */
    }       /* end for */
	str[MAX_STR_LEN-1] = 0;
	fprintf(outf,"%s\n",str);
}       /* end routine dump_stack */
#endif	/* STACK_DEBUG */
