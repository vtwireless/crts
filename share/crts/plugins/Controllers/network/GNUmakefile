
ROOT := ../../../../../

CPPFLAGS := -I$(ROOT)include -I$(ROOT)lib

INSTALL_DIR = $(PREFIX)/share/crts/plugins/Controllers/network


srcs := $(patsubst %.cpp,%,$(wildcard *.cpp))


define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


uhdTx.so_LDFLAGS := -luhd -ljansson -L$(ROOT)lib -lcrts -Wl,-rpath,$(abspath $(ROOT)lib)


# After we install the programs we reset the RPATH so that the programs
# can find the installed libcrts shared library at run-time.
INSTALL_RPATH = patchelf --set-rpath '$(PREFIX)/lib'

POST_INSTALL_COMMAND =\
 $(INSTALL_RPATH) $(INSTALL_DIR)/uhdTx.so


include $(ROOT)quickbuild.make
