SHELL=/bin/sh

PROGRAM=kbm

CC=gcc
CFLAGS=$(INC) -Wall -Wextra -std=c99

SRCDIR=src
_SRC=main.c
SRC=$(patsubst %,$(SRCDIR)/%,$(_SRC))
OBJ=$(SRC:.c=.o)

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(SRCDIR)/*.o $(PROGRAM)
