# This makefile can be used to build any of the modules
# that are at this directory level

ROOT := ../../../../..
-include $(ROOT)/config.make


UHD_CFLAGS := $(shell pkg-config uhd --cflags)
UHD_LIBS := $(shell pkg-config uhd --libs)

CPPFLAGS := -I$(ROOT)/include -I$(ROOT)/lib -I$(ROOT)/liquid-dsp/src/include

INSTALL_DIR = $(PREFIX)/share/crts/plugins/Filters/devel

RPATH := -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib

LIQUID_LDFLAGS := -L$(ROOT)/lib -lliquid 
LIBFEC_LDFLAGS := -L$(ROOT)/lib -lfec


# from: echo *.cpp
srcs :=

srcs := $(patsubst %.cpp,%,$(srcs))

define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


tx.so_CPPFLAGS := $(UHD_CFLAGS)
tx.so_LDFLAGS := $(UHD_LIBS)


include $(ROOT)/quickbuild.make
