CC?=gcc
CFLAGS=-Iinclude -I/usr/include/postgresql/ -Wall -g
LDFLAGS=-linih -lpq -lcurl
TEST_LDFLAGS=-lcunit -linih -lpq -lcurl
DEPS=$(wildcard include/*.h)
SRC=$(wildcard src/*.c)
OBJ=$(SRC:src/%.c=%.o)
TEST_SRC=$(wildcard tests/*.c)
TEST_OBJ=$(TEST_SRC:tests/%.c=%.o)

%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cnc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: tests/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(filter-out main.o, $(OBJ)) $(TEST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(TEST_LDFLAGS)

clean:
	rm -f *.o cnc test

valgrind: cnc
	valgrind --leak-check=full ./cnc -f configs/test.ini
