/*
    memmgt.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "token.h"
#include <stdlib.h>
#include <string.h>

long total_mem_used;
long peak_mem_used;
long misc_pool_used;

typedef struct hdr
{
    unsigned long size;
    void *caller;
    int line;
    char *file;
    struct hdr *next, *prev;
    unsigned long magic;
} Hdr;

#define PRE_MAGIC 	0x12345678
#define NOTALLOCED  	0x00010000
#define POST_MAGIC	0x87654321
#define NFG ((char *)0)

#if defined(DEBUG_MALLOC)
static Hdr *top, *bottom;
#endif

static char *check(Hdr *hdr) {
    char *msg = 0;
    unsigned long *end;

    if (hdr->magic != PRE_MAGIC)
    {
        if (hdr->magic == (PRE_MAGIC|NOTALLOCED))
        {
            msg = "%%%s-F-FATAL, %s:%d tried to free %08lX already free'd.\n";
        }
        else
        {
            msg = "%%%s-F-FATAL, %s:%d tried to free %08lX with corrupted header.\n";
        }
    }
    else
    {
        end = (unsigned long *)((char *)(hdr+1)+hdr->size);
        if (*end != POST_MAGIC)
        {
            msg = "%%%s-F-FATAL, %s:%d tried to free %08lX with corrupted tail.\n";
        }
    }
    return msg;
}

#if defined(DEBUG_MALLOC)
static void check_all( void ) {
    Hdr *h;
    char *msg;

    h = top;
    while (h)
    {
        msg = check(h);
        if (msg)
        {
            fprintf(stderr, msg, macxx_name, h->file, h->line, h+1);
            abort();
        }
        h = h->next;
        if (h == top) break;
    }
}
#endif

int mem_free(void *s, char *file, int line) {
    Hdr *hdr;
    char *msg=0;

    if (s == 0)
    {
        msg = "%s:%d tried to free %08lX.\n";
    }
    else
    {
        hdr = (Hdr *)s - 1;
        msg = check(hdr);
    }
    if (msg)
    {
        fprintf(stderr, msg, macxx_name, file, line, s);
        abort();
    }
#if defined(LLF) || defined(MACXX)
    total_mem_used -= hdr->size+sizeof(Hdr)+sizeof(long);
#endif
#if defined(DEBUG_MALLOC)
    check_all();
    if (hdr == top)
    {
        top = hdr->next;
    }
    if (hdr == bottom)
    {
        bottom = hdr->prev;
    }
    if (hdr->next) hdr->next->prev = 0;
    if (hdr->prev) hdr->prev->next = 0;
    hdr->next = hdr->prev = 0;
#endif
    free((char *)hdr);
    return(0);                  /* assume it worked */
}

void *mem_alloc(int nbytes, char *file, int line) {
    void *s;
    Hdr *hdr;
    unsigned long *end;
    int siz;

    nbytes = (nbytes + (sizeof(long)-1)) & ~(sizeof(long)-1);
    siz = nbytes + sizeof(Hdr) + sizeof(long);
    hdr = (Hdr *)calloc((unsigned int)siz,(unsigned int)1);  /* get some memory from OS */
    if (hdr == (Hdr *)0)
    {
        fprintf(stderr,"%%%s-F-FATAL, %s:%d Ran out of memory requesting %d bytes. Used %ld so far.\n",
                macxx_name, file, line, siz, total_mem_used);
#if defined(LLF) || defined(MACXX)
        display_mem();
#endif
        fprintf(stderr,"%s",emsg);
        abort();
    }
    hdr->size = nbytes;
    hdr->file = file;
    hdr->line = line;
    hdr->magic = PRE_MAGIC;
    s = (void *)(hdr+1);
    end = (unsigned long *)((char *)s+nbytes);
    *end = POST_MAGIC;
#if defined(DEBUG_MALLOC)
    if (top == 0)
    {
        top = hdr;
    }
    hdr->prev = bottom;
    if (bottom) bottom->next = hdr;
    bottom = hdr;
    hdr->next = 0;
    check_all();
#endif
#if defined(LLF) || defined(MACXX)
    total_mem_used += siz;
    if (total_mem_used > peak_mem_used) peak_mem_used = total_mem_used;
#endif
    return s;
}

void *mem_realloc(void *old, int nbytes, char *file, int line) {
    void *s;
    Hdr *hdr;
#if defined(DEBUG_MALLOC)
    Hdr *prev, *next;
#endif
    int siz;
    unsigned long *end;

    if (old != 0)
    {
        hdr = (Hdr *)old-1;
        siz = hdr->size;
        if (hdr->magic != PRE_MAGIC)
        {
            fprintf(stderr, "%%%s-F-FATAL, %s:%d realloc'd %08lX with corrupted header.\n",
                    macxx_name, file, line, (unsigned long)old);
            abort();
        }
#if defined(LLF) || defined(MACXX)
        total_mem_used -= siz+sizeof(Hdr)+sizeof(long);
#endif
        end = (unsigned long *)((char *)old+hdr->size);
        if (*end != POST_MAGIC)
        {
            fprintf(stderr, "%%%s-F-FATAL, %s:%d realloc'd %08lX with corrupted trailer.\n",
                    macxx_name, file, line, (unsigned long)old);
            if (hdr->line > 0) fprintf(stderr, "              area allocated by %s:%d\n", hdr->file, hdr->line);
            abort();
        }
#if defined(DEBUG_MALLOC)
        check_all();
        next = hdr->next;
        prev = hdr->prev;
#endif
        *end = 0;

        nbytes = (nbytes + (sizeof(long)-1)) & ~(sizeof(long)-1);
        siz = nbytes + sizeof(Hdr) + sizeof(long);
        s = (void *)realloc(hdr, siz);
        if (s == NFG)
        {
            fprintf(stderr,"%%%s-F-FATAL, %s:%d ran out of memory realloc'ing %ld bytes to %d. Used %ld so far.\n",
                    macxx_name, file, line, hdr->size, siz, total_mem_used);
#if defined(LLF) || defined(MACXX)
            display_mem();
#endif
            fprintf(stderr, "%s", emsg);
            abort();
        }
        hdr = (Hdr *)s;
        hdr->file = file;
        hdr->line = line;
        hdr->magic = PRE_MAGIC;
        if (nbytes > hdr->size)
        {     /* 0 the newly alloc'd  area */
            s = (char *)(hdr+1)+hdr->size;
            memset(s, 0, nbytes-hdr->size);
        }
        s = (void *)(hdr+1);
        end = (unsigned long *)((char *)s+nbytes);
        *end = POST_MAGIC;
        hdr->size = nbytes;
#if defined(DEBUG_MALLOC)
        hdr->next = next;
        hdr->prev = prev;
        if (next) next->prev = hdr;
        if (prev) prev->next = hdr;
        if (top == prev) top = hdr;
#endif
#if defined(LLF) || defined(MACXX)
        total_mem_used += siz;
        if (total_mem_used > peak_mem_used) peak_mem_used = total_mem_used;
#endif
        return s;
    }
    else
    {
        return mem_alloc(nbytes, file, line);
    }
}
