
-include ../../config.make

SUBDIRS := plugins

ifdef DOC
SUBDIRS += doc
endif

INSTALL_DIR = $(PREFIX)/share/crts

# We install all the files in examplePlugins/
INSTALLED := examplePlugins

include ../../quickbuild.make
