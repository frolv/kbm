SHELL=/bin/sh

PROGRAM=kbm

CC=gcc
RM=rm -f
CP=cp
MKDIR=mkdir -p
WINDRES=
IBTOOL=

CFLAGS=-Wall -Wextra -g -DKBM_DEBUG
LDFLAGS=
RESFLAGS=
IBFLAGS=

SRCDIR=src
RESDIR=misc

_SRC=main.c display.c keymap.c hotkey.c parser.c
SRC=$(patsubst %,$(SRCDIR)/%,$(_SRC))
_OBJC=application.m delegate.m
OBJC=$(patsubst %,$(SRCDIR)/%,$(_OBJC))
_HEAD=kbm.h display.h keymap.h hotkey.h parser.h
HEAD=$(patsubst %,$(SRCDIR)/%,$(_HEAD))
OBJ=$(SRC:.c=.o)
NIB=

APP=
APPCLEAN=

UNAME=$(shell uname -s)

ifeq ($(UNAME),Linux)
	CFLAGS+=$(shell pkg-config --cflags libnotify)
	LDFLAGS+=-lxcb -lxcb-keysyms -lxcb-util -lxcb-xtest \
		 $(shell pkg-config --libs libnotify)
endif
ifeq ($(UNAME),Darwin)
	LDFLAGS+=-framework AppKit -framework ApplicationServices \
		 -framework Foundation
	OBJ+=$(subst .m,.o,$(OBJC))
	HEAD+=$(SRCDIR)/delegate.h $(SRCDIR)/application.h
	NIB=$(RESDIR)/MainMenu.nib
	APP=createapp
	APPCLEAN=cleanapp
	IBTOOL=ibtool
	IBFLAGS=--compile
endif
ifneq (,$(findstring _NT-,$(UNAME)))
	WINDRES=windres
	RESFLAGS=-O coff
	LDFLAGS+=-mwindows -mconsole
	OBJ+=$(RESDIR)/$(PROGRAM).res
endif

.PHONY: all
all: $(PROGRAM) $(APP)

$(PROGRAM): $(OBJ) $(HEAD) $(NIB)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(SRCDIR)/%.o: $(SRCDIR)/%.m
	$(CC) $(CFLAGS) -c -o $@ $^

$(RESDIR)/$(PROGRAM).res: $(RESDIR)/$(PROGRAM).rc
	$(WINDRES) $^ -o $@ $(RESFLAGS)

$(RESDIR)/MainMenu.nib: $(RESDIR)/MainMenu.xib
	$(IBTOOL) $^ $(IBFLAGS) $@

$(APP): $(PROGRAM)
	$(MKDIR) $(PROGRAM).app/Contents/{MacOS,Resources}
	$(CP) $(RESDIR)/Info.plist $(PROGRAM).app/Contents
	$(CP) $(PROGRAM) $(PROGRAM).app/Contents/MacOS
	$(CP) $(RESDIR)/MainMenu.nib $(PROGRAM).app/Contents/Resources
	$(CP) $(RESDIR)/$(PROGRAM).png $(PROGRAM).app/Contents/Resources

.PHONY: clean $(APPCLEAN)
clean: $(APPCLEAN)
	$(RM) $(SRCDIR)/*.o $(PROGRAM)

$(APPCLEAN):
	$(RM) -r $(PROGRAM).app
