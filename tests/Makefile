# $Id: $(MAKEFILE),v 1.5 2008/09/19 22:06:47 shepperd Exp $

#    Makefile - rules for running various macxx tests.
#    Copyright (C) 2025 David Shepperd
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

HERE = ../
EXE =
ifeq ($(MINGW),1)
	EXE = .exe
	HERE = ..\\
endif
ifeq ($(MSYS2),1)
	EXE = .exe
endif

MAC65 = $(HERE)mac65
MAC68 = $(HERE)mac68
MAC69 = $(HERE)mac69
MAC68K = $(HERE)mac68k
MAC11 = $(HERE)mac11
MACAS = $(HERE)macas
MAC8080 = $(HERE)mac8080
MACZ80 = $(HERE)macz80

.SILENT:

ifeq ($(MINGW),1)
default:
	echo "Sorry, this won't work on Windows under Mingw. Use MSYS2 instead"
else
default: chk65 chk11 chkz80 chk8080 chk68 chk69 chkas chk68k 
endif

define chkIt
	$(1) $(2) -oltest -out $(3); \
	if [ $$? -ne 0 ]; then\
		exit 1;\
	fi;\
	cmp $(3).ol masters/$(3).gol;\
	if [ $$? -ne 0 ]; then \
		echo "*** $(3).ol FAILED $(2) ***";\
	else \
		echo "$(3).ol Passed $(2)";\
	fi;\
	$(1) $(2) -obtest -out $(3); \
	if [ $$? -ne 0 ]; then\
		exit 1;\
	fi;\
	cmp $(3).ob masters/$(3).gob;\
	if [ $$? -ne 0 ]; then \
		echo "*** $(3).ob FAILED $(2) ***";\
	else \
		echo "$(3).ob Passed $(2)";\
	fi;\
	$(1) $(2) -hexout -out $(3); \
	if [ $$? -ne 0 ]; then\
		exit 1;\
	fi;\
	cmp $(3).hex masters/$(3).ghex;\
	if [ $$? -ne 0 ]; then \
		echo "*** $(3).hex FAILED $(2) ***";\
	else \
		echo "$(3).hex Passed $(2)";\
	fi;\
	rm -f $(3).hex $(3).ol $(3).ob
endef

chkas: asap.mac Makefile
	$(call chkIt,$(MACAS),,asap)
	$(call chkIt,$(MACAS),-2,asap)

chk65: 6502.mac Makefile
	$(call chkIt,$(MAC65),,6502)
	$(call chkIt,$(MAC65),-2,6502)

chk68: 6800.mac Makefile
	$(call chkIt,$(MAC68),,6800)
	$(call chkIt,$(MAC68),-2,6800)

chk68k: 68k.mac Makefile
	$(call chkIt,$(MAC68K),,68k)

chk69: 6809.mac Makefile
	$(call chkIt,$(MAC69),,6809)
	$(call chkIt,$(MAC69),-2,6809)

chk11: pdp11.mac Makefile
	$(call chkIt,$(MAC11),,pdp11)
	$(call chkIt,$(MAC11),-2,pdp11)

chkz80: z80.mac Makefile
	$(call chkIt,$(MACZ80),,z80)
	$(call chkIt,$(MACZ80),-2,z80)

chk8080: 8080.mac Makefile
	$(call chkIt,$(MAC8080),,8080)
	$(call chkIt,$(MAC8080),-2,8080)



