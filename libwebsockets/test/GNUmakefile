
BUILD_NO_INSTALL := simpleClient

simpleClient_SOURCES := simpleClient.c
simpleClient_CPPFLAGS = -I$(LIBWEBSOCKETS_PREFIX)/include

simpleClient_LDFLAGS = -L$(LIBWEBSOCKETS_PREFIX)/lib -lwebsockets -Wl,-rpath,$(LIBWEBSOCKETS_PREFIX)/lib -lpthread


include ../../quickbuild.make
