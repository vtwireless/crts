
ROOT := ../../../../../

CPPFLAGS := -I$(ROOT)include -I$(ROOT)lib

INSTALL_DIR = $(PREFIX)/share/crts/plugins/Controllers/network


srcs :=\
 $(sort \
 $(patsubst %.cpp,%,$(wildcard *.cpp))\
 $(patsubst %.cpp.in,%,$(wildcard *.cpp.in)))


define MkSource
  $(1).so_SOURCES := $(1).cpp
endef
$(foreach src,$(srcs),$(eval $(call MkSource,$(src))))
undefine MkSource
undefine s


uhdTx.so_LDFLAGS := -luhd -ljansson -L$(ROOT)lib -lcrts -Wl,-rpath=\$$ORIGIN/$(ROOT)lib



include $(ROOT)quickbuild.make
