
INSTALL_DIR = $(PREFIX)/share/crts/doc

BUILD := html developmentObjectGraph_01.png

CLEANDIRS := latex html

%.png: %.dia
	dia $< --export=$@

html: Doxyfile
	doxygen

include ../../../quickbuild.make
