SUBDIRS := css js admin img tests scope spectrum floatWidget

INSTALL_DIR = $(PREFIX)/htdocs

INSTALLED := doc webDesktop


ifeq (,$(wildcard ./webDesktop/.git))

  # The file ./webDesktop/.git does not exist.

  # We build and clean webDesktop/ if it is not a
  # git repo checkout.  webDesktop is special in
  # that we may be developing it with CRTS and
  # not just downloading it.  So by default the
  # file ./webDesktop/.git will not exist and
  # so this will download that directory.
  #
  # TODO: For the case when there is a webDesktop/
  # that is a repo we do not need to download the
  # webDesktop.tar.gz file, but not a big deal...
  #
  BUILD += webDesktop
  CLEANDIRS += webDesktop
  INSTALLED += webDesktop

webDesktop: webDesktop.build
	./$<
endif


include ../quickbuild.make
