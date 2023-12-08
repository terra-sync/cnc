CC?=gcc
DEBUG ?= 0
CFLAGS=-Iinclude -I/usr/local/pgsql/include -Wall -g -DDEBUG=$(DEBUG)
LDFLAGS=-lcyaml -lpq -L/usr/local/pgsql/lib
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
