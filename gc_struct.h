/*
    gc_struct.h - Part of macxx, a cross assembler family for various micro-processors
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

#ifndef _GC_STRUCT_H_
#define _GC_STRUCT_H_ 1

#if defined(INTERNAL_PACKED_STRUCTS)
#include "pragma1.h"
#endif

struct qual {
   unsigned int noval:1;	/* value is not allowed */
   unsigned int optional:1;	/* value is optional */
   unsigned int number:1;	/* value must be a number */
   unsigned int output:1;	/* parameter is an output file */
   unsigned int string:1;	/* option is a string */
   unsigned int negate:1;	/* value is negatable */
   unsigned int error:1;	/* field is in error */
   unsigned int negated:1;	/* option is negated */
   unsigned int present:1;	/* option found on command line */
   int qualif;			/* value to .or. into cli_options */
   char *name;			/* pointer to qualifier ascii string */
   char index;			/* output file index */
   char *value;			/* pointer to user's value string */
};

#if defined(INTERNAL_PACKED_STRUCTS)
#include "pragma.h"
#endif

#endif  /* _GC_STRUCT_H_ */
