SUBDIRS := css js img tests

INSTALL_DIR = $(PREFIX)/htdocs

INSTALLED := doc x3dom
BUILD := index.html

# we must tell quickbuild that x3dom is a directory
CLEANDIRS := x3dom
CLEANERFILES := x3dom.tar.gz.BAD_SHA512

index.html: index.htm session_footer.htm
	cat $^ > $@

include ../quickbuild.make
