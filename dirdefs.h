/*
    dirdefs.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef MAC_PP
DIRDEF("DC.B",	op_dcb_b,	0)
DIRDEF("DC.BM",	op_dcb_bm,	0)
#if defined(MAC_TJ)
DIRDEF("DC.F",  op_float,	0)
#endif
DIRDEF("DC.W",	op_2mau,	DFLGEV)
DIRDEF("DC.WM",	op_2maum,	DFLGEV)
DIRDEF("DC.L",	op_4mau,	DFLGEV)
DIRDEF("DS.B",	op_blkm,	0)
DIRDEF("DS.W",	op_blk2m,	DFLGEV)
DIRDEF("DS.L",	op_blk4m,	DFLGEV)
DIRDEF("DCB.B",	op_dcbb,	0)
DIRDEF("DCB.L",	op_dcbl,	0)
DIRDEF("DCB.W",	op_dcbw,	0)
#if defined(MAC_AS)
DIRDEF("LDLIT", op_ldlit,	0)
#endif
DIRDEF("XDEF",	op_globl,	0)
DIRDEF("XREF",	op_globl,	0)
DIRDEF("XREF.S",  op_globb,	0)
#if defined(MAC_65)
DIRDEF(".ADDRESS", op_address,	0)
#endif
DIRDEF(".ALIGN",  op_align,       0)
DIRDEF(".ASCII",  op_ascii,	DFLGBM)
DIRDEF(".ASCIN",  op_ascin,	DFLGBM)
DIRDEF(".ASCIZ",  op_asciz,	DFLGBM)
DIRDEF(".ASECT",  op_asect,	0)
#if defined(MAC_65)
DIRDEF(".BANK",	op_bank,	0)
#endif
DIRDEF(".BLKB",	op_blkm,	0)
DIRDEF(".BLKW",	op_blk2m,	DFLGEV)
DIRDEF(".BLKL",	op_blk4m,	DFLGEV)
DIRDEF(".BLKM", op_blkm,	0)
DIRDEF(".BLK2M", op_blk2m,	DFLGEV)
DIRDEF(".BLK4M", op_blk4m,	DFLGEV)
DIRDEF(".BLK8M", op_blk8m,	DFLGEV)
DIRDEF(".BLKQ",   op_blk8m,      DFLGEV)
DIRDEF(".BSECT",  op_bsect,	0)
DIRDEF(".BYTE",	op_byte,	DFLGBM)
#if defined(MAC_TJ)
DIRDEF(".CCDEF", op_ccdef,	0)
#endif
DIRDEF(".CKSUM",  op_cksum,	DFLGBM)
DIRDEF(".COPY",	op_copy,	0)
#if defined(MAC_65)
DIRDEF(".CPU",	op_cpu,		0)
#endif
DIRDEF(".CROSS",  op_cross,	0)
DIRDEF(".CSECT",  op_csect,	0)
DIRDEF(".DCREF",  op_dcref,	0)
#endif  /* MAC_PP */
DIRDEF(".DEFINE", op_define,	0)
#ifndef MAC_PP
DIRDEF(".DEFSTACK", op_defstack,	0)
#if defined(MAC_65) || defined(MAC_69)
DIRDEF(".DPAGE",    op_dpage,	0)
#endif
DIRDEF(".DSABL",    op_dsabl,	0)
DIRDEF(".ECREF",    op_ecref,	0)
DIRDEF(".ENABL",    op_enabl,	0)
DIRDEF(".END",	op_end,		DFLCND|DFLMAC|DFLEND|DFLMEND)
#endif
DIRDEF(".ENDC",	op_endc,	DFLCND)
DIRDEF(".ENDM",	op_endm,	DFLMAC|DFLMEND)
#ifndef MAC_PP
DIRDEF(".ENDP", op_endp,	0)
#endif
DIRDEF(".ENDR",	op_endm,	DFLMAC|DFLMEND)
DIRDEF(".ERROR",  op_error,	0)
#ifndef MAC_PP
DIRDEF(".ESCAPE", op_escape,	0)
DIRDEF(".EVEN",	op_even,	0)
#if defined(MAC68K) || defined(MAC682K)
DIRDEF(".FILE", f1_eatit,	0) 
#endif
#if defined(MAC_TJ)
DIRDEF(".FLOAT", op_float,	0)
#endif
DIRDEF(".GETPOINTER", op_getpointer,	0)
DIRDEF(".GLOBB",  op_globb,	0)
DIRDEF(".GLOBL",  op_globl,	0)
DIRDEF(".GLOBR",  op_globr,     0)
DIRDEF(".GLOBS",  op_globb,	0)
#ifdef MAC_AS
DIRDEF(".HWORD",  op_2mau,	DFLGEV)
#endif
DIRDEF(".IDENT",  op_ident,	DFLGEV)
#endif  /* MAC_PP */
DIRDEF(".IF",	op_if,		DFLCND)
DIRDEF(".IFDF",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFEQ",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFF",	op_iff,		DFLCND)
DIRDEF(".IFG",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFGE",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFGT",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFL",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFLE",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFLT",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFNDF", op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFNE",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFNZ",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IFT",	op_ift,		DFLCND)
DIRDEF(".IFTF",	op_iftf,	DFLCND)
DIRDEF(".IFZ",	op_ifall,	DFLCND|DFLPARAM)
DIRDEF(".IIF",	op_iif,		DFLIIF)
DIRDEF(".INCLUDE", op_include,	0)
DIRDEF(".IRP",	op_irp,		DFLMAC)
DIRDEF(".IRPC",	op_irpc,	DFLMAC)
#ifndef MAC_PP
#if defined(MAC_AS)
DIRDEF(".LDLIT", op_ldlit,	0)
#endif
DIRDEF(".LENGTH", op_length,	0)
DIRDEF(".LIMIT",  op_limit,	DFLGEV)
#endif  /* MAC_PP */
DIRDEF(".LIST",	  op_list,	0)
#ifndef MAC_PP
#if defined(MAC68K) || defined(MAC682K)
DIRDEF(".LOC", f1_eatit,	0) 
#endif
DIRDEF(".LOCAL",  op_local,	0)
DIRDEF(".LONG",   op_4mau,	DFLGEV)
DIRDEF(".MACLIB", op_maclib,	0)
DIRDEF(".MACR",	op_macro,	DFLMAC)
#endif  /* MAC_PP */
DIRDEF(".MACRO",  op_macro,	DFLMAC)
#ifndef MAC_PP
DIRDEF(".MAU",	  op_mau,	0)
DIRDEF(".2MAU",	  op_2mau,	0)
DIRDEF(".4MAU",	  op_4mau,	0)
DIRDEF(".MCALL",  op_mcall,	DFLSMC)
#endif
DIRDEF(".MEXIT",  op_mexit,	0)
DIRDEF(".MPURGE", op_mpurge,	0)
DIRDEF(".NARG",	op_narg,	0)
DIRDEF(".NCHR",	op_nchr,	0)
DIRDEF(".NLIST",  op_nlist,	0)
#ifndef MAC_PP
DIRDEF(".NOCROSS", op_nocross,	0)
DIRDEF(".NTYPE",   op_ntype,	0)
DIRDEF(".ODD",	op_odd,		0)
#else
DIRDEF(".OUTFILE", op_outfile,	0)
#endif  /* MAC_PP */
#ifndef MAC_PP
DIRDEF(".PAGE",	op_page,	0)
DIRDEF(".PLACE", op_place,	0)
DIRDEF(".POP",	op_pop,		0)
DIRDEF(".PRINT", op_print,	0)
DIRDEF(".PROC",  op_proc,	0)
DIRDEF(".PSECT", op_psect,	0)
DIRDEF(".PST", f1_eatit,	DFLPST)
DIRDEF(".PUSH",	op_push,	0)
DIRDEF(".PUTPOINTER",	op_putpointer,	0)
#endif  /* MAC_PP */
DIRDEF(".RADIX", op_radix,	0)
#ifndef MAC_PP
DIRDEF(".RAD50", op_rad50,	DFLGEV)
DIRDEF(".RESTORE", op_restore,	0)
#endif
DIRDEF(".REPT",	 op_rept,	DFLMAC)
DIRDEF(".REXIT", op_rexit,	0)
#ifndef MAC_PP
DIRDEF(".SAVE", op_save,	0)
DIRDEF(".SBTTL", op_sbttl,	0)
#if defined(MAC68K) || defined(MAC682K)
DIRDEF(".SIZE", f1_eatit,	0) 
#endif
DIRDEF(".SUBSECT",op_subsect,	0)
DIRDEF(".SUBTITLE", op_sbttl,	0)
DIRDEF(".STABD", op_stabd,	0)
DIRDEF(".STABN", op_stabn,	0)
DIRDEF(".STABS", op_stabs,	0)
DIRDEF(".STRING",  op_asciz,	DFLGBM)
DIRDEF(".STATIC", op_static, 0)
DIRDEF(".SYMBOL",   op_symbol,	0)
DIRDEF(".TEST",	 op_test,	0)
DIRDEF(".TITLE", op_title,	0)
#if defined(MAC_65)
DIRDEF(".TRIPLET", op_triplet,	0)
#endif
#if defined(MAC68K) || defined(MAC682K)
DIRDEF(".TYPE", f1_eatit,	0) 
#endif
#endif /* MAC_PP */
DIRDEF(".UNDEFINE", op_undefine,	0)
#ifndef MAC_PP
#if defined(MAC_AS)
DIRDEF(".USING", op_using,	0)
#endif
DIRDEF(".VCTRS", op_vctrs,	DFLGBM /*DFLGEV*/)
#endif  /* ndef MAC_PP */
DIRDEF(".WARN",	op_warn,	0)
#ifndef MAC_PP
#ifdef MAC_AS
DIRDEF(".WORD",	op_4mau,	DFLGEV)
#else
DIRDEF(".WORD",  op_2mau,	DFLGEV)
#endif
#if defined(MAC68K) || defined(MAC682K)
DIRDEF("xref",	op_globl,	0)
DIRDEF("xref.s",op_globb,	0)
#endif
#endif /* ndef MAC_PP */
