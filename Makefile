CC?=gcc
DEBUG ?= 0
CFLAGS=-Iinclude -Wall -g -DDEBUG=$(DEBUG)
LDFLAGS=-lcyaml
DEPS=$(wildcard include/*.h)
SRC=$(wildcard src/*.c)
OBJ=$(SRC:src/%.c=%.o)

%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

cnc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o cnc

valgrind: cnc
	valgrind --leak-check=full ./cnc -f test.yaml
