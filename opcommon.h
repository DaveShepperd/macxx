/*
    opcommon.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _OPCOMMON_H_
#define _OPCOMMON_H_ 1

/* external references */
#include <string.h>

#include "token.h"
extern Opcpst perm_opcpst[];
extern Dirpst perm_dirpst[];
extern Opcode *opcode_lookup();
#include "memmgt.h"

void opcinit( void )          /* preloads the opcode table */
{
    Dirpst *dir = &perm_dirpst[0];
    Opcode *op;
#if !defined(MAC_PP)
    Opcpst *opc = &perm_opcpst[0];
#if defined(MAC_AS)
    SS_struct *sym_ptr;
    SEG_struct *segp;
    strcpy(token_pool, ".LITPOOL.");
    token_value = sizeof(".LITPOOL.")-1;
    literal_pool_sym = sym_ptr = sym_lookup(token_pool, SYM_INSERT_IF_NOT_FOUND);
    sym_ptr->ss_fnd = current_fnd;
    sym_ptr->ss_line = current_fnd->fn_line;
    sym_ptr->flg_global = 1;
    literal_pool_ptr = segp = (SEG_struct *)get_seg_mem(&sym_ptr, sym_ptr->ss_string);
    segp->flg_literal = 1;
    segp->seg_salign = macxx_salign;
    segp->seg_dalign = macxx_dalign;
    segp->seg_maxlen = 256*1024-(32*4);
#endif
#if defined(MAC_TJ)
    if (options[QUAL_JERRY]) macxx_target = "JERRY"; /* "1357785-002"; */
#endif
    for (;opc->name;++opc)
    {
        int len;
        unsigned long legal_am;
#if defined(MAC_65)
        if (options[QUAL_P816])
        {
            legal_am = opc->aaamodes;
        }
        else if (options[QUAL_CMOS])
        {
            legal_am = opc->aamodes;
        }
        else
        {
            legal_am = opc->amodes;
        }
        if (legal_am == 0L) continue;
#else
#if defined(MAC_TJ)
        if (options[QUAL_JERRY])
        {
            if ((opc->class&OPCL_JERRY) == 0) continue;
        }
        else
        {
            if ((opc->class&OPCL_TOM) == 0) continue;
        }
#endif
        legal_am = opc->amodes;
#endif
        if (token_pool_size <= max_opcode_length+1)
            get_token_pool(max_opcode_length+1, 1);
        len = strlen(opc->name);
        if (len > max_opcode_length) len = max_opcode_length;
        strncpy(token_pool,opc->name,len);
        *(token_pool+len) = 0;
        op = opcode_lookup(token_pool,1);
        ++len;
        token_pool += len;
        token_pool_size -= len;
        if ((op->op_class&OP_CLASS_MAC) == 0)
        {
            op->op_value = opc->value;
            op->op_class = opc->class;
                           op->op_amode = legal_am;
        }
    }
#endif
    while (dir->name)
    {
        int len;
        if (token_pool_size <= max_opcode_length+1)
            get_token_pool(max_opcode_length+1, 1);
        len = strlen(dir->name);
        if (len > max_opcode_length) len = max_opcode_length;
        strncpy(token_pool,dir->name,len);
        *(token_pool+len) = 0;
        op = opcode_lookup(token_pool,1);
        ++len;
        token_pool += len;
        token_pool_size -= len;
        if ((op->op_class&OP_CLASS_MAC) == 0)
        {
            op->op_func  = dir->func;
            op->op_class = dir->flags;
        }
		else if ( op->op_func != dir->func )
		{
			char err[256];
			snprintf(err, sizeof(err), "Duplicate PST entry while adding '%s'. Already have '%s'", token_pool, op->op_name);
			show_bad_token(NULL, err, MSG_FATAL);
		}
		++dir;
    }
    ust_init();
    return;
}
#endif /* _OPCOMMON_H_ */
