/*
    stack_ops.c - Part of macxx, a cross assembler family for various micro-processors
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

    01-29-2022	- Stack support added by - Tim Giddens

******************************************************************************/
#if !defined(VMS) && !defined(ATARIST)
	#include "add_defs.h"
	#include "token.h"
	#include "pst_tokens.h"
	#include "exproper.h"
	#include "listctrl.h"
	#include "strsub.h"
	#include "memmgt.h"
	#include <errno.h>
	#include <time.h>
	#include <string.h>
	#include <strings.h>
	#include <sys/types.h>
	#include <sys/stat.h>

	#include <stdlib.h>



/****************************************************************************
 * 
 * 01-29-2022 - User Stack Commands added by Tim Giddens 

Adds support for the HLLxxF.MAC (Higher Level Language - Formatted) Macros

Functions do not support all of the options, just enough to get HLLxxF working.

Missing commands added to MACxx
See MACxx.DOC

.DEFSTACK	- Defines User Stack
.POP		- Pop value from Stack
.PUSH		- Push value to Stack
.GETPOINTER	- Returns Stack Pointer
.PUTPOINTER	- Sets Stack Pointer

Note from MACxx.DOC  **************************
Note that the amount of memory used is the product of the stack size and
the element type size. Care should be taken as not to create an array so
large that there is no memory left for the assembler proper. On MS-DOS
systems, no single array can be larger than 65,536 bytes (32,768 words;
16,384 longs; 8,192 relative values). 

There is a stack pointer associated with each array. The pointer will be
pre-decremented when an item is .PUSH'ed and post-incremented when an item
is .POP'ped. The pointer always moves by one regardless of the size of 
the element.

The .GETPOINTER and .PUTPOINTER directives will respectively get or set
the stack pointer on a named stack.

**********************************************
Each Named Stack is set to one of four different data types (data size) by use of a Keyword

Keyword "BYTE"		char		Stack Type - 1 Byte/Entry	Entry =  8 Bits
Keyword "WORD"		short		Stack Type - 2 Bytes/Entry	Entry = 16 Bits
Keyword "LONG"		long		Stack Type - 4 Bytes/Entry	Entry = 32 Bits
Keyword "RELATIVE"				Stack Type - sizeof(SS_struct) Bytes/Entry	Entry = lots of Bits

The default type of "RELATIVE" is used if type is not specified on the command line.
*/

#define STACKS_DEBUG (0)

typedef enum
{
	TypeByte,
	TypeWord,
	TypeLong,
	TypeRelative
} StackType_t;

/* Structure to represent User Stack */
typedef struct user_stack {
	char *usr_stk_name;             /* Stack Name */
	union
	{
		unsigned char *bytes;
		unsigned short *words;
		unsigned long *longs;
		SS_struct *symbols;
		void *generic;
	} usr_stk_array;
	int usr_stk_ptr;                /* Stack Pointer */
	int usr_stk_size;               /* Stack Size (Number of Entrys) */
	size_t bytes;                   /* Stack size in bytes */
	StackType_t stackType;          /* Stack Type */
	struct user_stack *usr_stk_next;        /* Points to Next User Stack in the List */
} UserStack_t;
UserStack_t *usr_stk_first, *usr_stk_this, *usr_stk_new;

/** purge_expressions() - Free any expressions stored on a un-popped RELATIVE stack
 *
 *  At entry:
 *  pointer to stack
 *  
 *  At exit:
 *  Expressions free'd if there were any
**/
static void purge_expressions(UserStack_t *stkPtr)
{
	if ( stkPtr && stkPtr->stackType == TypeRelative )
	{
		int ii;
		for (ii=stkPtr->usr_stk_ptr; ii < stkPtr->usr_stk_size; ++ii)
		{
			SS_struct *symPtr;
			symPtr = stkPtr->usr_stk_array.symbols + ii;
			if ( symPtr->flg_exprs )
			{
				MEM_free((void *)symPtr->ss_exprs);
			}
		}
	}
}

/** purge_data_stacks() - Delete one or more of the user stacks.
 *  At entry:
 *  name - name of stack to delete. If null, delete all stacks.
 *  
 *  At exit:
 *  Stack(s) deleted. Returns nothing.
**/
void purge_data_stacks(const char *name)
{
	UserStack_t *curr,*next;

	if ( name )
	{
		UserStack_t **prev;
		
		/* we're to delete only one stack */
		/* point to the first pointer */
		prev = &usr_stk_first;
		/* while there are pointers to stacks */
		while ( (curr = *prev) )
		{
			if ( !strcmp(name, curr->usr_stk_name) )
			{
				/* tell previous guy about our next */
				*prev = curr->usr_stk_next;
				/* toss our stack stuff */
				purge_expressions(curr);
				MEM_free(curr);
				/* done */
				return;
			}
			/* not this one. Look at the next one if there is one */
			prev = &curr->usr_stk_next;
		}
	}
	else
	{
		curr = usr_stk_first;
		/* loop through all the stacks */
		while ( curr )
		{
			/* remember pointer to the next guy */
			next = curr->usr_stk_next;
			/* toss all the memory */
			purge_expressions(curr);
			MEM_free(curr);
			/* point to next guy */
			curr = next;
		}
		usr_stk_first = NULL;
	}
}

/*************************************************************************
 * 01-29-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *
 * function to create the user stack
 * struct user_stack* create_stack(char *tmp_name, unsigned int usr_stk_size, unsigned int tmp_type)
 *           - Reserves memory for a user stack to store data.
 *
 * At entry:
 * 	tmp_name	- string pointer to the name of stack
 * 	usr_stk_size	- size of stack 
 * 	tmp_type	- data type - byte, word, long, relative - aka 8/16/32/64 bits
 *
 * At exit:
 * 	returns pointer to the new stack created or (0 if error)
 *
 * Notes:
 * BYTE     = char      = 1 byte
 * WORD     = short     = 2 byte
 * LONG     = long      = 4 byte
 * SYMBOL   = struct    = ~32+ bytes
 * RELATIVE = struct    = ~32+ bytes
 * 
 * 
 ************************************************************************/

struct user_stack* create_stack(char *tmp_name, unsigned int usr_stk_size, StackType_t tmp_type, size_t *bytesPtr)
{
	int bytes, totBytes;
	int nameLen;

	/* Assume number of bytes is the same as the number of stack entries plus one */
	bytes = usr_stk_size + 1;
	switch (tmp_type)
	{
	case TypeByte:
/*		bytes *= sizeof(unsigned char); */
		break;
	case TypeWord:
		bytes *= sizeof(short);
		break;
	case TypeLong:
		bytes *= sizeof(long);
		break;
	case TypeRelative:
		bytes *= sizeof(SS_struct);
		break;
	default:
		bad_token(tkn_ptr, "Unknown User Stack \"Type Keyword\" while creating stack");
		f1_eatit();
		*bytesPtr = 0;
		return NULL;
	}
	
	nameLen = strlen(tmp_name) + 1;
	totBytes = sizeof(struct user_stack) + nameLen + bytes;
	/* Allocate enough space for both the stack, stack's name and array */
	usr_stk_new = (struct user_stack *)MEM_alloc(totBytes);      /* Get memory and setup stack */
	if ( !usr_stk_new )
	{
		*bytesPtr = totBytes;
		return NULL;            /* Failed to allocate memory */
	}
	/* Put the stack's array immediately after the stack (to keep pointer alignment if necessary) */
	usr_stk_new->usr_stk_array.generic = (void *)(usr_stk_new + 1);
	/* Put the stack's name immediately after the stack's array */
	usr_stk_new->usr_stk_name = (char *)(usr_stk_new->usr_stk_array.bytes+bytes);
	strncpy(usr_stk_new->usr_stk_name,tmp_name,nameLen-1);
	usr_stk_new->usr_stk_name[nameLen-1] = 0;
	usr_stk_new->usr_stk_size = usr_stk_size;
	usr_stk_new->usr_stk_ptr = usr_stk_size;
	usr_stk_new->stackType = tmp_type;

	if ( usr_stk_first == (struct user_stack *)NULL )
	{       /* if no stack already */
		usr_stk_first = usr_stk_this = usr_stk_new;
		usr_stk_first->usr_stk_next = NULL;       /* make sure new linked list next pointer is NULL */
	}
	else
	{
		/* not first stack */
		usr_stk_this = usr_stk_first;                   /* start at the beginning */
		while ( usr_stk_this->usr_stk_next != (struct user_stack *)NULL )
		{
			usr_stk_this = usr_stk_this->usr_stk_next;      /* Find last defined stack */
		}
		usr_stk_this->usr_stk_next = usr_stk_new;       /* set this as new last stack */
		usr_stk_this = usr_stk_new;
		usr_stk_new->usr_stk_next = NULL;     /* make sure any added linked list pointer next is NULL */
	}
	bytes += totBytes;
	*bytesPtr = bytes;            /* tell caller how much memory was allocated in bytes */
	usr_stk_this->bytes = bytes;  /* save size for posterity */
	return usr_stk_this;          /* return pointer to the new stack created */

}


/*************************************************************************
 * 01-29-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *
 * find_stack(char *tmp_name) - Searches the linked list for named stack
 *
 * function to locate Named user stack
 * At entry:
 * 	    tmp_name - string pointer to the name of stack
 *
 * At exit:
 *	   returns pointer to the named stack or (0 if error)
 *
 ************************************************************************/

struct user_stack* find_stack(char *tmp_name)
{
	int found = 0;

	if ( usr_stk_first == (struct user_stack *)NULL )          /* if no stack already */
	{
		show_bad_token((inp_ptr), "No user stacks defined\n", MSG_ERROR);
		return NULL;
	}
	else
	{                                   /* not first stack */
		usr_stk_this = usr_stk_first;                   /* start at the beginning */
		do
		{
			if ( strcmp(usr_stk_this->usr_stk_name, tmp_name) == 0 )   /* Find named stack */
			{
				found = 1;                  /* Found Named Stack */
				break;
			}
			usr_stk_this = usr_stk_this->usr_stk_next;          /* Try again */
		} while ( usr_stk_this != (struct user_stack *)NULL );          /* stop searching on NULL */


		if ( found == 0 )
		{                           /* if stack not found */
			sprintf(emsg, "User stack \"%s\" not found\n", tmp_name);
			show_bad_token((inp_ptr), emsg, MSG_ERROR);
			return NULL;
		}
	}

	return usr_stk_this;        /* return pointer to located stacked */

}


/*************************************************************************
 * 01-29-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *
 * Function to Define a User Stack
 *
 * At entry:
 *	Line pointer (inp_ptr) is pointing to the first token after the command
 *		command	  token,     token,      token     (CR/LF)or(LF) ("end of line")
 *		.DEFSTACK some_name, stack_size, stack_type
 *	inp_ptr here      ^
 *
 * Stack_Type is optional - The default type of "RELATIVE" is used if type not specified.
 * If you specify a stack type and it does not match one of the predefined types an error will be
 * reported and the user stack will not be defined. The current .DEFSTACK command will be terminated !!!
 *
 * When get_token() is called the current token (pointed to by inp_ptr) is read in and inp_ptr
 * is moved to the end of current token (which should be a comma) in current line of code.
 *
 * Usage	.DEFSTACK name, size, type
 *
 * Exp.
 *	.DEFSTACK STACK1,10.		  - Name = STACK1, Type = TypeRelative, Size set to 10 entries, Each entry = sizeof(SS_struct) bytes.
 *	.DEFSTACK STACK2,20,BYTE	  - Name = STACK2, Type = TypeByte, Size = 32 entries (Hex 20), Each entry = 8 bits.
 *	.DEFSTACK STACK3,41.,WORD	  - Name = STACK3, Type = TypeWord, Size = 41 entries, Each entry = 16 bits.
 *	.DEFSTACK STACK4,20.,LONG	  - Name = STACK4, Type = TypeLong, Size = 20 entries, Each entry = 32 bits.
 *	.DEFSTACK STACK5,30.,RELATIVE - Name = STACK5, Type = TypeRelative, Size = 30 entries, Each entry = sizeof(SS_struct) bytes.
 *
 *
 * Notes:
 * BYTE     = char      = 1 byte
 * WORD     = short     = 2 bytes
 * LONG     = long      = 4 bytes
 * RELATIVE = struct	= sizeof(SS_struct) (~32+) bytes 
 * 
 *
 ************************************************************************/

int op_defstack(void)
{

	char tmp_name[35];
	int tt;
	size_t totBytes;
	int tmp_size = 0;
	StackType_t tmp_type;
	struct user_stack *stk_test;

	/* variable token_pool = pointer to the string variable "token label" of the get_token function */

/*	squawk_syms = 1; */
	if ( get_token() != TOKEN_strng ) /* If token is not string label (Could be label of a value, keyword, etc) */
	{
		bad_token(tkn_ptr, "Expected User Stack \"Name\" here");
		f1_eatit();
		return 1;
	}

	strncpy(tmp_name, token_pool, sizeof(tmp_name) - 1);   /* move string label to User Stack Name */
	tmp_name[sizeof(tmp_name) - 1] = 0;
	if ( *inp_ptr == ',' )        /* any more tokens on this line of code */
	{
		comma_expected = 1; /* tell get_token() to expect a comma before the token */
		/* comma_expected will be reset to a value of "0" on return */
		if ( get_token() != EOL )
		{
			if ( exprs(0, &EXP0) < 0 )    /* Evaluate the token - get token to final value (do any math, etc) */
			{
				f1_eatit();
				return 1;
			}
			tmp_size = EXP0SP->expr_value;  /* save this string label value to User Stack Size */
		}
		/* Check for Stack Type to use */
		if ( *inp_ptr == ',' )
		{
			comma_expected = 1;
			if ( (tt = get_token()) != EOL )
			{
				/* Convert to Upper Case */
				int j = 0;
				while ( token_pool[j] )
				{
					token_pool[j] = toupper(token_pool[j]);
					++j;
				}

				/* Find User Stack Type - Keyword */
				if ( strcmp(token_pool, "BYTE") == 0 )
				{
					tmp_type = TypeByte;

				}
				else if ( strcmp(token_pool, "WORD") == 0 )
				{
					tmp_type = TypeWord;

				}
				else if ( strcmp(token_pool, "LONG") == 0 )
				{
					tmp_type = TypeLong;
				}
				else if ( strcmp(token_pool, "RELATIVE") == 0 || strcmp(token_pool,"SYMBOL") == 0 )
				{
					tmp_type = TypeRelative;
				}
				else
				{
					bad_token(tkn_ptr, "Unknown User Stack \"Type Keyword\" here");
					f1_eatit();
					return 1;
				}
			}
			else
			{
				bad_token(tkn_ptr, "Unknown User Stack \"Type Keyword\" here");
				f1_eatit();
				return 1;
			}
		}
		else
		{
			/*  Using Default User Stack Type = Relative */
			tmp_type = TypeRelative;
		}
	}
	if ( !tmp_size )
	{
		purge_data_stacks(tmp_name);
	}
	else
	{
		stk_test = create_stack(tmp_name, tmp_size, tmp_type, &totBytes);

		if ( stk_test != 0 )
		{
			if ( show_line && (list_bin || meb_stats.getting_stuff) )
			{
				list_stats.pf_value = stk_test->usr_stk_size;
				list_stats.f2_flag = 1;
			}

		}
		else
		{
			sprintf(emsg, "Failed to create User Stack - %s: %d entries of %d bytes", tmp_name, tmp_size, totBytes);
			show_bad_token((inp_ptr), emsg, MSG_ERROR);
			/* Eat rest of line to suppress warning error about end of line not reached */
			f1_eatit();
			return 1;
		}

	}

	f1_eatit();  /* skip rest of line */
	return 0;

}


/*************************************************************************
 *
 * 02-01-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *
 *  .PUSH
 *  Function to Set (push) value to Named User Stack
 * 
 * At entry:
 * 
 *
 * At exit:
 *	   data value pushed (saved) to the named stack
 *
 * Notes:
 * BYTE     = char      = 1 byte
 * WORD     = short     = 2 bytes
 * LONG     = long      = 4 bytes
 * SYMBOL   = SS_struct = Lots of data
 * RELATIVE = long long = 8 bytes
 * 
 * 
 * remember if it's pc or ABS value, no flags saved
 * 
 * Notes for 32bit stacks:
 * In This version:
 *             you can use the full 32 bits
 * 
 * Older MACxx :
 *             flag bits were in upper 16 bits and data in lower 16 bits of
 *             the 32 bit stack
 * 
 * 
 ************************************************************************/

int op_push(void)
{

	struct user_stack *usrstk;
	SS_struct dummySym, *symPtr;
	unsigned long data = 0;

	/* variable token_pool = pointer to the string variable "token label" of the get_token function */
	if ( get_token() != TOKEN_strng )     /* If token is not string label (Could be label of a value, keyword, etc) */
	{
		bad_token(tkn_ptr, "Expected User Stack \"Name\" here");
		f1_eatit();
		return 1;
	}
	else
	{
		usrstk = find_stack(token_pool);    /* Find Stack */
		if ( usrstk == NULL )
		{
			bad_token(inp_ptr, "Named User Stack Not Found here");
			f1_eatit();
			return 1;
		}
	}
	if ( *inp_ptr == ',' )        /* If no first token then it's an error */
	{
		int terms=0;
		while ( *inp_ptr == ',' )      /* any more tokens on this line of code */
		{
			int expRetV;

			++terms;
			if ( usrstk->usr_stk_ptr <= 0 )
			{
				list_stats.pf_value = usrstk->usr_stk_ptr;
				list_stats.f2_flag = 1;
				sprintf(emsg, "-P- User Stack \"%s\" Overflow - Pointer = %d\n", usrstk->usr_stk_name, usrstk->usr_stk_ptr);
				show_bad_token((inp_ptr), emsg, MSG_ERROR);
				f1_eatit();
				return 1;
			}
			comma_expected = 1;     /* tell get_token() to expect a comma before the token */
			if ( get_token() == EOL )
			{
#if STACKS_DEBUG
				snprintf(emsg, sizeof(emsg), "op_push(): Found EOL. terms=%d, stkPtr=%d of %d", terms, usrstk->usr_stk_ptr, usrstk->usr_stk_size);
				show_bad_token(NULL,emsg,MSG_WARN);
#endif
				break;
			}
			/* comma_expected will be reset to a value of "0" on return */
			/* One may push any random expression to the stack. Depending on the stack type
			   various amounts of data will be stored. For RELATIVE stacks, the entire
			   expression might be stashed. For the BYTE, WORD and LONG stacks, just the
			   integer absolute value of the expression will be stored. No error will be
			   produced if the expression does not resolve to an absolute value and the
			   value is pushed on a non-RELATIVE stack. User beware.
			*/
			expRetV = exprs(1, &EXP0);  /* Get an expression. It might be relative or complex */
			if ( expRetV < 0 )
			{
				/* nfg */
#if STACKS_DEBUG
				show_bad_token(NULL,"op_push(). exprs(1) return -1",MSG_ERROR);
#endif
				f1_eatit();
				return 1;
			}
			/* Get a pointer to a temp SS_struct. */
			symPtr = &dummySym;
			/* assume the value to push is to be 0 */
			data = 0;
			if ( expRetV == 1 )
			{
				/* we have a legit expression with a computed value of something. It might be garbage here but assume it isn't */
				data = EXP0SP->expr_value;
#if STACKS_DEBUG
				snprintf(emsg, sizeof(emsg), "op_push(). stack '%s', stackType=%d, stackPtr=%d, expRetV=1, expPtr=%d, exp_code=0x%X. value=0x%lX",
						 usrstk->usr_stk_name,
						 usrstk->stackType,
						 usrstk->usr_stk_ptr - 1,
						 EXP0.ptr,
						 EXP0SP->expr_code,
						 EXP0SP->expr_value);
				show_bad_token(NULL,emsg,MSG_WARN);
#endif
				if ( usrstk->stackType == TypeRelative )
				{
					/* salt our temp SS_struct in case we have to use it later */
					memset(symPtr,0,sizeof(dummySym));
					/* Tell the temp about the current file and line number */
					symPtr->ss_fnd = current_fnd;
					symPtr->ss_line = current_fnd->fn_line;
					/* pass the value too. It might be garbage, but just in case */
					symPtr->ss_value = data;
#if STACKS_DEBUG
					snprintf(emsg, sizeof(emsg), "Pushing 1 item with value of 0x%lX to stack '%s'[%d], type %d. exp.ptr=%d, exp[0].type=0x%02X",
							 data, usrstk->usr_stk_name, usrstk->usr_stk_ptr-1, usrstk->stackType,
							 EXP0.ptr, EXP0SP->expr_code);
					show_bad_token(NULL,emsg,MSG_WARN);
#endif
					if ( EXP0.ptr == 1 && (EXP0SP->expr_code == EXPR_SEG || EXP0SP->expr_code == EXPR_SYM || EXP0SP->expr_code == EXPR_VALUE) )
					{
						/* special cases */
						if ( EXP0SP->expr_code == EXPR_SEG  )
						{
							/* special case of a lone symbol defined as an offset to a section or the PC (which is always just a section+offset). */
							symPtr->flg_segment = 1;
							symPtr->ss_value = EXP0SP->expr_value;
							symPtr->ss_seg = EXP0SP->expt.expt_seg;
							symPtr->flg_defined = 1;
#if STACKS_DEBUG
							snprintf(emsg, sizeof(emsg), "Pushing 1 EXPR_SEG '%s' with value of 0x%lX to stack '%s'[%d], type %d. exp.ptr=%d, exp[0].type=0x%02X",
									 symPtr->ss_seg->seg_string,
									 data,
									 usrstk->usr_stk_name,
									 usrstk->usr_stk_ptr - 1,
									 usrstk->stackType,
									 EXP0.ptr,
									 EXP0SP->expr_code);
							show_bad_token(NULL,emsg,MSG_WARN);
#endif
						}
						else if (EXP0SP->expr_code == EXPR_SYM)
						{
							/* special case of a lone symbol. Possibly not yet defined or maybe just an absolute value? */
							symPtr = EXP0SP->expt.expt_sym;
#if STACKS_DEBUG
							snprintf(emsg, sizeof(emsg), "Pushing 1 EXPR_SYM '%s' with value of 0x%lX to stack '%s'[%d], type %d. exp.ptr=%d, exp[0].type=0x%02X",
									 symPtr->ss_string,
									 data,
									 usrstk->usr_stk_name,
									 usrstk->usr_stk_ptr - 1,
									 usrstk->stackType,
									 EXP0.ptr,
									 EXP0SP->expr_code);
							show_bad_token(NULL,emsg,MSG_WARN);
#endif
							if ( symPtr->flg_fwdReference )
							{
								/* In two pass mode (the only way to get forward references), don't allow one to save forward referenced "things" */
								show_bad_token((inp_ptr), "Cannot push a forward referenced symbol", MSG_ERROR);
								f1_eatit();
								return 1;
							}
							if ( !symPtr->flg_defined )
							{
								/* symbol not yet defined, so define it */
								symPtr->flg_abs = 1;
								symPtr->flg_defined = 1;
								symPtr->ss_fnd = current_fnd;
								symPtr->ss_line = current_fnd->fn_line;
								symPtr->ss_value = 0;
							}
						}
						else /* it must be EXPR_VALUE which is just a plain number */
						{
							symPtr->ss_value = EXP0SP->expr_value;
							symPtr->flg_abs = 1;
							symPtr->flg_defined = 1;
						}
					}
					else 
					{
						unsigned char *src, *dst;        /* mem pointers */
						EXP_stk *exp_ptr;
						int cnt;
						
						/* It is a complex expression. So clone the expression */
						cnt = EXP0.ptr * sizeof(EXPR_struct);
						sym_pool_used += cnt;
						exp_ptr = (EXP_stk *)MEM_alloc(cnt + (int)sizeof(EXP_stk));
						exp_ptr->ptr = EXP0.ptr;      /* set the number of entries in expression */
						exp_ptr->stack = (EXPR_struct *)(exp_ptr + 1); /* point to stack */
						symPtr->ss_exprs = exp_ptr;   /* tell symbol where expression is */
						dst = (unsigned char *)exp_ptr->stack;  /* point to area to load expression */
						src = (unsigned char *)EXP0SP;      /* point to source expression */
						memcpy(dst, src, cnt);       /* copy all the data */
						symPtr->flg_exprs = 1;    /* signal there's an expression of some sort */
						symPtr->flg_defined = 1;
					}
				}
			}
			else
			{
#if STACKS_DEBUG
				show_bad_token(NULL,"op_push(). exprs(1) return 0",MSG_ERROR);
#endif
			}

			/* Store data as same Type of Stack */
			/* decement stack pointer before pushing data on stack */
			--usrstk->usr_stk_ptr;
			switch (usrstk->stackType)
			{
			case TypeRelative:
				/* Each entry in a RELATIVE stack has room for an entire SS_struct */
				usrstk->usr_stk_array.symbols[usrstk->usr_stk_ptr] = *symPtr;
				break;
			case TypeLong:
				usrstk->usr_stk_array.longs[usrstk->usr_stk_ptr] = data;
				break;
			case TypeWord:
				data &= 0xFFFF;
				usrstk->usr_stk_array.words[usrstk->usr_stk_ptr] = (unsigned short)data;
				break;
			case TypeByte:
				data &= 0xFF;
				usrstk->usr_stk_array.bytes[usrstk->usr_stk_ptr] = (unsigned char)data;
				break;
			default:
				bad_token(tkn_ptr, "User Stack is set to an Unknown \"Type Keyword\" here");
				f1_eatit();
				return 1;
			}
#if STACKS_DEBUG
			snprintf(emsg, sizeof(emsg), "Pushed to entry %d: symPtr=%p, value=0x%lX, flg_exprs=%d, exp.ptr=%d, exp[0].code=0x%02X",
					 usrstk->usr_stk_ptr,
					 (void *)symPtr,
					 data,
					 symPtr->flg_exprs,
					 (symPtr->flg_exprs && symPtr->ss_exprs) ? symPtr->ss_exprs->ptr:0,
					 (symPtr->flg_exprs && symPtr->ss_exprs) ? symPtr->ss_exprs->stack->expr_code:0
					 );
			show_bad_token(NULL,emsg,MSG_WARN);
#endif

		}  /* End of While */
	}
	else
	{
		list_stats.pf_value = usrstk->usr_stk_ptr;
		list_stats.f2_flag = 1;
		bad_token(tkn_ptr, "Missing Stack argument to .PUSH command");
		f1_eatit();
		return 1;
	}
	list_stats.pf_value = usrstk->usr_stk_ptr;
	list_stats.f2_flag = 1;
	f1_eatit();     /* No tokens left, skip rest of line */
	return 0;

}

/*************************************************************************
 *
 * 02-01-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *
 *  .POP
 *  Function to Get (pop) value from Named User Stack
 * 
 * At entry:
 * 
 *
 * At exit:
 *	   data value pulled (retrieved) from the named stack
 *
 *
 * Notes:
 * BYTE     = char      = 1 byte
 * WORD     = short     = 2 bytes
 * LONG     = long      = 4 bytes
 * RELATIVE = struct    = ~32+ bytes
 * 
 ************************************************************************/

int op_pop(void)
{

	struct user_stack *usrstk;
	unsigned long data;

	/* variable token_pool = pointer to the string variable "token label" of the get_token function */

	if ( get_token() != TOKEN_strng ) /* If token is not string label (Could be label of a value, keyword, etc) */
	{
		bad_token(tkn_ptr, "Expected User Stack \"Name\" here");
		f1_eatit();
		return 1;
	}
	else
	{

		usrstk = find_stack(token_pool);
		if ( usrstk == NULL )
		{
			bad_token(inp_ptr, "Named User Stack Not Found pop");
			f1_eatit();
			return 1;
		}
	}

	if ( *inp_ptr == ',' )        /* If no first token then it's an error */
	{
		while ( *inp_ptr == ',' )      /* any more tokens on this line of code */
		{

			if ( usrstk->usr_stk_ptr >= usrstk->usr_stk_size )
			{
				list_stats.pf_value = usrstk->usr_stk_ptr;
				list_stats.f2_flag = 1;
				sprintf(emsg, "-P- User Stack \"%s\" Underflow - Pointer = %d\n", usrstk->usr_stk_name, usrstk->usr_stk_ptr);
				show_bad_token((inp_ptr), emsg, MSG_ERROR);
				f1_eatit();
				return 1;
			}
			comma_expected = 1;     /* tell get_token() to expect a comma before the token */
			/* comma_expected will be reset to a value of "0" on return */
			if ( get_token() != TOKEN_strng ) /* If token is not symbol string (Could be label of a value, keyword, etc) */
			{
				list_stats.pf_value = usrstk->usr_stk_ptr;
				list_stats.f2_flag = 1;
				bad_token(tkn_ptr, "Expected Variable \"Label\" pop");
				f1_eatit();
				return 1;
			}
			else
			{
				SS_struct *stkSymPtr;
				SS_struct *userSymPtr;
				
				stkSymPtr = NULL;
				data = 0;
				switch (usrstk->stackType)
				{
				case TypeByte:
					data = usrstk->usr_stk_array.bytes[usrstk->usr_stk_ptr];
					break;
				case TypeWord:
					data = usrstk->usr_stk_array.words[usrstk->usr_stk_ptr];
					break;
				case TypeLong:
					data = usrstk->usr_stk_array.longs[usrstk->usr_stk_ptr];
					break;
				case TypeRelative:
					stkSymPtr = usrstk->usr_stk_array.symbols+usrstk->usr_stk_ptr;
					data = stkSymPtr->ss_value;
					break;
				default:
					sprintf(emsg,  "User Stack is set to an Unspported type %d\n", usrstk->stackType);
					bad_token(tkn_ptr, emsg);
					f1_eatit();
					return 1;
				}
				/* Advance the stack pointer */
				++usrstk->usr_stk_ptr;
				if ( (strcmp(token_pool, ".") == 0) && (token_value == 1) )
				{
					if ( stkSymPtr )
					{
						if ( stkSymPtr->flg_exprs )
						{
							list_stats.pf_value = data;
							list_stats.f2_flag = 1;
							sprintf(emsg, "User Stack \"%s\" - Cannot pop PC direct from a absolute or non-\"segment+offset\" entry\n", usrstk->usr_stk_name);
							show_bad_token((inp_ptr), emsg, MSG_ERROR);
							f1_eatit();
							return 1;
						}
						if ( stkSymPtr->flg_segment )
						{
							if ( stkSymPtr->ss_seg != current_section )
							{
								snprintf(emsg, sizeof(emsg), "WARNING: Popped PC from section '%s' to section '%s'",
										 current_section->seg_string,
										 stkSymPtr->ss_seg->seg_string);
								show_bad_token(NULL,emsg,MSG_WARN);
							}
							current_section = stkSymPtr->ss_seg;
						}
					}
					current_offset = data;

				}
				else
				{
					userSymPtr = do_symbol(SYM_INSERT_IF_NOT_FOUND);       /* add level 0 symbol/label */
					if ( !userSymPtr )
					{
						bad_token(tkn_ptr, "Unable to insert symbol into symbol table");
						f1_eatit();
						return 1;
					}
					if ( userSymPtr->flg_label )
					{
						list_stats.pf_value = data;
						list_stats.f2_flag = 1;
						sprintf(emsg, "User Stack \"%s\" - Cannot pop into a symbol pre-defined as a label.\n", usrstk->usr_stk_name);
						show_bad_token((inp_ptr), emsg, MSG_ERROR);
						f1_eatit();
						return 1;
					}
#if 1
					if ( userSymPtr->flg_fwdReference )
					{
						list_stats.pf_value = data;
						list_stats.f2_flag = 1;
						sprintf(emsg, "User Stack \"%s\" - Cannot pop into a forward referenced symbol '%s'.\n", usrstk->usr_stk_name, userSymPtr->ss_string);
						show_bad_token((inp_ptr), emsg, MSG_ERROR);
						f1_eatit();
						return 1;
					}
#endif
					if ( userSymPtr->flg_exprs )
					{
						/* Toss any existing expression this symbol might have to avoid memory leaks */
						MEM_free(userSymPtr->ss_exprs);
						userSymPtr->flg_exprs = 0;
					}
					/* The symbol gets all the normal stuff */
					userSymPtr->flg_defined = 1;
					userSymPtr->flg_fwdReference = 0;
					userSymPtr->ss_value = data;
					userSymPtr->ss_fnd = current_fnd;
					userSymPtr->ss_line = current_fnd->fn_line;
					userSymPtr->flg_abs = 1;	/* assume absolute */
					if ( stkSymPtr )
					{
						/* It is a relative assignment. So copy the appropriate bits */
						userSymPtr->flg_abs = stkSymPtr->flg_abs;
						userSymPtr->flg_exprs = stkSymPtr->flg_exprs;
						userSymPtr->flg_segment = stkSymPtr->flg_segment;
						userSymPtr->ss_exprs = stkSymPtr->ss_exprs;
						/* clear anything in the stack entry so it won't get used or lost */
						stkSymPtr->flg_exprs = 0;
						stkSymPtr->ss_exprs = NULL;
					}
#if STACKS_DEBUG
					snprintf(emsg, sizeof(emsg), "Popping to '%s': stkSymPtr=%p, value=0x%lX, flg_exprs=%d, exp.ptr=%d, exp[0].code=0x%02X",
							 userSymPtr->ss_string,
							 (void *)stkSymPtr,
							 data,
							 stkSymPtr ? stkSymPtr->flg_exprs:0,
							 (stkSymPtr && stkSymPtr->flg_exprs && stkSymPtr->ss_exprs) ? stkSymPtr->ss_exprs->ptr:0,
							 (stkSymPtr && stkSymPtr->flg_exprs && stkSymPtr->ss_exprs) ? stkSymPtr->ss_exprs->stack->expr_code:0
							 );
					show_bad_token(NULL,emsg,MSG_WARN);
#endif
				}

			}

		} /* while */

	}
	else
	{
		list_stats.pf_value = usrstk->usr_stk_ptr;
		list_stats.f2_flag = 1;
		bad_token(tkn_ptr, "Missing Stack argument to .POP command");
		f1_eatit();
		return 1;
	}

	if ( show_line && (list_bin || meb_stats.getting_stuff) )
	{
		list_stats.pf_value = data;
		list_stats.f2_flag = 1;
	}

	f1_eatit();
	return 0;

}


/*************************************************************************
 *
 * 02-09-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *  
 *  .GETPO / .GETPOINTER
 *  Function to get the stack pointer value from Named User Stack
 *  
 * At entry:
 * 
 * At exit:
 *  variable passed from command will be set to the current stack pointer of Named stack
 *
 *
 ************************************************************************/

int op_getpointer(void)
{
	SS_struct *sym_ptr;
	struct user_stack *usrstk;
	int tt;

	/* variable token_pool = pointer to the string variable "token label" of the get_token function */
	if ( get_token() != TOKEN_strng ) /* If token is not string label (Could be label of a value, keyword, etc) */
	{
		bad_token(tkn_ptr, "Expected User Stack \"Name\" here");
		f1_eatit();
		return 1;
	}
	else
	{
		usrstk = find_stack(token_pool);
		if ( usrstk == 0 )
		{
			bad_token(inp_ptr, "Named User Stack Not Found getpointer");
			f1_eatit();
			return 1;
		}
	}

	if ( *inp_ptr == ',' )    /* If no first token then it's an error */
	{
		comma_expected = 1; /* tell get_token() to expect a comma before the token */
		/* comma_expected will be reset to a value of "0" on return */
		tt = get_token();
		if ( tt == TOKEN_strng )
		{
			sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND );  /* get a symbol, if not there   1= add   0 = no add = error */
			if ( sym_ptr == 0 )
			{
				bad_token(tkn_ptr, "ERROR-A-;ELSE ERROR  Expected Variable \"Label\" getpointer");
				f1_eatit();
				return 1;
			}
#if STACKS_DEBUG
			snprintf(emsg,sizeof(emsg),".getpointer(). Found '%s' at %p. Current value=0x%lX", sym_ptr->ss_string, (void *)sym_ptr, sym_ptr->ss_value);
			show_bad_token(NULL,emsg,MSG_WARN);
#endif
			if ( sym_ptr->flg_fwdReference || sym_ptr->flg_label || (sym_ptr->flg_defined && !sym_ptr->flg_abs) )
			{
				bad_token(tkn_ptr, "Cannot assign to symbol forward referenced or previously defined either as a label or with a complex expression");
				f1_eatit();
				return 1;
			}
			sym_ptr->flg_abs = 1;
			sym_ptr->flg_defined = 1;
			sym_ptr->ss_value = usrstk->usr_stk_ptr;    /* save user stack pointer to label value */
		}
		else
		{
			bad_token(tkn_ptr, "ERROR-E-;Expected Variable \"Label\" getpointer");
			f1_eatit();
			return 1;
		}
	}
	else
	{
		bad_token(tkn_ptr, "ERROR-E-;Expected Variable \"Label\" getpointer");
		f1_eatit();
		return 1;
	}

	if ( show_line && (list_bin || meb_stats.getting_stuff) )
	{
		list_stats.pf_value = usrstk->usr_stk_ptr;
		list_stats.f2_flag = 1;
	}

	f1_eatit();
	return 0;

}


/*************************************************************************
 *
 * 02-17-2022	added by Tim Giddens
 * 		adds support for HLLxxF
 *  
 *  .PUTPO / .PUTPOINTER
 *  Function to set the stack pointer value for Named User Stack
 *  
 * At entry:
 * 
 * At exit:
 *  variable passed from command used to set the Named stack pointer
 *
 ************************************************************************/

int op_putpointer(void)
{
	struct user_stack *usrstk;

	/* variable token_pool = pointer to the string variable "token label" of the get_token function */

	if ( get_token() != TOKEN_strng ) /* If token is not string label (Could be label of a value, keyword, etc) */
	{
		bad_token(tkn_ptr, "Expected User Stack \"Name\" here");
		f1_eatit();
		return 1;
	}
	else
	{
		usrstk = find_stack(token_pool);
		if ( usrstk == 0 )
		{
			bad_token(inp_ptr, "Named User Stack Not Found - putpointer");
			f1_eatit();
			return 1;
		}
	}

	if ( *inp_ptr == ',' )    /* If no first token then it's an error */
	{
		comma_expected = 1; /* tell get_token() to expect a comma before the token */
		/* comma_expected will be reset to a value of "0" on return */
		get_token();     /* setup the variables */

		/*  Call exprs with Force absolute which will cause error if not absolute */
		if ( exprs(0, &EXP0) < 0 )      /* evaluate the exprssion */
		{
			f1_eatit();
			return 1;
		}

		if ( (EXP0SP->expr_value > usrstk->usr_stk_size) || (EXP0SP->expr_value < 0) )
		{
			bad_token(tkn_ptr, "-P- Value Out of Range - less than zero or greater than stack size - putpointer");
			if ( show_line && (list_bin || meb_stats.getting_stuff) )
			{
				list_stats.pf_value = EXP0SP->expr_value;
				list_stats.f2_flag = 1;
			}
			f1_eatit();
			return 1;
		}
		else
		{
			usrstk->usr_stk_ptr = EXP0SP->expr_value;   /* save this ABS value to User Stack Pointer Value */
		}
	}
	else
	{
		bad_token(tkn_ptr, "ERROR-E-;Expected Variable  - putpointer");
		f1_eatit();
		return 1;
	}

	if ( show_line && (list_bin || meb_stats.getting_stuff) )
	{
		list_stats.pf_value = usrstk->usr_stk_ptr;
		list_stats.f2_flag = 1;
	}

	f1_eatit();
	return 0;

}

#endif	/* !defined(VMS) && !defined(ATARIST) */

