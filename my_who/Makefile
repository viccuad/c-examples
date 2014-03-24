CC = gcc
CFLAGS = -Wall -g
# flags -Wall warning all, -g debug
OBJETOS = my_who1.o my_who2.o my_who3.o

PROGS = my_who1 my_who2 my_who3

all: $(PROGS)

my_who1: my_who1.o
	$(CC) -g -o $@ $<

my_who2: my_who2.o
	$(CC) -g -o $@ $<

my_who3: my_who3.o
	$(CC) -g -o $@ $<

#$(OBJETOS): my_who.h     NO USAMOS my_who.h

%.o : %.c
	gcc -c $(CFLAGS) $<

clean :
	-rm $(OBJETOS) $(PROGS)
