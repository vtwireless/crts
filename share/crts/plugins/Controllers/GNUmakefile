# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../../

UHD_CFLAGS := $(shell pkg-config uhd --cflags)
UHD_LIBS := $(shell pkg-config uhd --libs)


CPPFLAGS := -I$(ROOT)include -I$(ROOT)lib

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))


srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))

# For now just this.
srcs := joystickTx dualTx


define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s



dualTx.so_CPPFLAGS := $(UHD_CFLAGS)
joystickTx.so_CPPFLAGS := $(UHD_CFLAGS)

dualTx.so_LDFLAGS := $(UHD_LIBS)
joystickTx.so_LDFLAGS := $(UHD_LIBS)


include $(ROOT)quickbuild.make
