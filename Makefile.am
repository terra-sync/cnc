bin_PROGRAMS = cnc
cnc_SOURCES = src/postgres.c src/cnc.c src/config.c src/db.c src/log.c src/main.c src/util.c src/email.c
cnc_LDADD = -linih -lpq -lcurl
cnc_CFLAGS = -Iinclude -I/usr/include/postgresql/ -Wall -g -DCOMMIT_HASH=\"@COMMIT_HASH@\"

check_PROGRAMS = test
test_SOURCES = src/cnc.c src/config.c src/db.c src/log.c src/postgres.c tests/cnc_test.c src/util.c
test_LDADD = -lcunit -linih -lpq -lcurl
test_CFLAGS = -Iinclude -I/usr/include/postgresql/ -Wall -Wno-return-type -Wno-alloc-size-larger-than -g -DCOMMIT_HASH=\"@COMMIT_HASH@\"

CLEANFILES = $(bin_PROGRAMS) $(check_PROGRAMS) *.o
