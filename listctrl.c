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

/******************************************************************************
Change Log

	03/26/2022	- Changed added support for MAC69  - Tim Giddens

******************************************************************************/
#include "token.h"
#include "listctrl.h"
#include "memmgt.h"
#include "exproper.h"
#include "utils.h"

int show_line = 1;
int list_level;
int list_radix = 16;
void (*reset_list_params)(int onoff);

#ifndef MAC_PP
int LLIST_OPC = 14;
int LLIST_OPR = 17;
LIST_stat_t meb_stats;
#else
char listing_line[LLIST_MAXSRC+2] = "    1";
#endif

LIST_stat_t list_stats;
LIST_Source_t list_source;
union list_mask lm_bits,saved_lm_bits,qued_lm_bits;

#if 0
/**************************************************tg*/
/*  01/20/2022  Adds Support for HLLxxF  - Tim Giddens */
#include "exproper.h"
#ifdef MAC_69
int LLIST_SRC = 40;  /* because HLLxxF uses 49 ??? for line start */
int LLIST_SRC_QUED = 40;
#else
int LLIST_SRC = 40;  /* because HLLxxF uses 41 ??? for line start */
int LLIST_SRC_QUED = 40;
#endif
int LLIST_REQ_NEWL = 0;
unsigned char LLIST_TXT_BUF[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char listing_temp[LLIST_MAXSRC+2] = "     1";
int tab_wth = 8;  /* width of tab character */
/*************************************************etg*/
#endif

int list_init(int onoff)
{
	if ( onoff == 0 )
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
	{ "BEX", LIST_BEX },      /* display binary extensions */
	{ "BIN", LIST_BIN },      /* display binary */
	{ "BYT", LIST_MES },      /* display macro expansion source */
	{ "BYTE", LIST_MES },      /* display macro expansion source */
	{ "CND", LIST_CND },      /* display unsatisfied conditionals */
	{ "COD", LIST_COD },      /* display binary "as stored" */
	{ "COM", LIST_COM },      /* display comments */
	{ "LD", LIST_LD },        /* display listing directives */
	{ "LOC", LIST_LOC },      /* display location counter */
	{ "MC", LIST_MC },        /* display macro calls */
	{ "MD", LIST_MD },        /* display macro definitions */
	{ "ME", LIST_ME | LIST_MES }, /* display macro expansions */
	{ "MEB", LIST_MEB },      /* display macro expansions which produce binary */
	{ "MES", LIST_MES },      /* display macro expansion source */
	{ "OCT", 0 },             /* display address and object data in octal */
	{ "SEQ", LIST_SEQ },      /* display sequence numbers */
	{ "SRC", LIST_SRC },      /* display source */
	{ "SYM", LIST_SYM },      /* display symbol table */
	{ "TOC", LIST_TOC },      /* display table of contents */
	{ 0, 0 }
};


/******************************************************************
 * 01/19/2022   added for HLLxxF support by TG
 *
 * list_do_args - sets the list args from the input file.
 */
static void list_do_args(char *list_opt)
{
/*
 * At entry:
 *	list_opt - pointer to the .list option
		   option is either SRC, SEQ, LOC, COM, or BIN
 * At exit:
 *	SRC - Sets	LLIST_SRC
 * 			LLIST_SRC_QUED
 *			LLIST_NEWL
 *
*/
/*
arg[ListArg_ID]      = Format ID #
arg[ListArg_Dst]     = Target of data field - line location
arg[ListArg_Que]     = In this context, .ne. means to queue
arg[ListArg_Radix]   = placeholder
arg[ListArg_LZ]      = placeholder
arg[ListArg_Sign]    = placeholder
arg[ListArg_NewLine] = Flag - .ne. means flush the line
*/
	int arg[ListArg_MAX];
	int arg_cnt;
	
	memset(arg,0,sizeof(arg));
	/* Do for directive SRC */
	if ( strcmp(list_opt, "SRC") == 0 )
	{
		int pos;
/*  SRC only uses the follow args
arg[ListArg_Dst]     = Target of data field - line location
arg[ListArg_Que]     = Flag - if .NE. just queue it 
arg[ListArg_NewLine] = Flag - if .NE. request a new list line
*/
		if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
		{
			return;
		}
/* in SRC - so data starts with arg[ListArg_Dst] */
		/* is there a key word */
		arg_cnt = list_args(arg, 3, ListArg_Dst, list_source.optTextBuf, sizeof(list_source.optTextBuf));
		if ( arg_cnt == 0 )       /* Error */
		{
			/* printf( "\n No SRC data so no change  \n" ); */
			return;
		}
		pos = arg[ListArg_Dst];
		if ( pos )
		{
			pos = limitSrcPosition(pos);
			if ( arg[ListArg_Que] == 0 )        /* Do now or que it */
			{
#ifndef MAC_PP
				int sLen;
				/* This it the tricky part. Since we're moving the source position immediately, we need to move it in the line buffer */
				/* First figure out long our source line is (the only buffer it could be in is in meb_stats) */
				sLen = strlen(meb_stats.listBuffer + list_source.srcPosition);
				/* Now check that we won't write past the end of the buffer */
				if ( sLen + pos > LLIST_MAXSRC )
				{
					/* trim sLen */
					sLen -= sLen+pos-LLIST_MAXSRC;
				}
				if ( sLen )
				{
					/* Have to use memmove() to ensure the data overwrites itseslf properly (move left or move right) */
					memmove(meb_stats.listBuffer + pos, meb_stats.listBuffer + list_source.srcPosition, sLen);
					/* Make sure new position has a '\n'+0 terminator */
					if ( meb_stats.listBuffer[pos+sLen-1] != '\n' )
					{
						meb_stats.listBuffer[pos+sLen] = '\n';
						++sLen;
					}
					meb_stats.listBuffer[pos+sLen] = 0;
				}
#endif
				list_source.srcPosition = pos;
				list_source.srcPositionQued = pos;
			}
			else
			{
				/* remember this for next time */
				list_source.srcPositionQued = pos;
			}
		}
	}
/* Following not supported as of now */
	if ( strcmp(list_opt, "SEQ") == 0 )
	{

	}
	if ( strcmp(list_opt, "LOC") == 0 )
	{

	}
	if ( strcmp(list_opt, "COM") == 0 )
	{

	}
	if ( strcmp(list_opt, "BIN") == 0 )
	{

	}
	return;

}
/*************************************************etg*/

#if 0		/* DMS: This whole function is broken. It is re-written below */
static int op_list_common(int onoff)
{
	int i;
/**************************************************tg*/
/*    01-22-2022  Fix up to support HLLxxF - Tim Giddens
*/

	char *ip;
	char s1, s2;
/*************************************************etg*/
	if ( lis_fp == 0 )
	{
		f1_eatit();
		return 1;
	}
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		list_level += onoff;
		if ( list_level == 0 )
		{
			lm_bits.list_mask = saved_lm_bits.list_mask;
		}
		else if ( list_level == -1 || list_level == 1 )
		{
			if ( onoff > 0 )
			{
				lm_bits.list_mask |=
					LIST_CND | LIST_LD | LIST_MC | LIST_MD | LIST_ME | LIST_SRC | LIST_MES | LIST_BIN | LIST_LOC;
			}
			else
			{
				lm_bits.list_mask = 0;
			}
		}
		if ( list_level > 0 )
			show_line = 1;
		if ( list_level == 0 )
			show_line = list_ld;
		if ( list_level < 0 )
			show_line = 0;
		qued_lm_bits.list_mask = lm_bits.list_mask;
		return 1;
	}
	while ( 1 )
	{
		int tt;
		unsigned long old_edmask;
		old_edmask = edmask;
		edmask &= ~(ED_LC | ED_DOL);
/**************************************************tg*/
/*    01-22-2022  Fix up to support HLLxxF - Tim Giddens
 Removed
		tt = get_token();

 Added   */

		if ( *inp_ptr == ',' )
			++inp_ptr;
		ip = inp_ptr;
		while ( (isspace(*ip)) && ((cttbl[(int)*ip] & (CT_SMC | CT_EOL)) == 0) )
		{
			++ip;
		} /* eat ws */
		if ( (cttbl[(int)*ip] & (CT_SMC | CT_EOL)) != 0 )
		{
			tt = EOL;
			return 0;
		}
		else
		{
			while ( ((*ip != ',') && (*ip != '(') && (*ip != '=')) && ((cttbl[(int)*ip] & (CT_SMC | CT_EOL)) == 0) )
			{
				++ip;
			}
			if ( (cttbl[(int)*ip] & (CT_SMC | CT_EOL)) != 0 )
			{
				tt = get_token();
			}
			else
			{
				s1 = *ip;        /* save the two chars after .LIST arg */
				*ip++ = ',';        /* and replace with a \comma\0 */
				s2 = *ip;
				*ip = 0;
				tt = get_token();
				*ip = s2;        /* restore the source record */
				*--ip = s1;
			}
		}
/*************************************************etg*/
		edmask = old_edmask;
		if ( tt == EOL )
			break;
/**************************************************tg*
 * 01-22-2022  Fix up to support HLLxxF - TG
 removed
		comma_expected = 1;
 *************************************************etg*/
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a symbolic string here");
		}
		else
		{
			for ( i = 0; list_stuff[i].string != 0; ++i )
			{
				tt = strcmp(token_pool, list_stuff[i].string);
				if ( tt <= 0 )
					break;
			}
			if ( tt != 0 )
			{
				bad_token(tkn_ptr, "Unknown listing directive");
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
					if ( onoff < 0 )
					{
						saved_lm_bits.list_mask &= ~list_stuff[i].flag;
					}
					else
					{
						saved_lm_bits.list_mask |= list_stuff[i].flag;
					}
/**************************************************tg*/
/*  01/24/2021   added for HLLxxF support  by TG 
			   Don't do if .nlist */
					if ( onoff == 1 && (list_stuff[i].flag&(LIST_BIN|LIST_COM|LIST_LOC|LIST_SEQ|LIST_SRC)))
					{
						list_do_args(list_stuff[i].string);
					}
/*************************************************etg*/
				}
			}
		}
	}
	if ( list_level == 0 )
		lm_bits.list_mask = saved_lm_bits.list_mask;
	if ( list_level > 0 )
	{
		lm_bits.list_mask = saved_lm_bits.list_mask |
			LIST_CND | LIST_LD | LIST_MC | LIST_MD | LIST_ME | LIST_SRC | LIST_MES | LIST_BIN | LIST_LOC;
	}
	qued_lm_bits.list_mask = lm_bits.list_mask;
	return 1;
}
#endif		/* Broken */

static int op_list_common(int onoff)
{
	char *ip;

	if ( lis_fp == 0 )
	{
		/* No listing file, so nothing to do */
		f1_eatit();
		return 1;
	}
	/* eat ws */
	ip = inp_ptr;
	while ( ((cttbl[(int)*ip]&CT_EOL) == 0) && (isspace(*ip)) )
		++ip;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		/* This is a .NLIST/.LIST directive with no arguments */
		list_level += onoff;
		if ( list_level == 0 )
		{
			lm_bits.list_mask = saved_lm_bits.list_mask;
		}
		else if ( list_level == -1 || list_level == 1 )
		{
			if ( onoff > 0 )
			{
				lm_bits.list_mask |=
					LIST_CND | LIST_LD | LIST_MC | LIST_MD | LIST_ME | LIST_SRC | LIST_MES | LIST_BIN | LIST_LOC;
			}
			else
			{
				lm_bits.list_mask = 0;
			}
		}
		if ( list_level > 0 )
			show_line = 1;
		if ( list_level == 0 )
			show_line = list_ld;
		if ( list_level < 0 )
			show_line = 0;
		qued_lm_bits.list_mask = lm_bits.list_mask;
		return 1;
	}
	/* There must be stuff on the line */
	while ( 1 )
	{
		int ii, tt;
		unsigned long old_edmask;

		old_edmask = edmask;
		edmask &= ~(ED_LC | ED_DOL);
		tt = get_token();
		edmask = old_edmask;
		if ( tt == EOL )
			break;
		if ( tt != TOKEN_strng )
		{
			/* The argument has to be a string */
			bad_token(tkn_ptr, "Expected a symbolic string here");
			f1_eatit();	/* skip the rest of the line */
			return 1;
		}
		for ( ii = 0; list_stuff[ii].string != 0; ++ii )
		{
			/* Look through our list of arguments we handle */
			tt = strcmp(token_pool, list_stuff[ii].string);
			if ( tt <= 0 )
				break;	/* Found one */
		}
		if ( tt != 0 )
		{
			if ( !strcmp(token_pool,"TTM") )
			{
				comma_expected = 1;
				continue;	/* For purposes of compatibility, just ignore this one */
			}
			bad_token(tkn_ptr, "Unknown listing directive");
			f1_eatit();	/* Skip the rest of the line */
			return 1;
		}
		if ( onoff == 1 )
		{
			/* .LIST directive has specials for BIN, COM, LOC, SEQ and SRC */
			if ( (list_stuff[ii].flag&(LIST_BIN|LIST_COM|LIST_LOC|LIST_SEQ|LIST_SRC)) )
			{
				/* eat ws */
				while ( ((cttbl[(int)*inp_ptr]&CT_EOL) == 0) && (isspace(*inp_ptr)) )
					++inp_ptr;
				if ( *inp_ptr == '=' || *inp_ptr == '(')
				{
					list_do_args(list_stuff[ii].string);
					comma_expected = 1;
					continue;
				}
				/* Nothing special with these options, just fall through to normal */
			}
		}
		if ( !list_stuff[ii].flag )      /* Special OCT keyword */
		{
			if ( reset_list_params )
				reset_list_params(onoff);
		}
		else
		{
			if ( onoff < 0 )
			{
				saved_lm_bits.list_mask &= ~list_stuff[ii].flag;
			}
			else
			{
				saved_lm_bits.list_mask |= list_stuff[ii].flag;
			}
		}
		comma_expected = 1;
	}
	if ( list_level == 0 )
		lm_bits.list_mask = saved_lm_bits.list_mask;
	if ( list_level > 0 )
	{
		lm_bits.list_mask = saved_lm_bits.list_mask |
			LIST_CND | LIST_LD | LIST_MC | LIST_MD | LIST_ME | LIST_SRC | LIST_MES | LIST_BIN | LIST_LOC;
	}
	qued_lm_bits.list_mask = lm_bits.list_mask;
/*	printf("op_list_common(): list_level=%d, lm_bits.list_mask=0x%04X\n", list_level, lm_bits.list_mask); */
	return 1;
}

int op_list(void)
{
	op_list_common(1);
	return 0;
}

int op_nlist(void)
{
	op_list_common(-1);
	return 0;
}

#ifdef MAC_PP
char hexdig[] = "0123456789ABCDEF";
#endif

void dump_hex4(long value, char *ptr)
{
	register int val;
	register char *dst;
	val = value & 0xFFFF;
	dst = ptr + 3;
	*dst = hexdig[val & 15];   /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	return;
}

void dump_hex8(long value, char *ptr)
{
	register char *dst;
	long val = value;
	dst = ptr + 7;
	*dst = hexdig[val & 15];   /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	val >>= 4;           /* shift */
	*--dst = hexdig[val & 15]; /* copy in nibble */
	return;
}

void dump_oct3(long value, char *ptr)
{
	register int val;
	register char *dst;
	val = value & 0xFF;
	dst = ptr + 2;
	*dst = '0' + (val & 7);   /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 7); /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 3); /* copy in nibble */
	return;
}

void dump_oct6(long value, char *ptr)
{
	register int val;
	register char *dst;
	val = value & 0xFFFF;
	dst = ptr + 5;
	*dst = '0' + (val & 7);   /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 7); /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 7); /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 7); /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 7); /* copy in nibble */
	val >>= 3;           /* shift */
	*--dst = '0' + (val & 1); /* copy in nibble */
	return;
}

int limitSrcPosition(int pos)
{
	if ( pos < LLIST_SIZE )
		pos = LLIST_SIZE;
	else if ( pos > LLIST_MAX_SRC_OFFSET )
		pos  = LLIST_MAX_SRC_OFFSET;
	return pos;
}

void clear_list(LIST_stat_t *lstat)
{
	lstat->f1_flag = 0;
	lstat->f2_flag = 0;
	list_source.srcPosition = limitSrcPosition(list_source.srcPositionQued);
	memset(lstat->listBuffer, ' ', list_source.srcPosition);
	lstat->listBuffer[list_source.srcPosition] = 0;
#ifndef MAC_PP
	lstat->pc_flag = 0;
	lstat->has_stuff = 0;
	lstat->list_ptr = LLIST_OPC;
#endif
	return;
}

void display_line(LIST_stat_t *lstat)
{
	char *chrPtr, *outLinePtr;
	outLinePtr = lstat->listBuffer;
	if ( list_seq )
	{
		if ( lstat->line_no != 0 )
		{
			sprintf(outLinePtr + LLIST_SEQ, "%5ld%c",
					lstat->line_no, (lstat->include_level > 0) ? '+' : ' ');
		}
	}
#ifndef MAC_PP
	if ( list_loc )
	{
		if ( lstat->pc_flag != 0 )
		{
			if ( list_radix == 16 )
				dump_hex4((long)lstat->pc, outLinePtr + LLIST_LOC);
			else
				dump_oct6((long)lstat->pc, outLinePtr + LLIST_LOC);
		}
	}
#endif
	if ( lstat->f1_flag != 0 )
	{
		if ( list_radix == 16 )
		{
			dump_hex8((long)lstat->pf_value, outLinePtr + LLIST_PF1);
			*(outLinePtr + LLIST_PF1 + 8) = lstat->f1_flag >> 8;
		}
		else
		{
			dump_oct6((long)lstat->pf_value, outLinePtr + LLIST_PF1);
			*(outLinePtr + LLIST_PF1 + 6) = lstat->f1_flag >> 8;
		}
	}
	else
	{
		if ( lstat->f2_flag != 0 )
		{
			if ( list_radix == 16 )
			{
				dump_hex8((long)lstat->pf_value, outLinePtr + LLIST_PF2);
#ifndef MAC_PP
				*(outLinePtr + LLIST_PF2 - 1) = '(';
				*(outLinePtr + LLIST_PF2 + 8) = ')';
#endif
			}
			else
			{
				dump_oct6((long)lstat->pf_value, outLinePtr + LLIST_PF2);
#ifndef MAC_PP
				*(outLinePtr + LLIST_PF2 - 1) = '(';
				*(outLinePtr + LLIST_PF2 + 6) = ')';
#endif
			}
		}
    }
	for ( chrPtr = outLinePtr; chrPtr < outLinePtr + list_source.srcPosition; ++chrPtr )
    {
		/* Replace any nul's with ' ' in area leading up to source position unless we find a newline first */
		if ( *chrPtr == '\n' )
			break;
		if ( *chrPtr == 0 )
			*chrPtr = ' ';
    }
	/* chrPtr will point to a nul if this is NOT a meb thing. I.e. nothing has yet been placed in the source region */
    if ( chrPtr[0] == 0)
	{
#ifndef MAC_PP
		int flg = line_errors_index;
		if ( !flg )
		{
			if ( macro_nesting == 0 )
				flg = list_src;
			else if ( macro_nesting == 1 )
				flg = list_mc;
			else
				flg = list_me || list_mes;
		}
		if ( flg )
		{
			chrPtr += deTab(inp_str, 8, 0, 0, lstat->listBuffer + list_source.srcPosition, sizeof(meb_stats.listBuffer) - list_source.srcPosition);
			/* make sure it is '\n'+nul terminated */
			if ( chrPtr[-1] != '\n' )
				*chrPtr++ = '\n';
			*chrPtr = 0;
		}
		else
		{
			if ( !strnrchr(outLinePtr,'\n',chrPtr-outLinePtr) )
			{
				/* It's just a blank line */
				*chrPtr++ = '\n';
				*chrPtr = 0;
			}
		}

#else
		if ( lstat->listBuffer+LLIST_MAXSRC - chrPtr - 2 > 0 )
			strncpy(chrPtr, inp_str, lstat->listBuffer+LLIST_MAXSRC - chrPtr - 2);
		lstat->listBuffer[LLIST_MAXSRC] = '\n';
		lstat->listBuffer[LLIST_MAXSRC + 1] = 0;
#endif
	}
	puts_lis(outLinePtr, 1);
	if ( line_errors_index != 0 )
	{
		int z;
		for ( z = 0; z < line_errors_index; ++z )
		{
			err_msg(line_errors_sev[z] | MSG_CTRL | MSG_NOSTDERR, line_errors[z]);
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
	if ( meb_stats.getting_stuff != 0 )
	{
		if ( line_errors_index == 0 )
		{
			show_line = 1;     /* assume to show it */
			return;
		}
		if ( meb_stats.has_stuff != 0 || meb_stats.list_ptr != LLIST_OPC )
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
	if ( show_line != 0 || line_errors_index != 0 )
	{
		display_line(&list_stats);
	}
	show_line = 1;       /* assume to show it next time */
	return;
}

#ifndef MAC_PP
int fixup_overflow(LIST_stat_t *lstat)
{
	char *lp;
	display_line(lstat);
	lstat->expected_pc = lstat->pc = current_offset;
	lstat->expected_seg = current_section;
	lstat->pc_flag = 1;
	lstat->line_no = 0;          /* don't print seq numb next time */
	lp = lstat->listBuffer + list_source.srcPosition;
	*lp++ = '\n';
	*lp = 0;
	return 0;
}

void n_to_list(int nibbles, long value, int tag)
{
	register unsigned long tv;
	register char *lpr;
	LIST_stat_t *lstat;
	int lcnt, nyb;

	if ( !pass )
		return;
	tv = value;
	nyb = nibbles;
	lcnt = LLIST_SIZE - nyb - 1;
	if ( tag != 0 )
		lcnt -= 1;
	if ( meb_stats.getting_stuff )
	{
		if ( meb_stats.pc_flag == 0 )
		{
			meb_stats.expected_pc = meb_stats.pc = current_offset;
			meb_stats.expected_seg = current_section;
			meb_stats.pc_flag = 1;
		}
		if ( line_errors_index != 0 )
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
			if ( (meb_stats.list_ptr != LLIST_OPC) &&
				 (meb_stats.pc_flag &&
				  ((meb_stats.expected_pc != current_offset) ||
				   (meb_stats.expected_seg != current_section))) )
			{
				fixup_overflow(&meb_stats);
			}
		}
	}
	else
	{
		lstat = &list_stats;
	}
	if ( lstat->list_ptr >= lcnt )
	{
		if ( !list_bex )
			return;
		fixup_overflow(lstat);
	}
	lpr = lstat->listBuffer + lstat->list_ptr + nyb;
	if ( tag != 0 )
	{
		*lpr = tag;
		lcnt = 2;
	}
	else
	{
		lcnt = 1;
	}
	lstat->list_ptr += lcnt + nyb;
	if ( list_radix == 16 )
	{
		do
		{
			*--lpr = hexdig[(int)(tv & 15)];
			tv >>= 4;
		} while ( --nyb > 0 );
	}
	else
	{
		int mask = (nyb == 3 || nyb == 11) ? 3:1;
		do
		{
			*--lpr = '0' + (tv & 7);
			tv >>= 3;
		} while ( --nyb > 1 );
		if ( nyb )
			*--lpr = '0' + (tv & mask);
	}
	return;
}
#endif

/* ListArg Error Messages */
typedef struct
{
	int max;
	int isFlag;
	char *msg;
} ListErrs_t;

static const ListErrs_t ListErrs[ListArg_MAX] =
{
	{ 15, 0, "Format ID number"},
	{132, 0, "Location on list line"},
	{  8, 0, "Number of fields to list"},
	{ 16, 0, "RADIX to used"},
	{255, 1, "Suppressed zeros"},
	{255, 1, "Prefixed signs"},
	{255, 1, "New line (placeholder)"}
};

/******************************************************************
 * 01/19/2022   added for HLLxxF support by TG
 *
 * list_args - gets the list args from the input file.
 */
/** list_args() - parse optional arguments on a .LIST directive
 * At entry:
 * @param arg - pointer to an int array
 * @param maxIdx - maximum number of arguments to expect
 * @param idx - starting element in array to process
 * @param optTextBuf - pointer to place to deposit any optional text
 * @param optTextBufSize - size of buffer above
 * Global pointers:
 *	inp_ptr - points to next source byte
 *	inp_str - contains the source line
 *
 * At exit:
 * @return Number of arguments parsed
 *	Array arg - filled with source args
 *
 * @note
 *  Array
 *      arg[ListArg_ID] = Format ID #
 *		arg[ListArg_Dst] = Target of data field - line location
 *		arg[ListArg_Len] = Lenght of field - lenght of data in bytes 
 *		arg[ListArg_Radix] = Field Radix
 *		arg[ListArg_LZ] = Flag - if .NE. include leading zeros 
 *		arg[ListArg_Sign] = Flag - if .NE. prefix a sign
 *		arg[ListArg_NewLine] = Flag - if .NE. request a new list line
 *
 *
 *   Setting with .LIST BIN() not supported for now
 */
int list_args(int arg[ListArg_MAX], const int maxIdx, int idx, unsigned char *optTextBuf, size_t optTextBufSize)
{
	int cr = current_radix;    /* save current */
	int numArgs = 0;
	
	current_radix = 10;      /* set RADIX to 10 */
	if ( optTextBuf )
	{
		if ( optTextBufSize > 0 )
			optTextBuf[0] = 0;
		if ( optTextBufSize > 1 )
			optTextBuf[1] = 0;
	}
	/* clear data buffer */
	memset(arg,0,ListArg_MAX*sizeof(int));
	/* eat optional equal sign */
	if ( *inp_ptr == '=' )
		inp_ptr++;
	if ( *inp_ptr == '(' )
	{
		/* eat paren */
		inp_ptr++;
		/* just in case */
		comma_expected = 0;
		while ( 1 )
		{
			if ( *inp_ptr == '/' )
			{
				arg[ListArg_NewLine] = 1;
				inp_ptr++;
				if ( *inp_ptr == ')' )
				{
					inp_ptr++;
					break;
				}
			}
			if ( *inp_ptr == ',' )        /* any more tokens on this line of code */
			{
				inp_ptr++;
			}
			get_token();     /* setup the variables */
			/*  Call exprs with Force absolute which will cause error if not absolute */
			exprs(0, &EXP0);      /* evaluate the exprssion */
			if ( *inp_ptr == '\'' )       /* is it text */
			{
				if ( optTextBuf && optTextBufSize > 2 )
				{
					*optTextBuf++ = EXP0SP->expr_value;
					--optTextBufSize;
				}
				while ( 1 )
				{
					inp_ptr++;
					if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
					{
						bad_token(inp_ptr, "Missing closing apostrophie");
						f1_eatit();
						current_radix = cr;
						return (0);
					}
					if ( *inp_ptr == '\'' )   /* is it end of text */
					{
						if ( *(inp_ptr + 1) == '\'' )   /* is it 2 in a row */
						{
							inp_ptr++;
							if ( optTextBuf && optTextBufSize > 1 )
							{
								*optTextBuf++ = *inp_ptr;
								*optTextBuf = 0;
								--optTextBufSize;
							}
							if ( optTextBufSize <= 1 )
							{
								/* Reached end of text data buffer (out of storage space) */
								bad_token(inp_ptr, "Line List text buffer overflow, Max of 16 Characters");
								f1_eatit();
								current_radix = cr;
								return (0);
							}
						}
						else
						{
							inp_ptr++;
							break;
						}
					}
					else
					{
						if ( optTextBuf && optTextBufSize > 1 )
						{
							*optTextBuf++ = *inp_ptr;
							*optTextBuf = 0;
							--optTextBufSize;
						}
						if ( optTextBufSize <= 1 )
						{
							/* Reached end of text data buffer (out of storage space) */
							bad_token(inp_ptr, "Line List text buffer overflow, Max of 16 Characters");
							f1_eatit();
							current_radix = cr;
							return (0);
						}
					}
				}
			}
			else
			{
				if ( ListErrs[idx].isFlag )
					arg[idx] = EXP0SP->expr_value ? 1 : 0;
				else
				{
					arg[idx] = EXP0SP->expr_value;
					if ( arg[idx] > ListErrs[idx].max )
					{
						sprintf(emsg, "%s set to max value of %d  , requested %d", ListErrs[idx].msg, ListErrs[idx].max, arg[idx]);
						show_bad_token((inp_ptr - 1), emsg, MSG_ERROR);
						arg[idx] = ListErrs[idx].max;
					}
				}
				idx++;
				++numArgs;
				if ( numArgs >= maxIdx )
				{
					sprintf(emsg, "Too many directive arguments, Found %d, Max of %d allowed", numArgs, maxIdx);
					show_bad_token((inp_ptr), emsg, MSG_ERROR);
					f1_eatit();
					current_radix = cr;
					return (0);
				}
			}
			if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			{
				bad_token(inp_ptr, "Missing closing parenthesis");
				f1_eatit();
				current_radix = cr;
				return (0);
			}
			if ( *inp_ptr == ')' )
			{
				inp_ptr++;
				break;
			}
			if ( *inp_ptr == ',' )        /* any more tokens on this line of code */
			{
				inp_ptr++;
			}
		}
	}
	current_radix = cr;      /* restore radix */
	return numArgs;
}



