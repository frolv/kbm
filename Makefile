SHELL=/bin/sh

PROGRAM=kbm

CC=gcc
CFLAGS=-Wall -Wextra -g -DKBM_DEBUG

SRCDIR=src
_SRC=main.c display.c keymap.c hotkey.c parser.c
SRC=$(patsubst %,$(SRCDIR)/%,$(_SRC))
_HEAD=kbm.h display.h keymap.h hotkey.h parser.h
HEAD=$(patsubst %,$(SRCDIR)/%,$(_HEAD))
OBJ=$(SRC:.c=.o)

ifeq ($(shell uname -s),Linux)
	LDFLAGS+=-lxcb -lxcb-keysyms -lxcb-util -lxcb-xtest
endif
ifeq ($(shell uname -s),Darwin)
	LDFLAGS+=-framework ApplicationServices
endif

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJ) $(HEAD)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

.PHONY: clean
clean:
	rm -f $(SRCDIR)/*.o $(PROGRAM)
