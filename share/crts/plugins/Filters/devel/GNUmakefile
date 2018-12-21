# This makefile can be used to build any of the modules
# that are at this directory level

ROOT := ../../../../..

CPPFLAGS := -I$(ROOT)/include

INSTALL_DIR = $(PREFIX)/share/crts/plugins/Filters/devel

RPATH := -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib



srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))

define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


include $(ROOT)/quickbuild.make
