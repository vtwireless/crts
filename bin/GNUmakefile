LIB := ../lib

CPPFLAGS := -I $(LIB) -I ../include -I ../liquid-dsp/src/include

BIN = $(PREFIX)/bin

# There to install files built in this directory
INSTALL_DIR = $(BIN)


BUILD_NO_INSTALL := mkTUN udpTunnelServer testSuidMkTUN mapTest


# It's very important that libcrts_radio link option be after the libuhd
# link option because in libctrs we have a hack that overrides some of the
# libuhd and libboost functionality. The UHD and BOOST libraries do bad
# things that no library should do.  See ../lib/libcrts_radio.cpp for
# details.
COMMON_LD :=\
 -Wl,--export-dynamic\
 -ldl -lpthread


mapTest_SOURCES := mapTest.cpp debug.c makeRingBuffer.cpp
mapTest_LDFLAGS := -lrt

crts_findUSRPs_SOURCES := crts_findUSRPs.cpp
# We add a link/load path (rpath) so that the module can load and find
# libuhd.so, the stupid "pkg-config uhd --libs" did not do this for us.
UHD_LIBS := $(shell pkg-config uhd --libs)\
 -Wl,-rpath=$(shell pkg-config uhd --variable=libdir)
crts_findUSRPs_LDFLAGS := $(UHD_LIBS)

crts_shell_SOURCES := crts_shell.cpp debug.c
crts_shell_LDFLAGS := -lreadline

crts_radio_SOURCES :=\
 crts_radio.cpp\
 stdoutOverride.cpp\
 get_opt.cpp\
 Thread.cpp\
 Stream.cpp\
 timer.cpp\
 Filter.cpp\
 Control.cpp\
 Controller.cpp\
 debug.c\
 makeRingBuffer.cpp\
 Filter_write.cpp\
 Filter_startStop.cpp\
 Feed.cpp\
 Stream_printGraph.cpp\
 Stream_startStop.cpp\
 crts_radio_usage.cpp

crts_radio_LDFLAGS := $(COMMON_LD) -lrt


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
 debug.c\
 $(LIB)/getTunViaUSocket.cpp\
 $(LIB)/tun_alloc.cpp
# This must spew to stderr to be compatible with crts_radio which may run
# this if it creates a TUN.


crts_mkTUN_SOURCES :=\
 crts_mkTUN.cpp\
 debug.c\
 $(LIB)/tun_alloc.cpp\
 $(LIB)/getTunViaUSocket.cpp

# Install crts_contestWebServer and crts_spectrumFeed too.
INSTALLED := crts_contestWebServer crts_spectrumFeed


include ../quickbuild.make
