AC_INIT([cnc], [m4_esyscmd_s([git describe --tags --always | tr -d '\n'])], [charmitor@posteo.net])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_CANONICAL_HOST

# Check for Cargo, the Rust package manager
AC_CHECK_PROG([CARGO], [cargo], [yes], [no])
AS_IF([test "x$CARGO" = "xno"], [AC_MSG_ERROR([Cargo is required to build Rust libraries.])])

# Detect macOS and adjust CFLAGS and LDFLAGS for Homebrew
AS_CASE([$host_os],
  [darwin*], [
    AC_DEFINE([MACOS], [1], [Define on macOS])
    AM_CONDITIONAL([MACOS], [true])
    # Attempt to determine the Homebrew prefix
    BREW_PREFIX=$(brew --prefix 2>/dev/null || echo "/usr/local")
    if test -d "$BREW_PREFIX/include"; then
      CFLAGS="$CFLAGS -I$BREW_PREFIX/include"
    fi
    if test -d "$BREW_PREFIX/lib"; then
      LDFLAGS="$LDFLAGS -L$BREW_PREFIX/lib"
    fi
  ],
  [*], [
    # Default case (for non-darwin platforms) can be left empty or used for other platform-specific settings
    AM_CONDITIONAL([MACOS], [false])
  ]
)

AS_CASE([$host_os],
  [darwin*], [
    # For macOS with Homebrew, adjust for non-standard PostgreSQL installation path
    PGSQL_PATH=$(brew --prefix postgresql@14 2>/dev/null)
    CPPFLAGS="$CPPFLAGS -I$PGSQL_PATH/include/postgresql@14/"
    LDFLAGS="$LDFLAGS -L$PGSQL_PATH/lib/postgresql@14/"
  ]
)

# Checks for libraries and headers
# Check for inih
AC_CHECK_LIB([inih], [ini_parse], [], [
  AC_MSG_ERROR([*** The inih library is required but missing. ***])
])

# Check for PostgreSQL
AC_CHECK_LIB([pq], [PQconnectdb], [], [
  AC_MSG_ERROR([*** libpq is required but missing. ***])
])

# Check for CUnit, needed for tests
AC_CHECK_LIB([cunit], [CU_initialize_registry], [], [
  AC_MSG_ERROR([*** libcunit1 is required but missing. ***])
])

# Checks for header files
AC_CHECK_HEADERS([postgresql/libpq-fe.h])

# Adjusting CFLAGS and LDFLAGS
CFLAGS="$CFLAGS -Iinclude -I/usr/include/postgresql/ -Wall -g -pedantic"

AC_SUBST([COMMIT_HASH], [$(git rev-parse HEAD)])

AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
