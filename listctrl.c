/*
    listctrl.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "listctrl.h"
#include "memmgt.h"

int show_line = 1;
int list_level;
int list_radix=16;
void (*reset_list_params)(int onoff);
#ifndef MAC_PP
    #if 0
        #define LLIST_OPC	14	/* opcode */
        #define LLIST_OPR	17	/* operands */
    #endif
int LLIST_OPC=14;
int LLIST_OPR=17;
char listing_meb[LLIST_MAXSRC+2] = "     1";
char listing_line[LLIST_MAXSRC+2] = "     1";
LIST_stat meb_stats = { listing_meb, listing_line+LLIST_MAXSRC+2};
#else
char listing_line[LLIST_MAXSRC+2] = "    1";
#endif
LIST_stat list_stats = { listing_line, listing_line+LLIST_MAXSRC+2};
union list_mask lm_bits,saved_lm_bits,qued_lm_bits;

int list_init(int onoff)
{
    if (onoff == 0)
    {
        list_level = -1;
        lm_bits.list_mask = 0;
        saved_lm_bits.list_mask = 0;
    }
    else
    {
        list_level = 0;
        lm_bits.list_mask = saved_lm_bits.list_mask = 
                            macxx_lm_default;
    }
    qued_lm_bits.list_mask = lm_bits.list_mask;
#ifndef MAC_PP
    list_stats.list_ptr = LLIST_OPC;
    meb_stats.list_ptr = LLIST_OPC;
#endif
    show_line = 1;       /* assume to show it */
    return 0;
}

static struct
{
    char *string;
    unsigned short flag;
} list_stuff[] = {
    { "BEX",LIST_BEX},      /* display binary extensions */
    { "BIN",LIST_BIN},      /* display binary */
    { "BYT",LIST_MES},      /* display macro expansion source */
    { "CND",LIST_CND},      /* display unsatisfied conditionals */
    { "COD",LIST_COD},      /* display binary "as stored" */
    { "COM",LIST_COM},      /* display comments */
    { "LD",LIST_LD},        /* display listing directives */
    { "LOC",LIST_LOC},      /* display location counter */
    { "MC",LIST_MC},        /* display macro calls */
    { "MD",LIST_MD},        /* display macro definitions */
    { "ME",LIST_ME|LIST_MEB|LIST_MES}, /* display macro expansions */
    { "MEB",LIST_MEB},      /* display macro expansions which produce binary */
    { "MES",LIST_MES},      /* display macro expansion source */
    { "OCT",0},             /* display address and object data in octal */
    { "SEQ",LIST_SEQ},      /* display sequence numbers */
    { "SRC",LIST_SRC},      /* display source */
    { "SYM",LIST_SYM},      /* display symbol table */
    { "TOC",LIST_TOC},      /* display table of contents */
    { 0,0}
};

static int op_list_common( int onoff)
{
    int i;
    if (lis_fp == 0)
    {
        f1_eatit();
        return 1;
    }
    if ((cttbl[(int)*inp_ptr]&(CT_SMC|CT_EOL)) != 0)
    {
        list_level += onoff;
        if (list_level == 0)
        {
            lm_bits.list_mask = saved_lm_bits.list_mask;
        }
        else if (list_level == -1 || list_level == 1)
        {
            if (onoff > 0)
            {
                lm_bits.list_mask |= 
                LIST_CND|LIST_LD|LIST_MC|LIST_MD|LIST_ME|LIST_SRC|LIST_MES|LIST_BIN|LIST_LOC;
            }
            else
            {
                lm_bits.list_mask = 0;
            }
        }
        if (list_level > 0) show_line = 1;
        if (list_level == 0) show_line = list_ld;
        if (list_level < 0) show_line = 0;
        qued_lm_bits.list_mask = lm_bits.list_mask;
        return 1;
    }
    while (1)
    {
        int tt;
        unsigned long old_edmask;
        old_edmask = edmask;
        edmask &= ~(ED_LC|ED_DOL);
        tt = get_token();
        edmask = old_edmask;
        if (tt == EOL) break;
        comma_expected = 1;
        if (tt != TOKEN_strng)
        {
            bad_token(tkn_ptr,"Expected a symbolic string here");
        }
        else
        {
            for (i=0;list_stuff[i].string != 0;++i)
            {
                tt = strcmp(token_pool,list_stuff[i].string); 
                if (tt <= 0) break;
            }
            if (tt != 0)
            {
                bad_token(tkn_ptr,"Unknown listing directive");
            }
            else
            {
                if ( !list_stuff[i].flag )      /* Special OCT keyword */
                {
                    if ( reset_list_params )
                        reset_list_params(onoff);
                }
                else
                {
                    if (onoff < 0)
                    {
                        saved_lm_bits.list_mask &= ~list_stuff[i].flag;
                    }
                    else
                    {
                        saved_lm_bits.list_mask |= list_stuff[i].flag;
                    }
                }
            }
        }
    }
    if (list_level == 0) lm_bits.list_mask = saved_lm_bits.list_mask;
    if (list_level > 0)
    {
        lm_bits.list_mask = saved_lm_bits.list_mask |
                            LIST_CND|LIST_LD|LIST_MC|LIST_MD|LIST_ME|LIST_SRC|LIST_MES|LIST_BIN|LIST_LOC;
    }
    qued_lm_bits.list_mask = lm_bits.list_mask;
    return 1;
}

int op_list(void)
{
    op_list_common(1);
    return 0;
}

int op_nlist(void )
{
    op_list_common(-1);
    return 0;
}

#ifdef MAC_PP
char hexdig[] = "0123456789ABCDEF";
#endif

void dump_hex4(long value,char *ptr)
{
    register int val;
    register char *dst;
    val = value&0xFFFF;
    dst = ptr + 3;
    *dst = hexdig[val&15];   /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    return;
}

void dump_hex8(long value,char *ptr)
{
    register char *dst;
    long val = value;
    dst = ptr + 7;
    *dst = hexdig[val&15];   /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    val >>= 4;           /* shift */
    *--dst = hexdig[val&15]; /* copy in nibble */
    return;
}

void dump_oct3(long value, char *ptr )
{
    register int val;
    register char *dst;
    val = value&0xFF;
    dst = ptr + 2;
    *dst = '0' + (val&7);   /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&7); /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&3); /* copy in nibble */
    return;
}

void dump_oct6(long value, char *ptr )
{
    register int val;
    register char *dst;
    val = value&0xFFFF;
    dst = ptr + 5;
    *dst = '0' + (val&7);   /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&7); /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&7); /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&7); /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&7); /* copy in nibble */
    val >>= 3;           /* shift */
    *--dst = '0' + (val&1); /* copy in nibble */
    return;
}

void clear_list(LIST_stat *lstat)
{
    lstat->f1_flag = 0;
    lstat->f2_flag = 0;
    memset(lstat->line_ptr,' ',LLIST_SIZE);
    *(lstat->line_ptr+LLIST_SIZE) = 0;
#ifndef MAC_PP
    lstat->pc_flag = 0;
    lstat->has_stuff = 0;
    lstat->list_ptr = LLIST_OPC;
#endif
    return;
}

void display_line(LIST_stat *lstat)
{
    register char *s,*lp;
    lp = lstat->line_ptr;
    if (list_seq)
    {
        if (lstat->line_no != 0 )
        {
            sprintf(lp+LLIST_SEQ,"%5ld%c",
                    lstat->line_no,(lstat->include_level > 0) ? '+':' ');
        }
    }
#ifndef MAC_PP
    if (list_loc)
    {
        if (lstat->pc_flag != 0)
        {
            if ( list_radix == 16 )
                dump_hex4((long)lstat->pc,lp+LLIST_LOC);
            else
                dump_oct6((long)lstat->pc,lp+LLIST_LOC);
        }
    }
#endif
    if (lstat->f1_flag != 0)
    {
        if ( list_radix == 16 )
        {
            dump_hex8((long)lstat->pf_value,lp+LLIST_PF1);
            *(lp+LLIST_PF1+8) = lstat->f1_flag >> 8;
        }
        else
        {
            dump_oct6((long)lstat->pf_value,lp+LLIST_PF1);
            *(lp+LLIST_PF1+6) = lstat->f1_flag >> 8;
        }
    }
    else
    {
        if (lstat->f2_flag != 0)
        {
            if ( list_radix == 16 )
            {
                dump_hex8((long)lstat->pf_value,lp+LLIST_PF2);
#ifndef MAC_PP
                *(lp+LLIST_PF2-1) = '(';
                *(lp+LLIST_PF2+8) = ')';
#endif
            }
            else
            {
                dump_oct6((long)lstat->pf_value,lp+LLIST_PF2);
#ifndef MAC_PP
                *(lp+LLIST_PF2-1) = '(';
                *(lp+LLIST_PF2+6) = ')';
#endif
            }
        }
    }
    for (s = lp; s < lp+LLIST_SIZE; ++s)
    {
        if (*s == 0) *s = ' ';
    }
    if (*s == 0)
    {
#ifndef MAC_PP
        int sho;
        sho = (macro_nesting > 0) ? list_mes : list_src;
        if (sho != 0 || line_errors_index != 0)
        {
#endif
            lstat->line_ptr[LLIST_MAXSRC] = '\n';
            lstat->line_ptr[LLIST_MAXSRC+1] = 0;
            if (lstat->line_end-s-2 > 0) strncpy(s, inp_str, lstat->line_end-s-2);
#ifndef MAC_PP
        }
        else
        {
            *s++ = '\n';
            *s = 0;
        }
#endif
    }
    puts_lis(lp,1);
    if (line_errors_index != 0)
    {
        int z;
        for (z=0;z<line_errors_index;++z)
        {
            err_msg(line_errors_sev[z]|MSG_CTRL|MSG_NOSTDERR,line_errors[z]);
            MEM_free(line_errors[z]);
        }
        line_errors_index = 0;
    }
    clear_list(lstat);
    return;
}

void line_to_listing(void)
{
#ifndef MAC_PP
    if (meb_stats.getting_stuff != 0)
    {
        if (line_errors_index == 0)
        {
            show_line = 1;     /* assume to show it */
            return;
        }
        if (meb_stats.has_stuff != 0 || meb_stats.list_ptr != LLIST_OPC)
        {
            int le_save;
            le_save = line_errors_index;
            line_errors_index = 0;
            display_line(&meb_stats);
            meb_stats.has_stuff = 0;
            line_errors_index = le_save;
        }
    }
#endif
    if (show_line != 0 || line_errors_index != 0)
    {
        display_line(&list_stats);
    }
    show_line = 1;       /* assume to show it */
    return;
}

#ifndef MAC_PP
int fixup_overflow(LIST_stat *lstat)
{
    char *lp;
    display_line(lstat);
    lstat->expected_pc = lstat->pc = current_offset;
    lstat->expected_seg = current_section;
    lstat->pc_flag = 1;
    lstat->line_no = 0;          /* don't print seq numb next time */
    lp = lstat->line_ptr + LLIST_SIZE;
    *lp++ = '\n';
    *lp = 0;
    return 0;
}

void n_to_list( int nibbles, long value, int tag)
{
    register unsigned long tv;
    register char *lpr;
    LIST_stat *lstat;
    int lcnt,nyb;
	
	if ( !pass )
		return;
	tv = value;
    nyb = nibbles;
    lcnt = LLIST_SIZE-nyb-1;
    if (tag != 0) lcnt -= 1;
    if (meb_stats.getting_stuff)
    {
        if (meb_stats.pc_flag == 0)
        {
            meb_stats.expected_pc = meb_stats.pc = current_offset;
            meb_stats.expected_seg = current_section;
            meb_stats.pc_flag = 1;
        }
        if (line_errors_index != 0)
        {
            int save;
            save = line_errors_index;
            line_errors_index = 0;
            fixup_overflow(&meb_stats);
            line_errors_index = save;
            lstat = &list_stats;
        }
        else
        {
            lstat = &meb_stats;
            if ((meb_stats.list_ptr != LLIST_OPC) &&
                (meb_stats.pc_flag &&
                 ((meb_stats.expected_pc != current_offset) ||
                  (meb_stats.expected_seg != current_section))))
            {
                fixup_overflow(&meb_stats);
            }
        }
    }
    else
    {
        lstat = &list_stats;
    }
    if (lstat->list_ptr >= lcnt)
    {
        if (!list_bex) return;
        fixup_overflow(lstat);
    }
    lpr = lstat->line_ptr + lstat->list_ptr + nyb;
    if (tag != 0)
    {
        *lpr = tag;
        lcnt = 2;
    }
    else
    {
        lcnt = 1;
    }
    lstat->list_ptr += lcnt+nyb;
    if ( list_radix == 16 )
    {
        do
        {
            *--lpr = hexdig[(int)(tv&15)];
            tv >>= 4;
        } while (--nyb > 0);
    }
    else
    {
        do
        {
            *--lpr = '0' + (tv&7);
            tv >>= 3;
        } while (--nyb > 1);
        if ( nyb )
            *--lpr = '0' + (tv&1);
    }
    return;
}
#endif
