#!/bin/sh

# Stop on errors
set -e

# Generate aclocal.m4, incorporating macros from installed packages
echo "Running aclocal..." 
aclocal


# Generate the configure script
echo "Running autoconf..."
autoconf

# Add missing files like install-sh, missing, etc., and generate Makefile.in
echo "Running automake..."
automake --add-missing --copy

# Optionally, if using libtool, you would also run libtoolize or glibtoolize here
# echo "Running libtoolize..."
# libtoolize --force

echo "Configuration files have been successfully generated."
echo "You can now run ./configure and then make."

