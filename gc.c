/*
    gc.c - Part of macxx, a cross assembler family for various micro-processors
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
/**************************************************************************
 *
 * This module (wholly) contains the code to obtain the command string from
 * the user. The calling convention:
 *
 * if (!getcommand()) exit(FALSE);
 *
 * The routine returns a TRUE/FALSE flag indicating success/failure.
 * The output filename data structures are located contigiously in memory.
 * The input filename data structures may or may not be located
 * contigiously in memory and there may be a variable number of them.
 * You must use the .next pointer to chain through them.
 * There are 3 output files available.
 * 
 * OBJECT - the main output file, absolute format either binary (.OB)
 *	or ascii (.OL).
 * LIST - The listing file. Contains the assembled source and object code
 *	in a human readable format.
 * BINARY - make a binary object file
 * WORK - File to hold the intermediate code (normally maintained in memory,
 *	but if there is not enough memory to hold it, you may have to re-run
 *	the job using a work file.
 *
 * The remaining command qualifiers are: 
 * 
 * /CROSS - display a cross reference of section and symbol names in the
 * 	listing file (ignored if no listing is requested).
 * /DEBUG - displays various debug messages during the assembly procedure
 * 
 ***********************************************************************/

#include <errno.h>
#include "token.h"
#include "gc_struct.h"
#include "add_defs.h"

#if defined(M_XENIX) || defined(sun) || defined(M_UNIX) || defined(unix)
    #define OPT_DELIM '-'		/* UNIX(tm) uses dashes as option delimiters */
char opt_delim[] = "-";
#else
    #define OPT_DELIM '/'		/* everybody else uses slash */
char opt_delim[] = "/";
#endif

#include "memmgt.h"

char options[QUAL_MAX];
char **cmd_assems, **cmd_includes, **cmd_outputs;
FN_struct *cmd_fnds;
int cmd_assems_index, cmd_outputs_index;
static int cmd_assems_size, cmd_includes_size, cmd_outputs_size;
static int cmd_includes_index;

/*****************************************************************************
 * The following are the command line options to be obtained from user.
 *****************************************************************************/

#if !defined(MAC_PP)
    #if !defined(NO_XREF)
        #define xref_desc	qual_tbl[QUAL_CROSS]
    #endif
    #define bin_desc	qual_tbl[QUAL_BINARY]
    #define cmos_desc	qual_tbl[QUAL_CMOS]
    #define p816_desc	qual_tbl[QUAL_P816]
    #define miser_desc	qual_tbl[QUAL_MISER]
    #define tmp_desc	qual_tbl[QUAL_TEMP]
    #define rel_desc	qual_tbl[QUAL_RELATIVE]
    #define boff_desc	qual_tbl[QUAL_BOFF]
    #define green_desc  qual_tbl[QUAL_GRNHILL]
	#define predef_desc qual_tbl[QUAL_PREDEFINE]
#endif
#define ide_desc	qual_tbl[QUAL_IDE_SYNTAX]
#define obj_desc	qual_tbl[QUAL_OUTPUT]
#define lis_desc	qual_tbl[QUAL_LIST]
#define deb_desc	qual_tbl[QUAL_DEBUG]
#define syml_desc	qual_tbl[QUAL_SYML]
#define opcl_desc	qual_tbl[QUAL_OPCL]
#define squeak_desc	qual_tbl[QUAL_SQUEAK]
#define incl_desc	qual_tbl[QUAL_INCLUDE]
#define assem_desc	qual_tbl[QUAL_ASSEM]

#ifdef VMS
    #define FILENAME_LEN 256	/* maximum length of filename in chars	*/
#else
    #define FILENAME_LEN 64		/* maximum length of filename in chars	*/
#endif

FN_struct output_files[OUT_FN_MAX]; /* declare space to keep output file specs */
FN_struct *next_inp;    /* pointer to next available file pointer */
FN_struct *last_inp;    /* pointer to last fnd used */
FN_struct *first_inp;   /* pointer to first fnd used */
FN_struct *fn_struct_pool; /* pointer to fn_struct pool */
int fn_struct_poolsize=0;   /* number of structs left in the pool */
int output_mode;        /* output mode */

#define DEFTYP(nam,string) char nam[] = {string};

DEFTYP(def_lis,".lis")
DEFTYP(def_mac,".mac")
DEFTYP(def_MAC,".MAC")
#if !defined(MAC_PP)
DEFTYP(def_ol, ".ol")
DEFTYP(def_ob,".ob")
DEFTYP(def_tmp,".tmp")
DEFTYP(def_od,".od")
#else
DEFTYP(def_asm,".asm")
#endif
#undef DEFTYP

#if defined(MAC_PP)
    #define DEF_OUT def_asm
#else
    #if defined(VMS)
        #define DEF_OUT def_ob
    #else
        #define DEF_OUT def_ol
    #endif
#endif

extern int max_qual;
extern struct qual qual_tbl[];

static struct
{
    struct qual *qdesc;
    char *defext;
} fn_ptrs[] = {
#ifndef lint
    {qual_tbl+QUAL_OUTPUT,DEF_OUT},  /* /OBJECT=filename */
    {qual_tbl+QUAL_LIST,def_lis},    /* /LIS=filename */
#if !defined(MAC_PP)
    {qual_tbl+QUAL_TEMP,def_tmp},    /* /TEMP=filename */
    {qual_tbl+QUAL_DEBUG,def_od}     /* /DEBUG=filename */
#else
    {0,0}
#endif
#else
    {0,0}
#endif
};

char *def_inp_ptr[] = {def_mac,def_MAC,NULL};
static char *def_out_ptr[] = {NULL,NULL};

/* The filenames are stored in free memory called fn_pool. The pool is */
/* managed by the fn_init routine by way of the following two variables */

int fn_pool_size=0;     /* records amount remaining in free pool */
char *fn_pool;          /* points to next char in free pool	 */

/***************************************************************************
 * FUNCTION fn_init:
 * This function initialises the fn_struct structure. It also adjusts the free
 * pool size if necessary.
 * The only required parameter is the pointer to the structure to be init'd
 * It always returns TRUE
 ***************************************************************************/

char *null_string="";   /* empty string */

void get_fn_pool( void )
{
    if (fn_pool_size < 2*FILENAME_LEN)
    { /* to save CPU time, the free pool */
        fn_pool_size = 1024;      /* a bunch of memory */
        fn_pool = (char *)MEM_alloc(fn_pool_size); /* the size required. It is assumed */
        misc_pool_used += fn_pool_size;
    }                    /* that most filenames will be short */
    return;
}

#if !defined(NO_XREF)
static FN_struct **xref_pool;   /* pointer to xref pool */
static int xref_pool_size=0;        /* size of xref free pool */

FN_struct **get_xref_pool( void )
{
    FN_struct **fn_ptr;
    if (xref_pool_size < XREF_BLOCK_SIZE)
    {
        int t;
        xref_pool_size = XREF_BLOCK_SIZE*256/sizeof(FN_struct **);
        t = xref_pool_size*sizeof(FN_struct **);
        xref_pool = (FN_struct **)MEM_alloc(t);
        misc_pool_used += t;
    }
    xref_pool_size -= XREF_BLOCK_SIZE;   /* dish them out n at a time */
    fn_ptr = xref_pool;
    xref_pool += XREF_BLOCK_SIZE;
    return(fn_ptr);
}
#endif

int fn_init(FN_struct *pointer)
{
    get_fn_pool();           /* get some free space in the pool */
#ifdef VMS
    pointer->d_length = FILENAME_LEN;    /* area length */
    pointer->s_type = DSC$K_DTYPE_T; /* string type (text) */
    pointer->s_class = DSC$K_CLASS_S;    /* string class (string) */
#endif
    pointer->fn_buff = fn_pool;      /* pointer to buffer */
    pointer->fn_name_only = null_string; /* assume no filename */
    pointer->fn_next = 0;        /* pointer to next structure */
    pointer->fn_present = 0;     /* flag indicating filename present */
    pointer->fn_line = 0;        /* start at line number 0 */
    *fn_pool = 0;            /* make sure array starts with 0 */
    pointer->macro_level = 0;        /* and there's no macro level */
    return(TRUE);            /* always return TRUE */
}

/**************************************************************************
 * Get a file descriptor structure.
 */
FN_struct *get_fn_struct(void)
/*
 * At entry:
 * 	no requirements
 * At exit:
 *	returns pointer to blank file descriptor structure init'd
 */
{
    if (fn_struct_poolsize <= 0)
    {   /* get some memory to hold the structs */
        int t = 16;
        fn_struct_pool = 
        (FN_struct *)MEM_alloc((int)sizeof(FN_struct)*t);
        fn_struct_poolsize = t;
        misc_pool_used += t*sizeof(FN_struct);
    }
    fn_init(fn_struct_pool);
    fn_struct_pool->fn_nam = NULL;
    --fn_struct_poolsize;
    return(fn_struct_pool++);
}

/* int gc_err; */

#if 0
    #if !defined(GCC)
extern int mkstemp( char *template );
    #endif
static char temp_filename[ 16 ];
#endif

FILE_name *image_name;

static int process_outfile(FN_struct *fnd, struct qual *desc_ptr, char **defname, int outn) {
    int erc = 0;
    fn_init(fnd);            /* destination and initialise it  */
    if (desc_ptr != 0 && desc_ptr->present && !desc_ptr->negated)
    {  /* is the option there? */
        fnd->fn_present = 1;  /* signal filename present */
        if (desc_ptr->value != 0)
        {
            FILE_name *fnamp;
            char *tdefname;

            fnd->fn_buff = desc_ptr->value;
            if (add_defs(desc_ptr->value,(char **)0,(char **)0,
                         ADD_DEFS_SYNTAX,&fnamp) == 0)
            {
                if (strlen(fnamp->name_only) == 0)
                {    /* if no name */
                    tdefname = (char *)MEM_alloc(strlen(fnamp->path)+strlen(*defname)+1);
                    if (tdefname == 0)
                    {
                        perror("out of memory parsing output filenames");
                        EXIT_FALSE;
                    }
                    strcpy(tdefname,fnamp->path);
                    strcat(tdefname, *defname);
                    MEM_free((char *)fnamp);
                    fnd->fn_buff = tdefname;
                }
            }
        }
        else
        {
            if (outn == OUT_FN_TMP)
            {
#if 1
                err_msg( MSG_FATAL, "Sorry, TEMP option is no longer supported\n" );
                return 1;
#else
                strcpy( temp_filename, "MACXXXXXX" );
                mkstemp( temp_filename ); /* create a dummy filename */
                fnd->fn_buff = temp_filename;
                def_out_ptr[0] = 0;
#endif
            }
            else
            {
                fnd->fn_buff = *defname;
            }
        }
        erc = add_defs(fnd->fn_buff,def_out_ptr,(char **)0,
                       ADD_DEFS_OUTPUT,&fnd->fn_nam); /* get user's defaults */
        if (erc == 0)
        {
            fnd->fn_name_only = fnd->fn_nam->name_type;
            fnd->fn_buff = fnd->fn_nam->full_name;
        }
        else
        {
            sprintf(emsg,"Unable to parse output filename \"%s\": %s",
                    fnd->fn_buff,error2str(errno));
            err_msg(MSG_FATAL,emsg);
        }
        if (fnd->fn_nam) *defname = fnd->fn_nam->name_only;
    }
    return erc;
}

/************************************************************************
 * FUNCTION getcommand:
 * The main routine in this module. It requires 2 inputs declared extern:
 */
extern int gc_argc;      /* arg count */
extern char **gc_argv;   /* arg value */
/*
 *************************************************************************/
static void add_def_obj(void)
{
	if (cmd_outputs_index >= cmd_outputs_size)
	{
		if (cmd_outputs)
		{
			cmd_outputs_size += cmd_outputs_size/2;
		}
		else
		{
			cmd_outputs_size = 16;
		}
		cmd_outputs = (char **)MEM_realloc((char *)cmd_outputs, cmd_outputs_size*sizeof(char *));
		memset((char *)(cmd_outputs+cmd_outputs_index), 0, (cmd_outputs_size-cmd_outputs_index)*sizeof(char *));
	}
	cmd_outputs[cmd_outputs_index++] = obj_desc.value;
}

int getcommand(void)
{
    char c,*s;
    int i,rms_errors=0;
    FN_struct *fnd;
    struct qual *desc_ptr;
    char **argv;

    argv = gc_argv;
	gc_pass = 1;
    add_defs(*argv,(char **)0,(char **)0,ADD_DEFS_SYNTAX,&image_name);
    s = *++argv;
    for (i=1; i < gc_argc;i++,s= *++argv)
    {
        while ( (c = *s) )
        {
            if (c == OPT_DELIM)
            {
#if !defined(VMS)
                c = *(s+1);
                if (c == 0 || c == OPT_DELIM)
                {
                    last_inp = next_inp;     /* record the pointer to this one */
                    next_inp = get_fn_struct();      /* get a fn structure */
                    next_inp->fn_buff = "NONAME.mac";    /* make the filename special */
                    next_inp->r_length = strlen(next_inp->fn_buff);
                    next_inp->fn_stdin = 1;      /* signal file is from stdin */
                    if (last_inp) last_inp->fn_next = next_inp; /* link last one to this one */
                    if (!first_inp) first_inp = next_inp;    /* record the first structure addr */
                    ++s;
                    if (c == OPT_DELIM) ++s;
                    continue;
                }
#endif
                s = do_option(s);
                if (assem_desc.present)
                {
                    if (cmd_assems_index >= cmd_assems_size)
                    {
                        if (cmd_assems)
                        {
                            cmd_assems_size += cmd_assems_size/2;
                        }
                        else
                        {
                            cmd_assems_size = 16;
                        }
                        cmd_assems = (char **)MEM_realloc((char *)cmd_assems, cmd_assems_size*sizeof(char *));
                        memset((char *)(cmd_assems+cmd_assems_index), 0, (cmd_assems_size-cmd_assems_index)*sizeof(char *));
                    }
                    cmd_assems[cmd_assems_index++] = assem_desc.value;
                    assem_desc.value = 0;
                    assem_desc.present = 0;
                }
                if (incl_desc.present)
                {
                    if (cmd_includes_index >= cmd_includes_size)
                    {
                        if (cmd_includes)
                        {
                            cmd_includes_size += cmd_includes_size/2;
                        }
                        else
                        {
                            cmd_includes_size = 16;
                        }
                        cmd_includes = (char **)MEM_realloc((char *)cmd_includes, cmd_includes_size*sizeof(char *));
                        memset((char *)(cmd_includes+cmd_includes_index), 0, (cmd_includes_size-cmd_includes_index)*sizeof(char *));
                    }
                    cmd_includes[cmd_includes_index++] = incl_desc.value;
                    incl_desc.value = 0;
                    incl_desc.present = 0;
                }
                if (obj_desc.present)
                {
					add_def_obj();
					obj_desc.value = 0;
					obj_desc.present = 0;
                }
                continue;
            }
            else if (c == ' ' || c == '\t' || c == ',')
            {
                ++s;        /* means skip it */
                continue;
            }
            else
            {
                char *beg;
                int len;
                last_inp = next_inp;        /* record the pointer to this one */
                next_inp = get_fn_struct();     /* get a fn structure */
                if (last_inp) last_inp->fn_next = next_inp; /* link last one to this one */
                if (!first_inp) first_inp = next_inp;   /* record the first structure addr */
                beg = s;
                while (1)
                {
                    c = *s++;
                    if (c == OPT_DELIM || c == ' ' || c == '\t' ||
                        c == ',' || c == 0)
                    {
                        --s;
                        break;
                    }
                }
                next_inp->r_length = len = s-beg;
                if (len > 0)
                {
                    if ((next_inp->fn_buff = (char *)MEM_alloc(len+1)) == 0)
                    {
                        perror("Out of memory during input filename token parsing");
                        EXIT_FALSE;
                    }
                    strncpy(next_inp->fn_buff,beg,len);
                    next_inp->fn_buff[len] = 0;
                }
                else
                {
                    next_inp->fn_buff = "";      /* make the filename null */
                }
            }              /* --if not delimter */
        }                 /* --while() */
    }                    /* --for() */
    squeak = squeak_desc.present;
    debug = deb_desc.present;
    if ((fnd = first_inp) == 0)
    {
        sprintf(emsg,"No files input.\n");
        err_msg(MSG_FATAL,emsg);
		gc_pass = 0;
        return FALSE;
    }
#if !defined(MAC_PP)
#if !defined(NO_XREF)
    if (options[QUAL_CROSS])
		lis_desc.present = 1;   /* cross defaults to /LIS */
#endif
    if (!boff_desc.present)
		options[QUAL_BOFF] = 1;  /* default to global branch offset testing */
    if (!rel_desc.present)
		options[QUAL_RELATIVE] = 1;
#if defined(VMS)
    if (!bin_desc.present)
		options[QUAL_BINARY] = 1; /* default to binary mode */
#endif
#if !defined(SUN)
    if (!miser_desc.present)
		options[QUAL_MISER] = 1;    /* default to miser mode */
#endif
	if ( !predef_desc.present || (predef_desc.present && !predef_desc.negated) )
		options[QUAL_PREDEFINE] = 1;
#endif	/* !defined(MAC_PP) */
#if 0
	if ( !ide_desc.present )
		options[QUAL_IDE_SYNTAX] = 1;	/* Default to IDE error syntax */
	if ( options[QUAL_IDE_SYNTAX] )
		options[QUAL_ABBREV] = 0;		/* Cannot have both ABBREV and IDE */
#endif
	if ( syml_desc.present )
	{
		if ( (int)syml_desc.value < 6 || (int)syml_desc.value > 16 )
		{
			sprintf(emsg, "Value on -symbol_length has to be 6 <= n <= 16. Value of %d ignored.", (int)syml_desc.value);
			err_msg(MSG_WARN, emsg);
			++gc_err;
		}
		max_symbol_length = (int)syml_desc.value;
	}
    if (opcl_desc.present)
	{
		if ( (int)opcl_desc.value < 6 || (int)opcl_desc.value > 16 )
		{
			sprintf(emsg, "Value on -opcode_length has to be 6 <= n <= 16. Value of %d ignored.", (int)opcl_desc.value);
			err_msg(MSG_WARN, emsg);
			++gc_err;
		}
        max_opcode_length = (int)opcl_desc.value;
	}
    while (fnd != 0)
    {
        int err;
        err = add_defs(fnd->fn_buff,def_inp_ptr,(char **)0,
                       fnd->fn_stdin?ADD_DEFS_SYNTAX:ADD_DEFS_INPUT,&fnd->fn_nam);
        if (err != 0)
        {
            sprintf(emsg,"Unable to parse input filename \"%s\": %s",
                    fnd->fn_buff,error2str(errno));
            err_msg(MSG_FATAL,emsg);
            rms_errors += err;
        }
        if (fnd->fn_nam)
        {
            fnd->fn_buff = fnd->fn_nam->full_name;
            fnd->fn_name_only = fnd->fn_nam->name_type;
#if 0
			printf("getcommand(): fnd=%p, fnd->fn_nam=%p, fnd->fn_nam->relative_name_only=%p '%s'\n",
				   (void *)fnd,
				   (void *)fnd->fn_nam,
				   fnd->fn_nam->relative_name,
				   fnd->fn_nam->relative_name
				   );
#endif
        }
        else
        {
            fnd->fn_name_only = fnd->fn_buff;
			printf("getcommand(): fnd=%p, fnd->fn_nam=NULL\n", (void *)fnd);
        }
        fnd = fnd->fn_next;
    }
#if !defined(MAC_PP)
    output_mode = options[QUAL_BINARY]*2 + 1;
#endif
    if (!rms_errors)
    {
        char *defname, *objdefname;
		if ( cmd_outputs_index == 0 )
		{
			add_def_obj();
		}
        defname = first_inp->fn_nam->name_only;   /* use the first file's name */

#if OUT_FN_OBJ != 0
        Phfffftt, You need to redo this code since OUT_FN_obj is not 0
#endif

        if (cmd_outputs_index > 0)
        {
            cmd_fnds = (FN_struct *)MEM_alloc(cmd_outputs_index*sizeof(FN_struct));
            desc_ptr=fn_ptrs[0].qdesc; /* get pointer to parameter descriptor */
            desc_ptr->present = 1;     /* there's at least one */
            objdefname = defname;
            for (i=0; i<cmd_outputs_index; ++i)
            {
                desc_ptr->value = cmd_outputs[i];
                if (i)
                {
                    def_out_ptr[0] = 0;
                    if (desc_ptr->value == 0 || strlen(desc_ptr->value) < 1)
                    {
                        char *th, numth[32];
                        switch (i)
                        {
                        case 5:  th = "sixth"; break;
                        case 4:  th = "fifth"; break;
                        case 3:  th = "fourth"; break;
                        case 2:  th = "third"; break;
                        case 1:  th = "second"; break;
                        default:
                            snprintf(numth, sizeof(numth), "%d'th", i+1);
                            th = numth;
                            break;
                        }
                        sprintf(emsg, "No filename provided on %s OUTPUT option", th);
                        err_msg(MSG_FATAL, emsg);
                        ++gc_err;
                        continue;
                    }
                }
                else
                {
#ifndef MAC_PP
                    def_out_ptr[0] = (output_mode > 1) ? def_ob : def_ol;
#else
                    def_out_ptr[0] = def_asm;
#endif
                }
                fnd = cmd_fnds + i;
                rms_errors += process_outfile(fnd, desc_ptr, &objdefname, 0);
                if (i==0) defname = objdefname;
                objdefname = 0;
            }
            output_files[0] = cmd_fnds[0];
        }
        for (i=1;i<OUT_FN_MAX;i++)
        {
            desc_ptr=fn_ptrs[i].qdesc; /* get pointer to parameter descriptor */
            if (desc_ptr == 0) break;  /* reached the end */
            fnd = &output_files[i];    /* get pointer to fn_struct struct of  */
            def_out_ptr[0] = fn_ptrs[i].defext;
            rms_errors += process_outfile(fnd, desc_ptr, &defname, i);
        }
    }
	gc_pass = 0;
    if (gc_err || rms_errors) return FALSE;  /* exit false if errors */
    return TRUE;             /* exit true if no errors */
}
