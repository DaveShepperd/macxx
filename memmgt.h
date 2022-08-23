/*
    memmgt.h - Part of macxx, a cross assembler family for various micro-processors
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

#if !defined(_MEMMGT_H_)
#define _MEMMGT_H_

extern void *mem_alloc(int size, char *file, int line);	/* external memory allocation routine */
extern void *mem_realloc(void *old, int size, char *file, int line);
extern int   mem_free(void *old, char *file, int line);
#define MEM_alloc(size) mem_alloc(size, __FILE__, __LINE__)
#define MEM_free(area) mem_free(area, __FILE__, __LINE__)
#define MEM_realloc(area, size) mem_realloc(area, size, __FILE__, __LINE__)

#endif

