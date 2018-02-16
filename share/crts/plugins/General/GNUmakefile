# This makefile can be used to build any of the modules
# that are at this directory level, share/crts/plugins/.

ROOT := ../../../../

CPPFLAGS := -I$(ROOT)include -I$(ROOT)lib -I$(ROOT)liquid-dsp/src/include

INSTALL_DIR = $(PREFIX)/share/crts/plugins/$(notdir $(CURDIR))


LIQUID_SRC := $(abspath $(ROOT)/liquid-dsp/src)
LIBFEC_SRC := $(abspath $(ROOT)/liquid-dsp/libfec/src)

SRC_RPATH := -Wl,-rpath,$(LIQUID_SRC):$(LIBFEC_SRC)

LIQUID_LDFLAGS := -L$(LIQUID_SRC) -lliquid
LIBFEC_LDFLAGS := -L$(LIBFEC_SRC) -lfec


# Make every *.cpp file into a DSO (dynamic shared object) plugin that is
# installed.

# TODO: This will not work in a build tree that is not the same as the
# source tree.  *.cpp is in the source.  That may explain why GNU automake
# never uses wildcards.
srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))

define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s





# TODO: make quickbuild.make do this patchelf thing:
#
# After we install the programs we reset the RPATH so that the programs
# can find the installed libliquid and libfec shared libraries at run-time.
#INSTALL_RPATH = patchelf --set-rpath '$(PREFIX)/lib'

#POST_INSTALL_COMMAND =\
# $(INSTALL_RPATH) $(INSTALL_DIR)/liquidFrame.so


# above we auto generate lines like this:
#*.so_SOURCES := *.cpp

include $(ROOT)quickbuild.make
