CC?=gcc
CFLAGS=-Iinclude -I/usr/include/postgresql/ -Wall -g
LDFLAGS=-linih -lpq -lcurl
DEPS=$(wildcard include/*.h)
SRC=$(wildcard src/*.c)
OBJ=$(SRC:src/%.c=%.o)

%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cnc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o cnc

valgrind: cnc
	valgrind --leak-check=full ./cnc -f test.ini
