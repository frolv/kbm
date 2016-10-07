SHELL=/bin/sh

PROGRAM=kbm

CC=gcc
RM=rm -f
WINDRES=

CFLAGS=-Wall -Wextra -g -DKBM_DEBUG
LDFLAGS=
RESFLAGS=

SRCDIR=src
RESDIR=misc

_SRC=main.c display.c keymap.c hotkey.c parser.c
SRC=$(patsubst %,$(SRCDIR)/%,$(_SRC))
_OBJC=delegate.m
OBJC=$(patsubst %,$(SRCDIR)/%,$(_OBJC))
_HEAD=kbm.h display.h keymap.h hotkey.h parser.h
HEAD=$(patsubst %,$(SRCDIR)/%,$(_HEAD))
OBJ=$(SRC:.c=.o)

UNAME=$(shell uname -s)

ifeq ($(UNAME),Linux)
	CFLAGS+=$(shell pkg-config --cflags libnotify)
	LDFLAGS+=-lxcb -lxcb-keysyms -lxcb-util -lxcb-xtest \
		 $(shell pkg-config --libs libnotify)
endif
ifeq ($(UNAME),Darwin)
	LDFLAGS+=-framework AppKit -framework ApplicationServices \
		 -framework Foundation
	LDFLAGS+= -sectcreate __TEXT __info_plist $(RESDIR)/Info.plist
	OBJ+=$(SRCDIR)/delegate.o
	HEAD+=$(SRCDIR)/delegate.h
endif
ifneq (,$(findstring _NT-,$(UNAME)))
	WINDRES=windres
	RESFLAGS=-O coff
	LDFLAGS+=-mwindows -mconsole
	OBJ+=$(RESDIR)/$(PROGRAM).res
endif

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJ) $(HEAD)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(SRCDIR)/%.o: $(SRCDIR)/%.m
	$(CC) $(CFLAGS) -c -o $@ $^

$(RESDIR)/$(PROGRAM).res: $(RESDIR)/$(PROGRAM).rc
	$(WINDRES) $^ -o $@ $(RESFLAGS)

.PHONY: clean
clean:
	$(RM) $(SRCDIR)/*.o $(PROGRAM)
