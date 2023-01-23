/*
    pass0.c - Part of macxx, a cross assembler family for various micro-processors
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
#if !defined(MAC_PP)
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
    {
        gbl_flg |= DEFG_LOCAL;
    }
    gbl_flg = f1_defg(gbl_flg); /* define a symbol */
#if 0
    list_stats.pf_value = EXP0.psuedo_value;
    if (macro_nesting > 0 && list_me )
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
#endif
    f1_eol();           /* better be at eol */
}

/*******************************************************************
 * Main entry for PASS0 processing
 */
void pass0( int file_cnt)
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
    SS_struct *sym_ptr;

    if (file_cnt == 0)
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
        opcinit();            /* seed the opcode table */
    }
    expr_message_tag = 1;    /* signal not to display messages */
    if (get_text() == EOF)
        return;
    show_line = 0 ; /* (list_level >= 0) ? 1 : 0; */
    list_stats.line_no = current_fnd->fn_line;
    list_stats.include_level = include_level;
    while (1)
    {
        comma_expected = 0;   /* no commas expected */
        tokt = tkn_flg ? get_token() : token_type;
        tkn_flg = 1;          /* assume to get next token */
        if (tokt == EOF)
        {
            if (current_section->seg_pc > current_section->seg_len)
            {
                current_section->seg_len = current_section->seg_pc;
            }
            return;
        }
        if (tokt == EOL)
        {
            if (get_text() == EOF)
            {
                token_type = EOF;
                tkn_flg = 0;
            }
            continue;
        }
#if defined(MAC68K) || defined(MAC682K)
        dotwcontext = 0;
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
                if (c == '=')
                {
                    gbl_flg &= ~DEFG_LABEL;  /* it's not a label */
                    found_symbol(gbl_flg, tokt); /* a := construct? */
                    continue;
                }
                if (condit_word < 0)
                {
#if defined(MAC68K) || defined(MAC682K)
                    white_space_section = 3;
                    no_white_space_allowed = 0;
#endif
                    continue;        /* in unsatisfied conditional */
                }
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
                    continue;        /* and continue */
                }
                if (condit_word < 0)
                {
                    f1_eatit();      /* eat rest of line */
                    continue;        /* and proceed */
                }
                list_stats.pc = current_offset;
                if (opc->op_class &OP_CLASS_OPC)
                {  /* if bit 15 set, it's an opcode */
                    do_opcode(opc);
                    list_stats.pc_flag = 1;      /* signal to print the PC */
                }
                else
                {
                    if (opc->op_class & OP_CLASS_MAC)
                    {  /* if bit 14 set, it's a macro */
                        macro_call(opc);
                        f1_eatit();           /* eat rest of line */
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
                                f1_eatit();     /* eat rest of line */
                            }
                            continue;
                        }
                    }
                }
                f1_eol();
                continue;
            }              /* -- found opcode/psuedo-op/macro */
            *(token_pool+max_opcode_length) = save_c;
        }                 /* -- type is string */
        if (condit_word < 0)
        {
            f1_eatit();            /* eat rest of line */
        }
        else
        {
            inp_ptr = tkn_ptr;     /* backup and */
            if ( *inp_ptr == '*' || *inp_ptr == '#' )
            {
                f1_eatit();         /* eat the line */
                continue;
            }
            if ( (edmask&(ED_WRD|ED_BYT)) )
            {
				if ( (edmask&ED_BYT) )
					op_byte();		/* pretend it's a .byte psuedo-op */
				else
					op_word();		/* else pretend it's a .word psuedo-op */
                f1_eol();           /* better be at eol */
            }
            else
            {
                f1_eatit();         /* eat the line */
            }
        }
        continue;
    }
}
#endif	/* !defined(MAC_PP) */
