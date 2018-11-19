# This directory just contains some shared C/C++ files that may
# be accessed from ../bin or ../share/ and so on.

INSTALL_DIR = $(PREFIX)/lib

INSTALLED := node_modules

libcrts.so_SOURCES := crts.cpp

libcrts.so_LDFLAGS := -ljansson

CPPFLAGS := -I../include


include ../quickbuild.make
