/*
    opcode.c - Part of macxx, a cross assembler family for various micro-processors
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
/********************************************************************
 *
 * This module does all the opcode/macro table managment. The table
 * storage technique used here is the chained hash table (see Compiler
 * Writing, Tremblay & Sorenson). The hash table size has been chosen
 * to minimize the memory required and yet to keep the number of collisions
 * low and the maximum chain to 2. A program was run to determine optimimum
 * values.
 *
 *******************************************************************/

#include "token.h"
#include "pst_tokens.h"		/* define compile time constants */

/* external references */

/* Static Globals */

Opcode *ophash[OP_HASH_SIZE];  /* hash table */
Opcode *opcode_pool;    /* pointer to next free opcode space */
int opcode_pool_size=0;     /* number of opcode spaces left */
Opcode *first_opcode;   /* pointer to first symbol of 'duplicate' list */
int new_opcode;         /* flag indicating that an insert happened */
/* value (can be added)	*/
/*   1 - symbol added to symbol table */
/*   2 - symbol added is first in hash table */
/*   4 - symbol added is duplicate symbol */

/************************************************************************
 * Get a block of memory to use for symbol table
 */
Opcode *get_opcode_block( int flag)
/*
 * At entry:
 *      flag:
 *		0 = don't take block from pool.
 *		1 = get a block and take it from the pool.
 *
 * At exit:
 *	returns pointer to next free symbol block (0'd)
 */
{
    if (opcode_pool_size <= 0)
    {
        int t = 50*sizeof(Opcode);
        misc_pool_used += t;
        opcode_pool = (Opcode *)MEM_alloc(t);
        opcode_pool_size = 50;
    }
    if (!flag) return(opcode_pool);
    --opcode_pool_size;      /* count it down */
    return(opcode_pool++);   /* get pointer to free space */
}

/*******************************************************************
 *
 * Opcode table lookup and insert
 */
Opcode *opcode_lookup(char *strng, int err_flag )
/*
 * At entry:
 *	strng - pointer to a null terminated symbol name string
 *      strlen - length of the string (including null)
 *      err_flag:
 *		0 = don't automatically insert the symbol
 *	      		into the symbol table if its not there.
 *		1 = automatically insert it if its not there.
 *		2 = insert it even if its already there but only
 *		    if the previous symbol is not a macro
 *
 * At exit:
 *	If err_flag is != 0, then returns with pointer to either new
 * 	symbol block if symbol not found or old symbol block if symbol
 * 	already in symbol table. If err_flag == 0, then returns
 * 	NULL if symbol not found in the symbol table, else returns with
 * 	pointer to old symbol block.
 *********************************************************************/
{
    int i,condit,j;
    Opcode *st,**last,*new;
    char upcase[34],*s,c;
    first_opcode = 0;
    new_opcode = 0;

/* Check for presence of free symbol block and add one if none */

    if (*strng == 0) return(0); /* not there, don't insert it */
    if (opcode_pool_size <= 0)
    {
        if (!get_opcode_block(0)) return(0);
    }
    new = opcode_pool;       /* get pointer to free space */

/* get the hash value */

    i = 0;
    if ((edmask&ED_LC) == 0)
    {
        s = strng;
        while ((c = *s++)) i = i*OP_HASH_INDX + c;
        s = strng;
    }
    else
    {
        char *s1;
        s = upcase;
        s1 = strng;
        for (j=0;j<sizeof(upcase)-1 && *s1 != 0;)
        {
            c = *s1++;
            c = _toupper(c);
            *s++ = c;
            i = i*OP_HASH_INDX + c;
        }
        *s = 0;
        s = upcase;
    }

    i %= OP_HASH_SIZE;
    if (i < 0) i = -i;

/* Pick up the pointer from the hash table to the opcode block */
/* If 0 then this is a table miss, add a new entry to the hash table */

    if (!(st = ophash[i]))
    {
        if (!err_flag) return(0); /* no opcode */
        st = ophash[i] = opcode_pool++; /* pick up pointer to new opcode block */
        --opcode_pool_size;   /* take from total */
        st->op_name = strng;  /* set the original string constant */
        new_opcode = 3;       /* 3 = opcode added and is first in the chain */
        return(st);       /* return pointing to new block */
    }

/* This is a table collision. Its linear search time from here on. */
/* The opcodes are stored ordered descending alphabetically in the list. The */
/* variable "last" is a pointer to a pointer that says where to */
/* deposit the backlink. If the result of a string compare denotes */
/* equality, the routine exits pointing to the found block. If the */
/* user's string is greater than the one tested against, then a new block */
/* is inserted in front of the tested block else a new one is added at */
/* the end of the list */

    last = ophash + i;       /* remember place to stuff backlink */

    while (1)
    {           /* loop through the whole ordered list */
        condit = strcmp(s,st->op_name);
        if (condit > 0) break; /* not there if user's is greater */
        if (condit == 0)
        {
            if (err_flag < 2) return(st); /* found it; not adding, so return */
            if (st->op_class == OP_CLASS_MAC) return(st); /* return if a MACRO */
            new->op_name = st->op_name;    /* use other guy's name */
            break;         /* else stick in a new one */
        }
        last = &st->op_next;  /* next place to store backlink */
        if (!(st = st->op_next)) break; /* get link to next block, exit if NULL */
    }

    if (!err_flag) return(0);    /* not there, don't insert one */

/* Have to insert a new block in between two others or at the end of the */
/* list. (st is 0 if inserting at the end). */

    new_opcode |= 1;     /* signal that we've added a opcode */
    opcode_pool++;       /* move pointer to free space */
    --opcode_pool_size;      /* count it down */
    *last = new;         /* point previous block to the new one (backlink) */
    new->op_next = st;       /* point to the following one */
    if (new->op_name == (char *)0) new->op_name = strng; /* point to the string */
    return(new);         /* return pointing to new block */
}    

