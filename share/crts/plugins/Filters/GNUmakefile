# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../..
-include $(ROOT)/config.make

QS_CPPFLAGS := $(shell pkg-config --cflags quickscope)
#QS_CPPFLAGS := -I$(shell pkg-config --variable=includedir quickscope)
QS_LDFLAGS := $(shell pkg-config --libs quickscope) -lm

UHD_CFLAGS := $(shell pkg-config uhd --cflags)

# We add a link/load path (rpath) so that the module can load and find
# libuhd.so, the stupid "pkg-config uhd --libs" did not do this for us.
UHD_LIBS := $(shell pkg-config uhd --libs)\
 -Wl,-rpath=$(shell pkg-config uhd --variable=libdir)


CPPFLAGS := -I$(ROOT)/include -I$(ROOT)/lib -I$(ROOT)/liquid-dsp/src/include

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))



LIQUID_LDFLAGS := -L$(ROOT)/lib -lliquid -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib

# uncomment to build quickscope.so
#BUILD_QUICKSCOPE := yes

# Make every *.cpp file into a DSO (dynamic shared object) plugin that is
# installed.



srcs :=\
 stdin.cpp\
 stdout.cpp\
 passThrough.cpp\
 copy.cpp\
 tx.cpp\
 rx.cpp\
 throttle.cpp\
 liquidFrame.cpp\
 liquidSync.cpp\
 liquidFMDemod.cpp\
 joystick.cpp\
 sink.cpp\
 fileIn.cpp\
 TUN.cpp



srcs := $(patsubst %.cpp,%,$(srcs))

define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


readline.so_LDFLAGS := -lreadline

UHD_PREFIX = $(shell pkg-config uhd --variable=prefix)

rx.so_CPPFLAGS := $(UHD_CFLAGS)
tx.so_CPPFLAGS := $(UHD_CFLAGS)

rx.so_LDFLAGS := $(UHD_LIBS)
tx.so_LDFLAGS := $(UHD_LIBS)



quickscope.so_CPPFLAGS := $(QS_CPPFLAGS)
quickscope.so_LDFLAGS := $(QS_LDFLAGS)


liquidSync.so_LDFLAGS := $(LIQUID_LDFLAGS)

liquidFrame.so_LDFLAGS := $(LIQUID_LDFLAGS)

liquidFMDemod.so_LDFLAGS := $(LIQUID_LDFLAGS)


# above we auto generate lines like this:
#*.so_SOURCES := *.cpp

include $(ROOT)/quickbuild.make
