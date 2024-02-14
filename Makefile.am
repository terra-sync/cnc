bin_PROGRAMS = cnc
cnc_SOURCES = src/cnc.c src/config.c src/db.c src/log.c src/main.c src/postgres.c
cnc_LDADD = -linih -lpq $(RUST_LIBS)
cnc_CFLAGS = -Iinclude -I/usr/include/postgresql/ -Wall -g

# Define Rust libraries and their paths
RUST_LIBS = rust/email/target/debug/libemail.so

# Use BUILT_SOURCES to ensure rust_libs is considered a prerequisite
BUILT_SOURCES = $(RUST_LIBS)

# Custom rule for building Rust libraries
$(RUST_LIBS):
	./scripts/build-rust-libs.sh

.PHONY: rust_libs

# Override the clean target to include Rust library cleaning
clean-local:
	@for dir in rust/*; do \
		(cd $$dir && cargo clean); \
	done

check_PROGRAMS = test
test_SOURCES = src/cnc.c src/config.c src/db.c src/log.c src/postgres.c tests/cnc_test.c
test_LDADD = -lcunit -linih -lpq
test_CFLAGS = -Iinclude -I/usr/include/postgresql/ -Wall -g

CLEANFILES = $(bin_PROGRAMS) $(check_PROGRAMS) *.o
