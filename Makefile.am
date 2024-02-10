bin_PROGRAMS = cnc
cnc_SOURCES = src/cnc.c src/config.c src/db.c src/email.c src/log.c src/main.c src/postgres.c
cnc_LDADD = -linih -lpq -lcurl
cnc_CFLAGS = -Iinclude -I/usr/include/postgresql/ -Wall -g

check_PROGRAMS = test
test_SOURCES = src/cnc.c src/config.c src/db.c src/email.c src/log.c src/postgres.c tests/cnc_test.c
test_LDADD = -lcunit -linih -lpq -lcurl
test_CFLAGS = -Iinclude -I/usr/include/postgresql/ -Wall -g

CLEANFILES = $(bin_PROGRAMS) $(check_PROGRAMS) *.o

