# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../../../
include $(ROOT)/config.make

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))

CPPFLAGS := -I$(ROOT)include

srcs :=\
 sleepPassThrough


define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource


include $(ROOT)quickbuild.make
