SHELL=/bin/sh

PROGRAM=kbm

CC=gcc
CFLAGS=$(INC) -Wall -Wextra -std=c99

SRCDIR=src
_SRC=main.c display.c
SRC=$(patsubst %,$(SRCDIR)/%,$(_SRC))
_HEAD=kbm.h display.h
HEAD=$(patsubst %,$(SRCDIR)/%,$(_HEAD))
OBJ=$(SRC:.c=.o)

ifeq ($(shell uname -s),Linux)
	LDFLAGS+=-lX11
endif

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJ) $(HEAD)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

.PHONY: clean
clean:
	rm -f $(SRCDIR)/*.o $(PROGRAM)
