# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../../


CPPFLAGS := -I$(ROOT)include -I$(ROOT)lib

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))



srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))

define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


include $(ROOT)quickbuild.make
