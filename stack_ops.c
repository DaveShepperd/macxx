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

int op_defstack( void )
{
	f1_eatit();
	return 0;
}

int op_push(void)
{
	f1_eatit();
	return 0;
}

int op_pop(void)
{
	f1_eatit();
	return 0;
}

int op_getpointer(void)
{
	f1_eatit();
	return 0;
}

int op_putpointer(void)
{
	f1_eatit();
	return 0;
}

#endif	/* !defined(VMS) && !defined(ATARIST) */

