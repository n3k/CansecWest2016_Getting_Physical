IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)


_DEPS = kernetix.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: client.c
	gcc -o client client.c -lreadline $(CFLAGS)

