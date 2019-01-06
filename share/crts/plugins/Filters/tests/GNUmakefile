# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../../..
-include $(ROOT)/config.make

INSTALL_DIR = $(PREFIX)/share/crts/plugins/Filters/tests

CPPFLAGS := -I$(ROOT)/include


srcs :=\
 sleepPassThrough\
 joystickTxController\
 count\
 countCheck\
 msequence


define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource


# This indirectly links with UHD by using the
# ../txControl.hpp interface.
#
joystickTxController.so_LDFLAGS := -luhd


include $(ROOT)/quickbuild.make
