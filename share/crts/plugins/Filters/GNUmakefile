# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../../
include $(ROOT)/config.make

QS_CPPFLAGS := $(shell pkg-config --cflags quickscope)
#QS_CPPFLAGS := -I$(shell pkg-config --variable=includedir quickscope)
QS_LDFLAGS := $(shell pkg-config --libs quickscope) -lm


CPPFLAGS := -I$(ROOT)include -I$(ROOT)lib -I$(ROOT)liquid-dsp/src/include

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))


LIQUID_SRC := $(abspath $(ROOT)/liquid-dsp/src)
LIBFEC_SRC := $(abspath $(ROOT)/liquid-dsp/libfec/src)

SRC_RPATH := -Wl,-rpath,$(LIQUID_SRC):$(LIBFEC_SRC)

LIQUID_LDFLAGS := -L$(LIQUID_SRC) -lliquid
LIBFEC_LDFLAGS := -L$(LIBFEC_SRC) -lfec

# uncomment to build quickscope.so
#BUILD_QUICKSCOPE := yes

# Make every *.cpp file into a DSO (dynamic shared object) plugin that is
# installed.

# TODO: This will not work in a build tree that is not the same as the
# source tree.  *.cpp is in the source.  That may explain why GNU automake
# never uses wildcards.

# from: echo *.cpp
#srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))
srcs :=\
 fileIn.cpp\
 fileOut.cpp\
 liquidFrame.cpp\
 liquidSync.cpp\
 passThrough.cpp\
 readline.cpp\
 rx.cpp\
 sin.cpp\
 sleepTest.cpp\
 sourceFeed.cpp\
 stdin.cpp\
 stdout.cpp\
 tx.cpp\
 joystick.cpp\
 sink.cpp
ifdef BUILD_QUICKSCOPE
srcs := $(srcs) quickscope.cpp
endif

srcs := $(patsubst %.cpp,%,$(srcs))

define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


readline.so_LDFLAGS := -lreadline

rx.so_LDFLAGS = -L$(UHD_PREFIX)/lib -luhd -Wl,-rpath,$(UHD_PREFIX)/lib
tx.so_LDFLAGS = -L$(UHD_PREFIX)/lib -luhd -Wl,-rpath,$(UHD_PREFIX)/lib


quickscope.so_CPPFLAGS := $(QS_CPPFLAGS)
quickscope.so_LDFLAGS := $(QS_LDFLAGS)


liquidSync.so_LDFLAGS :=\
 $(LIQUID_LDFLAGS)\
 $(LIBFEC_LDFLAGS)\
 $(SRC_RPATH)

liquidFrame.so_LDFLAGS :=\
 $(LIQUID_LDFLAGS)\
 $(LIBFEC_LDFLAGS)\
 $(SRC_RPATH)



# TODO: make quickbuild.make do this patchelf thing:
#
# After we install the programs we reset the RPATH so that the programs
# can find the installed libliquid and libfec shared libraries at run-time.
INSTALL_RPATH = patchelf --set-rpath '$(PREFIX)/lib'

POST_INSTALL_COMMAND =\
 $(INSTALL_RPATH) $(INSTALL_DIR)/liquidFrame.so &&\
 $(INSTALL_RPATH) $(INSTALL_DIR)/liquidSync.so


# above we auto generate lines like this:
#*.so_SOURCES := *.cpp

include $(ROOT)quickbuild.make
