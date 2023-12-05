/*
    sortsym.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "exproper.h"
#include "listctrl.h"
#include "memmgt.h"

static long tot_gbl,tot_udf;
static SS_struct **sorted_symbols=0;

extern SS_struct *hash[HASH_TABLE_SIZE];

static void show_undef_local(SS_struct *st)
{
    char *sn;
    sn = st->ss_string;
    if (st->flg_macLocal)
    {
        while (*sn)
        {
            if (*sn == '_')
            {
                ++sn;
                break;
            }
            ++sn;
        }
    }
	snprintf(emsg, ERRMSG_SIZE, "%s:%d: Undefined local symbol: '%s' (%s)", st->ss_fnd->fn_nam->relative_name, st->ss_line, sn, st->ss_string);
    err_msg(MSG_ERROR,emsg);
    st->flg_defined = 1;     /* define it as absolute */
    st->flg_abs = 1;
    st->ss_value = 0;
    return;
}

/**************************************************************************
 * Symbol table sort
 */
int sort_symbols(void)
/*
 * At entry:
 *	no requirements
 * At exit:
 *	returns the address of an array of pointers to ss_structs
 *	  sorted by symbol name ascending alphabetically. 
 */
{
    long i,j,ret;
    SS_struct **ls,*st;
    extern SEG_struct *find_segment();

    for (j=i=0;i<HASH_TABLE_SIZE;i++)
    {
        for (st=hash[(short)i] ; st != 0 ; st=st->ss_next)
        {
#if 0
            fprintf(stderr, "Looking at symbol {%s}, seg=%d, def=%d, gbl=%d, ref=%d, macLcl=%d, gasLcl=%d\n",
                    st->ss_string, st->flg_segment, st->flg_defined, st->flg_global, st->flg_ref, st->flg_macLocal, st->flg_gasLocal);
#endif
            if (st->flg_segment) continue;  /* ignore segments */
            if (!st->flg_defined && !st->flg_macLocal)
            {
                if ((edmask&ED_GBL) != 0) st->flg_global = 1;
                if (st->flg_ref && find_segment(st->ss_string, seg_list, seg_list_index) != 0) st->flg_global = 1;
                ++tot_udf;              /* count undefined */
            }
            if (st->flg_defined)
            {
                if (st->flg_exprs)
                {
                    EXP_stk *ep;
                    ep = st->ss_exprs;
                    ep->ptr = compress_expr(ep);
                    if (ep->ptr == 1)
                    {
                        if (ep->stack->expr_code == EXPR_VALUE)
                        {
                            st->ss_value = ep->stack->expr_value;
                            st->flg_exprs = 0;
                            st->ss_exprs = 0;
                            st->flg_abs = 1;
                        }
                        else if (ep->stack->expr_code == EXPR_SEG)
                        {
                            st->ss_seg = ep->stack->expr_seg;
                            st->ss_value = ep->stack->expr_value;
                            st->flg_exprs = 0;
                        }
                    }
                }
                if (debug)
                {
                    if (    (    st->flg_gasLocal
						     || (st->flg_label && !st->flg_global && !st->flg_register)
					        )
						 &&
                            (output_mode == OUTPUT_OBJ || output_mode == OUTPUT_OL)
					   )
                    {
                        outsym_def(st,output_mode);    /* write to obj file */
                    }
                }
                if (st->flg_macLocal)
					continue;    /* don't count local symbols */
            }
            else
            {
                if (st->flg_macLocal || st->ss_scope)
                {
                    show_undef_local(st);
                    continue;
                }
            }
            if (st->ss_scope == 0) j++;        /* count the records */
        }
    }
    if (j != 0)
    {            /* any records to sort? */
        int t = (j+1)*sizeof(SS_struct *);
        misc_pool_used += t;
        ls = (SS_struct **)MEM_alloc(t);
        sorted_symbols = ls;
        tot_gbl = j;
        for (i=ret=0;i<HASH_TABLE_SIZE;i++)
        {
            if ((st=hash[(short)i]) != 0)
            {
                do
                {
                    if (st->flg_segment) continue;  /* ignore segments */
                    if (st->flg_macLocal && st->flg_defined)
						continue;
                    if (st->ss_scope != 0) continue; /* don't sort nested syms */
                    if (ret < j) *ls++ = st;        /* record the pointer */
                    ++ret;
                } while ((st=st->ss_next) != 0);
            }
        }
        *ls = 0;              /* terminate the array */
        if (ret > j)
        {
            sprintf(emsg,"Counted %ld syms but tried to sort %ld",j,ret);
            err_msg(MSG_ERROR,emsg);
        }
        else
        {
            j = ret;
        }
        qksort(sorted_symbols,(unsigned int)j);
        ls = sorted_symbols;
        while ((st = *ls++) != 0)
        {
            if (!st->flg_global)
            {
                if ( !st->flg_defined )
                {
					snprintf(emsg,ERRMSG_SIZE,"%s:%d: Undefined symbol '%s'",
							 st->ss_fnd->fn_nam->relative_name, st->ss_line, st->ss_string);
                    err_msg(MSG_WARN,emsg);
                    st->flg_defined = 1;     /* define it as absolute */
                    st->flg_abs = 1;
                    st->ss_value = 0;
                }
            }
        }
        ls = sorted_symbols;
        while ((st = *ls++) != 0)
        {
            if (st->flg_global && (st->flg_defined || st->flg_ref))
            {
                if ( outxsym_fp )
                {
                    outsym_def(st,output_mode); /* write to obj file */
                }
#if 0
                fprintf(stderr, "Defined {%s} because: globl=%d, def=%d, ref=%d, outxsym_fp=%p\n",
                        st->ss_string, st->flg_global, st->flg_defined, st->flg_ref, outxsym_fp);
            }
            else
            {
                fprintf(stderr, "Didn't def {%s} because: globl=%d, def=%d, ref=%d\n",
                        st->ss_string, st->flg_global, st->flg_defined, st->flg_ref);
#endif
            }
        }
        if (list_sym)
        {
            char *map_page,*d;
            int col,line_page,lc;
            extern int lis_line;
            strncpy(lis_subtitle, "Symbol Table\n\n", sizeof(lis_subtitle));
            ls = sorted_symbols;
            misc_pool_used += 132*60;
            map_page = MEM_alloc(132*60);  /* get a whole map page of memory */
            while (1)
            {
                puts_lis((char *)0,0);  /* goto to TOF */
                col = 5;            /* set columns */
                line_page = lis_line;   /* set lines per page */
                if (((tot_gbl+line_page-1)/line_page) < col)
                {
                    while (1)
                    {
                        if ((line_page = (tot_gbl+col-1)/col) >= 5) break;
                        if (col == 1) break;
                        --col;
                    }
                }
                for (i=0;i<col && *ls;i++)
                {
                    for (lc=0;lc<line_page && *ls;lc++)
                    {
                        d = map_page + lc*132 + i*26; /* point to destination */
                        if ((st = *ls) != 0)
                        {
                            if (st->flg_defined)
                            {
                                SEG_struct **spp;
                                int seg_num = 0;
                                if (st->ss_seg != 0)
                                {
                                    spp = seg_list;
                                    while (*spp != 0)
                                    {
                                        if (*spp == st->ss_seg) break;
                                        ++spp;
                                    }
                                    seg_num = spp - seg_list;
                                }
                                if ( list_radix == 16 )
                                {
                                    if (seg_num > 0)
                                    {
                                        sprintf(d,"%-13.13s %08lX %02X \n",
                                                st->ss_string,st->ss_value,seg_num);
                                    }
                                    else
                                    {
                                        sprintf(d,"%-13.13s %08lX%c   \n",
                                                st->ss_string,st->ss_value,
                                                st->flg_register ? '%' : ' ');
                                    }
                                }
                                else
                                {
                                    if (seg_num > 0)
                                    {
                                        sprintf(d,"%-13.13s %06lo %03o  \n",
                                                st->ss_string,st->ss_value&0xFFFF,seg_num);
                                    }
                                    else
                                    {
                                        sprintf(d,"%-13.13s %06lo%c     \n",
                                                st->ss_string,st->ss_value&0xFFFF,
                                                st->flg_register ? '%' : ' ');
                                    }
                                }
                            }
                            else
                            {
                                if ( list_radix == 16 )
                                    sprintf(d,"%-13.13s xxxxxxxx    \n",st->ss_string);
                                else
                                    sprintf(d,"%-13.13s xxxxxx      \n",st->ss_string);
                            }
                            --tot_gbl; /* take from total */
                            ls++;
                        }     /* --if *ls != 0 */
                    }        /* --do vertical */
                }           /* --do horiz	*/
                for (i=line_page,lc=0;lc<i;lc++)
                {
                    puts_lis(map_page+lc*132,1);
                }           /* --for all lines, write */
                if (!*ls) break;    /* done 	*/
            }          /* --while(1)	*/
            MEM_free(map_page);    /* done with page */
        }             /* --if list_sym */
    }                /* --if symbols to sort */
    if (lis_fp != 0)
    {
        int seg_num;
        SEG_struct **spp,*seg_ptr;
        strncpy(lis_subtitle,"Segment summary",sizeof(lis_subtitle));
        puts_lis("\nSegment summary:\nNum  Length   Maxlen  Salign  Dalign c/o base name\n-------------------------------------------------------------------------\n",4);
        seg_num = 0;
        spp = seg_list;
        for (;seg_num<seg_list_index;++seg_num)
        {
            seg_ptr = *spp++;
            sprintf(emsg," %02X %08lX %08lX  %04X    %04X  %s    %c  %s\n",
                    seg_num,seg_ptr->seg_len,seg_ptr->seg_maxlen,
                    seg_ptr->seg_salign,seg_ptr->seg_dalign,
                    seg_ptr->flg_ovr?"OVR":"CON",
                    seg_ptr->flg_zero?'b':' ',seg_ptr->seg_string);
            puts_lis(emsg,1);
        }
		if ( options[QUAL_2_PASS] )
		{
			snprintf(emsg,sizeof(emsg),"Total instruction tags used: %d, total checks: %d\n", totalTagsUsed,totalTagsChecked);
			puts_lis(emsg,1);
		}
	}
    return 0;
}

