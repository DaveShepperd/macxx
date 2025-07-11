/*
    debug.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _DEBUG_H_
#define _DEBUG_H_ 1

#include <stdint.h>
#include "stab.h"
#include "segdef.h"

#define OD_MAGIC	0x12344321

typedef struct od_hdr {
   uint32_t magic;			/* file's magic number (for Unix compat) */
   uint32_t sec_size;		/* section vector table size in bytes */
   uint32_t file_size;		/* file vector table size in bytes */
   uint32_t sym_size;		/* symbol vector table size in bytes */
   uint32_t line_size;		/* line number vector table size in bytes */
   uint32_t string_size;		/* string table size in bytes */
   int32_t time_stamp;		/* creation time stamp (version #) */
   int32_t have_csrc;		/* flag indicating there's C src file(s) */
} OD_hdr;

typedef union {
   uint8_t *str_ptr;		/* converted to real ptr in memory */
   uint32_t index;			/* simply an index */
} OD_ptr;
   
typedef struct od_sec {
   uint32_t base;			/* section base address */
   uint32_t length;		/* length of section in bytes */
   uint32_t offset;		/* output offset */
   OD_ptr name;			/* ptr to name of section */
   uint32_t flags;			/* see segdef.h for details (VSEG_xxx) */
} OD_sec;

typedef struct od_file {
   OD_ptr name;			/* ptr to filename */
   OD_ptr version;		/* ptr to version number */
} OD_file;

typedef struct od_sym {
   OD_ptr name;			/* ptr to symbol name */
   uint8_t type;			/* symbol type */
   uint8_t other;			/* I think this is always 0 */
   int16_t desc;			/* used for various things */
   union {
      uint32_t value;		/* symbol's value (where appropriate) */
      void *ptr;		/* or ptr to some struct type or other */
   } val;
} OD_sym;

enum line_type { LINE_NUM,	/* source line number */
   		 LINE_NEW_SFILE,/* change of ASM file number */
   		 LINE_NEW_CFILESO,	/* C source file */
   		 LINE_NEW_CFILESOL,	/* C include file */
   		 LINE_NEW_BASE	/* new base address */
};

typedef struct {
   uint8_t  type;			/* set to LINE_NUM */
   uint8_t  length;		/* length of output in bytes */
   uint16_t sline;		/* asm line # */
   uint16_t cline;		/* C line # */
} OD_sline;			/* source line type */

typedef struct {
   uint8_t type;			/* set to LINE_SFILE or LINE_CFILE */
   uint8_t filenum;		/* set to filename index */
} OD_fline;

typedef struct {
   uint8_t type;			/* set to LINE_NEW_BASE */
   uint8_t segnum;		/* set to segment number */
   uint32_t offset;		/* and new offset from segment */
} OD_bline;

typedef union {
   OD_sline sl;			/* source line numbers */
   OD_bline bl;			/* base selection */
   OD_fline fl;			/* file selection */
} OD_line;

typedef struct od_elem {
   struct od_elem *next;	/* ptr to next element in chain */
   void *top;			/* ptr to first item in this list */
   void *item;			/* ptr to next available item in list */
   int alloc;			/* number of items allocated */
   int avail;			/* number of items available */
} OD_elem;

typedef struct od_top {
   OD_elem *top;		/* ptr to first element in chain */
   OD_elem *curr;		/* ptr to element in chain currently in use */
   long length;			/* total elements used in all chains */
   int index;			/* index of current element */
} OD_top;

extern OD_hdr od_hdr;		/* debug file header struct */
extern OD_top od_string;	/* string vector table */
extern OD_top od_sec;		/* section vector table */
extern OD_top od_file;		/* file vector table */
extern OD_top od_sym;		/* symbol vector table */
extern OD_top od_line;		/* line vector table */

#endif  /* _DEBUG_H_ */
