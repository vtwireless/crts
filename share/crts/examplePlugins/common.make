# This is a GNU make file that uses GNU make extensions

SHELL := /bin/bash
.ONESHELL:


CXXFLAGS := -g -Wall -Werror

CPPFLAGS := -DDEBUG

INC := $(shell pkg-config --cflags crts)


dependfiles := $(addsuffix .d, $(wildcard *.cpp))

nodepend := $(strip $(findstring clean, $(MAKECMDGOALS)))


plugins := $(strip $(patsubst %.cpp, %.so, $(wildcard *.cpp)))

CLEANFILES := $(dependfiles) $(plugins)


# begin make rules

build: $(plugins)

debug:
	@echo "plugins=$(plugins)"
	@echo "dependfiles=$(dependfiles)"


ifeq ($(nodepend),)
-include $(dependfiles)
endif

%.d: %
	$(CXX) $(CXXFLAGS) $(INC) -MM $< -MF $@ -MT $(patsubst %.cpp, %.lo, $<)
%.lo: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c -fPIC -o $@ $<
%.so: %.lo
	$(CXX) $(CXXFLAGS) $($@_LDFLAGS) -shared -o $@ $^

clean cleaner:
	rm -f $(CLEANFILES) *.lo
