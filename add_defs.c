/*
    add_defs.c - Part of macxx, a cross assembler family for various micro-processors
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

/****************************************************************************
 * add_defs.c - routine to process a filename. Decides which elements
 * exist in the filename setting flags appropriately and also applies defaults
 * to fields that are missing.
 */

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "add_defs.h"

#define FILEDEBUG 1
#if !defined(MALLOCDEBUG)
    #define MALLOCDEBUG 0
#endif
#ifndef STAND
    #define STAND 0
#endif

#ifdef VMS
    #include <unixio.h>
    #include <stat.h>
    #include <rms.h>
    #include <devdef.h>
    #define EXIT_FALSE exit(0x10000002)
#else
    #if defined(C68K)
        #include <types.h>
        #include <stat.h>
    #else
        #include <sys/types.h>
        #include <sys/stat.h>
    #endif
    #define EXIT_FALSE exit(1)

    #if defined(M_XENIX) || defined(sun) || defined(SUN) || defined(M_UNIX) || defined(unix)
        #define C_DELIM 	'-'	/* UNIX(tm) uses dashes as option delimiters */
        #define C_PATH		'/'	/* pathname indicator */
    #else
        #define C_DELIM 	'/'	/* VMS and MS-DOS use slash */
        #define C_PATH		'\\'	/* MS-DOS uses backslash as path */
    #endif
#endif

#if defined(VMS)
static char tmp_esa[NAM$C_MAXRSS+1]; /* room for longest filename */
static char tmp_rsa[NAM$C_MAXRSS+1]; /* room for longest filename */
static struct NAM nam;
static struct FAB fab;
#endif

#if defined(WIN32) || defined(MS_DOS)
    #define S_ISDIR(mode) ((mode & 0xF000)==S_IFDIR)
#endif

#if defined(VMS)
    #define strcpy_upc strcpy
    #define strncpy_upc strncpy

static FILE_name *fill_in_file( void )
{
    FILE_name *fnp;
    char *s;
    int i,len;
    if (nam.nam$b_rsl == 0)
    {
        s = nam.nam$l_esa;
        len = nam.nam$b_esl;
    }
    else
    {
        s = nam.nam$l_rsa;
        len = nam.nam$b_rsl;
    }
    i =  len+1+          /* full pathname plus a null */
         (nam.nam$l_name-s)+1+   /* path only plus a null */
         2*(nam.nam$b_name+nam.nam$b_type)+3+    /* 2 names+2 types+3 nulls */
         sizeof(FILE_name);  /* room the the FILE_name struct */

    fnp = (FILE_name *)malloc(i);
    if (fnp == 0)
    {
        perror("add_defs: Ran out of memory during filename processing");
        EXIT_FALSE;
    }
    fnp->wild_cards = (nam.nam$l_fnb&(1<<NAM$V_WILDCARD)) ? 1:0; /* WC flag */
    fnp->full_name = (char *)(fnp+1);
    strncpy_upc(fnp->full_name,s,len);
    fnp->full_name[len] = 0;
    fnp->path = fnp->full_name+len+1;
    i = nam.nam$l_name-s;
    strncpy_upc(fnp->path,s,i);
    fnp->path[i] = 0;
    fnp->name_type = fnp->path + i + 1;
    i = nam.nam$b_name+nam.nam$b_type;
    strncpy_upc(fnp->name_type,nam.nam$l_name,i);
    fnp->name_type[i] = 0;
    fnp->type_only = fnp->name_type+nam.nam$b_name;
    fnp->name_only = fnp->name_type+i+1;
    strncpy_upc(fnp->name_only,nam.nam$l_name,nam.nam$b_name);
    fnp->name_only[nam.nam$b_name] = 0;
    return fnp;
}
#else
    #if !defined(MS_DOS)
        #define strcpy_upc strcpy
        #define strncpy_upc strncpy
    #else

static void strcpy_upc(char *dst, char *src)
{
    char c;
    while ((c = *src++) != 0) *dst++ = islower(c)?toupper(c):c;
    *dst = 0;
    return;
}

static void strncpy_upc(char *dst, char *src, int len )
{
    char c;
    while (len > 0 && (c= *src++) != 0)
    {
        *dst++ = islower(c) ? toupper(c) : c;
        --len;
    }
    if (len > 0) *dst = 0;
}
    #endif

static FILE_name *fill_in_file( char *full)
{
    FILE_name *fnp;
    char *name,*type,*s;
    int pathlen,namelen,typelen,tot;
    name = strrchr(full,C_PATH);
    if (name != 0)
    {
        ++name;           /* skip over path terminator */
    }
    else
    {
        name = full;
    }
    pathlen = name-full;     /* compute length of pathname */
    type = strrchr(name,'.');    /* find filetype */
    if (type == 0)
    {
        namelen = strlen(name);
        typelen = 0;
    }
    else
    {
        namelen = type-name;
        typelen = strlen(type);
    }
    tot = 2*pathlen+3*namelen+2*typelen+sizeof(FILE_name)+8;
    fnp = (FILE_name *)malloc(tot);
    if (fnp == 0)
    {
        perror("add_defs: Ran out of memory during filename processing");
        EXIT_FALSE;
    }
    s = fnp->full_name = (char *)(fnp+1);
    strcpy_upc(s,full);
    s += pathlen+namelen+typelen;
    *s++ = 0;
    fnp->path = s;
    strncpy_upc(s,full,pathlen);
    s += pathlen;
    *s++ = 0;
    fnp->name_only = s;
    strncpy_upc(s,name,namelen);
    s += namelen;
    *s++ = 0;
    fnp->name_type = s;
    strncpy_upc(s,name,namelen+typelen);
    s += namelen+typelen;
    *s++ = 0;
    fnp->type_only = fnp->name_type+namelen;
#if MALLOCDEBUG
    fprintf(stderr,"Malloc'd %d at %08lX. Wrote %d\n",
            tot,fnp,s-(char *)fnp);
#endif
    return fnp;
}
#endif

#if defined(VMS)
/*************************************************************************
 * Find next file in the list (for wildcarded inputs)
 */
/*
 * At entry:
 *  retptr - ptr to result is placed in here 
 */
int next_file( FILE_name **retptr )
{
    int err,i;
    err = sys$search(&fab);
    if ((err&1) == 0)
    {
        vaxc$errno = err;
        errno = 65535;
        return 65535;
    }
    tmp_rsa[nam.nam$b_rsl] = 0;
    *retptr = fill_in_file();
    return 0;
}
#endif		/* if defined(VMS) */

#if !defined(VMS)

static char *our_cwd[2];

/***********************************************************************
 * Glue on path and filetype if not already present in filename string
 */
static char *add_ext( char *sptr, char **ext, char **path)
/*
 * At entry:
 *	sptr - ptr to filename string
 *	ext - ptr to array of ptrs to filetypes to append
 *	path - ptr to array of ptrs to pathnames to prepend
 *		(Note: path is not used on VMS systems)
 */
{
    int pathlen=0,extlen=0,namelen;
    char *lp,*rp,*typtr,*tmp;
    struct stat file_stat;
    if (sptr == 0) return sptr;
#if defined(MS_DOS)
#define DOS_PATH strchr(sptr,':') == 0 && 
#else
#define DOS_PATH
#endif
    if (DOS_PATH *sptr != C_PATH && path && *path)
    {
        pathlen = strlen(*path);
    }
#undef DOS_PATH
    namelen = strlen(sptr);
    if (ext && *ext) extlen = strlen(*ext);
    tmp = lp = malloc(pathlen+2*namelen+extlen+3);
    if (lp == 0)
    {
        perror("add_defs: ran out of room during filename processing");
        EXIT_FALSE;
    }
    *tmp = 0;
    if (pathlen != 0)
    {
        strcpy_upc(lp,*path);
        lp += pathlen;
        if ((*path)[pathlen-1] != C_PATH)
        {
            *lp++ = C_PATH;
            *lp = 0;
        }
    }
    if (namelen > 0)
    {
        strcpy_upc(lp,sptr);
        lp += namelen;
        if (*(lp-1) != C_PATH)
        {
            if (stat(tmp,&file_stat) >= 0)
            {
                if (S_ISDIR(file_stat.st_mode))
                {
                    *lp++ = C_PATH;
                    *lp = 0;
                    if ((rp=strrchr(sptr,C_PATH)) == 0 ) rp = sptr; /* skip the path */
                    strcpy_upc(lp,rp);
                    lp += strlen(rp);
                }
            }
        }
    }
    if ((rp=strrchr(tmp,C_PATH)) == 0 ) rp = tmp; /* skip the path */
    if ((typtr = strrchr(rp, '.')) != 0)
    {
        if (pathlen == 0)
        {
            free(tmp);     /* done with tmp area */
            return sptr;       /* already has a file type, return */
        }
    }
    else
    {
        if (( ext == 0 ||
              *ext == 0 ||
              (extlen=strlen(*ext)) == 0) &&
            pathlen == 0)
        {
            free(tmp);     /* done with tmp area */
            return sptr;       /* no change possible */
        }
    }
#if MALLOCDEBUG
    fprintf(stderr,"add_ext: malloc'd %d at %08lX\n",pathlen+namelen+extlen+1,rp);
#endif
    lp = rp + strlen(rp);
    if (typtr == 0 && extlen != 0) strcpy_upc(lp,*ext);
#if MALLOCDEBUG
    fprintf(stderr,"\tCopied %d bytes into %08lX\n",strlen(rp)+1,rp);
#endif
    return tmp;
}
#endif


/*************************************************************************
 * Add default fields to filename
 */
int add_defs( char *src_nam, char **inp_default_types, char **default_paths, int io, FILE_name **retptr)
/*
 * At entry:
 *
 *  src_nam - source filename filename string
 *  inp_default_types - default filetype string(s)
 *  default_paths - default path string(s)
 *  io  -  0 = test for input,
 *         1 = test for output,
 *        -1 = parse syntax only
 *  retptr - ptr to result is placed in here
 */
{
    int err;
    char *s;
    char **default_types;
    struct stat file_stat;
#if defined(VMS)
    nam = cc$rms_nam;
    nam.nam$l_esa = tmp_esa;
    nam.nam$b_ess = NAM$C_MAXRSS;
    nam.nam$l_rsa = tmp_rsa;
    nam.nam$b_rss = NAM$C_MAXRSS;
    nam.nam$b_nop = (io==ADD_DEFS_SYNTAX) ? (1<<NAM$V_SYNCHK) : 0;
    fab = cc$rms_fab;
    fab.fab$l_nam = &nam;
    fab.fab$l_fna = src_nam;
    fab.fab$b_fns = src_nam?strlen(src_nam):0;
    fab.fab$l_fop = (io==ADD_DEFS_OUTPUT) ? (1<<FAB$V_OFP) : 0;
#else
    err = 0;
#endif
    default_types = inp_default_types;
#if defined(VMS)
    do
    {
        if (default_types != 0)
        {
            fab.fab$l_dna = *default_types;
            fab.fab$b_dns = *default_types?strlen(*default_types):0;
        }
        err = sys$parse(&fab);
        tmp_esa[nam.nam$b_esl] = 0;
        tmp_rsa[nam.nam$b_rsl] = 0;
        if ((err&1) == 0)
        {
            vaxc$errno = err;
            errno = 65535;
            *retptr = (FILE_name *)0;  /* didn't parse */
            return err;
        }
        else
        {
            err = 0;
            if (io != ADD_DEFS_INPUT) break;
        }
        if ((fab.fab$l_dev&(DEV$M_FOD|DEV$M_DIR|DEV$M_AVL|DEV$M_MNT|DEV$M_IDV|DEV$M_ODV)) ==
            (DEV$M_FOD|DEV$M_DIR|DEV$M_AVL|DEV$M_MNT|DEV$M_IDV|DEV$M_ODV))
        {
            err = sys$search(&fab);
            if ((err&1) == 0)
            {
                if (err == RMS$_FNF)
                {
                    err = errno = ENOENT;
                    continue;
                }
                vaxc$errno = err;
                errno = 65535;
                break;
            }
        }
        err = 0;
        break;
    } while (default_types && *++default_types);
    *retptr = fill_in_file();
#else
    if (default_paths == 0)
    {
        if (our_cwd[0] == 0)
        {
            int cl;
            char *tmps;
            our_cwd[0] = (char *)malloc(256+2);
#if MALLOCDEBUG
            fprintf(stderr,"add_defs: malloc'd %d at %08lX\n",258,our_cwd[0]);
#endif
            tmps = (char *)getcwd(our_cwd[0],256);
            if (tmps != 0)
            {
                cl = strlen(tmps);
#if MALLOCDEBUG
                fprintf(stderr,"\tcopied %d bytes to it\n",cl+1);
#endif
                if (cl == 0 || tmps[cl-1] != C_PATH)
                {
                    tmps[cl++] = C_PATH;
                }
                tmps[cl++] = 0;
                tmps = (char *)malloc(cl);
#if MALLOCDEBUG
                fprintf(stderr,"add_defs: malloc'd %d bytes at %08lX\n",cl,tmps);
#endif
                if (tmps != 0)
                {
#if MALLOCDEBUG
                    fprintf(stderr,"\tCopied %d bytes to it\n",cl);
#endif
                    strcpy_upc(tmps,our_cwd[0]);
                    free(our_cwd[0]);
                    our_cwd[0] = tmps;
                }
            }
        }
        default_paths = our_cwd;
    }
#if !defined(VMS)
    if (io == ADD_DEFS_INPUT)
    {      /* on Unix, check filename "as is" first */
        if ((err=stat(src_nam,&file_stat)) >= 0)
        {
            if (!S_ISDIR(file_stat.st_mode))
            {
                *retptr = fill_in_file(src_nam);    /* not a directory, use name as supplied */
                return 0;
            }
        }
    }
#endif            
    s = 0;
    do
    {
        default_types = inp_default_types;    /* reset the type pointer */
        do
        {
            if (s != 0 && s != src_nam)
            {
                free(s);
#if MALLOCDEBUG
                fprintf(stderr,"add_defs: free'd %08lX\n",s);
#endif
            }
            s = add_ext(src_nam,default_types,default_paths);
            if (io != ADD_DEFS_INPUT) break;
            if ((err=stat(s,&file_stat)) < 0)
            {
                if (io == ADD_DEFS_EACCESS)
                {
                    errno = EACCES;
                    return EACCES;
                }
                errno = err = ENOENT;
                if (s != src_nam) continue; /* changed something, try the next */
            }
            break;
        } while (default_types && *++default_types);
    } while (err && err == ENOENT && default_paths && *++default_paths);
    *retptr = fill_in_file(s);
    if (s != 0 && s != src_nam)
    {
        free(s);
#if MALLOCDEBUG
        fprintf(stderr,"add_defs: free'd %08lX\n",s);
#endif
    }
#endif
    return err;
}

#if STAND
static char *defs[] = { ".ol",".ob",".vlda",0};

int main( int argc, char *argv[] )
{
    FILE_name *fnp;
    int io,err;
    if (argc <= 2)
    {
        io = ADD_DEFS_INPUT;
    }
    else
    {
        if (sscanf(argv[2],"%d",&io) != 1)
        {
            fprintf(stderr,"Error parsing i/o flag %s. S/B a 0 or 1\n",argv[2]);
            EXIT_FALSE;
        }
    }
    if ((err=add_defaults(argv[1],defs,(char *)0,io,&fnp)) != 0)
    {
        char *s,errmsg[132];
        if (fnp == 0)
        {
            s = argv[1];
        }
        else
        {
            s = fnp->full_name;
        }
        sprintf(errmsg,"Error parsing input \"%s\"",s);
        perror(errmsg);
        EXIT_FALSE;
    }
    fprintf(stderr,"Successfully parsed \"%s\"\n",
            argv[1]);
    fprintf(stderr,"\tFull name = \"%s\"\n",fnp->full_name);
    fprintf(stderr,"\tpath      = \"%s\"\n",fnp->path);
    fprintf(stderr,"\tname only = \"%s\"\n",fnp->name_only);
    fprintf(stderr,"\ttype only = \"%s\"\n",fnp->type_only);
    fprintf(stderr,"\tname_type = \"%s\"\n",fnp->name_type);
#if defined(VMS)
    if (fnp->wild_cards)
    {
        while (1)
        {
            free(fnp);
            if ((err=next_file(&fnp)) != 0)
            {
                if (errno == 65535 && vaxc$errno == RMS$_NMF) break;
                perror("Error during SYS$SEARCH");
            }
            fprintf(stderr,"Full name = \"%s\"\n",fnp->full_name);
            fprintf(stderr,"\tpath      = \"%s\"\n",fnp->path);
            fprintf(stderr,"\tname only = \"%s\"\n",fnp->name_only);
            fprintf(stderr,"\ttype only = \"%s\"\n",fnp->type_only);
            fprintf(stderr,"\tname_type = \"%s\"\n",fnp->name_type);
        }
    }
#endif
    free(fnp);
    return 0x10000001;
}
#endif
