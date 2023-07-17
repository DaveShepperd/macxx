/*
	psuedo_ops.c - Part of macxx, a cross assembler family for various micro-processors
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

	12/29/2022	- Added error checking to .COPY command		- Tim Giddens
			  Check to see if there is a List file and if .LIST is on.

	12/29/2022	- Fix to .COPY command to handle zeros at the end	- David Shepperd
			  of file being listed.

	12/28/2022	- Added support for the .COPY command.   - Tim Giddens

	12/14/2022	- Fixes to accomodate .PRINT and .LIST SRC options. - David Shepperd

	12/10/2022	- Added change log info.			- David Shepperd

	04/06/2022	- Fixed .PRINT command - suppress leading zeros not working correctly
			  Added supprot for RADIX to .PRINT command by Tim Giddens.

	01/26/2022	- Added support for the .PRINT command by Tim Giddens.
 
	01-05-2022	- Added support for the .RAD50 command by Tim Giddens.

******************************************************************************/
#include "add_defs.h"
#include "token.h"
#include "pst_tokens.h"
#include "exproper.h"
#include "listctrl.h"
#include "strsub.h"
#include "memmgt.h"
#include "utils.h"
#include <errno.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#if defined(VMS)
	#include <types.h>
	#include <stat.h>
#else
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

#if !defined(MAC_PP)
static char asc[128];       /* place to stuff text */
#endif

extern int no_white_space_allowed;

#if !defined(fileno)
extern int fileno(FILE *);
#endif
extern void move_pc(void);

int op_title(void)
{
	int c, len = 0;
	char *s = inp_ptr;
	if ( lis_fp != 0 )
	{
		while ( len < LLIST_SIZE - 1 )
		{
			c = *s;
			if ( (cttbl[c] & CT_EOL) == 1 )
				break;
			++s;
			++len;
		}
		if ( !(c=lis_title[0]) )
			c = '\r';
		snprintf(lis_title, sizeof(lis_title), "%c%-40s %s %s   %s\n",
				c, " ", macxx_name, macxx_version, ascii_date);
		if ( len > (int)sizeof(lis_title)-3 )
			len = sizeof(lis_title)-3;
		memcpy(lis_title + 1, inp_ptr, len);
	}
	f1_eatit();
	return 0;
}

int op_sbttl(void)
{
	int c, len = 0;
	char *s = inp_ptr;
	if ( lis_fp != 0 )
	{
		while ( len < 130 )
		{
			c = *s;
			if ( (cttbl[c] & CT_EOL) == 1 )
				break;
			++s;
			++len;
		}
		if ( len > (int)sizeof(lis_subtitle)-3 )
			len = sizeof(lis_subtitle)-3;
		memcpy(lis_subtitle, inp_ptr, len);
		s = lis_subtitle + len;
		*s++ = '\n';
		*s++ = '\n';
		*s = 0;
	}
	f1_eatit();
	return 0;
}

int op_radix(void)
{
	int i, cr = current_radix;
	long nrad;
	current_radix = 10;      /* set radix to 10 */
	get_token();         /* pickup the next token */
	i = exprs(1, &EXP0);      /* evaluate the expression */
	nrad = EXP0SP->expr_value;
	if ( i == 1 && EXP0SP->expr_code == EXPR_VALUE &&
		 (nrad >= 2 && nrad <= 16) )
	{
		current_radix = nrad;
	}
	else
	{
		bad_token(tkn_ptr, "Radix expression must resolve to a value between 2 and 16");
		current_radix = cr;
	}
	return 2;
}

#ifndef MAC_PP

void op_chkalgn(int size, int sayerr)
{
	int algn;
	if ( (algn = current_section->seg_salign) == 0 )
		return;
	if ( sayerr == 0 )
	{
		if ( algn < size )
		{
			show_bad_token((char *)0, "Alignment broader than specified for this PSECT; not guaranteed to hold during LLF", MSG_WARN);
		}
		algn = size;      /* minimize to size */
	}
	else
	{
		int i;
		i = macxx_min_dalign;
		if ( i < current_section->seg_dalign )
			i = current_section->seg_dalign;
		if ( algn > size )
			algn = size; /* minimize to size */
		if ( algn > i )
			algn = i;       /* minimize to max of dalign/min */
	}
	algn = 1 << algn;    /* get bit mask */
	if ( algn > 1 )
	{
		if ( (current_offset & (algn - 1)) != 0 )
		{
			if ( sayerr )
			{
				sprintf(emsg, "Current location is not a multiple of %d mau's", algn);
				bad_token((char *)0, emsg);
			}
			current_offset = (current_offset + (algn - 1)) & -algn;
		}
	}
	return;
}

static void op_blkx(int size)
{
	int i, bm;
	bm = 1 << size;      /* get size of element in bytes */
	if ( get_token() == EOL )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		i = EXP0SP->expr_value = EXP0.psuedo_value = 1;
	}
	else
	{
		i = exprs(1, &EXP0);
	}
	if ( i == 1 && EXP0SP->expr_code == EXPR_VALUE )
	{
		op_chkalgn(size, 0);
		current_offset += EXP0SP->expr_value * bm;
	}
	else
	{
		bad_token(tkn_ptr, "Expression must resolve to an absolute value");
	}
	return;
}

int op_blkb(void)
{
	op_blkx(0);
	return 3;
}

int op_blkw(void)
{
	op_blkx(1);
	return 3;
}

int op_blkl(void)
{
	op_blkx(2);
	return 3;
}

int op_blkq(void)
{
	op_blkx(3);
	return 3;
}

	#define ASC_COMMON_NONE  (0)
	#define ASC_COMMON_NULL  (1)
	#define ASC_COMMON_MINUS (2)
	#define ASC_COMMON_COMMA (3)

static void ascii_common(int arg)
{
	int term_c, c = 0;
	int len = 0, fake;
	LIST_stat_t *lstat;
	if ( meb_stats.getting_stuff )
	{
		lstat = &meb_stats;
		if ( (meb_stats.pc_flag != 0) &&
			 (meb_stats.expected_pc != current_offset ||
			  (meb_stats.expected_seg != current_section)) )
		{
			fixup_overflow(lstat);
		}
	}
	else
	{
		lstat = &list_stats;
		list_stats.pc_flag = 0;
	}
	if ( lstat->pc_flag == 0 )
	{
		lstat->pc = current_offset;
		lstat->pc_flag = 1;
	}
	term_c = *inp_ptr;
	if ( (cttbl[term_c] & (CT_EOL | CT_SMC)) != 0 )
	{
		bad_token(inp_ptr, "No arguments on line");
		return;
	}
	move_pc();           /* always set the PC */
	fake = 0;            /* assume we're not faking it */
	while ( 1 )
	{           /* for all blocks of text */
		term_c = *inp_ptr;    /* get term char */
		++inp_ptr;        /* eat the delimiter */
		while ( 1 )
		{       /* for all that will fit in asc */
			char *asc_ptr, *asc_end;
			asc_ptr = asc;
			asc_end = asc + sizeof(asc);
		doit_again:
			while ( 1 )
			{        /* for all delimited chars */
				if ( asc_ptr >= asc_end )
				{
					fake = 1;    /* we're gonna fake a split in sections */
					break;
				}
				c = *inp_ptr;   /* pickup user data */
				if ( (cttbl[c] & CT_EOL) != 0 || c == term_c )
				{
					fake = 0;    /* we're not faking it anymore */
					break;
				}
				if ( c == '\\' &&
					 (inp_ptr[1] >= '0' && inp_ptr[1] <= '3') &&
					 (inp_ptr[2] >= '0' && inp_ptr[2] <= '7') &&
					 (inp_ptr[3] >= '0' && inp_ptr[3] <= '7')
				   )
				{
					*asc_ptr++ = ((inp_ptr[1] - '0') << 6) | ((inp_ptr[2] - '0') << 3) | (inp_ptr[3] - '0');
					inp_ptr += 4;
				}
				else
				{
					*asc_ptr++ = c;
					++inp_ptr;
				}
			}          /* -- out of chars */
			len = asc_ptr - asc;   /* how much is in there */
			if ( !fake )
			{
				if ( c == term_c )
				{
					char *tp = inp_ptr;  /* remember this spot */
					++inp_ptr;   /* eat the terminator */
					if ( isspace(*inp_ptr) && !no_white_space_allowed )
					{
						while ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) == 0 && isspace(*inp_ptr) )
						{
							++inp_ptr;        /* eat ws between blocks */
						}
					}
					if ( c == '"' && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) == 0 )
					{
						inp_ptr = tp;     /* put inp_pointer back */
						*asc_ptr++ = c;   /* just move the char */
						++inp_ptr;        /* eat it */
						goto doit_again;  /* pretend this never happened */
					}
				}
				if ( (arg == ASC_COMMON_NULL || arg == ASC_COMMON_MINUS)
					 && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
				{
					if ( arg == ASC_COMMON_NULL )
					{      /* .ASCIZ */
						*asc_ptr++ = 0;   /* null terminate the string */
						++len;        /* add 1 char */
					}
					if ( arg == ASC_COMMON_MINUS )
					{      /* .ASCIN */
						*(asc_ptr - 1) |= 0x80; /* make last byte minus */
					}
				}
			}
			if ( len > 0 )
			{
				write_to_tmp(TMP_ASTNG, len, asc, sizeof(char));
				asc_ptr = asc;
				if ( show_line && (list_bin || meb_stats.getting_stuff) )
				{
					int tlen;
					char *dst;
					tlen = len;
					while ( 1 )
					{
						int z;
						if ( tlen <= 0 )
							break;
						z = (LLIST_SIZE - lstat->list_ptr) / 3;
						if ( z <= 0 )
						{
							if ( !list_bex )
								break;
							fixup_overflow(lstat);
							z = (LLIST_SIZE - LLIST_OPC) / 3;
							lstat->pc = current_offset + (asc_ptr - asc);
							lstat->pc_flag = 1;
						}
						dst = lstat->listBuffer + lstat->list_ptr;
						if ( tlen < z )
							z = tlen;
						lstat->list_ptr += z * 3;
						tlen -= z;
						do
						{
							unsigned char c1;
							c1 = *asc_ptr++;
							*dst++ = hexdig[c1 >> 4];
							*dst++ = hexdig[c1 & 0x0F];
							++dst;
						} while ( --z > 0 );
					}            /* -- for each item in asc [while (1)]*/
				}               /* -- list_bin != 0 */
				current_offset += len;
			}              /* -- something to write (len > 0) */
			if ( !fake )
			{
				if ( arg != ASC_COMMON_COMMA && *inp_ptr == expr_open )
				{    /* have an expression? */
					char *ip, s1, s2, *strt;
					int nst, val, abs;
					EXPR_struct *exp_ptr;
					nst = 0;         /* assume top level */
					strt = ip = inp_ptr + 1;   /* point to place after expr_open */
					while ( 1 )
					{          /* find matching end */
						int chr;
						chr = *ip++;      /* find end pointer */
						if ( (cttbl[chr] & CT_EOL) != 0 )
						{
							--ip;          /* too far, backup 1 */
							break;
						}
						if ( chr == expr_close )
						{
							--nst;
							if ( nst < 0 )
								break;
						}
						else if ( chr == expr_open )
						{
							++nst;
						}
					}
					s1 = *ip;        /* save the last two chars */
					*ip++ = '\n';        /* and replace with a \n\0 */
					s2 = *ip;
					*ip = 0;
					get_token();     /* setup the variables */
					exprs(1, &EXP0);      /* evaluate the exprssion */
					*ip = s2;        /* restore the source record */
					*--ip = s1;
					if ( !no_white_space_allowed )
					{
						while ( isspace(*inp_ptr) )
							++inp_ptr; /* eat ws */
					}
					++current_offset;    /* move pc */
					exp_ptr = EXP0.stack;
					val = EXP0.psuedo_value & 255;
					if ( arg == 2 && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
					{
						if ( EXP0.ptr == 1 &&
							 exp_ptr->expr_code == EXPR_VALUE )
						{
							exp_ptr->expr_value |= 0x80;
						}
						else
						{
							exp_ptr += EXP0.ptr;
							exp_ptr->expr_code = EXPR_VALUE;
							(exp_ptr++)->expr_value = 0x80;
							exp_ptr->expr_code = EXPR_OPER;
							exp_ptr->expr_value = EXPROPER_OR;
							EXP0.ptr += 2;
						}
						val |= 0x80;
					}
					abs = EXP0.ptr == 1 && exp_ptr->expr_code == EXPR_VALUE;
					if ( show_line && list_bin )
					{
						int fs;
						fs = abs ? 3 : 4;
						if ( list_bex || lstat->list_ptr <= LLIST_SIZE - fs )
						{
							char *s;
							if ( lstat->list_ptr > LLIST_SIZE - fs )
								fixup_overflow(lstat);
							s = lstat->listBuffer + lstat->list_ptr;
							*s++ = hexdig[((unsigned char)val) >> 4];
							*s++ = hexdig[val & 15];
							if ( !abs )
								*s = 'x';
							lstat->list_ptr += fs;
						}
					}
					if ( abs )
					{
						int epv;
						epv = EXP0SP->expr_value;
						if ( epv > 255 || epv < -256 )
						{
							snprintf(emsg, ERRMSG_SIZE, "Byte truncation error. Desired: %08X, stored: %02X",
									epv, epv & 255);
							show_bad_token(strt, emsg, MSG_WARN);
							EXP0SP->expr_value = epv & 0xFF;
						}
						write_to_tmp(TMP_BSTNG, 1, (char *)&EXP0SP->expr_value, sizeof(char));
					}
					else
					{
						EXP0.tag = 'b';
						EXP0.tag_len = 1;
						write_to_tmp(TMP_EXPR, 0, &EXP0, 0);
					}
					if ( arg == 1 && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
					{
						static char zero = 0;
						if ( show_line && list_bin )
						{
							if ( list_bex || lstat->list_ptr <= LLIST_SIZE - 3 )
							{
								char *s;
								if ( lstat->list_ptr > LLIST_SIZE - 3 )
									fixup_overflow(lstat);
								s = lstat->listBuffer + lstat->list_ptr;
								*s++ = '0';
								*s = '0';
								lstat->list_ptr += 3;
							}
						}
						write_to_tmp(TMP_BSTNG, 1, &zero, sizeof(char));
					}
				}               /* -- have an expression */
				break;          /* do the next group */
			}
			else
			{
				continue;           /* do the next fake group */
			}
		}                 /* -- while each group */
		if ( c != term_c )
		{
			bad_token(inp_ptr, "No matching delimiter");
			break;
		}
		if ( arg == ASC_COMMON_COMMA || (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			break;
	}                    /* -- for all items in inp_str */
	out_pc = current_offset;
	meb_stats.expected_seg = current_section;
	meb_stats.expected_pc = current_offset;
	return;
}

int op_ascii(void)
{
	ascii_common(ASC_COMMON_NONE);
	return 0;
}

int op_asciz(void)
{
	ascii_common(ASC_COMMON_NULL);
	return 0;
}

int op_ascin(void)
{
	ascii_common(ASC_COMMON_MINUS);
	return 0;
}

int op_dcbx(int siz, int tc)
{
	char c;
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	EXP0.tag = tc;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		EXP0SP->expr_value = EXP0.psuedo_value = 0;
		EXP0.ptr = 1;
		EXP0.tag_len = 1;
		p1o_any(&EXP0);       /* null arg means insert a 0 */
		return 1;
	}
	get_token();
	exprs(0, &EXP1);      /* get the first expression */
	if ( *inp_ptr == ',' )
	{   /* if the next item is a comma */
		if ( !no_white_space_allowed )
		{
			while ( c = *++inp_ptr,isspace(c) ); /* skip over white space */
		}
	}
	if ( get_token() != EOL )
	{
		exprs(1, &EXP0);
		EXP0.tag_len = EXP1SP->expr_value;
	}
	else
	{
		EXP0.tag_len = 1;
	}
	EXP1.ptr = 0;
	p1o_any(&EXP0);      /* output element 0 */
	return 1;
}

static int op_dcb_with_mask(int inpMask)
{
	long epv;
	char *otp = 0;
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	while ( 1 )
	{           /* for all items on line */
		int cc;
		cc = *inp_ptr;
		if ( no_white_space_allowed && isspace(cc) )
			break;
		while ( isspace(cc) )
			cc = *++inp_ptr;
		if ( (cttbl[cc] & (CT_EOL | CT_SMC)) != 0 )
			break;
		if ( cc == ',' )     /* if the next item is a comma */
		{
			++inp_ptr;          /* eat comma */
			EXP0SP->expr_code = EXPR_VALUE;
			epv = EXP0SP->expr_value = 0;
			EXP0.ptr = 1;
		}
		else
		{
			if ( cc == '\'' || cc == '"' )
			{
				ascii_common(ASC_COMMON_COMMA);
				cc = *inp_ptr;
				if ( no_white_space_allowed && isspace(cc) )
					break;
				while ( isspace(cc) )
				{
					cc = *++inp_ptr;
				}
				if ( cc == ',' )  /* if the next item is a comma */
				{
					++inp_ptr;      /* eat comma */
				}
				continue;
			}
			if ( get_token() == EOL )
				break;
/*            printf("dcb: token: %s, type = %d, no_white_space=%d\n",
				token_pool, token_type, no_white_space_allowed );
*/
			otp = tkn_ptr;     /* remember beginning of expression */
			exprs(1, &EXP0);    /* pickup the expression */
			epv = EXP0SP->expr_value;
			if ( *inp_ptr == ',' )  /* if the next item is a comma */
			{
				++inp_ptr;      /* eat comma */
			}
		}
		if ( EXP0.ptr <= 1 && EXP0SP->expr_code == EXPR_VALUE )
		{
			if ( !inpMask )
			{
				if ( epv > 255 || epv < -256 )
				{
					snprintf(emsg, ERRMSG_SIZE, "Byte truncation error. Desired: %08lX, stored: %02lX",
							epv, epv & 255);
					show_bad_token(otp, emsg, MSG_WARN);
				}
			}
			EXP0SP->expr_value = epv & 0xFF;
		}
		else if ( !inpMask )
		{
			EXP_stk *eps = &EXP0;
			EXPR_struct *expr_ptr;

			if ( eps->ptr >= EXPR_MAXDEPTH )
			{
				bad_token(NULL, "Too many terms in expression");
				return (eps->ptr = -1);
			}
			expr_ptr = eps->stack + eps->ptr; /* compute start place for eval */
			expr_ptr->expr_code = EXPR_VALUE;
			expr_ptr->expr_value  = 0xFF;
			++expr_ptr;
			expr_ptr->expr_code = EXPR_OPER;
			expr_ptr->expr_value = EXPROPER_AND;
			eps->ptr += 2;
			compress_expr(&EXP0);
		}
		EXP0.tag = 'b';
		EXP0.tag_len = 1;
		p1o_byte(&EXP0);      /* output element 0 */
	}
	return 1;
}

int op_dcb_b(void)
{
	return op_dcb_with_mask(0);
}

int op_dcb_bm(void)
{
	return op_dcb_with_mask(1);
}

int op_dcbb(void)
{
	return op_dcbx(0, 'b');
}

int op_dcbw(void)
{
	return op_dcbx(1, (edmask & ED_M68) ? 'W' : 'w');
}

char default_long_tag[2] = { 'L', 'l' };

int op_dcbl(void)
{
	return op_dcbx(2, (edmask & ED_M68) ? default_long_tag[0] : default_long_tag[1]);
}

static int op_byte_with_mask(int inpMask)
{
	char *otp = 0;
	long epv;
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	EXP0.tag = 'b';
	EXP0.tag_len = 1;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		EXP0SP->expr_value = EXP0.psuedo_value = 0;
		EXP0.ptr = 1;
		p1o_byte(&EXP0);      /* null arg means insert a 0 */
		return 1;
	}
	while ( 1 )
	{           /* for all items on line */
		char cc;
		cc = *inp_ptr;
		if ( no_white_space_allowed && isspace(cc) )
			break;
		while ( isspace(cc) )
			cc = *++inp_ptr;
		if ( (cttbl[(int)cc] & (CT_EOL | CT_SMC)) != 0 )
			break;
		if ( cc == ',' )
		{    /* if the next item is a comma */
			cc = *++inp_ptr;
			EXP0SP->expr_code = EXPR_VALUE;
			epv = EXP0SP->expr_value = 0;
			EXP0.ptr = 1;
		}
		else
		{
			if ( get_token() == EOL )
				break;
			otp = tkn_ptr;     /* remember beginning of expression */
			exprs(1, &EXP0);    /* pickup the expression */
			epv = EXP0SP->expr_value;
			if ( *inp_ptr == ',' )
			{ /* if the next item is a comma */
				cc = *++inp_ptr;
			}
		}
		if ( EXP0.ptr <= 1 && EXP0SP->expr_code == EXPR_VALUE )
		{
			if ( !inpMask )
			{
				if ( epv > 255 || epv < -256 )
				{
					snprintf(emsg, ERRMSG_SIZE, "Byte truncation error. Desired: %08lX, stored: %02lX",
							epv, epv & 255);
					show_bad_token(otp, emsg, MSG_WARN);
				}
			}
			EXP0SP->expr_value = epv & 0xFF;
		}
		else if ( !inpMask )
		{
			EXP_stk *eps = &EXP0;
			EXPR_struct *expr_ptr;

			if ( eps->ptr >= EXPR_MAXDEPTH )
			{
				bad_token(NULL, "Too many terms in expression");
				return (eps->ptr = -1);
			}
			expr_ptr = eps->stack + eps->ptr; /* compute start place for eval */
			expr_ptr->expr_code = EXPR_VALUE;
			expr_ptr->expr_value  = 0xFF;
			++expr_ptr;
			expr_ptr->expr_code = EXPR_OPER;
			expr_ptr->expr_value = EXPROPER_AND;
			eps->ptr += 2;
			compress_expr(&EXP0);
		}
		p1o_byte(&EXP0);      /* output element 0 */
	}
	return 1;
}

int op_byte(void)
{
	return op_byte_with_mask((edmask & ED_TRUNC) ? 0 : 1);
}

static int op_word_with_mask(int inpMask)
{
	char *otp;
	long epv;
	op_chkalgn(1, 1);     /* check pc for alignment */
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	EXP0.tag = (edmask & ED_M68) ? 'W' : 'w';
	EXP0.tag_len = 1;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		EXP0SP->expr_value = EXP0.psuedo_value = 0;
		EXP0.ptr = 1;
		p1o_word(&EXP0);      /* null arg means insert a 0 */
		return 1;
	}
	while ( 1 )
	{           /* for all items on line */
		char cc;
		cc = *inp_ptr;
		if ( no_white_space_allowed && isspace(cc) )
			break;
		while ( isspace(cc) )
			cc = *++inp_ptr;
		if ( (cttbl[(int)cc] & (CT_EOL | CT_SMC)) != 0 )
			break;
		if ( cc == ',' )
		{    /* if the next item is a comma */
			cc = *++inp_ptr;
			EXP0SP->expr_code = EXPR_VALUE;
			epv = EXP0SP->expr_value = EXP0.psuedo_value = 0;
			EXP0.ptr = 1;
		}
		else
		{
			if ( get_token() == EOL )
				break;
			otp = tkn_ptr;     /* remember beginning of expression */
			exprs(1, &EXP0);    /* pickup the expression */
			epv = EXP0SP->expr_value;
			if ( *inp_ptr == ',' )
			{ /* if the next item is a comma */
				cc = *++inp_ptr;
			}
			if ( EXP0.ptr <= 1 && EXP0SP->expr_code == EXPR_VALUE )
			{
				if ( !inpMask )
				{
					if ( epv > 65535l || epv < -65536l )
					{
						snprintf(emsg, ERRMSG_SIZE, "Word truncation error. Desired: %08lX, stored: %04lX",
								epv, epv & 65535);
						show_bad_token(otp, emsg, MSG_WARN);
						EXP0SP->expr_value &= 0xFFFF;
					}
				}
				EXP0SP->expr_value = epv & 0xFFFF;
			}
			else if ( !inpMask )
			{
				EXP_stk *eps = &EXP0;
				EXPR_struct *expr_ptr;

				if ( eps->ptr >= EXPR_MAXDEPTH )
				{
					bad_token(NULL, "Too many terms in expression");
					return (eps->ptr = -1);
				}
				expr_ptr = eps->stack + eps->ptr; /* compute start place for eval */
				expr_ptr->expr_code = EXPR_VALUE;
				expr_ptr->expr_value  = 0xFFFF;
				++expr_ptr;
				expr_ptr->expr_code = EXPR_OPER;
				expr_ptr->expr_value = EXPROPER_AND;
				eps->ptr += 2;
				compress_expr(&EXP0);
			}
		}
		p1o_word(&EXP0);      /* dump element 0 */
	}
	return 1;
}

int op_word(void)
{
	return op_word_with_mask((edmask & ED_TRUNC) ? 0 : 1);
}

int op_long(void)
{
	op_chkalgn(2, 1);     /* check pc for alignment */
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	EXP0.tag = (edmask & ED_M68) ? default_long_tag[0] : default_long_tag[1];
	EXP0.tag_len = 1;
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		EXP0SP->expr_code = EXPR_VALUE;
		EXP0SP->expr_value = EXP0.psuedo_value = 0;
		EXP0.ptr = 1;
		p1o_long(&EXP0);      /* null arg means insert a 0 */
		return 1;
	}
	while ( 1 )
	{           /* for all items on line */
		char cc;
		cc = *inp_ptr;
		if ( no_white_space_allowed && isspace(cc) )
			break;
		while ( isspace(cc) )
			cc = *++inp_ptr;
		if ( (cttbl[(int)cc] & (CT_EOL | CT_SMC)) != 0 )
			break;
		if ( cc == ',' )
		{    /* if the next item is a comma */
			cc = *++inp_ptr;
			EXP0SP->expr_code = EXPR_VALUE;
			EXP0SP->expr_value = 0;
			EXP0.ptr = 1;
		}
		else
		{
			if ( get_token() == EOL )
				break;
			exprs(1, &EXP0);    /* pickup the expression */
			if ( *inp_ptr == ',' )
			{ /* if the next item is a comma */
				cc = *++inp_ptr;
				if ( !no_white_space_allowed )
				{
					while ( isspace(cc) )
						cc = *++inp_ptr;
				}
			}
		}
		p1o_long(&EXP0);      /* dump element 0 */
	}
	return 1;
}

#endif			/* #ifndef MAC_PP */

int condit_level;   /* current condition level */
int condit_nest;    /* current condition nest level */
long condit_polarity;   /* 32 conditional polarities */
long condit_word;   /* 32 condition flags */

#include "opcnds.h"

typedef enum
{
	DFNDF_FALSE = 0,
	DFNDF_TRUE,
	DFNDF_NDF,
	DFNDF_DF,
	DFNDF_NEXIST,
	DFNDF_EXIST
} DFNDF_sense;

static int if_dfndf_exprs(DFNDF_sense sense, int flag)
{
	int sfd, sfnd, retval, toktyp, termcnt;

	sfd = sense & DFNDF_TRUE;  /* 0 = testing for ndf, 1 = testing for df */
	sfnd = sfd ^ DFNDF_TRUE; /* opposite sense */
	retval = 0;          /* assume false */
	termcnt = 0;         /* no terms yet */

	while ( (toktyp = get_token()) != EOL )
	{
		if ( toktyp == TOKEN_strng )
		{
			SS_struct *sPtr;
			*(token_pool + max_symbol_length) = 0;
			sPtr = sym_lookup(token_pool, SYM_DO_NOTHING);
			if ( sense == DFNDF_NDF || sense == DFNDF_DF )
			{
				if ( sPtr && sPtr->flg_defined )
					retval = sfd;
				else
					retval = sfnd;
			}
			else
			{
				if ( sPtr )
					retval = sfd;
				else
					retval = sfnd;
			}
			++termcnt;     /* got a term */
		}
		else if ( toktyp == TOKEN_oper )
		{
			int tkn, imv;
			tkn = token_value;
			if ( tkn == expr_open )
			{
				retval = if_dfndf_exprs(sense, 1);
				if ( *inp_ptr != expr_close )
				{
					sprintf(emsg, "Unbalanced %c%c's in expression",
							expr_open, expr_close);
					bad_token(inp_ptr, emsg);
					f1_eatit();
					return 0;
				}
				++inp_ptr;      /* eat the expr_close */
				++termcnt;      /* got at least 1 term */
			}
			else if ( tkn == '&' || tkn == '!' || tkn == '|' )
			{
				imv = if_dfndf_exprs(sense, 0);  /* get 1 term */
				if ( tkn == '&' )
				{
					retval &= imv;
				}
				else
				{
					retval |= imv;
				}
			}
			else if ( tkn == expr_close )
			{
				inp_ptr = tkn_ptr;      /* backup to expr_close */
				return retval;      /* done */
			}
			else
			{
				bad_token(tkn_ptr, "IF DF and NDF allow for only '&' and '!'");
				f1_eatit();
				return 0;
			}
		}
		else
		{
			bad_token(tkn_ptr, "Unvalid expression syntax");
			f1_eatit();
			return 0;
		}
		if ( flag == 0 )
			break; /* quit if to get only 1 term */
		if ( (cttbl[(int)*inp_ptr] & (CT_COM | CT_SMC | CT_EOL)) != 0 )
			break;
	}
	if ( termcnt != 1 )
	{
		if ( termcnt < 1 )
		{
			bad_token((char *)0, "Too few terms in expression");
		}
		else
		{
			bad_token((char *)0, "Too many terms in expression");
		}
		f1_eatit();
		return 0;
	}
	return retval;
}

static unsigned long if_dfndf(DFNDF_sense sense)
{
	int val;
	val = if_dfndf_exprs(sense, 1);
	return (val ? 0l : 0x80000000L);
}

static unsigned long op_ifcondit(char *condition)
{
	int i, l;
	CCN_nums tcon;
	unsigned long j;
	long ans = 0;
	DFNDF_sense dfcond;
	struct ccn_struct *ccs;

	j = 0x80000000L;     /* assume false */
	l = strlen(condition);
	tcon = CCN_MAX;      /* assume nfg */
	for ( ccs = ccns; ccs->name != 0; ++ccs )
	{
		if ( ccs->length < l )
			continue;
		if ( ccs->length > l )
			break;
		i = strcmp(condition, ccs->name);
		if ( i > 0 )
			continue;
		if ( i == 0 )
			tcon = ccs->value;
		break;
	}
	if ( tcon == CCN_MAX )
	{
		char tBuf[132];
		snprintf(tBuf, sizeof(tBuf), "Unknown conditional argument: '%s'", condition);
		bad_token((char *)0, tBuf);
		return j;
	}
	dfcond = DFNDF_FALSE;
	if ( tcon == CCN_DF )
		dfcond = DFNDF_DF;
	if ( tcon == CCN_NDF )
		dfcond = DFNDF_NDF;
	if ( tcon == CCN_EXIST )
		dfcond = DFNDF_EXIST;
	if ( tcon == CCN_NEXIST )
		dfcond = DFNDF_NEXIST;
	if ( dfcond != DFNDF_FALSE )
	{
		j = if_dfndf(dfcond);
		return j;
	}
	if ( tcon < CCN_DIF )
	{
		get_token();      /* pickup a token */
		if ( tcon == CCN_ABS || tcon == CCN_REL )
		{
			exprs(1, &EXP0);
			j = (EXP0.ptr == 1 && EXP0SP->expr_code == EXPR_VALUE);
			if ( tcon == CCN_REL )
				j = !j;
			return j ? 0 : 0x80000000L;
		}
		exprs(0, &EXP0);       /* get an ABS expression */
		ans = EXP0SP->expr_value;
	}
	switch (tcon)
	{
	case CCN_IDN:
	case CCN_DIF:
		{
			char *arg1, *arg2;
			int l1, l2;
			/* eat leading whitespace on arg1 */
			while ( (cttbl[(int)*inp_ptr] & CT_WS) )
				++inp_ptr;
			arg1 = inp_ptr;
			/* skip to end of non-whitespace string */
			while ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC | CT_COM | CT_WS)) == 0 )
				++inp_ptr;
			if ( *inp_ptr == ',' )
			{
				l1 = inp_ptr - arg1;
				/* eat comma */
				++inp_ptr;
				/* eat leading whitespace on arg2 */
				while ( (cttbl[(int)*inp_ptr] & CT_WS) )
					++inp_ptr;
				arg2 = inp_ptr;
				/* skip to end of non-whitespace string */
				while ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC | CT_COM | CT_WS)) == 0 )
					++inp_ptr;
				l2 = inp_ptr - arg2;
				if ( l1 == l2 )
				{
					if ( strncmp(arg1, arg2, l1) == 0 )
					{
						j = 0;
					}
				}
			}
			if ( tcon == CCN_DIF )
				j ^= 0x80000000L;
			break;
		}
	case CCN_B:
	case CCN_NB:
		{
			char *arg1;
			int l1;
			if ( macro_level == 0 )
			{
				bad_token(tkn_ptr, "Conditional only available in a macro");
				f1_eatit();
				break;
			}
			arg1 = inp_ptr;
			if ( *inp_ptr == macro_arg_open )
			{
				l1 = 0;
				++inp_ptr;
				arg1 = inp_ptr;
				while ( 1 )
				{
					int c;
					c = *inp_ptr;
					if ( c == macro_arg_close )
					{
						--l1;
						if ( l1 < 0 )
							break;
					}
					else if ( c == macro_arg_open )
					{
						++l1;
					}
					else if ( (cttbl[c] & CT_EOL) != 0 )
					{
						break;
					}
					++inp_ptr;
				}
			}
			else
			{
				while ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC | CT_COM)) == 0 )
					++inp_ptr;
			}
			l1 = inp_ptr - arg1;
			if ( l1 == 0 )
				j = 0;
			if ( tcon == CCN_NB )
				j ^= 0x80000000L;
			if ( *inp_ptr == macro_arg_close )
				++inp_ptr;    /* eat the character */
			break;
		}
	case CCN_EQ:
		if ( ans == 0 )
			j = 0;
		break;
	case CCN_NE:
		if ( ans != 0 )
			j = 0;
		break;
	case CCN_GT:
		if ( ans > 0 )
			j = 0;
		break;
	case CCN_GE:
		if ( ans >= 0 )
			j = 0;
		break;
	case CCN_LT:
		if ( ans < 0 )
			j = 0;
		break;
	case CCN_LE:
		if ( ans <= 0 )
			j = 0;
		break;
	default:
		{
			char tBuf[132];
			snprintf(tBuf, sizeof(tBuf), "Unknown conditional argument: '%s'", condition);
			bad_token((char *)0, tBuf);
		}
		break;
	}
	return j;
}

int op_if(void)
{
	unsigned long retv = 0;
	int tt;

	if ( condit_word < 0 )
	{   /* if currently an unsatisfied conditional */
		++condit_level;       /* just bump the conditional level */
		f1_eatit();       /* eat the rest of the line */
		return 0;         /* and exit */
	}
	else
	{
		unsigned long old_edmask;
		old_edmask = edmask;
		edmask &= ~(ED_LC | ED_DOL);
		tt = get_token();
		edmask = old_edmask;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Invalid conditional argument");
			retv = 0x80000000L;
		}
		else if ( condit_nest > 31 )
		{  /* room for another condition? */
			bad_token(tkn_ptr, "Cannot nest conditionals more than 31 levels");
		}
	}
	if ( *inp_ptr == ',' )
		++inp_ptr;
	if ( retv == 0 )
	{
		retv = op_ifcondit(token_pool);
	}
	else
	{
		f1_eatit();       /* else eat the line */
	}
	++condit_nest;       /* bump the nest level */
	++condit_level;      /* and the level */
	condit_word = ((unsigned long)condit_word >> 1) | retv;
	condit_polarity = ((unsigned long)condit_polarity >> 1) | retv;
	return 0;
}

int op_ifall(Opcode *opc)
{
	unsigned long retv = 0;

	if ( condit_word < 0 )
	{   /* if currently an unsatisfied conditional */
		++condit_level;       /* just bump the conditional level */
		f1_eatit();       /* eat the rest of the line */
		return 0;         /* and exit */
	}
	else if ( condit_nest > 31 )
	{  /* room for another condition? */
		bad_token(tkn_ptr, "Cannot nest conditionals more than 32 levels");
	}
	if ( retv == 0 )
	{
		retv = op_ifcondit(opc->op_name + 3);
	}
	else
	{
		f1_eatit();       /* eat the line */
	}
	++condit_nest;       /* bump the nest level */
	++condit_level;      /* and the level */
	condit_word = ((unsigned long)condit_word >> 1) | retv;
	condit_polarity = ((unsigned long)condit_polarity >> 1) | retv;
	return 0;
}

int op_endc(void)
{
	if ( condit_nest == 0 )
	{
		bad_token(tkn_ptr, "Not inside a conditional block");
		return 0;
	}
	if ( condit_nest != condit_level )
	{
		--condit_level;
		return 0;
	}
	if ( --condit_nest == 0 )
	{
		condit_word = 0;
		condit_polarity = 0;
		condit_level = 0;
	}
	else
	{
		condit_polarity <<= 1;
		condit_word <<= 1;
		--condit_level;
	}
	return 0;
}

int op_iif(void)
{
	unsigned long retv = 0;
	char *tp;

	tp = tkn_ptr;        /* remember where the .IIF starts */
	if ( condit_word < 0 )
	{   /* if currently an unsatisfied conditional */
		retv = 0x80000000L;
	}
	else
	{
		int tt;
		unsigned long old_edmask;
		old_edmask = edmask;
		edmask &= ~(ED_LC | ED_DOL);
		tt = get_token();
		edmask = old_edmask;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Invalid conditional argument");
			retv = 0x80000000L;
		}
	}
	if ( *inp_ptr == ',' )
		++inp_ptr;
	if ( retv == 0 )
	{
		retv = op_ifcondit(token_pool);
		if ( *inp_ptr == ',' )
			++inp_ptr;
		if ( !list_cnd && line_errors_index == 0 )
		{
			strcpy(tp, inp_ptr);    /* zap the conditional from the input */
			inp_ptr = tp;      /* move the pointer */
		}
#ifdef MAC_PP
		inp_str_partial = inp_ptr;    /* don't output the .iif */
#endif
	}
	else
	{
		f1_eatit();       /* else eat the line */
	}
	if ( retv == 0 )
		return 1;
	return 0;
}

int op_iff(void)
{
	unsigned long t;
	if ( condit_nest == 0 )
	{
		bad_token(tkn_ptr, "Not inside a conditional block");
		return 0;
	}
	if ( condit_nest != condit_level )
	{
		return 0;
	}
	t = (condit_polarity & 0x80000000) ^ 0x80000000L;
	condit_word &= 0x7FFFFFFFL;
	condit_word |= t;
	return 0;
}

int op_ift(void)
{
	unsigned long t;
	if ( condit_nest == 0 )
	{
		bad_token(tkn_ptr, "Not inside a conditional block");
		return 0;
	}
	if ( condit_nest != condit_level )
	{
		return 0;
	}
	t = condit_polarity & 0x80000000L;
	condit_word &= 0x7FFFFFFFL;
	condit_word |= t;
	return 0;
}

int op_iftf(void)
{
	if ( condit_nest == 0 )
	{
		bad_token(tkn_ptr, "Not inside a conditional block");
		return 0;
	}
	if ( condit_nest != condit_level )
	{
		return 0;
	}
	condit_word &= 0x7FFFFFFFL;
	return 0;
}

#ifndef MAC_PP
static struct
{
	char *string;
	unsigned short flag;
} edstuff[] = {
	{ "MOS", ED_MOS },
	{ "AMA", ED_AMA },
	{ "ABS", ED_ABS },
	{ "LSB", ED_LSB },
	{ "USD", ED_USD },
	{ "LC", 0 },
	{ "LOWER_CASE", ED_LC },
	{ "M68", ED_M68 },
	{ "GBL", ED_GBL },
	{ "GLOBAL", ED_GBL },
	{ "WORD", ED_WRD },
	{ ".WORD", ED_WRD },
	{ "BYTE", ED_BYT },
	{ ".BYTE", ED_BYT },
	{ "DOLLAR_HEX", ED_DOL },
	{ "PC_RELATIVE", ED_PCREL },
	{ "DOT_LOCAL", ED_DOTLCL },
	{ "CR", ED_CR },
	{ "TRUNC_CHECK", ED_TRUNC },
	{ 0, 0 }
};

static int op_edcommon(int onoff)
{
	int i;
	while ( 1 )
	{
		int tt, flg;
		unsigned long old_edmask;
		old_edmask = edmask;
		edmask &= ~(ED_LC | ED_DOL);
		tt = get_token();
		edmask = old_edmask;
		if ( tt == EOL )
			break;
		comma_expected = 1;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a symbolic string here");
		}
		else
		{
			for ( i = 0; edstuff[i].string != 0; ++i )
			{
				tt = strncmp(token_pool, edstuff[i].string, 5);
				if ( tt == 0 )
					break;
			}
			flg = edstuff[i].flag;
			if ( tt != 0 )
			{
				flg = (token_value < 4) ? 4 : token_value;
				if ( strncmp(token_pool, "BIG_ENDIAN", flg) == 0 )
				{
					flg = ED_M68;
				}
				else if ( strncmp(token_pool, "LITTLE_ENDIAN", flg) == 0 )
				{
					if ( onoff < 0 )
					{
						edmask |= ED_M68;
					}
					else
					{
						edmask &= ~ED_M68;
					}
					continue;
				}
				else
				{
					bad_token(tkn_ptr, "Unknown enabl/dsabl argument");
					continue;
				}
			}
			if ( flg == 0 )
				continue;
			if ( onoff < 0 )
			{
				edmask &= ~flg;
			}
			else
			{
				edmask |= flg;
			}
			if ( edstuff[i].flag & ED_LSB )
			{  /* if messing with an LSB, */
				current_lsb = next_lsb++;   /* set it up */
				autogen_lsb = 65000;  /* autolabel for macro processing */
			}
		}
	}
	return 0;
}

int op_enabl(void)
{
	op_edcommon(1);
	return 0;
}

int op_dsabl(void)
{
	op_edcommon(-1);
	return 0;
}

static int op_glbcomm(int code)
{
	int tt;
	SS_struct *sym_ptr;

	while ( 1 )
	{
		tt = get_token();
		comma_expected = 1;       /* expect a comma */
		if ( tt == EOL )
			break;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a symbol name");
			continue;
		}
		*(token_pool + max_symbol_length) = 0;
		switch (code)
		{
		case 0:
			{
				sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);
				sym_ptr->flg_global = 1;    /* signal it's global */
				break;
			}
		case 1:
			{
				sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);
				sym_ptr->flg_global = 1;    /* signal it's global */
				sym_ptr->flg_base = 1;  /* and whether it's base page */
				break;
			}
		case 2:
			{
				sym_ptr = do_symbol(SYM_INSERT_IF_NOT_FOUND);
				sym_ptr->flg_global = 1;    /* signal it's global */
				sym_ptr->flg_register = 1;  /* signal it's a register */
				break;
			}
		case 3:
			{
				sym_ptr = do_symbol(SYM_INSERT_HIGHER_SCOPE); /* declare it local */
				break;
			}
		case 4:
			{
				sym_ptr = do_symbol(SYM_INSERT_LOCAL); /* declare it static */
				break;
			}
		}
	}
	return 0;            /* done */
}

int op_globl(void)
{
	op_glbcomm(0);
	return 0;
}

int op_globb(void)
{
	op_glbcomm(1);
	return 0;
}

int op_globr(void)
{
	op_glbcomm(2);
	return 0;
}

int op_local(void)
{
	op_glbcomm(3);
	return 0;
}

int op_static(void)
{
	op_glbcomm(4);
	return 0;
}

int op_page(void)
{
	show_line = 0;       /* don't display .page */
	lis_line = 0;
	return 0;
}

	#ifndef MAC_PP

static int bad_mau(void)
{
	bad_token((char *)0, "Sorry, elements greater than 32 bits are not supported at this time");
	return 0;
}

int op_blkm(void)
{
	op_blkx(0);
	return 3;
}

int op_blk2m(void)
{
	op_blkx(1);
	return 3;
}

int op_blk4m(void)
{
	op_blkx(2);
	return 3;
}

int op_blk8m(void)
{
	op_blkx(3);
	return 3;
}

int op_mau(void)
{
	if ( macxx_mau == 8 )
		return op_byte_with_mask((edmask & ED_TRUNC) ? 0 : 1);
	if ( macxx_mau == 16 )
		return op_word_with_mask((edmask & ED_TRUNC) ? 0 : 1);
	return op_long();
}

int op_maum(void)
{
	if ( macxx_mau == 8 )
		return op_byte_with_mask(1);
	if ( macxx_mau == 16 )
		return op_word_with_mask(1);
	return op_long();
}

int op_2mau(void)
{
	if ( macxx_mau == 8 )
		return op_word_with_mask((edmask & ED_TRUNC) ? 0 : 1);
	if ( macxx_mau == 16 )
		return op_long();
	return bad_mau();
}

int op_2maum(void)
{
	if ( macxx_mau == 8 )
		return op_word_with_mask(1);
	if ( macxx_mau == 16 )
		return op_long();
	return bad_mau();
}

int op_4mau(void)
{
	if ( macxx_mau == 8 )
		return op_long();
	return bad_mau();
}

	#endif

static struct
{
	char *name;
	char min;
	char max;
} length_names[] = {
	{ "SYMBOL", 6, 32 },
	{ "OPCODE", 6, 32 },
	{ "MAU", 8, 32 },
	{ 0, 0, 0 }
};

int op_length(void)
{
	int tt;
	while ( 1 )
	{
		int slen;
		unsigned long old_edmask;
		old_edmask = edmask;
		edmask &= ~(ED_LC | ED_DOL);
		tt = get_token();
		edmask = old_edmask;
		comma_expected = 1;
		if ( tt == EOL )
			break;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a symbol name here");
			f1_eatit();
			return 0;
		}
		for ( tt = 0; length_names[tt].name != 0; ++tt )
		{
			if ( strncmp(token_pool, length_names[tt].name, (int)token_value) == 0 )
			{
				break;
			}
		}
		slen = length_names[tt].min;
		if ( *inp_ptr == '=' )
		{
			char *tp = 0;
			++inp_ptr;
			comma_expected = 0;
			if ( get_token() != EOL )
			{
				tp = tkn_ptr;
				exprs(0, &EXP0);
			}
			slen = EXP0SP->expr_value;
			if ( length_names[tt].name != 0 )
			{
				if ( slen < length_names[tt].min || slen > length_names[tt].max )
				{
					sprintf(emsg, "Length parameter must be between %d and %d",
							length_names[tt].min, length_names[tt].max);
					bad_token(tp, emsg);
					if ( slen < length_names[tt].min )
						slen = length_names[tt].min;
					if ( slen > length_names[tt].max )
						slen = length_names[tt].max;
				}
			}
			comma_expected = 1;
		}
		if ( length_names[tt].name != 0 )
		{
			switch (tt)
			{
			case 0:
				max_opcode_length = slen;
				opcinit();       /* re-seed the opcode table */
				break;
			case 1:
				max_symbol_length = slen;
				break;
			case 2:
				{
					if ( slen != 8 && slen != 16 && slen != 32 )
					{
						bad_token((char *)0, "MAU's other than 8, 16 or 32 are not supported at this time");
					}
					else
					{
						macxx_mau = slen;
						macxx_mau_byte = 1;
						macxx_mau_word = 2;
						macxx_mau_long = 4;
						if ( slen == 16 )
						{
							macxx_mau_word = 1;
							macxx_mau_long = 2;
						}
						else if ( slen == 32 )
						{
							macxx_mau_word = 1;
							macxx_mau_long = 1;
						}
						macxx_bytes_mau = macxx_mau >> 3;
						macxx_nibbles_byte = (macxx_mau_byte * macxx_mau) / 4;
						macxx_nibbles_word = (macxx_mau_word * macxx_mau) / 4;
						macxx_nibbles_long = (macxx_mau_long * macxx_mau) / 4;
					}
				}
			}
		}
	}
	return 0;
}
#endif

int op_include(void)
{
	int rms_err;
	FN_struct *tfnd;

	if ( include_level >= 8 )
	{
		tkn_ptr = inp_ptr;
		bad_token((char *)0, "Include nest level too deep");
		f1_eatit();
		return 0;
	}
	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		bad_token((char *)0, "No filename argument present");
		return 0;
	}
	tfnd = get_fn_struct();
	rms_err = 0;
	if ( *inp_ptr == '"' )
	{
		++inp_ptr;
		++rms_err;
	}
	tfnd->fn_buff = inp_ptr++;
	while ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC | CT_WS)) == 0 )
		++inp_ptr;
	tfnd->r_length = inp_ptr - tfnd->fn_buff;
	if ( rms_err )
	{
		if ( *(inp_ptr - 1) == '"' )
		{
			--tfnd->r_length;
		}
		else
		{
			tfnd->fn_buff -= 1;
		}
	}
	if ( tfnd->r_length > fn_pool_size )
		get_fn_pool();
	strncpy(fn_pool, tfnd->fn_buff, (int)tfnd->r_length);
	*(fn_pool + tfnd->r_length) = 0;
	tfnd->fn_buff = fn_pool;
	fn_pool += tfnd->r_length;
	tfnd->r_length += 1;
	fn_pool_size -= tfnd->r_length;
	rms_err = add_defs(tfnd->fn_buff, def_inp_ptr, cmd_includes,
					   ADD_DEFS_INPUT, &tfnd->fn_nam);
	if ( tfnd->fn_nam != 0 )
	{
		tfnd->fn_buff = tfnd->fn_nam->full_name;
		tfnd->fn_name_only = tfnd->fn_nam->name_type;
	}
	if ( rms_err || (tfnd->fn_file = fopen(tfnd->fn_buff, "r")) == 0 )
	{
		sprintf(emsg, "Unable to open \"%s\" for input: %s",
				tfnd->fn_buff, error2str(errno));
		show_bad_token((char *)0, emsg, MSG_ERROR);
		if ( tfnd->fn_file != 0 )
			fclose(tfnd->fn_file);
	}
	else
	{
		tfnd->fn_next = current_fnd;
		current_fnd->macro_level = macro_level;
		macro_level = 0;
		current_fnd = tfnd;
		if ( squeak )
			printf("Processing file %s\n", current_fnd->fn_buff);
#if !defined(MAC_PP)
		if ( obj_fp != 0 )
			write_to_tmp(TMP_FILE, 1, &current_fnd, sizeof(FN_struct *));
		tfnd->fn_buff = tfnd->fn_nam->full_name;
		if ( options[QUAL_DEBUG] )
		{
			struct stat file_stat;
			if ( fn_pool_size < 26 )
				(void)get_fn_pool();
			fstat(fileno(current_fnd->fn_file), &file_stat);
			strcpy((char *)fn_pool, ctime(&file_stat.st_mtime));
			current_fnd->fn_version = fn_pool;
			*(fn_pool + 24) = 0;
			fn_pool_size -= 25;
			fn_pool += 25;
			dbg_flush(1);
			add_dbg_file(current_fnd->fn_buff, current_fnd->fn_version, (int *)0);
		}
#endif
		++include_level;
	}
	return 0;
}

#ifndef MAC_PP
	#define PS_ABS		0x0001
	#define PS_REL		0x0002
	#define PS_CON		0x0004
	#define PS_OVR		0x0008
	#define PS_BASE		0x0010
	#define PS_OUT  	0x0020
	#define PS_NOOUT	0x0040
	#define PS_RO   	0x0080
	#define PS_RW   	0x0100
	#define PS_SALGN	0x0200
	#define PS_DALGN	0x0400
	#define PS_MAXLN	0x0800
	#define PS_DATA		0x1000
	#define PS_CODE		0x2000

static struct
{
	char *name;
	unsigned short flags;
} psect_table[] = {
	{ "ABS", PS_ABS },
	{ "REL", PS_REL },
	{ "CON", PS_CON },
	{ "OVR", PS_OVR },
	{ "BSE", PS_BASE },
	{ "BASE", PS_BASE },
	{ "OUT", PS_OUT },
	{ "NOOUT", PS_NOOUT },
	{ "RO", PS_RO },
	{ "RW", PS_RW },
	{ "TEXT", PS_DATA },
	{ "CODE", PS_CODE },
	{ 0, 0 }
};

static void change_section(SEG_struct *new_seg)
{
	if ( current_section->seg_pc > current_section->seg_len )
	{
		current_section->seg_len = current_section->seg_pc;
	}
	if ( (edmask & ED_LSB) == 0 )
	{
		current_lsb = next_lsb++;     /* set it up */
		autogen_lsb = 65000;        /* autolabel for macro processing */
	}
	current_section = new_seg;
	return;
}

static int op_segcomm(int type, int new_one, SEG_struct *new_seg,
					  int flags, int salign, int dalign, unsigned long maxlen)
{
#if 0
	if ( squeak )
	printf("op_segcomm(): pass=%d, type=%d, new_one=%d, flags=%04X, maxLen=%ld, Segment '%s': len=%ld, base=%ld, rel=%ld\n",
		   pass, type, new_one, flags, maxlen,
		   new_seg->seg_string, new_seg->seg_len, new_seg->seg_base, new_seg->rel_offset);
#endif
	if ( flags != 0 )
	{
		if ( new_seg->seg_len == 0 )
		{
			new_one = 1;   /* pretend it's new if len = 0 */
		}
		if ( (flags & (PS_ABS | PS_REL)) == (PS_ABS | PS_REL) )
		{
			bad_token((char *)0, "Can't be both ABS and REL");
			flags &= ~PS_ABS;
		}
		if ( (flags & (PS_ABS | PS_BASE)) == (PS_ABS | PS_BASE) )
		{
			bad_token((char *)0, "Can't be both ABS and BASE");
			flags &= ~PS_ABS;
		}
		if ( (flags & (PS_CON | PS_OVR)) == (PS_CON | PS_OVR) )
		{
			bad_token((char *)0, "Can't be both CON and OVR");
			flags &= ~PS_OVR;
		}
		if ( (flags & (PS_RW | PS_RO)) == (PS_RW | PS_RO) )
		{
			bad_token((char *)0, "Can't be both RO and RW");
			flags &= ~PS_RO;
		}
		if ( (flags & (PS_CODE | PS_DATA)) == (PS_CODE | PS_DATA) )
		{
			bad_token((char *)0, "Can't be both CODE and DATA");
			flags &= ~PS_CODE;
		}
		if ( (flags & PS_ABS) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( !new_seg->flg_abs )
				{
					bad_token((char *)0, "Segment previously declared REL");
				}
			}
			else
			{
				new_seg->flg_abs = 1;
				if ( (flags & (PS_CON | PS_OVR)) == 0 )
					flags |= PS_OVR;
			}
		}
		if ( (flags & PS_REL) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( new_seg->flg_abs )
				{
					bad_token((char *)0, "Segment previously declared ABS");
				}
			}
		}
		if ( (flags & PS_OVR) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( !new_seg->flg_ovr )
				{
					bad_token((char *)0, "Segment previously declared CON");
				}
			}
			else
			{
				new_seg->flg_ovr = 1;
			}
		}
		if ( (flags & PS_CON) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( new_seg->flg_ovr )
				{
					bad_token((char *)0, "Segment previously declared OVR");
				}
			}
		}
		if ( (flags & PS_DATA) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( !new_seg->flg_data )
				{
					bad_token((char *)0, "Segment previously declared CODE");
				}
			}
			else
			{
				new_seg->flg_data = 1;
			}
		}
		if ( (flags & PS_CODE) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( new_seg->flg_data )
				{
					bad_token((char *)0, "Segment previously declared DATA");
				}
			}
		}
		if ( (flags & PS_RO) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( !new_seg->flg_ro )
				{
					bad_token((char *)0, "Segment previously declared RW");
				}
			}
			else
			{
				new_seg->flg_ro = 1;
			}
		}
		if ( (flags & PS_RW) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( new_seg->flg_ro )
				{
					bad_token((char *)0, "Segment previously declared RO");
				}
			}
		}
		if ( (flags & PS_BASE) != 0 )
		{
			if ( new_one == 0 )
			{
				if ( !new_seg->flg_zero )
				{
					bad_token((char *)0, "Segment not previously declared BASE");
				}
			}
			else
			{
				new_seg->flg_zero = 1;
			}
		}
		if ( (flags & (PS_OUT | PS_NOOUT)) != 0 )
		{
			new_seg->flg_noout = ((flags & PS_NOOUT) != 0);
		}
	}
	if ( new_one == 0 )
	{
		if ( (flags & PS_SALGN) != 0 && new_seg->seg_salign != salign )
		{
			bad_token((char *)0, "Conflicting segment alignment factors");
		}
		if ( (flags & PS_DALGN) != 0 && new_seg->seg_dalign != dalign )
		{
			bad_token((char *)0, "Conflicting data alignment factors");
		}
		if ( (flags & PS_MAXLN) != 0 && new_seg->seg_maxlen != maxlen )
		{
			bad_token((char *)0, "Conflicting segment max lengths");
		}
	}
	else
	{
		if ( salign > 31 )
		{
			show_bad_token((char *)0, "Segment alignment > 31 is meaningless",
						   MSG_WARN);
			salign = 31;
		}
		if ( dalign > 31 )
		{
			show_bad_token((char *)0, "Data alignment > 31 is meaningless",
						   MSG_WARN);
			salign = 31;
		}
		if ( dalign > salign )
		{
			show_bad_token((char *)0, "DALIGN > SALIGN cannot be enforced",
						   MSG_WARN);
			dalign = salign;
		}
		new_seg->seg_salign = salign;
		new_seg->seg_dalign = dalign;
		new_seg->seg_maxlen = maxlen;
	}
	change_section(new_seg);
	list_stats.pc_flag = 1;
	list_stats.pc = current_offset & 0xFFFF;
	return 0;
}

SEG_struct* find_segment(char *name, SEG_struct **segl, int segi)
{
	int i;
	SEG_struct *new_seg = 0;
	for ( i = 0; i < segi; ++i )
	{
		new_seg = *(segl + i);
		if ( strcmp(name, new_seg->seg_string) == 0 )
			break;
		new_seg = 0;
	}
	return new_seg;
}

static int get_userssegname(char *alt_name)
{
	if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC | CT_COM)) != 0 )
	{
		strcpy(token_pool, alt_name);
	}
	else
	{
		char *np;
		unsigned long old_edmask;
		old_edmask = edmask;
		edmask &= ~ED_DOL;
		get_token();          /* pickup the next token */
		edmask = old_edmask;
		if ( token_type != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a section name here");
			f1_eatit();
			return 0;
		}
		np = token_pool + ((token_value > max_symbol_length) ? max_symbol_length : token_value);
		if ( (edmask & ED_LC) == 0 )
			*np++ = '_';
		*np++ = 0;
		token_value = np - token_pool;
	}
	return 1;
}

static SEG_struct* get_segname(char *alt_name, int *new)
{
	SEG_struct *new_seg = 0;
	SS_struct *sym_ptr;
	*new = 0;
	if ( get_userssegname(alt_name) == 0 )
		return 0;
	new_seg = find_segment(token_pool, seg_list, seg_list_index);
	if ( new_seg == 0 )
	{
		sym_ptr = sym_lookup(token_pool, SYM_DO_NOTHING);
		if ( sym_ptr != 0 && sym_ptr->flg_defined )
		{
			show_bad_token(tkn_ptr, "Name already in use as a symbol/label name",
						   MSG_WARN);
		}
		new_seg = get_seg_mem(&sym_ptr, token_pool);
		*new = 1;
	}
	return new_seg;
}

int op_psect(void)
{
	int new_one, flags = 0, salign = 0, dalign = 0;
	unsigned long maxlen = 0;
	SEG_struct *new_seg;

	if ( (new_seg = get_segname(".REL.", &new_one)) == 0 )
	{
		return 1;
	}
#if 0
	if ( squeak )
	printf("op_psect(): Found section .REL.. new_one=%d\n", new_one);
#endif
	if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) == 0 )
	{
		while ( 1 )
		{
			int tt;
			unsigned long old_edmask;
			old_edmask = edmask;
			edmask &= ~(ED_LC | ED_DOL);
			comma_expected = 1;
			tt = get_token();
			edmask = old_edmask;
			if ( tt == EOL )
				break;
			if ( tt != TOKEN_strng )
			{
				bad_token(tkn_ptr, "Expected a string here");
				continue;
			}
			for ( tt = 0;; ++tt )
			{
				if ( psect_table[tt].name == 0 )
				{
					int segdat = 0;
					if ( strncmp(token_pool, "DATA", (int)token_value) == 0 )
					{
						segdat = PS_DALGN;
					}
					else if ( strncmp(token_pool, "SEGMENT", (int)token_value) == 0 )
					{
						segdat = PS_SALGN;
					}
					else if ( strncmp(token_pool, "MAX_LENGTH", (int)token_value) == 0 )
					{
						segdat = PS_MAXLN;
					}
					if ( segdat == 0 )
					{
						bad_token(tkn_ptr, "Unknown PSECT keyword");
						break;
					}
					else
					{
						if ( *inp_ptr == '=' )
						{
							++inp_ptr;
							if ( get_token() != EOL )
							{
								exprs(0, &EXP0);
								if ( (segdat & PS_DALGN) != 0 )
									dalign = EXP0SP->expr_value;
								if ( (segdat & PS_SALGN) != 0 )
									salign = EXP0SP->expr_value;
								if ( (segdat & PS_MAXLN) != 0 )
									maxlen = EXP0SP->expr_value;
							}
						}
						flags |= segdat;
						break;
					}
				}
				else
				{
					if ( strcmp(token_pool, psect_table[tt].name) == 0 )
					{
						flags |= psect_table[tt].flags;
						break;
					}
				}
			}
		}
	}
	op_segcomm(0, new_one, new_seg, flags, salign, dalign, maxlen);
	return 1;
}

static int make_absseg(int new_one, SEG_struct *new_seg)
{
	int flags = 0;
	if ( new_one != 0 )
		flags = PS_OVR | PS_RW | PS_ABS;
	op_segcomm(0, new_one, new_seg, flags, 0, 0, 0l);
	return 1;
}

SEG_struct* get_absseg(void)
{
	SEG_struct *segp;
	SS_struct *sym;
	segp = find_segment(".ABS.", seg_list, seg_list_index);
	if ( segp == 0 )
	{
		segp = get_seg_mem(&sym, ".ABS.");
		make_absseg(1, segp);
	}
	return segp;
}

int op_asect(void)
{
	int new_one;
	SEG_struct *new_seg;
	new_seg = get_segname(".ABS.", &new_one);
	if ( new_seg == 0 )
		return 1;
	return make_absseg(new_one, new_seg);
}

int op_csect(void)
{
	int new_one, flags = 0;
	SEG_struct *new_seg;
	new_seg = get_segname(".REL.", &new_one);
	if ( new_seg == 0 )
		return 1;
	if ( new_one != 0 )
		flags = PS_OVR | PS_RW;
	op_segcomm(0, new_one, new_seg, flags, 0, 0, 0l);
	return 1;
}

int op_bsect(void)
{
	int new_one, flags = 0;
	SEG_struct *new_seg;
	new_seg = get_segname(".BASE.", &new_one);
	if ( new_seg == 0 )
		return 1;
	if ( new_one != 0 )
	{
		if ( strcmp(new_seg->seg_string, ".BASE.") != 0 )
		{
			flags = PS_OVR | PS_BASE | PS_RW;
		}
		else
		{
			flags = PS_CON | PS_BASE | PS_RW;
		}
	}
	op_segcomm(0, new_one, new_seg, flags, 0, 0, 256l);
	return 1;
}

int op_subsect(void)
{
	SEG_struct *new_seg;

	if ( get_userssegname("") == 0 )
		return 0;
	new_seg = find_segment(token_pool, subseg_list, subseg_list_index);
	if ( new_seg == 0 )
	{
		new_seg = get_subseg_mem();
		if ( new_seg == 0 )
			return 0;
		memcpy(new_seg, current_section, sizeof(SEG_struct));
		new_seg->seg_string = token_pool;
		token_pool += token_value;
		token_pool_size -= token_value;
		new_seg->rel_offset = 0;
		new_seg->flg_subsect = 1;
		new_seg->flg_abs = 0;     /* subsection can't be absolute */
		new_seg->seg_pc = 0;
		new_seg->seg_base = 0;
		new_seg->seg_len = 0;
		new_seg->seg_maxlen = 0;
	}
	change_section(new_seg);
	return 1;
}

int op_place(void)
{
	SEG_struct *new_seg;

	if ( get_userssegname("") == 0 )
		return 0;
	new_seg = find_segment(token_pool, subseg_list, subseg_list_index);
	if ( new_seg == 0 )
	{
		bad_token(tkn_ptr, "Unknown subsection name");
		f1_eatit();
		return 0;
	}
	change_section(*(seg_list + new_seg->seg_index)); /* switch to proper section */
	new_seg->seg_string = " ";       /* make it unreachable */
	new_seg->rel_offset = current_section->seg_pc;
	current_section->seg_pc += new_seg->seg_len;
	return 1;
}

int dump_subsects(void)
{
	SEG_struct *new_seg;
	int i;

	if ( current_section->seg_len < current_section->seg_pc )
	{
		current_section->seg_len = current_section->seg_pc;
	}
	for ( i = 0; i < subseg_list_index; ++i )
	{
		new_seg = *(subseg_list + i);
		if ( new_seg->rel_offset != 0 )
			continue;
		new_seg->seg_string = " ";        /* make it unreachable */
		new_seg->rel_offset = current_section->seg_len;
		current_section->seg_len += new_seg->seg_len;
	}
	current_section->seg_pc = current_section->seg_len;
	return 1;
}

int op_vctrs(void)
{
	SEG_struct *save_seg;
	unsigned long save_pc, save_aspc;
	int tt;
	save_pc = current_offset;
	save_seg = current_section;
	current_section = get_absseg();  /* point to an abs section */
	save_aspc = current_offset;      /* but save the location counter */
	tt = get_token();
	if ( tt != EOL )
	{
		exprs(0, &EXP0);           /* pickup the value */
		current_offset = EXP0SP->expr_value;
		list_stats.pc = current_offset & 0xffff;
		list_stats.pc_flag = 1;
		if ( *inp_ptr == ',' )
			++inp_ptr;         /* if stopped on a comma, eat it */
		op_word();         /* pretend the rest of the line is a .word */
	}
	current_offset = save_aspc;      /* restore the ASECT pc */
	current_section = save_seg;      /* restore the sectoin ptr */
	current_offset = save_pc;        /* and the pc in case we're in ASECT */
	return 1;                /* done */
}

int op_cksum(void)
{
	SEG_struct *save_seg;
	unsigned long save_pc, save_aspc;
	int tt;
	save_pc = current_offset;
	save_seg = current_section;
	current_section = get_absseg();  /* point to an abs section */
	save_aspc = current_offset;      /* but save the location counter */
	tt = get_token();
	if ( tt != EOL )
	{
		exprs(0, &EXP0);           /* pickup the address */
		current_offset = EXP0SP->expr_value;
		list_stats.pc = current_offset & 0xffff;
		list_stats.pc_flag = 1;
		if ( *inp_ptr == ',' )
			++inp_ptr;         /* if stopped on a comma, eat it */
		op_byte();         /* pretend the rest of the line is a .byte */
	}
	current_offset = save_aspc;      /* restore the ASECT pc */
	current_section = save_seg;      /* restore the sectoin ptr */
	current_offset = save_pc;        /* and the pc in case we're in ASECT */
	return 1;                /* done */
}

int op_align(void)
{
	int t;
	t = (1 << current_section->seg_dalign); /* assume default */
	if ( get_token() != EOL )
	{        /* anything on line? */
		if ( exprs(0, &EXP0) < 1 || EXP0.ptr != 1 || EXP0SP->expr_code != EXPR_VALUE )
		{
			bad_token((char *)0, "Alignment expression must be absolute");
		}
		else
		{
			int tmp;
			tmp = EXP0SP->expr_value;
			if ( tmp > 31 )
			{
				show_bad_token((char *)0, "Alignment value over 31 is meaningless",
							   MSG_WARN);
			}
			else
			{
				if ( tmp > current_section->seg_salign )
				{
					show_bad_token((char *)0, "LLF won't enforce this alignment",
								   MSG_WARN);
				}
				t = 1 << tmp;
			}
		}
	}
	current_offset = (current_offset + (t - 1)) & -t;
	return 1;
}

int op_even(void)
{
	if ( current_section->seg_salign < 1 )
	{
		show_bad_token((char *)0, "Segment alignment doesn't enforce this",
					   MSG_WARN);
	}
	current_offset = (current_offset + 1) & -2;
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	return 1;
}

int op_odd(void)
{
	if ( current_section->seg_salign < 1 )
	{
		show_bad_token((char *)0, "Segment alignment doesn't enforce this",
					   MSG_WARN);
	}
	current_offset |= 1;
	list_stats.pc = current_offset;
	list_stats.pc_flag = 1;
	return 1;
}
#endif

typedef struct
{
#ifndef MAC_PP
	SEG_struct *segptr;
	unsigned long pc;
	int lsb;
#endif
	int outfile;
} SEG_save;

static SEG_save *seg_save;
static int seg_save_index;
static int seg_save_size;

int op_save(void)
{
	SEG_save *ssp;
	if ( seg_save_index >= seg_save_size )
	{
		seg_save_size += 16;
		seg_save = (SEG_save *)MEM_realloc((char *)seg_save, seg_save_size * sizeof(SEG_save));
	}
	ssp = seg_save + seg_save_index;
#ifndef MAC_PP
	ssp->segptr = current_section;
	ssp->pc = current_offset;
	ssp->lsb = current_lsb;
#endif
	ssp->outfile = current_outfile;
	++seg_save_index;
	return 1;
}

static void swap_outfile(int new)
{
	cmd_fnds[current_outfile] = output_files[0];
	output_files[0] = cmd_fnds[new];
	current_outfile = new;
}

int op_restore(void)
{
	SEG_save *ssp;
	if ( seg_save_index <= 0 )
	{
		bad_token((char *)0, "No preceeding .SAVE");
		f1_eatit();
		return 0;
	}
	--seg_save_index;
	ssp = seg_save + seg_save_index;
#ifndef MAC_PP
	if ( current_section == ssp->segptr && current_offset != ssp->pc )
	{
		err_msg(MSG_WARN, "Restored to same section. PC may not be pointing where you think it should.");
	}
	change_section(ssp->segptr);
	current_offset = ssp->pc;
	current_lsb = ssp->lsb;
#endif
	if ( current_outfile != ssp->outfile )
	{
#ifndef MAC_PP
		flushobj();
#endif
		swap_outfile(ssp->outfile);
	}
	return 1;
}

#ifdef MAC_PP
int op_outfile(void)
{
	int i, outfile;

	if ( (cttbl[(int)*inp_ptr] & (CT_SMC | CT_EOL)) != 0 )
	{
		i = 1;
		EXP0SP->expr_value = 0;           /* assume 0 */
		EXP0SP->expr_code = EXPR_VALUE;
	}
	else
	{
		get_token();      /* else pickup the next token */
		i = exprs(1, &EXP0);   /* evaluate the expression */
	}
	outfile = EXP0SP->expr_value;
	if ( i == 1 && EXP0SP->expr_code == EXPR_VALUE &&
		 (outfile >= 0 && outfile < cmd_outputs_index) )
	{
		if ( current_outfile != outfile )
		{
			swap_outfile(outfile);
		}
	}
	else
	{
		bad_token(tkn_ptr, "Selected output file not provided on command line");
	}
	return 2;
}
#endif

int op_error(void)
{
	int tt;
	if ( (tt = get_token()) != EOL )
	{
		exprs(0, &EXP0);
		list_stats.pf_value = EXP0SP->expr_value;
		list_stats.f1_flag = 1;
		tt = 2;
	}
	else
	{
		tt = 0;
	}
	show_bad_token((char *)0, "Directive generated error",
				   MSG_ERROR);
	return tt;
}

int op_warn(void)
{
	int tt;
	if ( (tt = get_token()) != EOL )
	{
		exprs(0, &EXP0);
		list_stats.pf_value = EXP0SP->expr_value;
		list_stats.f1_flag = 1;
		tt = 2;
	}
	else
	{
		tt = 0;
	}
	show_bad_token((char *)0, "Directive generated warning",
				   MSG_WARN);
	return tt;
}

static struct escape_chars
{
	char *desc;
	char *arg;
} escape_args[] = {
	{ "MACRO_OPEN", &macro_arg_open },
	{ "MACRO_CLOSE", &macro_arg_close },
	{ "MACRO_ESCAPE", &macro_arg_escape },
	{ "MACRO_GENSYM", &macro_arg_gensym },
	{ "MACRO_GENVAL", &macro_arg_genval }
};

#define ESC_SIZE (sizeof(escape_args)/sizeof(struct escape_chars))

int op_escape(void)
{
	int tt, esc_indx;
	while ( 1 )
	{
		struct escape_chars *esc;
		esc = escape_args;
		esc_indx = 0;
		if ( (tt = get_token()) == EOL )
			break;
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a string argument here");
			f1_eatit();
			return 0;
		}
		for ( esc_indx = 0; esc_indx < ESC_SIZE; ++esc_indx, ++esc )
		{
			if ( strncmp(token_pool, esc->desc, (int)token_value) == 0 )
				break;
		}
		if ( esc_indx >= ESC_SIZE )
		{
			bad_token(tkn_ptr, "Unrecognised argument");
			f1_eatit();
			return 0;
		}
		if ( *inp_ptr == '=' )
		{
			char *ep;
			++inp_ptr;     /* eat the '=' */
			get_token();   /* prime the pump */
			ep = tkn_ptr;
			exprs(0, &EXP0);
			if ( EXP0.ptr == 1 )
			{
				tt = EXP0SP->expr_value;
				if ( !isgraph(tt) )
				{
					bad_token(ep, "Expression must resolve to a value between 0x21 to 0x7E");
				}
				else
				{
					if ( esc_indx < 2 )
					{
						cctbl[(int)*esc->arg] = CC_PCX;
						cttbl[(int)*esc->arg] = CT_PCX;
						cctbl[tt] = CC_OPER;
						cttbl[tt] = CT_OPER;
					}
					*esc->arg = tt;
				}       /* -- isgraph(tt) */
			}      /* -- exprs(0) == 1 */
		}         /* -- *inp_ptr == '=' */
		comma_expected = 1;
	}            /* -- while (1) */
	return 0;
}

#ifndef MAC_PP
int op_test(void)
{
	get_token();         /* prime the pump */
	exprs(1, &EXP0);      /* get an expression */
	if ( EXP0.ptr <= 0 )
	{
		bad_token((char *)0, "Invalid expression syntax for directive");
		f1_eatit();
		return 0;
	}
	if ( EXP0.ptr == 1 && EXP0SP->expr_code == EXPR_VALUE )
	{
		if ( EXP0SP->expr_value != 0 )
		{
			bad_token((char *)0, "Value of expression in .TEST returns true");
		}
		f1_eatit();
		return 0;
	}
	tkn_ptr = inp_ptr;       /* remember where we started */
	if ( output_mode == OUTPUT_OL )
	{
		while ( cttbl[(int)*inp_ptr] != CT_EOL )
		{
			if ( *inp_ptr == '"' )
			{
				bad_token(inp_ptr, "Double quotes not allowed in text");
				f1_eatit();
				return 0;
			}
			++inp_ptr;
		}
	}
	else
	{
		while ( cttbl[(int)*inp_ptr] != CT_EOL )
			++inp_ptr;
	}
	write_to_tmp(TMP_TEST, 0, &EXP0, 0);
	write_to_tmp(TMP_ASTNG, (int)(inp_ptr - tkn_ptr), tkn_ptr, sizeof(char));
	return 0;
}
#endif

void op_purgedefines(struct str_sub *sub)
{
	int siz;
	struct str_sub *sub_save;

	if ( !(sub_save = sub) )
		return;
	for ( siz = 0; siz < 64; ++siz, ++sub )
	{
		if ( sub->name_len == -1 )
		{
			op_purgedefines((struct str_sub *)sub->name);
			break;
		}
		if ( sub->name != 0 )
			MEM_free(sub->name);
		if ( sub->value != 0 )
			MEM_free(sub->value);
	}
	MEM_free((char *)sub_save);
	return;
}

#if SHOW_TEXT
static void showText(const char *title, char *msg)
{
	char tBuf[128];
	int len;

	len = snprintf(tBuf, sizeof(tBuf), "%s", msg);
	if ( len > 1 && tBuf[len - 1] == '\n' )
		tBuf[len - 1] = 0;
	printf("%s'%s'\n", title, tBuf);
}
#else
	#define showText(a,b) do { ; } while (0)
#endif

int op_define(void)
{
	int tt, siz;
	char *s, c;
	struct str_sub *sub;
	if ( string_macros != 0 && presub_str )
	{
		strcpy(inp_str, presub_str);
		inp_ptr = inp_str;
/*		showText("op_define: input string: ", inp_str); */
		while ( 1 )
		{
			tt = get_token();
			if ( tt != TOKEN_strng && tt != EOL )
			{
#if SHOW_TEXT
				char tBuf[64];
				snprintf(tBuf, sizeof(tBuf), "op_define: tt=%d, token_pool=", tt);
				showText(tBuf, token_pool);
#endif
				continue;
			}
			if ( tt == EOL )
			{
				bad_token((char *)0, "Unable to find .DEFINE in line");
				f1_eatit();
				return 0;
			}
			if ( strncasecmp(token_pool, ".define", 7) != 0 )
				continue;
			break;
		}
	}
	if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) == 0 )
	{
		tt = get_token();
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a string here");
			f1_eatit();
			return 0;
		}
		if ( (sub = string_macros) != 0 )
		{
			for ( siz = 0; sub->name != 0;)
			{
				if ( sub->name_len == -1 )
				{
					sub = (struct str_sub *)sub->name;
					siz = 0;
					continue;
				}
				if ( sub->name_len == token_value )
				{
					if ( strcmp(token_pool, sub->name) == 0 )
					{
						if ( sub->value != 0 )
						{
							MEM_free(sub->value);
							sub->value = 0;
							sub->value_len = 0;
						}
						break;
					}
				}
				++siz;
				++sub;
			}
			if ( siz >= 63 )
			{
				sub->name = (char *)MEM_alloc(64 * (int)sizeof(struct str_sub));
				sub->name_len = -1;
				sub = (struct str_sub *)sub->name;
				sub->name = 0;
			}
		}
		else
		{
			sub = (struct str_sub *)MEM_alloc(64 * (int)sizeof(struct str_sub));
			string_macros = sub;
		}
		if ( sub->name == 0 )
		{
			sub->name = (char *)MEM_alloc((int)token_value + 1);
			sub->name_len = token_value;
		}
		strcpy(sub->name, token_pool);
		s = inp_ptr;
		while ( (cttbl[(int)*s] & (CT_SMC | CT_EOL)) == 0 )
			++s;
		--s;
		while ( (cttbl[(int)*s] & CT_WS) != 0 )
			--s;
		++s;
		tt = s - inp_ptr;
		sub->value = (char *)MEM_alloc(tt + 1);
		sub->value_len = tt;
		c = *s;
		*s = 0;
		strcpy(sub->value, inp_ptr);
		*s = c;
		inp_ptr = s;
	}
	return 1;
}

int op_undefine(void)
{
	int tt;
	struct str_sub *sub;
	if ( string_macros != 0 )
	{
		strcpy(inp_str, presub_str);
		inp_ptr = inp_str;
		while ( 1 )
		{
			tt = get_token();
			if ( tt != TOKEN_strng && tt != EOL )
				continue;
			if ( tt == EOL )
			{
				bad_token((char *)0, "Unable to find .UNDEFINE in line");
				f1_eatit();
				return 0;
			}
			if ( strncmp(token_pool, ".UNDEFINE", max_opcode_length) != 0 )
				continue;
			break;
		}
	}
	if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) == 0 )
	{
		tt = get_token();
		if ( tt != TOKEN_strng )
		{
			bad_token(tkn_ptr, "Expected a string here");
			f1_eatit();
			return 0;
		}
		if ( (sub = string_macros) != 0 )
		{
			for (; sub->name != 0;)
			{
				if ( sub->name_len == -1 )
				{
					sub = (struct str_sub *)sub->name;
					continue;
				}
				if ( sub->name_len == token_value )
				{
					if ( strcmp(token_pool, sub->name) == 0 )
					{
						struct str_sub *sub_save, *bott, *top;
						MEM_free(sub->name);
						sub->name = 0;
						sub->name_len = 0;
						if ( sub->value != 0 )
						{
							MEM_free(sub->value);
							sub->value = 0;
							sub->value_len = 0;
						}
						sub_save = sub++;
						bott = top = 0;
						for (; sub->name != 0;)
						{
							if ( sub->name_len == -1 )
							{
								bott = sub;
								sub = (struct str_sub *)sub->name;
								top = sub;
								continue;
							}
							++sub;
						}
						if ( sub == top )
						{
							op_purgedefines(top);
							bott->name_len = 0;
							bott->name = 0;
							sub = bott - 1;
						}
						else
						{
							sub -= 1;
						}
						if ( sub != sub_save )
						{
							sub_save->name = sub->name;
							sub_save->name_len = sub->name_len;
							sub_save->value = sub->value;
							sub_save->value_len = sub->value_len;
							sub->name = 0;
							sub->name_len = 0;
							sub->value = 0;
							sub->value_len = 0;
						}
						return 1;
					}        /* -- if (strcmp() == 0) */
				}           /* -- if (token_value == ) */
				++sub;
			}          /* -- for (sub) */
		}             /* -- if (string_macros) */
	}
	else
	{         /* -- if (argument) */
		if ( (sub = string_macros) != 0 )
		{
			op_purgedefines(sub);
			string_macros = 0;
		}
	}
	return 1;
}

int next_procblk = -SCOPE_PROC; /* next available proc block number */

int op_proc(void)
{
	if ( current_procblk == 0 )
	{  /* currently at the top level */
		current_procblk = next_procblk;
		current_scopblk = 0;
		current_scope = next_procblk;
		next_procblk += -SCOPE_PROC;
	}
	else
	{
		if ( current_scopblk >= ~SCOPE_PROC )
		{
			sprintf(emsg, "PROC nesting restricted to %d levels", -SCOPE_PROC);
			bad_token((char *)0, emsg);
			return 0;
		}
		++current_scopblk;
		++current_scope;
	}
	if ( show_line != 0 )
	{
		sprintf(list_stats.listBuffer + LLIST_RPT - 1, "(%d.%d)", current_procblk / -SCOPE_PROC,
				current_scopblk);
	}
#if !defined(MAC_PP)
	write_to_tmp(TMP_SCOPE, sizeof(current_scope), &current_scope, sizeof(char));
#endif
	return 0;
}

int op_endp(void)
{
	if ( current_procblk == 0 )
	{  /* currently at the top level */
		bad_token((char *)0, "Not in a procedure block");
	}
	else
	{
		if ( show_line != 0 )
		{
			sprintf(list_stats.listBuffer + LLIST_RPT - 1, "(%d.%d)", current_procblk / -SCOPE_PROC,
					current_scopblk);
		}
		if ( current_scopblk > 0 )
		{
			--current_scopblk;
			--current_scope;
		}
		else
		{
			current_scope = current_scopblk = current_procblk = 0;
		}
	}
	return 0;
}

#ifndef MAC_PP
/****************************************************************************/
/* 01-05-2022 added support for the .RAD50 command - David Shepperd code - edited by TG 

Adds support for RAD50
( The packing of three RAD50 characters into a 16 Bit WORD )

DEC loved Octal numbers.
It's called RAD50 because the number Decimal 40 equals the number Octal 50


The packing formula.

WORD=((x1*40)+x2)*40+x3

Where
x1 = the first character
x2 = the second character
x3 = the third character


Example .Rad50 commands.

.RAD50 /string 1/..../string n/
.RAD50 /ABC/

.RAD50 /ABC/<VAL1>/DEF/    For MAC65, MAC68, MAC69
.RAD50 <VAL1><VAL2><VAL3>  For MAC65, MAC68, MAC69

.RAD50 /ABC/(VAL1)/DEF/    For other MACxx's
.RAD50 (VAL1)(VAL2)(VAL3)  For other MACxx's



Note:
Variables must resolve to a valid RAD50 character
As of now, a variable must resolve immediately to an absolute value.
You can not use a variable that gets resolved at link time because
the value is packed into a word.

One line of MACxx assembly code can contain up to 255 characters
but our storage buffer for the packed data is only 128 bytes so we
must split the data into two passes.  On the first pass we fill buffer
then write the packed data to the *.ol file, reset the buffer and process
the remaining. Actually the code as written, could do much more data but
MACxx line size is limited to 255 characters.



								  RAD50 - 40-character repertoire
			****************************************************
			; Translate ASCII character code to RAD50          ;
			;                                                  ;
			; RAD50 for PDP-11, VAX                            ;
			;                                                  ;
			; Note this is Decimal numbers                     ;
			;                                                  ;
			; 0     = space                                    ;
			; 1-26  = A-Z                                      ;
			; 27    = $                                        ;
			; 28    = .                                        ;
			; 29    = %   - unused code for other DEC machines ;
			; 30-39 = 0-9                                      ;
			;                                                  ;
			****************************************************


*/

static void rad50_common(void)
{
	int term_c, c = 0, y = 0, x = 0, term_expr = 0;
	int len = 0, rad50_count = 0;
	LIST_stat_t *lstat;
	if ( meb_stats.getting_stuff )
	{
		lstat = &meb_stats;
		if ( (meb_stats.pc_flag != 0) &&
			 (meb_stats.expected_pc != current_offset ||
			  (meb_stats.expected_seg != current_section)) )
		{
			fixup_overflow(lstat);
		}
	}
	else
	{
		lstat = &list_stats;
		list_stats.pc_flag = 0;
	}
	if ( lstat->pc_flag == 0 )
	{
		lstat->pc = current_offset;
		lstat->pc_flag = 1;
	}
	term_c = *inp_ptr;
	if ( (cttbl[term_c] & (CT_EOL | CT_SMC)) != 0 )
	{
		bad_token(inp_ptr, "No arguments on line");
		return;
	}
	move_pc();           /* always set the PC */
	term_c = *inp_ptr;    /* get termination delimiter */
	if ( term_c == expr_open )
	{
		term_expr = 1;    /* set if expression */
		term_c = expr_close;    /* set expression terminating delimiter */
	}
	++inp_ptr;        /* eat the terminating delimiter */

	while ( 1 )    /* Set up Buffer */
	{       /* for all that will fit in asc
			   asc is a buffer of 128 bytes used for temp storage */
		char *asc_ptr, *asc_end;
		asc_ptr = asc;
		/* save four spaces for packing at the end of buffer if
		   not three bytes yet */
		asc_end = asc + sizeof(asc) - 4;
		while ( 1 )
		{        /* process line of characters */
			if ( asc_ptr >= asc_end )
			{
				break;   /* Reached end of data buffer (out of storage space) */
			}
			if ( term_expr == 1 )    /* have an expression ? */
			{
				char *ip, s1, s2;
				int nst;

				rad50_count = 1;
				nst = 0;         /* assume top level */
				ip = inp_ptr;   /* point to place after expr_open */
				while ( 1 )
				{          /* find matching expr_close*/
					int chr;
					chr = *ip++;      /* find end pointer */
					if ( (cttbl[chr] & CT_EOL) != 0 )
					{
						--ip;          /* too far, backup 1 */
						bad_token(ip, "Missing expression bracket");
						/*Eat rest of line to suppress warning error about
						  end of line not reached*/
						f1_eatit();
						return;
					}
					if ( chr == expr_close )
					{
						--nst;
						if ( nst < 0 )
							break;
					}
					else if ( chr == expr_open )  /* Nested expression */
					{
						++nst;
					}
				}  /* end of while find matching expr_close*/
				s1 = *ip;        /* save the two chars after expr_close */
				*ip++ = 0x0A;        /* and replace with a \linefeed\0 */
				s2 = *ip;
				*ip = 0;
				get_token();     /* setup the variables */
				/*  Call exprs with Force absolute which will cause error if not absolute */
				exprs(0, &EXP0);      /* evaluate the exprssion */
				*ip = s2;        /* restore the source record */
				*--ip = s1;
				if ( !no_white_space_allowed )
				{
					while ( isspace(*inp_ptr) ) /* eat ws */
					{
						++inp_ptr;
					}
				}
				c = EXP0SP->expr_value;
				if ( c >= 40 )
				{
					sprintf(emsg, "Unknown RADIX-50 Character - Hex Value = %X", c);
					show_bad_token((inp_ptr), emsg, MSG_ERROR);
					/*Eat rest of line to suppress warning error about end of line not reached*/
					f1_eatit();
					return;
				}
				/* Each y assignment packs the remaining bytes with spaces */
				if ( x == 0 )
					y = c * 1600;
				if ( x == 1 )
					y = y + (c * 40);
				if ( x == 2 )
					y = y + c;
				x++;
				/* three bytes yet - put in buffer */
				if ( x >= 3 )
				{
					if ( (edmask & ED_M68) == 0 )
					{
						/* For little endian CPU's */
						*asc_ptr++ = y & 0xff;
						*asc_ptr++ = y >> 8 & 0xff;
					}
					else
					{
						/* For big endian CPU's*/
						*asc_ptr++ = y >> 8 & 0xff;
						*asc_ptr++ = y & 0xff;
					}
					x = 0;
					y = 0;
				}
				c = *inp_ptr;   /* pick up ending delimiter */
				term_expr = 0;    /* expression complete */
			}
			else   /* not an expression */
			{
				while ( 1 )   /* convert loop */
				{
					c = *inp_ptr;   /* pickup user data */
					if ( ((cttbl[c] & (CT_EOL | CT_SMC)) != 0) || (c == term_c) )
					{
						break;    /* reached EOL or ending delimiter */
					}
					rad50_count = 1;
					if ( c == ' ' )
					{
						c = 0;
					}
					else if ( c == '$' )
					{
						c = 27;
					}
					else if ( c == '.' )
					{
						c = 28;
					}
					else if ( c == '%' )
					{
						c = 29;
					}
					else if ( c >= '0' && c <= '9' )
					{
						c = c - '0' + 30;
					}
					else if ( c >= 'A' && c <= 'Z' )
					{
						c = c - 'A' + 1;
					}
					/* Set lower case to upper case */
					else if ( c >= 'a' && c <= 'z' )
					{
						c = c - 'a' + 1;
					}
					else
					{
						sprintf(emsg, "Unknown RADIX-50 Character - Hex Value = %X", c);
						show_bad_token((inp_ptr), emsg, MSG_ERROR);
						/*Eat rest of line to suppress warning error about end of line not reached*/
						f1_eatit();
						return;
					}
					/* Each y assignment packs the remaining bytes with spaces */
					if ( x == 0 )
						y = c * 1600;
					if ( x == 1 )
						y = y + (c * 40);
					if ( x == 2 )
						y = y + c;
					x++;
					/* three bytes yet - put in buffer */
					if ( x >= 3 )
					{
						if ( (edmask & ED_M68) == 0 )
						{
							/* For little endian CPU's */
							*asc_ptr++ = y & 0xff;
							*asc_ptr++ = y >> 8 & 0xff;
						}
						else
						{
							/* For big endian CPU's*/
							*asc_ptr++ = y >> 8 & 0xff;
							*asc_ptr++ = y & 0xff;
						}
						x = 0;
						y = 0;
					}
					++inp_ptr;    /* point to next data byte */
				}   /* end of while convert loop */
			}   /* end of else an expression */
			if ( ((cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0) && (c != term_c) )
			{
				bad_token(inp_ptr, "Missing terminating delimiter");
				/*Eat rest of line to suppress warning error about end of line not reached*/
				f1_eatit();
				return;
			}
			else if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			{
				break;  /* Reached EOL - End Nicely */
			}
			/* if no data between delimiters - ".RAD50 //"  */
			/* Only do this if the three byte packing queue is empty  */
			if ( (rad50_count == 0) && ((x == 0) || (x >= 3)) )
			{
				y = 0;
				if ( (edmask & ED_M68) == 0 )
				{
					/* For little endian CPU's */
					*asc_ptr++ = y & 0xff;
					*asc_ptr++ = y >> 8 & 0xff;
				}
				else
				{
					/* For big endian CPU's */
					*asc_ptr++ = y >> 8 & 0xff;
					*asc_ptr++ = y & 0xff;
				}
				x = 0;
			}
			++inp_ptr;   /* eat the terminator */
			if ( isspace(*inp_ptr) && !no_white_space_allowed )
			{
				while ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) == 0 && isspace(*inp_ptr) )
				{
					++inp_ptr;        /* eat ws between blocks */
				}
			}
			if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			{
				break;   /* Reached EOL - stop */
			}
			c = *inp_ptr;    /* pickup user data */
			if ( c == expr_open )    /* test for expression */
			{
				term_expr = 1;    /* set if expression */
				term_c = expr_close;    /* new end delimiter */
				++inp_ptr;    /* eat the delimiter */
			}
			else
			{
				term_c = c;    /* new end delimiter */
				++inp_ptr;    /* eat the delimiter */
				rad50_count = 0;   /* reset count */
			}   /* end of test for expression */
		}    /* Buffer full or Reached the end of characters to process */
		/* if true end of line and less than three bytes - fill remaing bytes with space */
		if ( ((cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0) && (x > 0) && (x < 3) )
		{
			if ( (edmask & ED_M68) == 0 )
			{
				/* For little endian CPU's */
				*asc_ptr++ = y & 0xff;
				*asc_ptr++ = y >> 8 & 0xff;
			}
			else
			{
				/* For big endian CPU's */
				*asc_ptr++ = y >> 8 & 0xff;
				*asc_ptr++ = y & 0xff;
			}
			x = 0;
			y = 0;
		}
		/* if any packed data write it to *.ol file and write info into the *.lst file */
		len = asc_ptr - asc;   /* how much is in there */
		if ( len > 0 )
		{
			/* write to *.ol file */
			write_to_tmp(TMP_ASTNG, len, asc, sizeof(char));
			/* write to *.lis file */
			asc_ptr = asc;
			if ( show_line && (list_bin || meb_stats.getting_stuff) )
			{
				int tlen;
				char *dst;
				int n_ct;  /* nibble count */
				/* RAD50 packing always creates a word */
				/* writing a word (two bytes) at a time to *.lis so cut lenght in half */
				tlen = len / 2;
				if ( list_radix == 16 )
				{
					/* writing four hex nibbles and the space for a count of 5 */
					n_ct = 5;
				}
				else
				{
					/* OCTAL - writing six octal nibbles and the space for a count of 7 */
					n_ct = 7;
				}
				while ( 1 )
				{
					int z;
					if ( tlen <= 0 )
						break;
					z = (LLIST_SIZE - lstat->list_ptr) / n_ct;
					if ( z <= 0 )
					{
						if ( !list_bex )
							break;
						fixup_overflow(lstat);
						z = (LLIST_SIZE - LLIST_OPC) / n_ct;
						lstat->pc = current_offset + (asc_ptr - asc);
						lstat->pc_flag = 1;
					}
					dst = lstat->listBuffer + lstat->list_ptr;
					if ( tlen < z )
						z = tlen;
					lstat->list_ptr += z * n_ct;
					tlen -= z;
					do
					{
						unsigned char c1;
						unsigned char c2;
						unsigned int c3;

						/* For *.lis files - swap low high bytes for little endian CPU's
						   so that it shows as High byte Low byte in the *.lst file
						   Unless LIST_COD is on which states list as used in *.ol file */
						if ( ((edmask & ED_M68) == 0) && ((lm_bits.list_mask & LIST_COD) == 0) )
						{
							c2 = *asc_ptr++;
							c1 = *asc_ptr++;
						}
						/* For *.lis files - big endian CPU's */
						else
						{
							c1 = *asc_ptr++;
							c2 = *asc_ptr++;
						}

						if ( list_radix == 16 )
						{
							*dst++ = hexdig[(c1 >> 4) & 0x0F];
							*dst++ = hexdig[c1 & 0x0F];
							*dst++ = hexdig[(c2 >> 4) & 0x0F];
							*dst++ = hexdig[c2 & 0x0F];
						}
						else
						{

							c3 = (c1 << 8) | c2;

							*dst++ = ((c3 >> 15) & 7) + 0x30;
							*dst++ = ((c3 >> 12) & 7) + 0x30;
							*dst++ = ((c3 >> 9) & 7) + 0x30;
							*dst++ = ((c3 >> 6) & 7) + 0x30;
							*dst++ = ((c3 >> 3) & 7) + 0x30;
							*dst++ = ((c3)&7) + 0x30;
						}
						++dst;
					} while ( --z > 0 );
				}    /* -- for each item in asc [while (1)]*/
			}    /* -- list_bin != 0 */
			current_offset += len;
		}    /* -- something to write (len > 0) */
		/* if end of characters to process Stop, otherwise reset buffer and continue*/
		if ( (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			break;
	}    /* End of Set up Buffer */
	if ( c != term_c )
	{
		bad_token(inp_ptr, "No matching delimiter");
	}
	out_pc = current_offset;
	meb_stats.expected_seg = current_section;
	meb_stats.expected_pc = current_offset;
	return;
}

int op_rad50(void)
{
	rad50_common();
	return 0;
}

/******************************************************************
 * 01/26/2022   added for HLLxxF support by TG
 * 04/06/2022   updated by TG - suppress leading zeros not working correctly - Fixed
 *              added supprot for RADIX
 *
 * .PRINT
 * op_print - Prints any valid number and optional text to the listing line.
 *            If used inside a macro, allows printing text to the macro call line
 *
 */
int op_print(void)
{
	LIST_stat_t *lstat;
	char rad_num[18];
	long sav_data=0;
	int tt;

/*
 * At entry:
 * 	    arg - pointer to an int array with a size of at least seven bytes
 *	inp_ptr - points to next source code byte
 *	inp_str - contains the source code line
 *
 * At exit:
 *	Number data and any optional text is printed to the LIST file *.lst
 *
 *	returns a 0 if no args, otherwise returns the number of args
 *
 * Notes:
 *
 *        .PRINT number_data(arg[ListArg_Dst],arg[ListArg_Len],arg[ListArg_Radix],arg[ListArg_NLZ],arg[ListArg_Sign],arg[ListArg_NewLine])
 *
 *  Exp.
 *        .PRINT number_data(43,3,10,1)
 *
 *              num_data = Any valid number to be printed
 *	Not Used   arg[ListArg_ID]      = Format ID #  -  (this is not used in .PRINT - arg[1] is first arg
 *	required   arg[ListArg_Dst]     = Target of data field - line location
 *	required   arg[ListArg_Len]     = Field lenght - Number of characters to print 
 *	optional   arg[ListArg_Radix]   = Field Radix - print number using this radix             
 *                                     0 = current selected radix  - Default
 *                                     1 = set to MACxx default radix
 *                                     2 = set radix Binary
 *                                     8 = set radix Octal
 *                                    10 = set radix Decimal
 *                                    16 = set radix Hexadecimal
 *	optional   arg[ListArg_LZ]     = Flag - if .NE. include leading zeros   - Default No - set to zero
 *	optional   arg[ListArg_Sign]    = Flag - if .NE. prefix a sign           - Default No - set to zero
 *	optional   arg[ListArg_NewLine] = Flag - if .NE. request a new list line - Default No - set to zero
 *                                  0 = No
 *                                  / = Yes - New line requested
 * NOTE: The '/' in the text indicating 'NewLine' can technically appear in any of those of the positions.
 * The arg[ListArg_NewLine] is only a placeholder and will not be set in any case.
 *
 *	To print optional text put it before or after the options 
 *	Format - line location 'optional text'
 *
 *  Exp.
 *        .PRINT number_data(40'optional text',43,3,10,1)
 *  	  .PRINT number_data(43,3,10,1,46'appended text')
 *  	  .PRINT number_data(44.,/,3)	; Prints 3 chars of number_data in current radix at column 44. Forces line to print.
 *
 */

	if ( meb_stats.getting_stuff )   /* is this a macro line */
	{
		lstat = &meb_stats;
	}
	else
	{
		lstat = &list_stats;
	}

/* Need to do here - test to see if anything on line (args) if not exit */

	if ( *inp_ptr != '(' )
	{
		if ( (tt = get_token()) != EOL )   /* pick up number */
		{
			exprs(0, &EXP0);
			sav_data = EXP0SP->expr_value;
		}
	}
	if ( *inp_ptr == '(' )
	{
		int arg[ListArg_MAX];
		int ltoaRadix;
/*        int arg_cnt; */

		/* pickup any options */
		/* arg_cnt = */ list_args(arg, ListArg_MAX, ListArg_Dst, list_source.optTextBuf, sizeof(list_source.optTextBuf));

		switch (arg[ListArg_Radix])
		{
		default:
		case 0:
			ltoaRadix = current_radix;	/* Use whatever is the current radix */
			break;
		case 1:
			ltoaRadix = list_radix;  /* set MACxx default listing radix */
			break;
		case 2:		/* Binary */
		case 8:		/* Octal */
		case 10:	/* Decimal */
		case 16:	/* Hexadecimal */
			ltoaRadix = arg[ListArg_Radix];
			break;
		}
		longToAscii(sav_data, rad_num, sizeof(rad_num), arg[ListArg_Len], arg[ListArg_LZ], ltoaRadix);
		if ( (arg[ListArg_Dst] != 0) && (arg[ListArg_Dst] < 256) )   /* valid line location */
		{
			int idx;
			char *lp, *lpe;
			lp = lstat->listBuffer - 1 + arg[ListArg_Dst];
			lpe = lp+arg[ListArg_Len];
			for (idx=0; rad_num[idx] && lp < lpe; ++idx)
				*lp++ = rad_num[idx];
		}
		
		/* If optional text to print - buffer (not NULL) */
		if ( list_source.optTextBuf[0] != 0 )
		{
			int x = 0;
			while ( 1 )
			{
				*(lstat->listBuffer - 1 + list_source.optTextBuf[0] + x) = list_source.optTextBuf[1 + x];
				x++;
				if ( list_source.optTextBuf[1 + x] == 0 )
					break;
			}
		}

		if ( list_source.reqNewLine )
		{
			line_to_listing();   /* FORCE LINE TO PRINT */
			list_source.reqNewLine = 0;
		}

		tt = 0;
	}
	else  /* if no options print - Directive generated error command*/
	{
		if ( tt != EOL )
		{
			if ( show_line && (list_bin || meb_stats.getting_stuff) )
			{
				list_stats.pf_value = EXP0SP->expr_value;
				list_stats.f1_flag = 1;
				tt = 2;
			}
		}
		else
		{
			tt = 0;
		}

		show_bad_token((char *)0, "Directive generated error",
					   MSG_ERROR);
	}

	return 0;
}


/******************************************************************
 * 12/28/2022   added .copy Command by Tim Giddens
 *
 *
 *	.copy -	Copies a text file into the listing file
 * 
 * At entry:
 * 	inp_ptr - must be pointing the text file name. 
 *
 * At exit:
 *	The named text file is printed to the LIST file *.lst
 *
 * Exp.
 *		.COPY filename.ext
 *		.COPY WSMAIN.LNK
 *
 *******************************************************************/
int op_copy(void)
{
	int tt;
	char buff[256];
	FILE *file_cpy;

	/* If no List file - do nothing just return */
	if ( lis_fp == 0 )
	{
		f1_eatit();
		return 1;
	}

	/* is .LIST turned on ? */
	if ( show_line > 0)
	{
		tt = get_token();
		if ( tt == TOKEN_strng )
		{
			/* open the file */    
			file_cpy = fopen(token_pool,"rt");    
			if ( !file_cpy )
			{
				sprintf(emsg," Unable to open file \"%s\" - file not found!", token_pool);
				show_bad_token(tkn_ptr, emsg, MSG_ERROR);
				f1_eatit();	/* eat rest of line */
				return -1;
			}
			line_to_listing();
			show_line = 0;		/* Don't show the .copy line again */
			buff[sizeof(buff)-1] = 0;
			while( fgets(buff,sizeof(buff)-2,file_cpy) )
			{
				int len = strlen(buff);
				if ( len )
				{
					/* only print non-null lines */
					if ( len >= (int)sizeof(buff) - 2 || buff[len - 1] != '\n' )
					{
						/* make sure all lines end in a newline */
						buff[len++] = '\n';
						buff[len] = 0;
					}
					puts_lis(buff, 1);
				}
			}
			fclose(file_cpy);
		}
		else
		{
			sprintf(emsg," Missing file name ");
			show_bad_token(tkn_ptr, emsg, MSG_ERROR);
			f1_eatit();	/* eat rest of line */
			return -1;

		}
	}
	f1_eatit();		/* eat rest of line */
	return 0;

}

#endif	/* defined(MAC_PP) */

