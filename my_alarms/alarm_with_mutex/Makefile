CC = gcc
CFLAGS=-Wall -g
LFLAGS=-g -lpthread

PROG=Alarm_with_mutex
OBJPROG=alarma_mutex.o alarma_thread.o alarma_inserta.o

# Para depurar usar: make debug=1
ifeq ($(debug),1)
 CFLAGS += -DDEBUG
endif

all: $(PROG)

$(PROG): $(OBJPROG)
	$(CC) $(LFLAGS) -o $@ $^

clean:
	-rm -f $(PROG) *~ *.o
