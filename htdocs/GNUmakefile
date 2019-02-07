SUBDIRS := css js img tests vis

INSTALL_DIR = $(PREFIX)/htdocs

INSTALLED := doc x3dom
BUILD := index.html

# we must tell quickbuild that x3dom is a directory
CLEANDIRS := x3dom

ifeq (,$(wildcard ./webDesktop/.git))

  # The file ./webDesktop/.git does not exist.

  # We build and remove webDesktop/ if it is not a
  # git repo checkout.  webDesktop is special in
  # that we may be developing it with CRTS and
  # not just downloading it.  So by default the
  # file ./webDesktop/.git will not exist and
  # so this will download that directory.
  #
  # TODO: For the case when there is a webDesktop/
  # that is a repo we do not need to download the
  # webDesktop.tar.gz file, but not a bad deal...
  #
  BUILD += webDesktop
  CLEANDIRS += webDesktop
  INSTALLED += webDesktop

webDesktop: webDesktop.build
	./$<

endif


$(warning BUILD=$(BUILD))


CLEANERFILES := x3dom.tar.gz.BAD_SHA512

index.html: index.htm session_footer.htm
	cat $^ > $@

include ../quickbuild.make
