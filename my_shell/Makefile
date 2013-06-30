CFLAGS=-g

PROG=my_Shell
OBJPROG=my_shell.o shell_rutinas.o builtins.o entorno_lib.o

all: $(PROG)

$(PROG): $(OBJPROG)
	gcc -g -o $@ $^

clean:
	rm $(PROG) *~ *.o
