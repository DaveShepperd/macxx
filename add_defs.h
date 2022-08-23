/*
    add_defs.h - Part of macxx, a cross assembler family for various micro-processors
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

#if !defined(_ADD_DEFS_)
#define _ADD_DEFS_ 1
typedef struct file_name {
#if defined(VMS)
   int wild_cards;
#endif
   char *full_name;
   char *path;
   char *name_only;
   char *type_only;
   char *name_type;
} FILE_name;

#define ADD_DEFS_EACCESS -2		/* try to access file for input */
#define ADD_DEFS_SYNTAX	 -1		/* check syntax only */
#define ADD_DEFS_INPUT	  0		/* check the file for input */
#define ADD_DEFS_OUTPUT	  1		/* check the file for output */

extern int add_defs(
#if defined(GCC)
   		    char *name,		/* string having filename */
   		    char **types,	/* ptr to array of strings having filetypes */
   		    char **paths,	/* ptr to array of paths to search for file */
   		    int io,		/* command (one of the ADD_DEFS_xxx above) */
   		    FILE_name **retptr	/* ptr to results is placed here */
#endif
);

/************************************************************************
 * The add_defs routine will construct a filename from the bits supplied 
 * to it by the parameters. Items such as path and filetype will be prepended
 * or appended apprpriately if necessary. If the io flag is set to
 * ADD_DEFS_INPUT, the routine will test for the presence of the file
 * and if not there, AND there was no filetype on the filename, try the next
 * filetype in the list and continue. If all the types are exhausted (or
 * there was one on the input name), it'll try all the paths in the pathlist
 * (and each path with the list of filetypes). The current default path is
 * assumed as the last in the list. The filetype and path lists are NULL
 * terminated arrays of pointers to null terminated strings.
 *
 * The pathlist is ignored on VMS systems and if "io" is ADD_DEFS_OUTPUT 
 * or ADD_DEFS_SYNTAX, then only the first (if any) pathname is used
 * on non-VMS systems. Either the type list and the pathlist parameter
 * may be NULL indicating that portion of the filename is not to be
 * changed (although if no path is specified on the input filename, 
 * the current default directory will always be prepended).
 *
 * add_defs will malloc enough memory to accomodate all the new filename
 * strings and parts of filename strings as well as the FILE_name structure
 * which contains containing pointers to these strings in one single malloc.
 * Therefore, the memory can be free()'d with a free(*retptr);
 * 
 * An example of calling:
 *
 * #include "add_defs.h"
 * #define NULLP (char *)0
 * char *filename = "foo";	users filename
 * char *types[] = {".h",".c",NULLP}; 		list of file types
 * char *paths[] = {"/usr/include",NULLP};	list of paths
 * FILE_name *fnp;
 * 
 * if (!add_defs(filename,types,paths,ADD_DEFS_xxx,&fnp)) {
 *	error processing...fnp may be NULLP if some major error
 *	otherwise fnp will point to the name of the file that
 *	couldn't be opened.
 * }
 * do stuff...
 * free(fnp);					done with all of it
 *********************************************************************/
#endif
