LIB := ../lib

CPPFLAGS := -I $(LIB) -I ../include -I ../liquid-dsp/src/include

BIN = $(PREFIX)/bin

# There to install files built in this directory
INSTALL_DIR = $(BIN)


LIBCRTS_SRC := $(abspath $(CURDIR)/../lib)

SRC_RPATH := -Wl,-rpath,$(LIBCRTS_SRC)

LIBCRTS_LDFLAGS := -L$(LIBCRTS_SRC) -lcrts_radio


BUILD_NO_INSTALL := test_link mkTUN udpTunnelServer testSuidMkTUN


# It's very important that libcrts_radio link option be after the libuhd
# link option because in libctrs we have a hack that overrides some of the
# libuhd and libboost functionality. The UHD and BOOST libraries do bad
# things that no library should do.  See ../lib/libcrts_radio.cpp for
# details.
COMMON_LD :=\
 $(SRC_RPATH)\
 $(LIBCRTS_LDFLAGS)\
 -Wl,--export-dynamic\
 -ldl -lpthread


# Notice running 'ldd ./test_link' spew lot-o-libs and running
# './test_link' spews UHD stuff; even though it does not use any of the
# APIs that it links with.  This is an important test.
test_link_SOURCES := test_link.cpp
test_link_LDFLAGS = $(COMMON_LD)

crts_radio_SOURCES :=\
 crts_radio.cpp\
 stdoutOverride.cpp\
 get_opt.cpp\
 Thread.cpp\
 Stream.cpp\
 timer.cpp\
 Filter.cpp\
 Control.cpp\
 Controller.cpp
# stdout may be needed for user pipe line or whatever.
crts_radio_LDFLAGS := $(COMMON_LD)


# The program crts_mkTUN need a special installation so it can startup as
# effective user root and create a TUN device and route it and then it
# goes back to being a regular user.  The top source directory GNUmakefile
# has a target "install_root" that does this to the installed version of
# crts_mkTUN.  This is only needed for testing.
sudo_build: crts_mkTUN
	sudo chown root:root crts_mkTUN
	sudo chmod u+s crts_mkTUN


mkTUN_SOURCES :=\
 mkTUN.cpp\
 $(LIB)/debug.c\
 $(LIB)/getTunViaUSocket.cpp\
 $(LIB)/tun_alloc.cpp
# This must spew to stderr to be compatible with crts_radio which may run
# this if it creates a TUN.


crts_mkTUN_SOURCES :=\
 crts_mkTUN.cpp\
 $(LIB)/debug.c\
 $(LIB)/tun_alloc.cpp\
 $(LIB)/getTunViaUSocket.cpp


# TODO: make quickbuild.make do this patchelf thing:
#
# After we install the programs we reset the RPATH so that the programs
# can find the installed libliquid and libfec shared libraries at run-time.
INSTALL_RPATH = patchelf --set-rpath '$(PREFIX)/lib'

POST_INSTALL_COMMAND =\
 $(INSTALL_RPATH) $(BIN)/crts_radio


include ../quickbuild.make
