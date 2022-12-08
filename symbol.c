/*
    symbol.c - Part of macxx, a cross assembler family for various micro-processors
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
 * This module does all the symbol table managment. The symbol table
 * storage technique used here is the chained hash table (see Compiler
 * Writing, Tremblay & Sorenson). The hash table size is chosen here to
 * quite large since memory is not as much at a premium on the VAX as on
 * other computers (i.e. PDP-11). You may consider reducing the hash table
 * size if you are to port this to a memory limited system.
 * Tremblay/Sorenson suggest that the hash table size be a prime number. 
 *
 *******************************************************************/

#include "token.h"		/* define compile time constants */

#define DEBUG 9

/* external references */

/* Static Globals */

long sym_pool_used;
SS_struct *hash[HASH_TABLE_SIZE];  /* hash table */
SS_struct *symbol_pool; /* pointer to next free symbol space */
int symbol_pool_size=0;     /* number of symbol spaces left */
SS_struct *first_symbol;    /* pointer to first symbol of 'duplicate' list */
int new_symbol;         /* flag indicating that an insert happened */
/* value (can be added)	*/
short current_procblk;      /* current procedure block number */
short current_scopblk;      /* current scope level within block */
short current_scope;        /* sum of the two above */

/************************************************************************
 * Get a block of memory to use for symbol table
 */
SS_struct *get_symbol_block(int flag)
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
    if (symbol_pool_size <= 0)
    {
        int t = 256*sizeof(SS_struct);
        sym_pool_used += t;
        symbol_pool = (SS_struct *)MEM_alloc(t);
        symbol_pool_size = 256;
    }
    if (!flag) return(symbol_pool);
    --symbol_pool_size;      /* count it down */
    return(symbol_pool++);   /* get pointer to free space */
}

#define SSNULL ((SS_struct *)0)
#define CPNULL ((char *)0)

/*******************************************************************
 *
 * Symbol table lookup and insert
 */
SS_struct *sym_lookup( char *strng, int err_flag )
/*
 * At entry:
 *	strng - pointer to a null terminated symbol name string
 *      err_flag:
 *		0 = don't automatically insert the symbol
 *	      		into the symbol table if its not there.
 *		1 = automatically insert it if its not there.
 *		2 = insert at current scope level (if 0, may match
 *			on symbol at lower scope level)
 *
 * At exit:
 *	If err_flag is != 0, then returns with pointer to either new
 * 	symbol block if symbol not found or old symbol block if symbol
 * 	already in symbol table. If err_flag == 0, then returns
 * 	NULL if symbol not found in the symbol table, else returns with
 * 	pointer to old symbol block.
 *********************************************************************/
{
    int i,condit=1;
    SS_struct *st,**last,*new,*prev;
    first_symbol = 0;
    new_symbol = 0;

/* Check for presence of free symbol block and add one if none */

    if (*strng == 0) return(SSNULL); /* not there, don't insert it */
    if (symbol_pool_size <= 0)
    {
        if (!get_symbol_block(0)) return(SSNULL);
    }

/* hash the symbol name */

    i = hashit(strng,HASH_TABLE_SIZE,11);

/* Pick up the pointer from the hash table to the symbol block */
/* If NULL then this is a table miss, add a new entry to the hash table */

    if ((st = hash[i]) == SSNULL)
    {
        if (!err_flag)
			return(SSNULL); /* no symbol */
        st = hash[i] = symbol_pool++; /* pick up pointer to new symbol block */
        --symbol_pool_size;   /* take from total */
        st->ss_string = strng;    /* set the string constant */
        new_symbol = SYM_ADD|SYM_FIRST; /* symbol added and is first in the chain */
        return(st);       /* return pointing to new block */
    }

/* This is a table collision. Its linear search time from here on. */
/* The symbols are stored ordered alphabetically in the list. The */
/* variable "last" is a pointer to a pointer that says where to */
/* deposit the backlink. If the result of a string compare denotes */
/* equality, the routine exits pointing to the found block. If the */
/* user's string is less than the one tested against, then a new block */
/* is inserted in front of the tested block else a new one is added at */
/* the end of the list */

    last = hash + i;     /* remember place to stuff backlink */
    prev = 0;            /* remember ptr to previous */

    while (1)
    {           /* loop through the whole ordered list */
        condit = strcmp(strng,st->ss_string);
        if (condit < 0) break;    /* not there if user's is less */
        if (condit == 0)
        {
            char *tst;
            SS_struct *zero;
            if (st->ss_scope == current_scope) return(st);
            if (current_scope == 0) break; /* not there */
            zero = (st->ss_scope == 0) ? st : 0;
            tst = st->ss_string;
            prev = st;
            while (1)
            {
                if (tst != st->ss_string)
                {
                    condit = 0;      /* add a dup */
                    break;           /* not there */
                }
                condit = (st->ss_scope&SCOPE_PROC) - current_procblk;
                if (condit > 0) break;  /* not there */
                if (condit == 0)
                {
                    condit = (st->ss_scope&~SCOPE_PROC) - current_scopblk;
                    if (condit == 0) return(st);    /* match */
                    if (condit < 0)
                    {        /* symbol is at higher level */
                        if ((err_flag&2) != 0)
                        {  /* add lower level? */
                            condit = 0;    /* yep, signal we're to add a dup */
                            break;     /* and add it */
                        }
                        else
                        {
                            return(st);   /* else match on lower level */
                        }
                    }
                }
                last = &st->ss_next;    /* next place to store backlink */
                prev = st;          /* remember the previous one */
                if (!(st = st->ss_next))
                {  /* get link to next block, exit if NULL */
                    condit = 0;
                    break;           
                }
            }
            if (zero != 0 && (err_flag&2) == 0) return(zero);
            break;         /* always add a new one */
        }
        last = &st->ss_next;  /* next place to store backlink */
        prev = st;        /* remember the last one */
        if (!(st = st->ss_next)) break; /* get link to next block, exit if NULL */
    }

    if (!err_flag) return(SSNULL); /* not there, don't insert one */

/* Have to insert a new block in between two others or at the end of the */
/* list. (st is NULL if inserting at the end). */

    new_symbol |= SYM_ADD;   /* signal that we've added a symbol */
    new = symbol_pool++;     /* get pointer to free space */
    --symbol_pool_size;      /* count it down */
    *last = new;         /* point previous block to the new one (backlink) */
    new->ss_next = st;       /* point to the following one */
    if (condit == 0)
    {       /* if adding a duplicate */
        new_symbol |= SYM_DUP;    /* signal a duplicate */
        if (prev != (SS_struct *)0)
        {
            prev->flg_more = 1;    /* tell other guy there's more */
            new->ss_string = prev->ss_string;  /* use previous's ptr */
        }
        else
        {          /* this one is first */
            if (st != 0)
            {
                new->ss_string = st->ss_string; /* use next's ptr */
            }
            else
            {
                sprintf(emsg,"Internal error adding duplicate (%s) to symbol table",strng);
                bad_token((char *)0,emsg);
                new->ss_string = " INTERNAL ERROR ";
            }
        }
    }
    else
    {
        new->ss_string = strng;   /* point to the string */
    }
    return(new);         /* return pointing to new block */
}    

/*********************************************************************
 * Delete symbol from symbol table.
 */
SS_struct *sym_delete(SS_struct *old_ptr)
/*
 * At entry:
 *	old_ptr - pointer to symbol block to delete
 * At exit:
 *	symbol removed from the symbol table if present. Routine always
 *	returns old_ptr.
 */
{
    int i;
    SS_struct *st,**last;

/* compute the hash value */

    i = hashit(old_ptr->ss_string,HASH_TABLE_SIZE,11);

/* Pick up the pointer from the hash table to the symbol block */
/* If NULL then this is a table miss, nothing to delete */

    if (!(st = hash[i])) return(old_ptr); /* he can have the old one */

/* There is a symbol in the symbol table pointed to by the hash table. */
/* The first entry is special in that the backlink is actually the hash */
/* table itself. Otherwise, this routine finds the occurance of the old_ptr */
/* in the chain and plucks it out by patching the link fields. */

    last = hash + i;     /* place to stash backlink */

    while (1)
    {           /* loop through the whole ordered list */
        if (st == old_ptr)
        {
            *last = old_ptr->ss_next; /* pluck it out, set the backlink */
            return(old_ptr);   /* he can have the old block */
        }
        last = &st->ss_next;  /* next place to store backlink */
        if (!(st = st->ss_next)) break; /* get link to next block, exit if NULL */
    }
    return(old_ptr);     /* not in the table, give 'em the old one */
}    

#if !defined(NO_XREF)

void do_xref_symbol(SS_struct *sym_ptr,int defined)
{
#ifdef lint
    sym_ptr->flg_defined = defined;
#else
#ifdef MACXX
    FN_struct **fnp_ptr;
    int i=0,j=0;
    if ((fnp_ptr = sym_ptr->ss_xref) == 0)
    {
        fnp_ptr = sym_ptr->ss_xref = get_xref_pool();
        *fnp_ptr = (FN_struct *)-1; /* reserve first place for "defined in" file */
    }
    if (*fnp_ptr == (FN_struct *)-1 && defined != 0)
    {
        *fnp_ptr = current_fnd;   /* defined file is first in the list */
        j++;
    }
    if (j != 0)
    {
        fnp_ptr++;        /* skip the defined file */
        i++;          /* count it */
        while (*fnp_ptr != 0 && *fnp_ptr != current_fnd)
        {
            i++;
            fnp_ptr++;
            if (i < XREF_BLOCK_SIZE-1) continue;
            if ((fnp_ptr= *((FN_struct ***)fnp_ptr)) == 0) return;
            i = 0;
            continue;
        }
        if (*fnp_ptr == current_fnd) *fnp_ptr = (FN_struct *)-1;
        return;
    }
    while (*fnp_ptr != 0 )
    {
        if (*fnp_ptr++ == current_fnd) return;    /* already referenced */
        i++;
        if ( i < XREF_BLOCK_SIZE-1) continue;
        if (*fnp_ptr == 0)
        {  /* off the end and no more? */
            *((FN_struct ***)fnp_ptr) = get_xref_pool(); /* get a new block of memory */
        }
        fnp_ptr = *((FN_struct ***)fnp_ptr);
        i = 0;
        continue;
    }
    *fnp_ptr = current_fnd;  /* cross reference the symbol */
#endif
#endif
    return;
}
#endif
