SHELL=/bin/sh

PROGRAM=kbm

CC=gcc
CFLAGS=$(INC) -Wall -Wextra -std=c99

SRCDIR=src
_SRC=main.c display.c keymap.c hotkey.c
SRC=$(patsubst %,$(SRCDIR)/%,$(_SRC))
_OC_SRC=event.m
OC_SRC=$(patsubst %,$(SRCDIR)/%,$(_OC_SRC))
_HEAD=kbm.h display.h keymap.h hotkey.h
HEAD=$(patsubst %,$(SRCDIR)/%,$(_HEAD))
OBJ=$(SRC:.c=.o)

ifeq ($(shell uname -s),Linux)
	LDFLAGS+=-lxcb -lxcb-keysyms -lxcb-util
endif
ifeq ($(shell uname -s),Darwin)
	LDFLAGS+=-framework AppKit
	OBJ+=$(OC_SRC:.m=.o)
endif

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJ) $(HEAD)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

.PHONY: clean
clean:
	rm -f $(SRCDIR)/*.o $(PROGRAM)
