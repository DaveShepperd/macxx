# $Id: Makefile,v 1.5 2008/09/19 22:06:47 shepperd Exp $

#    Makefile - build rules for making macxx, a cross assembler family for various micro-processors
#    Copyright (C) 2008 David Shepperd
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#

COREO = symbol.o hashit.o memmgt.o dumdir.o dummy.o opcnds.o err2str.o add_defs.o utils.o

ALLO0 = macxx.o gc_table.o gc.o opcode.o
ALLO1 = endlin.o outx.o pass2.o macros.o psuedo_ops.o
ALLO2 = listctrl.o sortsym.o stack_ops.o qksort.o
ALLO3 = $(COREO)

ALLO65 = exprs65.o char_tbl65.o opc65.o pass0.o pass1.o pst65.o debug.o help.o
ALLO68 = exprs68.o char_tbl65.o opc68.o pass0.o pass1.o pst68.o debug.o help.o
ALLOAS = exprs.o char_table.o opcas.o pass0.o pass1.o pstas.o debug.o help.o
ALLOTJ = exprs.o char_table.o opctj.o pass0.o pass1.o psttj.o debug.o help.o
ALLO68K = exprs68k.o char_table.o opc68k.o pass068k.o pass168k.o pst68k.o m68types.o debug.o help.o
ALLO682K = exprs682k.o char_table.o opc682k.o pass068k.o pass168k.o pst682k.o m682types.o debug.o help.o
ALLO69 = stack_ops.o exprs69.o char_tbl65.o opc69.o pass0.o pass1.o pst69.o debug.o help.o
ALLO11 = exprs.o char_table.o opc11.o pass0.o pass1.o pst11.o m11types.o debug.o help.o

ALLOPP0 = macxx_pp.o pass1_pp.o lstctlpp.o opcode_pp.o
ALLOPP1 = gc_tblpp.o gc_pp.o exprs_pp.o macros_pp.o
ALLOPP2 = psdo_ops_pp.o pstpp.o opcpp.o char_table.o
ALLOPP3 = help_pp.o $(COREO)

ALLH = structs.h header.h ct.h token_defs.h token.h 

MODE = -m32
OPT = #-O -DNDEBUG
DBG = -g 

# For Generic Unix/x86
DEFINES = -DEXTERNAL_PACKED_STRUCTS -DM_UNIX -DINCLUDE_FLOAT -DNO_XREF -DGCC -DHAS_STRERROR -D_ISOC99_SOURCE #-DMAX_LINE_ERRORS=12 #-DDEBUG_TXT_INP
CHKS = -Wall -ansi -pedantic -Wno-char-subscripts #-std=c99
CFLAGS = $(OPT) $(DBG) $(DEFINES) $(MODE) -I. $(CHKS)
CC = gcc
L = $(CC) $(MODE)
C = $(CC) -c $(CFLAGS) 
ECHO = /bin/echo -e

.SILENT:

%.o : %.c
	$(ECHO) "\tCompiling $<...";\
	rm -f $(basename $@).err $(basename $@).warn;\
	$C -o $@ $< > $(basename $@).err 2>&1;\
	if [ $$? -ne 0 ]; then\
	    $(ECHO) "Errors in $(basename $@).err";\
	    cat $(basename $@).err;\
	else\
	    if [ -s $(basename $@).err ]; then\
	        mv $(basename $@).err $(basename $@).warn;\
	        $(ECHO) "Warnings in $(basename $@).warn";\
	        cat $(basename $@).warn;\
            else\
	        rm -f $(basename $@).err;\
	    fi;\
	fi
	    

%.E : %.c
	$(ECHO) "\tCompiling $< to $@ ..."
	$(CC) $(CFLAGS) -E $< > $@

all : macas mac65 mac68 mac68k mac682k mac69 macpp mactj mac11
	echo Done

define link_it
	$(ECHO) "\tlinking $@..."
	$L $(DBG) -o $@ $(filter-out Makefile,$^)
endef

macpp : Makefile $(ALLOPP0) $(ALLOPP1) $(ALLOPP2) $(ALLOPP3) 
	$(link_it)

mactj : Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLOTJ) 
	$(link_it)

mac65 : Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLO65) 
	$(link_it)

mac68:  Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLO68)
	$(link_it)

mac68k : Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLO68K) 
	$(link_it)

mac682k : Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLO682K) 
	$(link_it)

mac69:  Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLO69)
	$(link_it)

macas : Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLOAS) 
	$(link_it)

mac11 : Makefile $(ALLO0) $(ALLO1) $(ALLO2) $(ALLO3) $(ALLO11)
	$(link_it)
	
dmpod : dmpod.o
	$(link_it)

gc_pp.o : gc.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h gc_struct.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

gc_tblpp.o : gc_table.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h \
  ct.h qual_tbl.h gc_struct.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

help_pp.o : help.c add_defs.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

lstctlpp.o : listctrl.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h listctrl.h lstcnsts.h exproper.h
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

macxx_pp.o : macxx.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h \
  ct.h qual_tbl.h version.h strsub.h
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

macros_pp.o : macros.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

memmgt.o : memmgt.c 
	$(ECHO) "\tCompiling $<..."
	$C -DMACXX -o $@ $<

opcode_pp.o : opcode.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

pass1_pp.o : pass1.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h listctrl.h lstcnsts.h strsub.h exproper.h
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

psdo_ops_pp.o : psuedo_ops.c add_defs.h token.h token_defs.h structs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h strsub.h \
  opcnds.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

exprs65.o : exprs.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  exproper.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_65 -o $@ $<

exprs68.o : exprs.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  exproper.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_68 -o $@ $<

exprs68k.o : exprs.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  exproper.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC68K -o $@ $<

exprs682k.o : exprs.c token.h token_defs.h m682k.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h exproper.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC682K -o $@ $<

exprs69.o : exprs.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  exproper.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_69 -o $@ $<

exprs_pp.o : exprs.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  exproper.h listctrl.h lstcnsts.h 
	$(ECHO) "\tCompiling $<..."
	$C -DMAC_PP -o $@ $<

pass068k.o : pass0.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h listctrl.h lstcnsts.h strsub.h exproper.h
	$(ECHO) "\tCompiling $<..."
	$C -DMAC68K -o $@ $<

pass168k.o : pass1.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h listctrl.h lstcnsts.h strsub.h exproper.h
	$(ECHO) "\tCompiling $<..."
	$C -DMAC68K -o $@ $<

.PHONY: clean

clean : 
	/bin/rm -f mac65 mac68 macas mac68k macpp mac682k mactj mac11 *.o *.b *@ *.d

add_defs.o : add_defs.c add_defs.h
char_table.o : char_table.c ct.h 
char_tbl65.o : char_tbl65.c ct.h 
debug.o : debug.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  debug.h stab.h stab.def segdef.h 
dmpgdb.o : dmpgdb.c symseg.h 
dmpod.o : dmpod.c debug.h stab.h stab.def segdef.h 
dumdir.o : dumdir.c 
dummy.o : dummy.c 
edits.o : edits.c 
endlin.o : endlin.c header.h outx.h token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  listctrl.h lstcnsts.h exproper.h vlda_structs.h pragma.h pragma1.h segdef.h
err2str.o : err2str.c  
exprs.o : exprs.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  exproper.h listctrl.h lstcnsts.h 
gc.o : gc.c  token.h token_defs.h header.h memmgt.h ct.h qual_tbl.h gc_struct.h 
gc_table.o : gc_table.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h gc_struct.h 
gdbtst.o : gdbtst.c 
gototst.o : gototst.c 
hashit.o : hashit.c 
help.o : help.c add_defs.h 
listctrl.o : listctrl.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h listctrl.h lstcnsts.h utils.h
m11types.o : m11types.c /usr/include/stdio.h token.h \
  token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h pdp11.h exproper.h listctrl.h lstcnsts.h pst_tokens.h pst_structs.h 
m682types.o : m682types.c token.h \
  token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h m682k.h exproper.h listctrl.h lstcnsts.h pst_tokens.h pst_structs.h 
m68types.o : m68types.c token.h token_defs.h \
  structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  m68k.h exproper.h listctrl.h lstcnsts.h pst_tokens.h pst_structs.h 
macros.o : macros.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h listctrl.h lstcnsts.h utils.h
macxx.o : macxx.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h \
  ct.h qual_tbl.h version.h  strsub.h listctrl.h
opc11.o : opc11.c token.h token_defs.h pst_tokens.h pst_structs.h exproper.h \
  structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h pdp11.h opcommon.h 
opc65.o : opc65.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h psttkn65.h opcommon.h 
opc68.o : opc68.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h psttkn68.h opcommon.h 
opc682k.o : opc682k.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h m682k.h opcommon.h 
opc68k.o : opc68k.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h m68k.h opcommon.h 
opc69.o : opc69.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h psttkn69.h opcommon.h
opcas.o : opcas.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h op_class.h opcommon.h 
opcnds.o : opcnds.c opcnds.h 
opcode.o : opcode.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h 
opcpp.o : opcpp.c lstcnsts.h pst_tokens.h pst_structs.h opcommon.h memmgt.h 
opctj.o : opctj.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h tjop_class.h opcommon.h 
outx.o : outx.c outx.h token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  vlda_structs.h pragma1.h segdef.h \
  pragma.h exproper.h 
pass1.o : pass1.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h listctrl.h lstcnsts.h strsub.h exproper.h
pass2.o : pass2.c outx.h token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  vlda_structs.h pragma1.h segdef.h pragma.h vlda_structs.h 
pst11.o : pst11.c pst_tokens.h pst_structs.h pdp11.h opcs11.h dirdefs.h 
pst65.o : pst65.c pst_tokens.h pst_structs.h psttkn65.h dirdefs.h 
pst68.o : pst68.c pst_tokens.h pst_structs.h psttkn68.h dirdefs.h 
pst682k.o : pst682k.c pst_tokens.h pst_structs.h dirdefs.h opcs682k.h 
pst68k.o : pst68k.c pst_tokens.h pst_structs.h dirdefs.h opcs68k.h 
pst69.o : pst69.c pst_tokens.h pst_structs.h psttkn69.h dirdefs.h
pst816.o : pst816.c pst_tokens.h pst_structs.h psttkn65.h dirdefs.h 
pstas.o : pstas.c pst_tokens.h pst_structs.h dirdefs.h asap_ops.h op_class.h 
pstpp.o : pstpp.c pst_tokens.h pst_structs.h dirdefs.h 
psttj.o : psttj.c pst_tokens.h pst_structs.h dirdefs.h tjop_class.h tj_ops.h 
psuedo_ops.o : psuedo_ops.c add_defs.h token.h token_defs.h structs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h strsub.h opcnds.h utils.h
qksort.o : qksort.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h 
sortsym.o : sortsym.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  listctrl.h lstcnsts.h 
stack_ops.o : stack_ops.c add_defs.h token.h token_defs.h structs.h header.h memmgt.h ct.h \
  qual_tbl.h pst_tokens.h pst_structs.h exproper.h listctrl.h lstcnsts.h strsub.h opcnds.h 
strerr.o : strerr.c /usr/include/stdio.h 
symbol.o : symbol.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h 
test.o : test.c token.h token_defs.h structs.h add_defs.h header.h memmgt.h ct.h qual_tbl.h \
  pst_tokens.h pst_structs.h listctrl.h lstcnsts.h 
tjtst.o : tjtst.c tjop_class.h tj_ops.h 
types.o : types.c stab.def
utils.o : utils.c utils.h
