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

int show_line = 1;
int list_level;
int list_radix = 16;
void (*reset_list_params)(int onoff);
#ifndef MAC_PP
	#if 0
		#define LLIST_OPC	14	/* opcode */
		#define LLIST_OPR	17	/* operands */
	#endif
int LLIST_OPC = 14;
int LLIST_OPR = 17;
char listing_meb[LLIST_MAXSRC+2] = "     1";
char listing_line[LLIST_MAXSRC+2] = "     1";
LIST_stat meb_stats = { listing_meb, listing_line + LLIST_MAXSRC + 2 };
#else
char listing_line[LLIST_MAXSRC+2] = "    1";
#endif
LIST_stat list_stats = { listing_line, listing_line + LLIST_MAXSRC + 2 };
union list_mask lm_bits,saved_lm_bits,qued_lm_bits;

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
void list_do_args(char *list_opt);
char listing_temp[LLIST_MAXSRC+2] = "     1";
int tab_wth = 8;  /* width of tab character */
/*************************************************etg*/
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
	{ "ME", LIST_ME | LIST_MEB | LIST_MES }, /* display macro expansions */
	{ "MEB", LIST_MEB },      /* display macro expansions which produce binary */
	{ "MES", LIST_MES },      /* display macro expansion source */
	{ "OCT", 0 },             /* display address and object data in octal */
	{ "SEQ", LIST_SEQ },      /* display sequence numbers */
	{ "SRC", LIST_SRC },      /* display source */
	{ "SYM", LIST_SYM },      /* display symbol table */
	{ "TOC", LIST_TOC },      /* display table of contents */
	{ 0, 0 }
};

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
					if ( onoff == 1 )
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

void clear_list(LIST_stat *lstat)
{
	lstat->f1_flag = 0;
	lstat->f2_flag = 0;
	memset(lstat->line_ptr, ' ', LLIST_SIZE);
	*(lstat->line_ptr + LLIST_SIZE) = 0;
/**************************************************tg*/
/*  01/27/2022  Adds Support for HLLxxF  - TG */

	LLIST_SRC = LLIST_SRC_QUED;

/*************************************************etg*/
#ifndef MAC_PP
	lstat->pc_flag = 0;
	lstat->has_stuff = 0;
	lstat->list_ptr = LLIST_OPC;
#endif
	return;
}

void display_line(LIST_stat *lstat)
{
/**************************************************tg*/
/*  01/26/2022  Adds Support for HLLxxF  - TG */

	char *tmp_inp_str;
	int tab_cnt;
/*************************************************etg*/
	register char *s, *lp;
	lp = lstat->line_ptr;
	if ( list_seq )
	{
		if ( lstat->line_no != 0 )
		{
			sprintf(lp + LLIST_SEQ, "%5ld%c",
					lstat->line_no, (lstat->include_level > 0) ? '+' : ' ');
		}
	}
#ifndef MAC_PP
	if ( list_loc )
	{
		if ( lstat->pc_flag != 0 )
		{
			if ( list_radix == 16 )
				dump_hex4((long)lstat->pc, lp + LLIST_LOC);
			else
				dump_oct6((long)lstat->pc, lp + LLIST_LOC);
		}
	}
#endif
	if ( lstat->f1_flag != 0 )
	{
		if ( list_radix == 16 )
		{
			dump_hex8((long)lstat->pf_value, lp + LLIST_PF1);
			*(lp + LLIST_PF1 + 8) = lstat->f1_flag >> 8;
		}
		else
		{
			dump_oct6((long)lstat->pf_value, lp + LLIST_PF1);
			*(lp + LLIST_PF1 + 6) = lstat->f1_flag >> 8;
		}
	}
	else
	{
		if ( lstat->f2_flag != 0 )
		{
			if ( list_radix == 16 )
			{
				dump_hex8((long)lstat->pf_value, lp + LLIST_PF2);
#ifndef MAC_PP
				*(lp + LLIST_PF2 - 1) = '(';
				*(lp + LLIST_PF2 + 8) = ')';
#endif
			}
			else
			{
				dump_oct6((long)lstat->pf_value, lp + LLIST_PF2);
#ifndef MAC_PP
				*(lp + LLIST_PF2 - 1) = '(';
				*(lp + LLIST_PF2 + 6) = ')';
#endif
			}
		}
	}
/**************************************************tg*/
/*  01/26/2022  Adds Support for HLLxxF  - TG
added*/


	for ( s = lp; s < lp + LLIST_SIZE; ++s )   /* Convert all zero's to spaces */
	{
		if ( *s == 0 )
			*s = ' ';
	}


/*    if (*s == 1)  * List macro call source line ? */
	if ( 0 == 1 )  /* List macro call source line ? */
	{
#ifndef MAC_PP
		if ( list_src != 0 || line_errors_index != 0 )
		{
#endif

			if ( listing_temp[LLIST_SIZE] == '\t' )
			{
				for ( ; s < (lp + LLIST_SRC + tab_wth); ++s )
				{
					*s = ' ';
				}
				strcpy(s, &listing_temp[LLIST_SIZE + 1]);
			}
			else
			{
				for ( ; s < (lp + LLIST_SRC); ++s )
				{
					*s = ' ';
				}
				strcpy(s, &listing_temp[LLIST_SIZE]);
			}

#ifndef MAC_PP
		}
		else
		{
			*s++ = '\n';
			*s = 0;
		}
#endif

	}





	if ( *s == 0 )   /* List source code ? (0 for yes) */


/*************************************************etg*/
	{
#ifndef MAC_PP
		int sho;
		sho = (macro_nesting > 0) ? list_mes : list_src;
		if ( sho != 0 || line_errors_index != 0 )
		{
#endif
/**************************************************tg*/
/* 01/19/2022   added by TG 


*/


			for ( ; s < (lp + LLIST_SRC); ++s )
			{
				*s = ' ';
			}



			tmp_inp_str = inp_str;
			for ( tab_cnt = 0; tab_cnt < tab_wth; ++tab_cnt )
			{
				if ( *tmp_inp_str != '\t' )
				{
					*s++ = *tmp_inp_str++;
				}
				else
				{

					for ( ; s < (lp + LLIST_SRC + tab_wth); ++s )
					{
						*s = ' ';

					}
					++tmp_inp_str;  /* skip tab */
					break;
				}
			}









			lstat->line_ptr[LLIST_MAXSRC] = '\n';
			lstat->line_ptr[LLIST_MAXSRC + 1] = 0;
			if ( lstat->line_end - s - 2 > 0 )
				strncpy(s, tmp_inp_str, lstat->line_end - s - 2);


/*            if (lstat->line_end-s-2 > 0) strncpy(s, tmp_inp_str, lstat->line_end-s-2);
 *************************************************etg*/
/*            lstat->line_ptr[LLIST_MAXSRC] = '\n';
			lstat->line_ptr[LLIST_MAXSRC+1] = 0;
			if (lstat->line_end-s-2 > 0) strncpy(s, inp_str, lstat->line_end-s-2);
*/

#ifndef MAC_PP
		}
		else
		{
			*s++ = '\n';
			*s = 0;
		}
#endif
	}


	puts_lis(lp, 1);
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
	show_line = 1;       /* assume to show it */
	return;
}

#ifndef MAC_PP
int fixup_overflow(LIST_stat *lstat)
{
	char *lp;
	display_line(lstat);
	lstat->expected_pc = lstat->pc = current_offset;
	lstat->expected_seg = current_section;
	lstat->pc_flag = 1;
	lstat->line_no = 0;          /* don't print seq numb next time */
	lp = lstat->line_ptr + LLIST_SIZE;
	*lp++ = '\n';
	*lp = 0;
	return 0;
}

void n_to_list(int nibbles, long value, int tag)
{
	register unsigned long tv;
	register char *lpr;
	LIST_stat *lstat;
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
	lpr = lstat->line_ptr + lstat->list_ptr + nyb;
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
		do
		{
			*--lpr = '0' + (tv & 7);
			tv >>= 3;
		} while ( --nyb > 1 );
		if ( nyb )
			*--lpr = '0' + (tv & 1);
	}
	return;
}
#endif


/******************************************************************
 * 01/19/2022   added for HLLxxF support by TG
 *
 * list_args - gets the list args from the input file.
 */
int list_args(int *arg, int cnt)
/*
 * At entry:
 * 	    arg - pointer to an int array with a size of at least seven
 *	inp_ptr - points to next source byte
 *	inp_str - contains the source line
 *
 * At exit:
 *	Array arg - filled with source args
 *
 *	returns a 0 if no args, otherwise returns the number of args
 *
 * Notes:
 *	Array	arg[0] = Format ID #
 *		arg[1] = Target of data field - line location
 *		arg[2] = Lenght of field - lenght of data in bytes 
 *		arg[3] = Field Radix
 *		arg[4] = Flag - if .NE. suppress leading zeros 
 *		arg[5] = Flag - if .NE. prefix a sign
 *		arg[6] = Flag - if .NE. request a new list line
 *
 *
 *   Setting with .LIST BIN() not supported for now
 */

{


/*
as of now these options not supported

Format ID's
Number of Fields

prefixed signs


*/


	int cr = current_radix;    /* save current */
	char *err_arg[6];
	int err_cnt;
	unsigned char *txtbuf_ptr, *txtbuf_end;


	/* Error Codes for testing */
	int tst[7] = { 15, 132, 8, 16, 255, 255, 0 };
	/*  Max values of Variables
	tst[0] =  15.	;no more than 15 formats (ID's)
	tst[1] = 132.	;line location can't be greater than 132
	tst[2] =   8.	;number fields can't be greater than 8
	tst[3] =  16.	;RADIX can't be greater than 16
	tst[4] =  -1	;no limit on number of leading zeros suppressed
	tst[5] =  -1	;no limit on number of prefixed signs
	tst[6] =   0	;not used - flag to request a new listing line
*/

	/* Error Messages */
	err_arg[0] = "Format ID number";
	err_arg[1] = "Location on list line";
	err_arg[2] = "Number of fields to list";
	err_arg[3] = "RADIX to used";
	err_arg[4] = "Suppressed zeros";
	err_arg[5] = "Prefixed signs";



	current_radix = 10;      /* set RADIX to 10 */








	txtbuf_ptr = LLIST_TXT_BUF;
	/* save one space null at the end of buffer */
	txtbuf_end = LLIST_TXT_BUF + sizeof(LLIST_TXT_BUF) - 1;

	/* clear text buffer */
	for ( err_cnt = 0; err_cnt <= (sizeof(LLIST_TXT_BUF)); err_cnt++ )
	{
		txtbuf_ptr[err_cnt] = 0;
	}

	/* clear data buffer */
	for ( err_cnt = 0; err_cnt <= 6; err_cnt++ )
	{
		arg[err_cnt] = 0;
	}

	/* error count */
	err_cnt = 6 - cnt;



	if ( *inp_ptr == '=' )
		inp_ptr++;
	if ( *inp_ptr == '(' )
	{
		inp_ptr++;

		/* just in case */
		comma_expected = 0;


		while ( 1 )
		{

			if ( *inp_ptr == '/' )
			{
				LLIST_REQ_NEWL += 1;

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
				*txtbuf_ptr = EXP0SP->expr_value;




				txtbuf_ptr++;

				while ( 1 )
				{



					inp_ptr++;


					if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
					{
						bad_token(inp_ptr, "Missing closing apostrophie");
						f1_eatit();
						return (0);
					}

					if ( *inp_ptr == '\'' )   /* is it end of text */
					{
						if ( *(inp_ptr + 1) == '\'' )   /* is it 2 in a row */
						{
							inp_ptr++;


							*txtbuf_ptr = *inp_ptr;
							txtbuf_ptr++;

							if ( txtbuf_ptr >= txtbuf_end )
							{

								/* Reached end of text data buffer (out of storage space) */
								bad_token(inp_ptr, "Line List text buffer overflow, Max of 16 Characters");
								f1_eatit();

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


						*txtbuf_ptr = *inp_ptr;
						txtbuf_ptr++;

						if ( txtbuf_ptr >= txtbuf_end )
						{

							/* Reached end of text data buffer (out of storage space) */
							bad_token(inp_ptr + 1, "Line List text buffer overflow, Max of 16 Characters");
							f1_eatit();

							return (0);

						}



					}


				}
			}
			else
			{
				arg[cnt] = EXP0SP->expr_value;
				if ( arg[cnt] > tst[cnt] )
				{
					sprintf(emsg, "%s set to max value of %d  , requested %d", err_arg[cnt], tst[cnt], arg[cnt]);
					show_bad_token((inp_ptr - 1), emsg, MSG_ERROR);
					arg[cnt] = tst[cnt];
/*                        sprintf(emsg,"Set to max value of %d", arg[cnt]);
						show_bad_token((inp_ptr),emsg,MSG_ERROR);
						f1_eatit();

						return (0);
*/

				}

				cnt++;
				if ( cnt == 6 )
				{
					sprintf(emsg, "Too many directive arguments, Max of %d allowed", err_cnt);
					show_bad_token((inp_ptr), emsg, MSG_ERROR);
					f1_eatit();
					return (0);
				}
			}



			if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			{
				bad_token(inp_ptr, "Missing closing parenthesis");
				f1_eatit();
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

	return (cnt);

}


/******************************************************************
 * 01/19/2022   added for HLLxxF support by TG
 *
 * list_do_args - sets the list args from the input file.
 */
void list_do_args(char *list_opt)
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


	int arg[7] = { 0, 0, 0, 0, 0, 0, 0 };
/*
arg[0] = Format ID #
arg[1] = Target of data field - line location
arg[2] = Lenght of field - lenght of data in bytes or Flag - if .NE. just queue it 
arg[3] = Field Radix
arg[4] = Flag - if .NE. suppress leading zeros 
arg[5] = Flag - if .NE. prefix a sign
arg[6] = Flag - if .NE.
*/

	int arg_cnt;
	int start;

	/* Do for directive SRC */
	if ( strcmp(list_opt, "SRC") == 0 )
	{

/*  SRC only uses the follow args
arg[1] = Target of data field - line location
arg[2] = Flag - if .NE. just queue it 
arg[6] = Flag - if .NE. request a new list line
*/
		if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
		{
/*
							f1_eatit();
*/
			return;
		}




/* in SRC - so data starts with arg[1] */
		start = 1;

		/* is there a key word */
		arg_cnt = list_args(arg, start);


		if ( arg_cnt == 0 )       /* Error */
		{
			/* printf( "\n No SRC data so no change  \n" ); */
			return;
		}

		if ( arg[2] == 0 )        /* Do now or que it */
		{
			LLIST_SRC = arg[1] - 1;       /* because HLLxxF uses 41  for line start */
			LLIST_SRC_QUED = arg[1] - 1;  /* because HLLxxF uses 41  for line start */
		}
		else
		{
			LLIST_SRC_QUED = arg[1] - 1;  /* because HLLxxF uses 41 ??? for line start */
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


