/*
    vlda_structs.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _VLDA_STRUCTS_H_
#define _VLDA_STRUCTS_H_ 1

#if defined(EXTERNAL_PACKED_STRUCTS)
#include "pragma1.h"
#endif

#include <inttypes.h>

#define VLDA_MAJOR 3	/* version number (ala 1.0) */
#if 0
#define VLDA_MINOR 1
#else
#define VLDA_MINOR 0
#endif

/************************************************************************
 * 		WARNING WARNING - DANGER DANGER				*
 *									*
 * 	DO NOT CHANGE THE ORDER OF THE FOLLOWING ITEMS!!!		*
 *									*
 * You can add items in front of VLDA_EXPR_SYM and at the end, but	*
 * nowhere inbetween. Doing so may make all existing vlda files		*
 * unusable.								*
 ************************************************************************/
 
enum vlda_nums {
   VLDA_ABS   	=0,	/* vlda relocatible text record */
   VLDA_TXT	=8,	/* vlda relocatible text record */
   VLDA_GSD,		/* psect/symbol definition record */
   VLDA_ORG,		/* set the PC record */
   VLDA_ID,		/* misc header information */
   VLDA_EXPR,		/* expression */
   VLDA_TPR,		/* transparent record (raw data follows */
   VLDA_SLEN,		/* segment length */
   VLDA_XFER,		/* transfer address */
   VLDA_TEST,		/* test and display message if result is false */
   VLDA_DBGDFILE,	/* dbg file specification */
   VLDA_DBGSEG,		/* dbg segment descriptors */
   VLDA_BOFF,		/* branch offset out of range test */
   VLDA_OOR,		/* operand value out of range test */

   VLDA_EXPR_SYM=0,	/* expression component is a symbol */
   VLDA_EXPR_VALUE,	/* expression component is an absolute value */
   VLDA_EXPR_OPER,	/* expression component is an operator */
   VLDA_EXPR_L,		/* expression component is an L */
   VLDA_EXPR_B,		/* expression component is a  B */
   VLDA_EXPR_TAG,	/* expression tag follows */
   VLDA_EXPR_CSYM,	/* symbol with 1 byte identifier */
   VLDA_EXPR_CVALUE,	/* 1 byte value */
   VLDA_EXPR_WVALUE,	/* 2 byte value */
   VLDA_EXPR_1TAG,	/* tag with implied count of 1 */
   VLDA_EXPR_CTAG,	/* tag with 1 byte count */
   VLDA_EXPR_WTAG,	/* tag with 2 byte count */
   VLDA_EXPR_0VALUE	/* value of 0 */
};

/************************************************************************
 *	End of DO NOT MOVE						*
 ************************************************************************/

#if ALIGNMENT == 0
typedef uint8_t Sentinel;
#else
#if ALIGNMENT == 1
typedef uint16_t Sentinel;
#else
typedef uint32_t Sentinel;
#endif
#endif

typedef struct vlda_abs {
   Sentinel vlda_type;		/* record type: VLDA_ABS or VLDA_XFER */
   uint32_t vlda_addr;
} VLDA_abs;

typedef struct vlda_id {
   Sentinel vid_rectyp;		/* record type: VLDA_ID */
   Sentinel vid_siz;		/* size of this structure in bytes */
   uint16_t vid_maj;	/* major version number of creating image */
   uint16_t vid_min;	/* minor version number of creating image */
   uint16_t vid_symsiz;	/* size of symbol structure */
   uint16_t vid_segsiz;	/* size of segment structure */
   int16_t vid_image;		/* offset to image name */
   int16_t vid_target;		/* offset to target string */
   int16_t vid_time;		/* offset to date/time string */
   uint8_t vid_errors;	/* error count */
   uint8_t vid_warns;	/* warning count */
#if 0
   uint16_t vid_maxtoken;	/* maximum token length */
#endif
} VLDA_id;

#include "segdef.h"

typedef struct vlda_sym {
   Sentinel vsym_rectyp;	/* record type: VLDA_GSD */
   uint16_t vsym_flags;		/* symbol flags (VSEG_DEF bit is 0) */
   int16_t vsym_noff;		/* offset to symbol name */
   uint16_t vsym_ident;		/* symbol's ident */
   int32_t vsym_value;		/* value */
   int16_t vsym_eoff;		/* offset to expression */
} VLDA_sym; 

typedef struct vlda_seg {
   Sentinel vseg_rectyp;	/* record type: VLDA_GSD */
   uint16_t vseg_flags;		/* segment flags (VSEG_DEF bit is 1)*/
   int16_t vseg_noff;		/* offset to symbol name */
   uint16_t vseg_ident;		/* symbol's ident */
   int16_t vseg_salign;		/* alignment value */
   int16_t vseg_dalign;		/* alignment value */
   int32_t vseg_base;		/* value */
   uint32_t vseg_maxlen;	/* maximum length for segment group */
   int32_t vseg_offset;		/* output offset value */
} VLDA_seg;

typedef struct vlda_slen {
   Sentinel vslen_rectyp;	/* record type: VLDA_SLEN  */
   uint16_t vslen_ident;	/* segment's ident */
   int32_t vslen_len;		/* segment length */
} VLDA_slen;

typedef struct vlda_test {
   Sentinel vtest_rectyp;	/* record type: VLDA_TEST */
   uint16_t vtest_eoff;		/* offset to expression (s/b sizeof this struct) */
   uint16_t vtest_soff;		/* offset to message (soff-eoff = sizeof expression) */
} VLDA_test;

typedef union vexp {
   Sentinel *vexp_len;		/* number of items */
   Sentinel *vexp_type;		/* type code */
   Sentinel *vexp_oper;		/* operator */
   char *vexp_chp;		/* pointer to char (for length computations) */
   uint16_t *vexp_ident;	/* identifier */
   int32_t *vexp_const;		/* constant */
#if ALIGNMENT > 0
#if ALIGNMENT == 1
   int16_t *vexp_byte;		/* 68000 gets complicated, don't use bytes */
#else
   int32_t *vexp_byte;		/* Sparc gets complicated, don't use bytes */
#endif
#else
   char *vexp_byte;
#endif
   int16_t *vexp_word;
   int32_t *vexp_long;
} VLDA_vexp;

typedef struct vlda_dbgdfile {
   Sentinel type;		/* record type: VLDA_DBGDFILE */
   uint16_t name;		/* index to filename */
   uint16_t version;	/* index to version */
} VLDA_dbgdfile;

typedef struct vlda_dbgseg {
   Sentinel type;		/* record type: VLDA_DBGSEG */
   uint32_t base;		/* section base */
   uint32_t length;		/* section length */
   uint32_t offset;		/* section offset */
   uint16_t name;		/* offset to section name */
   uint16_t flags;		/* section flags */
} VLDA_dbgseg;

#if defined(EXTERNAL_PACKED_STRUCTS)
#include "pragma.h"
#endif

#endif /* _VLDA_STRUCTS_H_ */
