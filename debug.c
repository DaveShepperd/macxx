/*
    debug.c - Part of macxx, a cross assembler family for various micro-processors
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

#include <unistd.h>
#include "token.h"
#include "debug.h"
#include "memmgt.h"
#include "exproper.h"

OD_hdr od_hdr;      /* debug file header struct */
OD_top od_string;   /* string vector table */
OD_top od_sec;      /* section vector table */
OD_top od_file;     /* file vector table */
OD_top od_sym;      /* symbol vector table */
OD_top od_line;     /* line vector table */
static OD_elem elem_pool; /* pool for element structs */
static OD_line *prev_linp; /* previous line so we can patch the length field */
static SEG_struct *prev_line_segp;
static uint32_t prev_line_offset;
static uint16_t prev_cline;
static FN_struct *prev_fnd;
static char cwd[2048];  /* place to put default path */
static int cwd_len; /* length of path */

static OD_elem *get_od_elem(OD_top *top)
{
    OD_elem *ep;
    if (elem_pool.avail == 0)
    {
        elem_pool.item = (void *)MEM_alloc(512*sizeof(OD_elem));
        if (elem_pool.item == 0)
        {
            perror("Ran out of memory allocating 512 element structs");
            EXIT_FALSE;
        }
        elem_pool.alloc = elem_pool.avail = 512;
    }
    ep = (OD_elem *)elem_pool.item;   
    elem_pool.item = (void *)(ep+1);
    --elem_pool.avail;
    if (top->curr != 0)
    {
        top->curr->next = ep;
    }
    top->curr = ep;
    if (top->top == 0) top->top = ep;
    return ep;
}

static OD_elem *get_elem_mem(OD_top *top, int size, int qty, int advise)
{
    int i;
    OD_elem *ep;
    if (qty == 0) qty = 1;
    if (size == 0) size = 1;
    if (advise == 0) advise = 16384;
    if (size > advise) advise = size;
    ep = get_od_elem(top);
    i = advise/size;
    if (i < qty) i = qty;
    ep->item = ep->top = (void *)MEM_alloc(i*size);
    ep->alloc = i;
    ep->avail = i;
    return ep;
}

static OD_line *get_odline( int qty )
{
    OD_line *linp;
    OD_elem *ep;
    ep = od_line.curr;
    if (!ep || ep->avail < qty)
    {
        ep = get_elem_mem(&od_line,sizeof(OD_line),qty,0);
    }
    linp = (OD_line *)ep->item;
    ep->item = (void *)(linp+1);
    --ep->avail;
    od_line.length += 1;
    return linp;
}

int dbg_init( void )
{
    char *s;
    OD_elem *ep;
    cwd[0] = 0;
    cwd_len = 0;
#if !defined(VMS)
    if (getcwd(cwd, sizeof(cwd)-2))
    {
        cwd_len = strlen(cwd);
#if !defined(MSDOS)
        cwd[cwd_len++] = '/';
#else
        cwd[cwd_len++] = '\\';
#endif
        cwd[cwd_len] = 0;
    }
#endif
    ep = get_elem_mem(&od_string,sizeof(char),1,0);
    s = (char *)ep->item; 
    *s++ = 0;            /* strings skip the zero'th byte */
    ep->item = (void *)s;
    --ep->avail;
    ++od_string.length;      /* count the skipped byte in the length */
    get_elem_mem(&od_sec,sizeof(OD_sec),1,64*sizeof(OD_sec));
    get_elem_mem(&od_file,sizeof(OD_file),1,32*sizeof(OD_file));
    get_elem_mem(&od_sym,sizeof(OD_sym),1,0);
    get_elem_mem(&od_line,sizeof(OD_line),1,0);
    return 0;
}

int32_t dbg_flush( int flag )
{
    if (prev_linp != 0)
    {
        int32_t diff;
        if (prev_line_segp != 0)
        {
            diff = prev_line_segp->seg_pc-prev_line_offset;
        }
        else
        {
            diff = 255;
        }
        if (diff < 0 || diff > 255) diff = 255;
        prev_linp->sl.length = diff;
        if (flag) prev_linp = 0;
        return diff;
    }
    if (flag) prev_linp = 0;
    return -1;
}

static int ckpath(char *name, int len)
{
#if defined(VMS)
    return 0;
#else
    if (len == 0) return 0;
#if !defined(MSDOS)
    if (name[0] == '/') return 0;
#else
    if (name[0] == '\\' || name[1] == ':') return 0;
#endif
    return 1;
#endif
}

int add_dbg_file( char *name, char *version, int *indx)
{
    int nlen,vlen,plen=0;
    char *s;
    OD_elem *ep;
    OD_file *fp;
    ep = od_file.curr;
    if (ep->avail < 1)
    {
        ep = get_elem_mem(&od_file,sizeof(OD_file),1,32*sizeof(OD_file));
    }
    fp = (OD_file *)ep->item;
    ep->item = (void *)(fp+1);
    ep->avail -= 1;
    od_file.length += 1;
    nlen = strlen(name);
    if (ckpath(name, nlen)) plen = cwd_len;
    vlen = strlen(version);
    ep = od_string.curr;
    if (ep->avail < nlen+vlen+2)
    {
        ep = get_elem_mem(&od_string,sizeof(char),nlen+vlen+plen+2,0);
    }
    s = (char *)ep->item;
    fp->name.index = od_string.length;
    if (indx) *indx = od_string.length;
    if (plen)
    {
        strcpy(s, cwd);
        s += plen;
    }
    strcpy(s,name);
    s += nlen;
    *s++ = 0;
    od_string.length += nlen+plen+1;
    fp->version.index = od_string.length;
    strcpy(s,version);
    s += vlen;
    *s++ = 0;
    ep->item = (void *)s;
    ep->avail -= nlen+vlen+plen+2;
    od_string.length += vlen+1;
    return od_file.index++;
}

int find_dbg_file(char *name)
{
    int i=0, len;
    char *newname=name;
    OD_elem *ep,*strep;
    OD_file *fp;
    ep = od_file.top;
    len = strlen(name);
    if (ckpath(name, len))
    {
        newname = (char *)MEM_alloc(len+cwd_len+1);
        strcpy(newname, cwd);
        strcpy(newname+cwd_len, name);
    }
    do
    {
        int j;
        int32_t strndx;
        fp = (OD_file *)ep->top;
        for (j=0;j<ep->alloc-ep->avail;++j,++fp,++i)
        {
            strndx = 0;
            strep = od_string.top;
            do
            {
                if (fp->name.index >= strndx && fp->name.index < strep->alloc-strep->avail) break;
                strndx += strep->alloc-strep->avail;
            } while ((strep=strep->next) != 0);
            if (strep == 0)
            {
                if (newname != name) MEM_free(newname);
                return -1;
            }
            if (strcmp(newname,(char *)(strep->top) + fp->name.index-strndx) == 0)
            {
                if (newname != name) MEM_free(newname);
                return i;
            }
        }
    } while ((ep=ep->next) != 0);
    if (newname != name) MEM_free(newname);
    return -1;               /* not in there */
}   

static OD_sym *op_stabx(char *str, int len, int stabd )
{
    char c,*s;
    int i,type=0,other=0,desc=0;
    OD_elem *ep;
    OD_sym *sym;
    for (i=0;i<(stabd?3:4);++i)
    {
        while (c = *inp_ptr,isspace(c))
            ++inp_ptr; /* skip over white space */
        if (*inp_ptr == ',')
            ++inp_ptr;    /* eat commas */
        if (get_token() == EOL)
            break;
        exprs(1,&EXP0);           /* pickup the expression */
        if (i==0)
            type = EXP0SP->expr_value;
        else
            if (i==1)
                other = EXP0SP->expr_value;
            else
                if (i==2)
                    desc = EXP0SP->expr_value;
    }
    if (stabd == 0)
    {
        if (EXP0.ptr != 1)
        {
            bad_token(tkn_ptr,"This must resolve to a single term");
            return(OD_sym *)0;
        }
        if (EXP0SP->expr_code == EXPR_VALUE)
        {
            other = 0;
        }
        else if (EXP0SP->expr_code == EXPR_SYM)
        {
            other = 1;
        }
        else if (EXP0SP->expr_code == EXPR_SEG)
        {
            other = 2;
        }
        else
        {
            bad_token(tkn_ptr,"This term must be a constant or a label");
            return(OD_sym *)0;
        }
    }
    ep = od_sym.curr;
    if (ep->avail < 1)
    {
        ep = get_elem_mem(&od_sym,sizeof(OD_sym),1,0);
    }
    sym = (OD_sym *)ep->item;
    ep->item = (void *)(sym+1);
    --ep->avail;
    od_sym.length += 1;
    sym->type = type;
    sym->desc = desc;
    if (stabd)
    {
        sym->other = current_section->seg_index;
        sym->val.value = current_offset;
        return sym;
    }
    if (type == N_SO || type == N_SOL)
    {
        OD_line *linp;
        int tmp;
        if ((desc = find_dbg_file(str)) < 0) desc = add_dbg_file(str,"",&tmp);
        sym->name.index = tmp;
        sym->desc = desc;
        if (dbg_flush(0) != 0)
        {
            linp = get_odline(1);
        }
        else
        {
            linp = prev_linp;
        }
        if (type == N_SOL)
        {
            linp->fl.type = LINE_NEW_CFILESOL;
        }
        else
        {
            linp->fl.type = LINE_NEW_CFILESO;
        }
        linp->fl.filenum = desc;
        prev_cline = 0;
        prev_linp = 0;
    }
    else
    {
        if (str != 0)
        {
            ep = od_string.curr;
            if (ep->avail < len)
            {
                ep = get_elem_mem(&od_string,sizeof(char),len,0);
            }
            s = (char *)ep->item;
            sym->name.index = od_string.length;
            strcpy(s,str);
            s += len;
            ep->item = (void *)s;
            ep->avail -= len;
            od_string.length += len;
        }
        else
        {
            sym->name.str_ptr = (uint8_t *)0;
        }
    }
    if ((sym->other = other) != 0)
    {
        if (other == 2)
        {
            sym->other = EXP0SP->expr_seg->seg_index;
            sym->val.value = EXP0SP->expr_value;
        }
        else
        {
            sym->val.ptr = (void *)EXP0SP->expr_sym;
        }
    }
    else
    {
        sym->val.value = EXP0SP->expr_value;
    }
    return sym;
}

static char *tmp_str;
static int tmp_str_size;

int op_stabs( void )      /* .stabs "name",type,0,value */
{
    char *dst;
    int len;
    if (!options[QUAL_DEBUG])
    {
        f1_eatit();
        return 0;
    }
    if (*inp_ptr != '"')
    {
        bad_token((char *)0,"Invalid .stabs directive");
        return 0;
    }
    ++inp_ptr;           /* eat opening '"' */
    tkn_ptr = inp_ptr;
    if (tmp_str_size < inp_str_size)
    {
        if (tmp_str) MEM_free(tmp_str);
        tmp_str = MEM_alloc((tmp_str_size=inp_str_size));
    }
    dst = tmp_str;
    while (*inp_ptr != 0 && *inp_ptr != '"') *dst++ = *inp_ptr++;
    *dst++ = 0;
    if (*inp_ptr != '"')
    {
        bad_token(inp_ptr,"Expected a closing quote here");
        return 0;
    }
    len = dst-tmp_str;
    ++inp_ptr;               /* eat closing quote */
    od_hdr.have_csrc = 1;        /* indicate there are C files */
    op_stabx(tmp_str,len,0);
    return 0;
}

int op_stabn( void )              /* .stabn type,0,0,label */
{                   /* 	(block identifier) */
    if (!options[QUAL_DEBUG])
    {
        f1_eatit();
        return 0;
    }
    op_stabx((char *)0,0,0);     /* same as stabs w/o name string */
    return 0;
}

int op_stabd( void )              /* .stabd 68,0,line_number */
{
    OD_sym *sym;
    if (!options[QUAL_DEBUG])
    {
        f1_eatit();
        return 0;
    }
    sym = op_stabx((char *)0,0,1);
    prev_cline = sym->desc;
    return 0;
}

int dbg_line( int flag)
{
    OD_line *linp;
    int32_t diff;
    diff = current_pc-prev_line_offset;   
    if (prev_linp == 0 || prev_line_segp != current_section ||
        prev_fnd != current_fnd ||
        diff < 0 || diff > 255)
    {
        dbg_flush(0);
        if (prev_fnd != current_fnd)
        {
            int filenum;
            linp = get_odline(1);
            if ((filenum = find_dbg_file(current_fnd->fn_buff)) < 0)
            {
                filenum = add_dbg_file(current_fnd->fn_buff,current_fnd->fn_version,(int *)0);
            }
            linp->fl.type = LINE_NEW_SFILE;
            linp->fl.filenum = filenum;
            prev_fnd = current_fnd;
        }
        linp = get_odline(1);
        linp->bl.type = LINE_NEW_BASE;
        linp->bl.segnum = current_section->seg_index;
        linp->bl.offset = current_pc;
        prev_line_offset = current_pc;
        prev_line_segp = current_section;
    }
    else
    {
        if (diff == 0)
        {
            prev_linp->sl.sline = current_fnd->fn_line;
            prev_linp->sl.cline = prev_cline;
            return 0;      /* no code generated, do nothing */
        }
        prev_linp->sl.length = diff;      
    }
    linp = get_odline(1);
    linp->sl.type = LINE_NUM;
    linp->sl.sline = current_fnd->fn_line;
    linp->sl.cline = prev_cline;
    linp->sl.length = 0;
    prev_linp = linp;
    prev_line_offset = current_pc;
    prev_line_segp = current_section;
    return 0;
}

static int32_t dbg_tot( OD_elem *ep, int size)
{
    int32_t t=0;
    do
    {
        t += size*(ep->alloc-ep->avail);
    } while ((ep=ep->next) != 0);
    return t;
}

static int32_t write_dbg(OD_elem *ep, int size, char *ident )
{
    int32_t accum = 0;
    do
    {
        int qty;
        qty = ep->alloc-ep->avail;
        accum += qty*size;
        if (fwrite((char *)ep->top,size,qty,deb_fp) != qty)
        {
            sprintf(emsg,"Error writing %d \"%s\" elements to debug file",
                    qty,ident);
            perror(emsg);
            return -1;
        }
    } while ((ep=ep->next) != 0);
    return accum;
}

int dbg_output( void )
{
    OD_elem *ep;
    int i;
    ep = od_sec.top;
    for (i=0;i<seg_list_index;++i)
    {
        int flg;
        OD_sec *dbg_segp;
        OD_elem *sep;
        SEG_struct *seg_ptr;
        char *s;
        int len;
        seg_ptr = *(seg_list+i);
        if (ep->avail < 1)
        {
            ep = get_elem_mem(&od_sec,sizeof(OD_sec),1,16);
            ++od_sec.index;
        }
        dbg_segp = (OD_sec *)ep->item;
        ep->item = (void *)(dbg_segp+1);
        ep->avail -= 1;
        od_sec.length += 1;
        len = strlen(seg_ptr->seg_string)+1;
        sep = od_string.curr;
        if (sep->avail < len)
        {
            sep = get_elem_mem(&od_string,sizeof(char),len,0);
        }
        s = (char *)sep->item;
        dbg_segp->name.index = od_string.length;
        dbg_segp->length  = seg_ptr->seg_len;
        flg = VSEG_DEF;       /* segments are defined */
        if (seg_ptr->flg_abs)     flg |= VSEG_ABS;
        if (seg_ptr->flg_zero)    flg |= VSEG_BPAGE;
        if (seg_ptr->flg_ro)  flg |= VSEG_RO;
        if (seg_ptr->flg_noout)   flg |= VSEG_NOOUT;
        if (seg_ptr->flg_ovr) flg |= VSEG_OVR;
        if (seg_ptr->flg_based)   flg |= VSEG_BASED;
        if (seg_ptr->flg_data)    flg |= VSEG_DATA;
        dbg_segp->flags = flg;
        strcpy(s,seg_ptr->seg_string);
        s += len;
        sep->item = (void *)s;
        sep->avail -= len;
        od_string.length += len;
    }
    ep = od_sym.top;
    do
    {
        OD_sym *sym;
        sym = (OD_sym *)ep->top;
        for (i=0;i<ep->alloc-ep->avail;++i,++sym)
        {
            if (sym->other == 1)
            {
                SS_struct *syp;
                syp = (SS_struct *)sym->val.ptr;
                if (!syp->flg_label)
                {
                    fprintf(stderr,"debug: symbol \"%s\" is not a label\n",
                            syp->ss_string);
                    sym->other = 0;
                    sym->val.value = 0;
                }
                else
                {
                    sym->other = syp->ss_seg->seg_index;
                    sym->val.value = syp->ss_value;
                }
            }
        } 
    } while ((ep = ep->next) != 0);    
    od_hdr.magic     = OD_MAGIC;
    od_hdr.sec_size  = dbg_tot(od_sec.top,sizeof(OD_sec));
    od_hdr.file_size = dbg_tot(od_file.top,sizeof(OD_file));
    od_hdr.sym_size  = dbg_tot(od_sym.top,sizeof(OD_sym));
    od_hdr.line_size = dbg_tot(od_line.top,sizeof(OD_line));
    od_hdr.string_size   = dbg_tot(od_string.top,sizeof(char));
    od_hdr.time_stamp    = unix_time;
    i = 0;
    if (fwrite(&od_hdr,sizeof(OD_hdr),1,deb_fp) != 1)
    {
        perror("Error writing hdr section to debug file");
        ++i;
    }
    else if (write_dbg(od_sec.top,sizeof(OD_sec),"OD_sec") != od_hdr.sec_size)
    {
        ++i;
    }
    else if (write_dbg(od_file.top,sizeof(OD_file),"OD_file") != od_hdr.file_size)
    {
        ++i;
    }
    else if (write_dbg(od_sym.top,sizeof(OD_sym),"OD_sym") != od_hdr.sym_size)
    {
        ++i;
    }
    else if (write_dbg(od_line.top,sizeof(OD_line),"OD_line") != od_hdr.line_size)
    {
        ++i;
    }
    else if (write_dbg(od_string.top,sizeof(char),"OD_char") != od_hdr.string_size)
    {
        ++i;
    }
    if (i != 0)
    {
        perror("Error writing debug file");
    }
    return 0;
}
    
