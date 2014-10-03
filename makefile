# $Id: makefile,v 1.1.1.1 2001/05/19 23:09:49 root Exp $

PROJ = sdfdump

CC = icc.exe
LINKER = link386.exe
!ifdef DEBUG
DEBUG_COPT = /Fl+ /Ti+
DEBUG_LINKOPT = /DE
!endif
!ifdef LIBC
ADD_COPT = /I$(LIBCPATH) /Re
!else
ADD_COPT = /Se /Gi+
!endif
COPT = /c /G4 $(DEBUG_COPT) $(ADD_COPT) /Gs- /Fo$@ %s
EXE_COPT = $(COPT)
DLL_COPT = /Ge- $(COPT)
!ifdef LIBC
LINKLIB = LIBCS+OS2386
ADD_LINKOPT = /NOD
!else
LINKLIB =
!endif
LINKOPT = $(DEBUG_LINKOPT) $(ADD_LINKOPT) /PMTYPE:VIO
LINKDEF = $(PROJ).DEF

.SUFFIXES: .c .obj .exe

all: $(PROJ).exe

OBJS = $(PROJ).obj
$(PROJ).exe: $(OBJS)
 $(LINKER) $(LINKOPT) $(OBJS),$(@B),NUL,$(LINKLIB),$(LINKDEF);
!if "$(DEBUG)" == ""
 LXLITE /CS /BDX- /I- /ZS $(PROJ).exe
 packexe $(PROJ).exe
!endif

$(PROJ).obj: $(PROJ).c
 $(CC) $(COPT)
