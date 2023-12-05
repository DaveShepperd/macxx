/*
    macxx.c - Part of macxx, a cross assembler family for various micro-processors
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
/************************************************************************
 * Macxx - Generic macro assembler. Initially built to compile 6502 code
 *
 */
#include "token.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>               /* get time stuff */
#ifdef VMS
    #define unlink delete
#endif

#include "version.h"
#include "memmgt.h"
#include "strsub.h"
#include "listctrl.h"

extern int gc_pass;
struct stat file_stat;
unsigned long edmask;           /* enabl/disabl mask */
FN_struct *current_fnd;     /* global current_fnd for error handlers */
int current_outfile;        /* which entry in cmd_fnds[] which is the output file */
int warning_enable=1;           /* set TRUE if warnings are enabled */
int info_enable;                /* set TRUE if info messages are enabled */
int error_count[MSG_FATAL+1];	/* error counts */
int pass=0;                     /* pass indicator flag */
static char *months[] =
{ "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"};
char ascii_date[128 /*24*/];
int debug=0;                    /* debug status value */
int squeak=0;

#ifdef TIME_LIMIT
/************************************************************************
 * Dummy get-time routine. This is a red-herring routine that calls the
 * VMS $GETTIM system service figuring that someone might assume it is
 * what is used to set the time limit and patch it out.
 */
static void vmstime(tptr)
long *tptr;
{
    int stat;
    stat = sys$gettim(tptr);
	if ( !(stat & 1) )
		err_msg(MSG_FATAL, "Error getting system time.");
    return;
}
#endif

/************************************************************************
 * Error message handlers. All errors (except fatal) are reported via
 * this mechanism which may display the message in the MAP file as
 * well as stderr.
 */

void err_msg( int severity, const char *msg )
/*
 * At entry:
 *      severity - low order 3 bits = error severity:
 *              0 - warning
 *              1 - success
 *              2 - error
 *              3 - informational
 *              4 - fatal
 *              5 - continue
 *              8 - use msg as control string for sprintf
 *             16 - don't output message to stderr
 *             32 - include inp_str in print control
 *  		   64 - do not add anything extra to message
 *      msg - pointer to error message string
 */
{
    int spr=1,ctrl,serr,pinpstr, inlen=0, noExtra, noCount;
    static char sev_c[]="WSEIF";
    static char *sev_s[]= {"WARN","SUCCESS","ERROR","INFO","FATAL"};
    char *lemsg=NULL;
    const char *lmp=NULL;

 	if ( !pass && !gc_pass )	/* No errors during pass 0 */
		return;
#if 0
	{
		int mLen = msg?strlen(msg):0;
		const char *eol="";
		if ( mLen && msg[mLen-1] != '\n' )
			eol = "\n";
		printf("err_msg(): pass=%d, severity=0x%02X, ptr=%s%s", pass, severity, msg?msg:"<empty>\n", eol);
	}
#endif
    ctrl = severity & MSG_CTRL;
    serr = severity & MSG_NOSTDERR;
    pinpstr = severity & MSG_PINPSTR;
	noExtra = severity & MSG_NO_EXTRA;
	noCount = severity & MSG_DONT_COUNT;
    severity &= 7;
    if (!noCount && !serr && severity <= MSG_FATAL)
        ++error_count[severity];
    switch (severity)
    {
    default:                 /* eat any unknown severity message */
    case MSG_SUCCESS:		 /* success text is always eaten */
		return;
    case MSG_WARN:          /* warning */
        if (!warning_enable)
            return;         /* because we're gonna eat warnings */
        break;                 /* maybe goes to MAP too */
    case MSG_INFO:          /* info */
        if (!info_enable)
            return;         /* because we're gonna eat infos */
        break;                 /* and into map maybe */
    case MSG_ERROR:           /* error */
    case MSG_FATAL:        /* fatal */
        break;
    case MSG_CONT:
        spr = 0;               /* don't do the sprintf */
        lmp = msg;             /* point to source string */
        break;
    }
    if ( !noExtra )
    {
        if ( !lmp )
        {
            inlen = inp_str_size + strlen(msg) + 22;
            lmp = lemsg = MEM_alloc(inlen);
        }
        if ( !options[QUAL_ABBREV] )
        {
            if (spr)
            {
                char lmBuf[20];
                if ( !options[QUAL_IDE_SYNTAX])
                {
                    snprintf(lmBuf,sizeof(lmBuf),"%%%s-%c-%s,",macxx_name,sev_c[severity],sev_s[severity]);
                }
                else
                {
                    lmBuf[0] = sev_c[severity];
                    lmBuf[1] = '-';
                    lmBuf[2] = 0;
                }
                if (ctrl)
                {
                    if (pinpstr)
                    {
                        char *is;
                        int len;
                        if (strings_substituted != 0 && presub_str)
                        {
                            is = presub_str;
                            len = strlen(is);
                        }
                        else
                        {
                            is = inp_str;
                            len = inp_len;
                        }
                        if (len == 0)
                            is = "";
                        snprintf(lemsg,inlen,msg,is,lmBuf);
                    }
                    else
                    {
                        snprintf(lemsg,inlen,msg,lmBuf);
                    }
                }
                else
                {
                    snprintf(lemsg,inlen,"%s%s\n",lmBuf,msg);
                }
            }
        }
        else if ( lemsg )
        {
            char *needNL="";
            int mLen;
            mLen = strlen(msg);
            if ( mLen && msg[mLen-1] != '\n')
                needNL = "\n";
            if ( current_fnd )
            {
                snprintf(lemsg,inlen,"%s:%d: %s%s",
                    current_fnd->fn_name_only, current_fnd->fn_line, msg, needNL );
            }
            else
            {
                snprintf(lemsg,inlen,"%s%s", msg, needNL );
            }
            lmp = lemsg;
        }
    }
    if ( !lmp )
        lmp = msg;
    if ( lmp)
    {
        if ( serr == 0 )
            fputs(lmp,stderr);         /* write string to stderr */
        if (lis_fp)
        {                /* LIS file? */
            puts_lis(lmp,0);          /* write string to lis file too */
        }
    }
    if ( lemsg )
        MEM_free(lemsg);
    return;                      /* done */
}

#ifdef TIME_LIMIT
static long image_name_length,login_time[2];

struct item_list
{
    short buflen;
    short item_code;
    char *bufadr;
    int *retlen;
};

readonly static struct item_list jpi_item[2] = {
    { 256,519,emsg,&image_name_length},
    { 8,518,login_time,0}
};
readonly static int item_terminator=0;

#endif

time_t unix_time;
int gc_argc;
char **gc_argv;

#ifdef VMS
    #define TFOPEN_ARGS "w+","fop=tmp"
    #define FOPEN_ARGS  "w","rfm=var","mrs=32767"
    #define BOPEN_ARGS  "w","rfm=var","mrs=32767"
#else
    #define TFOPEN_ARGS "wb"
    #define BOPEN_ARGS  "wb"
    #define FOPEN_ARGS  "w"
#endif
#define DOPEN_ARGS  "wb"

int were_mac65, were_mac68, were_mac69, were_mactj, were_mac68k, were_mac682k, were_macas, were_mac11, were_macpp;
/************************************************************************
 * MACXX main entry.
 */
int main(int argc, char *argv[])
/*
 * At exit:
 *      Returns FATAL if any fatal errors
 *      Returns ERROR if any error errors
 *      Returns WARNING if any warnings
 *      Else Returns SUCCESS
 */
{
    int i,file_cnt;
#ifdef TIME_LIMIT
    long timed_out,*link_time,systime[2];
#endif
    struct tm *our_time;                 /* current time (for sym/sec and map files */
	char c,*s,*d;

    lap_timer((char *)0);                /* mark start of image */
    edmask = macxx_edm_default;          /* set default edmask */   
    get_token_pool(MAX_TOKEN, 0);
    strcpy(emsg,"\001 Version \002, \003");
    gc_argc = argc;                      /* pass argument pointers to GC */
    gc_argv = argv;
    unix_time = time((long *)0);         /* get ticks since 1970 */
    our_time = localtime(&unix_time);    /* get current time of year */
    snprintf(ascii_date,sizeof(ascii_date),"\"%s %02d %4d %02d:%02d:%02d\"",
            months[our_time->tm_mon],our_time->tm_mday,our_time->tm_year+1900,
            our_time->tm_hour,our_time->tm_min,our_time->tm_sec);
#if defined(VMS) || defined(MS_DOS)
    for (i=0;macxx_name[i];++i)
    {
        if (islower(macxx_name[i])) macxx_name[i] = toupper(macxx_name[i]);
    }
#endif
	s = emsg;
	d = token_pool;
	while ((c = *s++) != 0)
	{
		if (c == 1)
		{
			strcpy(d,macxx_name);
			d += strlen(macxx_name);
		}
		else if (c == 2)
		{
			strcpy(d,macxx_version);
			d += strlen(macxx_version);
		}
		else if ( c == 3 )
		{
			strcpy(d,macxx_descrip);
			d += strlen(macxx_descrip);
		}
		else
		{
			*d++ = c;
		}
	}
	*d = 0;
	if (macxx_name[3] == '6')
	{
		if ( macxx_name[4] == '5')
			were_mac65 = 1;
		else if (macxx_name[4] == '9')
			were_mac69 = 1;
		else if (macxx_name[4] == '8')
		{
			if ( macxx_name[5] == 'k')
				were_mac68k = 1;
			else if (macxx_name[5] == '2' && macxx_name[6] == 'k')
				were_mac682k = 1;
			else
				were_mac68 = 1;
		}
	}
	else if (macxx_name[3] == 't' && macxx_name[4] == 'j')
		were_mactj = 1;
	else if (macxx_name[3] == '1' && macxx_name[4] == '1')
		were_mac11 = 1;
	else if (macxx_name[3] == 'a' && macxx_name[4] == 's')
		were_macas = 1;
#if defined(MS_DOS) 
	fprintf(stderr,"%s\n",token_pool);
#endif
	if (argc < 2)
	{
#if !defined(MS_DOS) 
		fprintf(stderr,"%s\n",token_pool);
#endif
		return display_help();
	}
    if (!getcommand())
    {         /* process input command options */
        EXIT_FALSE;
    }
    list_source.srcPosition = LLIST_SIZE;
    list_source.srcPositionQued = LLIST_SIZE;
    init_exprs();
#if !defined(MAC_PP)
    outx_init();
#endif
    current_fnd = first_inp;     /* get input first file name */
#if !defined(MAC_PP)
	if ( options[QUAL_2_PASS] )
	{
		int ii;
		
		if ( !were_mac65 && !were_mac68 && !were_mac69 )
		{
			fputs("Sorry, the -2_pass option is not available in this assembler\n",stderr);
			EXIT_FALSE;
		}
		pass = 0;
		for (file_cnt=0;;file_cnt++)
		{
			if (current_fnd->fn_file == 0)
			{
				if (current_fnd->fn_stdin == 0)
				{
					if ((current_fnd->fn_file = fopen(current_fnd->fn_buff,"r")) == 0)
					{
						sprintf(inp_str,"Error opening %s for input:\n",current_fnd->fn_buff);
						perror(inp_str);
						EXIT_FALSE;
					}
				}
				else
				{
					fputs("Cannot rewind stdin. So two pass assembler option not available\n",stderr);
					EXIT_FALSE;
				}
				if (token_pool_size < 26)
				{
					get_token_pool(26, 0);
				}
				stat(current_fnd->fn_buff,&file_stat);
				strcpy(token_pool,ctime(&file_stat.st_mtime));
				current_fnd->fn_version = token_pool;
				*(token_pool+24) = 0;
				token_pool_size -= 25;
				token_pool += 25;
			}
#ifdef TIME_LIMIT
			if (timed_out && (file_cnt&3 != login_time[0]))
			{
				EXIT_TRUE;
			}
#endif
			if (squeak)
				printf("Processing file %s\n", current_fnd->fn_nam->relative_name);
			pass0(file_cnt); /* else do .MAC file input */
			if (fclose(current_fnd->fn_file) != 0)
			{
				sprintf(emsg,"Unable to close file: %s",
						current_fnd->fn_buff);
				pass = 1;	/* fake it so error messages show up */
				err_msg(MSG_ERROR,emsg);
				perror("\n\t");
				show_bad_token((char *)0,emsg,MSG_ERROR|MSG_NOSTDERR);
				pass = 0;	/* back to pass 0 */
			}
			current_fnd->fn_file = NULL;
			current_fnd->fn_line = 0;
			if (include_level > 0)
				--include_level;
			if (!(current_fnd=current_fnd->fn_next))
				break;
			macro_level = current_fnd->macro_level;   /* restore the macro level */
			current_fnd->macro_level = 0;             /* and make sure we're emtpy */
		}
		macro_level = 0;
		current_fnd = first_inp;	/* get input first file name */
		current_lsb=1;			    /* current local symbol block number */
		next_lsb=2;					/* next available local symbol block number */
		autogen_lsb=65000;			/* autolabel for macro processing */
		memset(&list_stats,0,sizeof(list_stats));
		memset(&meb_stats,0,sizeof(meb_stats));
		clear_outbuf();
		list_source.srcPosition = LLIST_SIZE;
		list_source.srcPositionQued = LLIST_SIZE;
		get_text_assems = 0;
		edmask = macxx_edm_default;
		show_line = 1;
		list_level = 0;
		list_radix = 16;
		for (ii=0; ii < seg_list_index; ++ii)
		{
			SEG_struct *segPtr;
			
			segPtr = seg_list[ii];
			segPtr->rel_offset = 0;
			segPtr->seg_pc = 0;
			segPtr->seg_base = 0;
		}
		op_purgedefines(string_macros);
		purge_data_stacks(NULL);
		string_macros = NULL;
	}
#endif
    if (output_files[OUT_FN_LIS].fn_present)
    {
        if (!(lis_fp = fopen(output_files[OUT_FN_LIS].fn_buff,"w")))
        {
            sprintf(emsg,"Error creating LIS file: %s\n",
                    output_files[OUT_FN_LIS].fn_buff);
            perror(emsg);
            EXIT_FALSE;
        }
        list_init(1);
    }
    else
    {
        list_init(0);
    }
    if (output_files[OUT_FN_OBJ].fn_present)
    {
#if defined(MAC_PP)
        int ii;
        FN_struct *fnp;
        for (fnp=cmd_fnds, ii=0; ii<cmd_outputs_index; ++ii, ++fnp)
        {
            fnp->fn_file = fopen(fnp->fn_buff, "w");
            if (fnp->fn_file == 0)
            {
                sprintf(emsg,"Error creating OBJ file: %s\n", fnp->fn_buff);
                perror(emsg);
                EXIT_FALSE;
            }
        }
        output_files[0] = cmd_fnds[0];
#else
        if (options[QUAL_BINARY])
        {
            obj_fp = fopen(output_files[OUT_FN_OBJ].fn_buff,BOPEN_ARGS);
            outx_width = MAX_LINE-1;
        }
        else
        {
            obj_fp = fopen(output_files[OUT_FN_OBJ].fn_buff,"w");
        }
        if (obj_fp == 0)
        {
            sprintf(emsg,"Error creating OBJ file: %s\n",
                    output_files[OUT_FN_OBJ].fn_buff);
            perror(emsg);
            EXIT_FALSE;
        }
#endif
#if !defined(MAC_PP)
        if (output_files[OUT_FN_DEB].fn_present)
        {
            deb_fp = fopen(output_files[OUT_FN_DEB].fn_buff,"wb");
            if (deb_fp == 0)
            {
                sprintf(emsg,"Error creating DEB file: %s\n",
                        output_files[OUT_FN_DEB].fn_buff);
                perror(emsg);
                EXIT_FALSE;
            }
            output_files[OUT_FN_DEB].fn_version = ascii_date;
        }
#endif
    }
#ifndef MAC_PP
    if (output_files[OUT_FN_TMP].fn_present &&
        output_files[OUT_FN_OBJ].fn_present)
    {
        if ((tmp_fp = fopen(output_files[OUT_FN_TMP].fn_buff,
                            TFOPEN_ARGS)) == (FILE *)0)
        {
            sprintf(emsg,"Error creating TMP file: %s\n",
                    output_files[OUT_FN_TMP].fn_buff);
            perror(emsg);
            EXIT_FALSE;
        }
    }
#endif
    lap_timer("Command fetch");          /* mark the lap time */
    inp_str_size = MAX_LINE;     /* init a buffer to use later */
    inp_str = MEM_alloc(inp_str_size);
#ifdef TIME_LIMIT
    vmstime(systime);
    timed_out = 1;
    if (sys$getjpi(0,0,0,&jpi_item,0,0,0)&1)
    {
        login_time[0] = login_time[0]&0x40000000 ? 1:0;
        if (login_time[1] < TIME_LIMIT) timed_out = 0;
    }
#endif
#if !defined(MAC_PP)
    if (options[QUAL_DEBUG])
	{
		dbg_init(); /* init the debug structs */
	}
#endif
	pass = 1;
    for (file_cnt=0;;file_cnt++)
    {
        if (current_fnd->fn_file == 0)
        {
            if (current_fnd->fn_stdin == 0)
            {
                if ((current_fnd->fn_file = fopen(current_fnd->fn_buff,"r")) == 0)
                {
                    sprintf(inp_str,"Error opening %s for input:\n",current_fnd->fn_buff);
                    perror(inp_str);
                    EXIT_FALSE;
                }
            }
            else
            {
                current_fnd->fn_file = stdin;
            }
#if 0
            if (file_list_size < 1)
            {
                file_list_size += 32;
                file_list = (FN_struct **)MEM_realloc(file_list,file_list_size*sizeof(FN_struct *));
            }
            file_list[file_list_index] = current_fnd;
            current_fnd->fn_filenum = file_list_index++;
#endif
            if (token_pool_size < 26) get_token_pool(26, 0);
            stat(current_fnd->fn_buff,&file_stat);
            strcpy(token_pool,ctime(&file_stat.st_mtime));
            current_fnd->fn_version = token_pool;
            *(token_pool+24) = 0;
            token_pool_size -= 25;
            token_pool += 25;
#if !defined(MAC_PP)
            if (options[QUAL_DEBUG]) add_dbg_file(current_fnd->fn_buff,current_fnd->fn_version,(int *)0);
#endif
        }
#ifdef TIME_LIMIT
        if (timed_out && (file_cnt&3 != login_time[0]))
        {
            EXIT_TRUE;
        }
#endif
        if (squeak)
		{
			printf ("Processing file %s\n",current_fnd->fn_buff);
		}
#if !defined(MAC_PP)
        if (obj_fp != 0) write_to_tmp(TMP_FILE,1,&current_fnd,sizeof(FN_struct *));
#endif
        pass1(file_cnt); /* else do .MAC file input */
        if (current_fnd->fn_stdin == 0 && fclose(current_fnd->fn_file) != 0)
        {
            sprintf(emsg,"Unable to close file: %s",
                    current_fnd->fn_buff);
            err_msg(MSG_ERROR,emsg);
            perror("\n\t");
            show_bad_token((char *)0,emsg,MSG_ERROR|MSG_NOSTDERR);
        }
        current_fnd->fn_file = 0;
        if (include_level > 0) --include_level;
#if !defined(MAC_PP)
        dbg_flush(1);
#endif
        if (!(current_fnd=current_fnd->fn_next)) break;
        macro_level = current_fnd->macro_level;   /* restore the macro level */
        current_fnd->macro_level = 0;             /* and make sure we're emtpy */
    }
    lap_timer("File inputs");            /* mark the lap time */
#ifndef MAC_PP
    if (output_files[OUT_FN_OBJ].fn_present)
    {
#ifdef TIME_LIMIT
        if (login_time[1] > systime[1])
        {
            err_msg(MSG_FATAL,"Fatal internal error #232, please submit an SPR");
            EXIT_FALSE;
        }
#endif
        outid(obj_fp,output_mode);
#if !defined(MAC_PP)
        if (options[QUAL_DEBUG]) out_dbgfname(obj_fp,&output_files[OUT_FN_DEB]);
#endif
        outxsym_fp = obj_fp;
        for (i=0;i<seg_list_index;++i)
        {
            outseg_def(*(seg_list+i));
        }
    }
    if (squeak) printf ("Sort symbols\n");
    dump_subsects();             /* flush out any subsections */
    sort_symbols();              /* sort the symbols */
    lap_timer("Symbol sort/definitions");
    if (squeak)
        printf ("Write object and list files\n");
    current_fnd = &output_files[OUT_FN_OBJ];
    inp_len = 0;                 /* tell everybody there's no input */
    pass = 2;                    /* signal doing pass 2 */
	if ( !error_count[MSG_FATAL] && !error_count[MSG_ERROR] )
		 pass2();                /* do output processing only if no errors */
#if 0
    if (deb_fp) dbg_add_local_syms();
#endif
    current_fnd = 0;
    if (tmp_fp)
    {
        fclose(tmp_fp);           /* close the temp file */
#ifdef ATARIST
        delete("MACXX0000.TMP");
#endif
    }
#endif
    if (obj_fp)
    {
        fclose(obj_fp);           /* close the temp file */
    }
    lap_timer("Obj/lis file outputs");
    if (squeak)
    {
        printf ("Finish up\n");
        display_mem();
/*      fputs(emsg,stdout); */
    }
    lap_timer("Image clean up"); /* display accumulated times */
    show_timer();                /* display all accumulated times and stuff */
    info_enable = 1;             /* enable informational message */
    if ( (i=(error_count[MSG_FATAL] | error_count[MSG_ERROR] | error_count[MSG_WARN])) )
    {
        sprintf(emsg,"Completed with %d error(s) and %d warning(s)",
                error_count[MSG_FATAL]+error_count[MSG_ERROR],error_count[MSG_WARN]);
        err_msg(MSG_INFO,emsg);
        if (error_count[MSG_FATAL]+error_count[MSG_ERROR] > 0 && 
            output_files[OUT_FN_OBJ].fn_present)
        {
            if (unlink(output_files[OUT_FN_OBJ].fn_buff) < 0)
            {
                sprintf(inp_str,"Error deleting file: %s\n",current_fnd->fn_buff);
                perror(inp_str);
            }
            err_msg(MSG_WARN,"No object file produced");
        }
    }
    if (lis_fp) fclose(lis_fp);
    if (deb_fp) fclose(deb_fp);
    if (squeak) printf("A total of %ld statements were processed\n",record_count);
#ifdef VMS
    if (error_count[MSG_FATAL]) return 0x10000004;
    if (error_count[MSG_ERROR]) return 0x10000002;
    if (error_count[MSG_WARN]) return 0x10000000;
    return 0x10000001;
#else
    if (i) exit(1);
    exit(0);
#endif
    return 1;            /* keep lint happy */
}

#ifdef VMS
    #include errno

int exit_false( void )
{
    char *em;
    if (errno == 0xFFFF) exit (vaxc$errno);
    exit (0x10000004);
}
#endif
