
INSTALL_DIR = $(PREFIX)/share/crts/doc

BUILD := html developmentObjectGraph_01.png

CLEANDIRS := latex html


# We generate the file Doxyfile from Doxyfile.in
# This tells quickbuild to add a comment to
# the top of the generated file Doxyfile
#
Doxyfile_GEN_COMMENT := \# This is a generated file


%.png: %.dia
	dia $< --export=$@

html: Doxyfile
	doxygen

include ../../../quickbuild.make
