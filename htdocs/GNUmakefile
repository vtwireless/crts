SUBDIRS := css js admin img tests scope spectrum floatWidget

INSTALL_DIR = $(PREFIX)/htdocs

# We can't install webDesktop directory if it is a repo
# with a .git/ directory in it; so we pick each file to
# install from webDesktop/
INSTALLED := doc sessionHeader.htm

BUILD += webDesktop

webDesktop: webDesktop.build
	./$<


include ../quickbuild.make
