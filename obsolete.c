/* All the functions in this module are kept for reference purposes. They are not used anymore */

/* DMS - re-wrote the ascii and rad50 functions completely. Parts of them are used in the new op_db and op_dc directives. */

#ifndef OLD_ASCII_COMMON
#define OLD_ASCII_COMMON (0)
#endif
#ifndef NEW_ASCII_COMMON
#define NEW_ASCII_COMMON (0)
#endif
#ifndef OLD_RAD50_COMMON
#define OLD_RAD50_COMMON (0)
#endif

#if !defined(MAC_PP) && OLD_ASCII_COMMON
static char asc[128];      /* place to stuff text */
#endif

#if OLD_RAD50_COMMON || OLD_ASCII_COMMON
static char *tmpAscStr;
static int tmpAscStrLen;

static char *getTmpStr(int minLen)
{
	if ( tmpAscStrLen < minLen )
	{
		tmpAscStrLen = minLen;
		tmpAscStr = (char *)realloc(tmpAscStr,tmpAscStrLen);
		if ( !tmpAscStr )
		{
			return NULL;
		}
	}
	return tmpAscStr;
}
#endif	/* OLD_RAD50_COMMON */

#if OLD_ASCII_COMMON
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
		if ( term_c == expr_close )   /* fix for variable function */
			term_c = expr_open;
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
				if ( term_c == expr_open )  /* fix for variable function */
				{
					fake = 0;    /* we have an expression */
					break;
				}
				c = *inp_ptr;   /* pickup user data */
				if ( (cttbl[c] & CT_EOL) != 0 || c == term_c )
				{
					fake = 0;    /* we're not faking it anymore */
					break;
				}
				/* Allows a three digit octal number to be entered within the delimiters
				   example:  \377 would be FF hex -  /ABC\377DEF/    */
				if ( c == '\\' && (arg&ASC_COMMON_ESCAPES) )
				{
					uint8_t whatToPass=inp_ptr[1];
					/* Allows for standard 'C' string escape processing */
					switch (whatToPass)
					{
					case 'a':	/* alert/bell */
						whatToPass = '\a';
						break;
					case 'b':	/* backspace */
						whatToPass = '\b';
						break;
					case 'e':	/* escape */
						whatToPass = 033;
						break;
					case 'f':	/* formfeed */
						whatToPass = '\f';
						break;
					case 'n':	/* newline */
						whatToPass = '\n';
						break;
					case 'r':	/* carriage return */
						whatToPass = '\r';
						break;
					case 't':	/* tab */
						whatToPass = '\t';
						break;
					case 'v':	/* vertical tab */
						whatToPass = '\v';
						break;
					case '\\':	/* backslash */
						whatToPass = '\\';
						break;
					case '\'':	/* apostrohpe */
						whatToPass = '\'';
						break;
					case '"':	/* double quote */
						whatToPass = '"';
						break;
					case '?':	/* question mark */
						whatToPass = '?';
						break;
					case 'x':	/* hex number */
						{
							uint32_t val=0;
							char *iptr = inp_ptr+2;
							while ( 1 )
							{
								char ch0;
								ch0 = *iptr;
								if ( islower(ch0 ) )
									ch0 = toupper(ch0);
								if ( (ch0 >= '0' && ch0 <= '9') )
								{
									val <<= 4;
									val |= ch0 - '0';
								}
								else if ( (ch0 >= 'A' && ch0 <= 'F') )
								{
									val <<= 4;
									val |= 10 + ch0 - 'A';
								}
								else
									break;
								++iptr;
							}
							if ( iptr > inp_ptr+2 )
							{
								whatToPass = val&0xFF;
								inp_ptr = iptr-2;
							}
						}
						break;
					case '0':
					case '1':
					case '2':
					case '3':	/* octal number */
						if (    (inp_ptr[2] >= '0' && inp_ptr[2] <= '7')
						     && (inp_ptr[3] >= '0' && inp_ptr[3] <= '7')
						   )
						{
							whatToPass = ((whatToPass - '0') << 6) | ((inp_ptr[2] - '0') << 3) | (inp_ptr[3] - '0');
							inp_ptr += 2;
						}
						break;
#if 0
					/* should also handle unicode here. Not going to */
					case 'u':
					case 'U':
#endif
					default:
						break;
					}
					*asc_ptr++ = whatToPass;
					inp_ptr += 2;
				}
				else if ( c == '\\' &&
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
				/*   This allows the character double quote (") to be embeded in a string when
					 the delimiter is also set to double quote (")   */
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
				/* If reached true end of data fix last byte for .ASCIN and .ASCIZ */
				if (    (arg&(ASC_COMMON_NULL|ASC_COMMON_MINUS))
					 && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
				{
					if ( (arg&ASC_COMMON_NULL) )
					{      /* .ASCIZ */
						*asc_ptr++ = 0;   /* null terminate the string */
						++len;        /* add 1 char */
					}
					if ( (arg&ASC_COMMON_MINUS) )
					{      /* .ASCIN */
						/*	04-16-2022 - TG
							This is not correct but it's the way the old version
					  of MACxx worked  Firefox took advantage of this in 
					  in file FFMES.MAC
							Not 100 percent sure if it's a problem here or if the macro
							called us with a null     .ascin //   aka   .ascin /null/   
					  But if you need an end of text marker then you would need this
	*/
						if ( asc_ptr == asc )
						{
							*asc_ptr++ = 0;
							++len;        /* add 1 char */
						}
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
					int tlen, n_ct; /* fix for OCTAL listing */
					char *dst;
					tlen = len;
					/*    01-16-2022  Support for Octal listing  */
					if ( list_radix == 16 )
					{   /* HEX - two hex nibbles and space for a count of 3 */
						n_ct = 3;
					}
					else
					{   /* OCTAL - three octal nibbles and space for a count of 4 */
						n_ct = 4;
					}
					while ( 1 )
					{
						int z;
						if ( tlen <= 0 )
							break;
						z = (LLIST_SIZE - lstat->list_ptr) / n_ct;  /* fix for OCTAL listing */
						if ( z <= 0 )
						{
							if ( !list_bex )
								break;
							fixup_overflow(lstat);
							z = (LLIST_SIZE - LLIST_OPC) / n_ct;    /* fix for OCTAL listing */
							lstat->pc = current_offset + (asc_ptr - asc);
							lstat->pc_flag = 1;
						}
						dst = lstat->listBuffer + lstat->list_ptr;
						if ( tlen < z )
							z = tlen;
						lstat->list_ptr += z * n_ct;    /* fix for OCTAL listing */
						tlen -= z;
						do
						{
							uint8_t c1;
							c1 = *asc_ptr++;
							if ( list_radix == 16 ) /* fix for OCTAL listing */
							{
								*dst++ = hexdig[c1 >> 4];
								*dst++ = hexdig[c1 & 0x0F];
							}
							else
							{
								*dst++ = ((c1 >> 6) & 7) + 0x30;
								*dst++ = ((c1 >> 3) & 7) + 0x30;
								*dst++ = (c1 & 7) + 0x30;
							}
							++dst;
						} while ( --z > 0 );
					}            /* -- for each item in asc [while (1)]*/
				}               /* -- list_bin != 0 */
				current_offset += len;
			}              /* -- something to write (len > 0) */
			if ( !fake )
			{
				if ( !(arg&ASC_COMMON_COMMA) && term_c == expr_open )   /* fix for variable function */
				{    /* have an expression? */
					char *ip, s1, s2, *strt;
					int nst, val, abs;
					EXPR_struct *exp_ptr;
					nst = 0;         /* assume top level */
					strt = ip = inp_ptr;
					/* point to place after expr_open */ /* fix for variable function */
					while ( 1 )
					{          /* find matching end */
						int chr;
						chr = *ip++;      /* find end pointer */
						if ( (cttbl[chr] & CT_EOL) != 0 )
						{
							--ip;          /* too far, backup 1 */
							bad_token(ip, "Missing Expression Bracket"); /* fix for variable function */
							f1_eatit();
							return;
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

					/*	fix for variable function -
						on exit of this loop c contains the ending delimiter and
					   pointer inp_ptr points to the next starting delimiter */
					if ( term_c == expr_open )
						term_c = expr_close;
					c = *inp_ptr++;
					if ( !no_white_space_allowed )
					{
						while ( isspace(*inp_ptr) )
							++inp_ptr; /* eat ws */
					}

					if ( (arg&ASC_COMMON_MINUS) && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
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
						if ( list_radix == 16 )     /* fix for OCTAL listing */
						{
							fs = abs ? 3 : 4;
						}
						else
						{
							fs = abs ? 4 : 5;
						}
						if ( list_bex || lstat->list_ptr <= LLIST_SIZE - fs )
						{
							char *s;
							if ( lstat->list_ptr > LLIST_SIZE - fs )
								fixup_overflow(lstat);
							s = lstat->listBuffer + lstat->list_ptr;
							if ( list_radix == 16 ) /* fix for OCTAL listing */
							{
								*s++ = hexdig[((uint8_t)val) >> 4];
								*s++ = hexdig[val & 15];
							}
							else
							{
								*s++ = ((val >> 6) & 7) + 0x30;
								*s++ = ((val >> 3) & 7) + 0x30;
								*s++ = (val & 7) + 0x30;
							}
							if ( !abs )
								*s = 'x';
							lstat->list_ptr += fs;
						}
					}
					if ( abs )
					{
						int epv;
						epv = EXP0SP->expr_value;
						if ( (edmask&ED_TRUNC) && (epv > 255 || epv < -256) )
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
					if ( (arg&ASC_COMMON_NULL) && (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
					{
						static char zero = 0;
						if ( show_line && list_bin )
						{
							int fs;     /* fix for OCTAL listing */
							if ( list_radix == 16 )
							{
								fs = 3;
							}
							else
							{
								fs = 4;
							}
							if ( list_bex || lstat->list_ptr <= LLIST_SIZE - fs )
							{
								char *s;
								if ( lstat->list_ptr > LLIST_SIZE - fs )
									fixup_overflow(lstat);
								s = lstat->listBuffer + lstat->list_ptr;
								*s++ = '0';
								*s = '0';
								if ( list_radix != 16 )     /* fix for OCTAL listing */
								{
									*++s = '0';
									lstat->list_ptr += 1;
								}
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
		if ( (arg&ASC_COMMON_COMMA) || (cttbl[(int)*inp_ptr] & (CT_EOL | CT_SMC)) != 0 )
			break;
	}                    /* -- for all items in inp_str */
	out_pc = current_offset;
	meb_stats.expected_seg = current_section;
	meb_stats.expected_pc = current_offset;
	return;
}
#endif	/* OLD_ASCII_COMMON*/

#if OLD_RAD50_COMMON
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
	{
		char *asc_ptr, *asc_end;
#if OLD_ASCII_COMMON
		/* for all that will fit in asc
			asc is a buffer of 128 bytes used for temp storage */
		asc_ptr = asc;
		asc_end = asc + sizeof(asc) - 4;
#else
		/* for all that will fit in tmpAscStr
			tmpAscStr is a dynamic buffer of at least 256 bytes used for temp storage */
		asc_ptr = getTmpStr(256);
		asc_end = tmpAscStr + tmpAscStrLen;
#endif
		/* save four spaces for packing at the end of buffer if
		   not three bytes yet */
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
#if OLD_ASCII_COMMON
		len = asc_ptr - asc;   /* how much is in there */
#else
		len = asc_ptr - tmpAscStr;   /* how much is in there */
#endif
		if ( len > 0 )
		{
			/* write to *.ol file */
#if OLD_ASCII_COMMON
			write_to_tmp(TMP_ASTNG, len, asc, sizeof(char));
			asc_ptr = asc;
#else
			write_to_tmp(TMP_ASTNG, len, tmpAscStr, sizeof(char));
			asc_ptr = tmpAscStr;
#endif
			/* write to *.lis file */
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
#if OLD_ASCII_COMMON
						lstat->pc = current_offset + (asc_ptr - asc);
#else
						lstat->pc = current_offset + (asc_ptr - tmpAscStr);
#endif
						lstat->pc_flag = 1;
					}
					dst = lstat->listBuffer + lstat->list_ptr;
					if ( tlen < z )
						z = tlen;
					lstat->list_ptr += z * n_ct;
					tlen -= z;
					do
					{
						uint8_t c1;
						uint8_t c2;
						unsigned int c3;

						/* For *.lis files - swap low high bytes for little endian CPU's
						   so that it shows as High byte Low byte in the *.lst file
						   Unless LIST_COD is on which states list as used in *.ol file */
						if ( ((edmask & ED_M68) == 0) && ((lm_bits & LIST_COD) == 0) )
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

#endif

