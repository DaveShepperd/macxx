/*
    pstas.c - Part of macxx, a cross assembler family for various micro-processors
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
#include "pst_tokens.h"
#define MAC_AS

#define DIRDEF(name,func,flags) extern int func();
#include "dirdefs.h"
#undef DIRDEF

#define OPCDEF(name,namec,class,val) {name,(val << 11),OP_CLASS_OPC+class,0},\
{namec,(val << 11)|0x20,OP_CLASS_OPC+class,0},
#define BR_DEF(name,namec,class,val) {name,((val+0x20)<<6),OP_CLASS_OPC+class,0},
#define LS_DEF(name,namec,class,val,sz) {name,(val<<11)|sz,OP_CLASS_OPC+class,0},\
{namec,(val<<11)|sz|0x20,OP_CLASS_OPC+class,0},
#define NOC_DEF(name,namec,class,val) {name,(val << 11),OP_CLASS_OPC+class,0},

Opcpst perm_opcpst[] = {
#include "asap_ops.h"
   	{ 0,0,0,0 }
};

#define DIRDEF(name,func,flags) {name,func,flags},

Dirpst perm_dirpst[] = {
#include "dirdefs.h"
   	{ 0,0,0 }
};
