/*
    strsub.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _STRSUB_H_
#define _STRSUB_H_ 1

struct str_sub {
   int name_len;
   char *name;
   int value_len;
   char *value;
};

extern struct str_sub *string_macros;
extern char *presub_str;
extern int strings_substituted;
extern void op_purgedefines(struct str_sub *sub);

#endif /* _STRSUB_H_ */
