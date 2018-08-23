
BUILD_NO_INSTALL := simpleClient

simpleClient_SOURCES := simpleClient.c
simpleClient_CPPFLAGS = -I$(NOPOLL_PREFIX)/include/nopoll

simpleClient_LDFLAGS =\
 -L$(NOPOLL_PREFIX)/lib -lnopoll -Wl,-rpath,$(NOPOLL_PREFIX)/lib\
 -lssl\
 -lpthread


include ../../quickbuild.make
