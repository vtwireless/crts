# This makefile can be used to build any of the modules
# that are at this directory level.

ROOT := ../../../..

UHD_CFLAGS := $(shell pkg-config uhd --cflags)
UHD_LIBS := $(shell pkg-config uhd --libs)


CPPFLAGS := -I$(ROOT)/include -I$(ROOT)/lib

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))


srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))

# For now just this.
srcs := joystickTx dualTx shell webClient logger webClient_rate


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

# The relative path linking thing is awesome.  We build the plugin so that
# the linker finds the path to libcrts.so by looking relative to where
# this webClient.so file is.  So now we don't need to change the file
# webClient.so when we install it, so long as the installed relative
# paths of webClient.so and libcrts.so stays the same.  It's the $ORIGIN
# key word that adds this magic when building the webClient.so file.  In
# the past the install process had to re-link dynamic shared objects just
# before putting them in their installed directories, and GNU autotools
# has (or had) a butt ugly linker loader wrapper script to work around the
# limitations of the linker loader system.
#
# The linker reads $ORIGIN as "link path relative".  $$ keeps make from
# eating the single $, and \$ keeps bash from eating the $.
#
webClient.so_LDFLAGS := -ljansson -lcrts -L$(ROOT)/lib -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib


# webClient_rate is like webClient_rate with a throughput rate thingy
# added.
webClient_rate.so_LDFLAGS := -ljansson -lcrts -L$(ROOT)/lib -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib



include $(ROOT)/quickbuild.make
