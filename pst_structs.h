/*
    pst_structs.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _PST_STRUCTS_H
#define _PST_STRUCTS_H_ 1

#if defined(INTERNAL_PACKED_STRUCTS)
#include "pragma1.h"
#endif

struct opcpst {
   char *name;			/* ptr to name */
   uint16_t value;	/* base value */
#if !defined(MAC_TJ)
   OPClass class;		/* opcode class */
#else
   unsigned int class;
#endif
#if defined(MAC_68K)
   int bwl;
#else
   uint32_t amodes;	/* legal address modes */
#endif
#if defined(MAC_65)
   uint32_t aamodes;	/* alternate address modes */
   uint32_t aaamodes;	/* alternate alternate address modes */
#endif
};

typedef struct opcpst Opcpst;
   
struct dirpst {
   char *name;			/* ptr to name */
   int (*func)();		/* ptr to directive handler */
   DIRFlags flags;	/* directive flags */
};

typedef struct dirpst Dirpst;

typedef struct macargs {
   char **mac_keywrd;	/* pointer to array of char *'s containing keywords */
   char **mac_default;	/* pointer to array of char *'s containing defaults */
   uint8_t *mac_gsflag;	/* ptr to array of char's having generated sym flag */
   uint8_t *mac_body;	/* pointer to macro body */
   uint8_t *mac_end;	/* pointer to macro EOM */
   int mac_numargs;	/* number of arguments in definition */
} Macargs;

typedef struct opcode {
   char *op_name;		/* opcode/directive/macro name */
   struct opcode *op_next;	/* ptr to next opcode in chain */
   union {
      uint32_t val;	/* base value of opcode */
      int (*fnc)();		/* ptr to handler if psuedo-op */
      struct macargs *mrg;	/* ptr to macro argument struct if macro */
   } types;
   uint32_t op_amode;	/* address modes (> 16 bits rqd for 65816) */
   int16_t op_class;		/* opcode class */
} Opcode;   

extern Opcode *opcode_lookup(char *strng, int err_flag );
extern void do_opcode(Opcode *opc);
extern int macro_call(Opcode *opc);
extern void deleteAllMacros(void);
extern void free_macbody( uint8_t *link );

#define op_value types.val
#define op_func  types.fnc
#define op_margs types.mrg

typedef struct mcall_struct {
   uint8_t *marg_top;		/* pointer to top of macro (for rewind) */
   uint8_t *marg_ptr;		/* pointer to next char in macro to read */
   uint8_t *marg_end;		/* pointer to EOM character */
   char **marg_args;		/* pointer to array of pointers of args */
   int marg_cndlvl;		/* saved conditional level */
   int marg_cndnst;		/* saved conditional nest */
   long marg_cndpol;		/* saved polarity */
   long marg_cndwrd;		/* saved conditions */
   struct mcall_struct *marg_next; /* pointer to previous mcall struct */
   uint16_t marg_count;	/* loop count for .REPT */
   uint16_t marg_icount;	/* initial loop or argument count */
   uint16_t marg_flag;	/* macro type (.IRP(C)/.REPT/macro_call */
} Mcall_struct;

#define OP_HASH_SIZE 359
#define OP_HASH_INDX 31

#define MAC_EOM  0xFF
#define MAC_LINK 0xFE

#if defined(INTERNAL_PACKED_STRUCTS)
#include "pragma.h"
#endif
#endif      /* _PST_STRUCTS_H_ */
