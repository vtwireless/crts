
# There to install files built in this directory
INSTALL_DIR = $(PREFIX)/lib

# TODO: We are now running crts_radio without linking it to libuhd until
# an RX or TX module is loaded after we enter main() so this bug fix using
# a libcrts_radio.so preloaded library is no longer needed.
#
# The libcrts_radio.so is not for CRTS users.  It's so that the program
# crts_radio may override bad behavior in libuhd.so.  See file README and
# libcrts_radio.c.

libcrts_radio.so_SOURCES := debug.c
libcrts_radio.so_CPPFLAGS := -DSPEW_FILE=stderr -I ../include


include ../quickbuild.make
