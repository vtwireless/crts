INSTALL_DIR = $(PREFIX)/htdocs/tests

BUILD := simpleClient.html

simpleClient.html: simpleClient.htm session_footer.htm
	cat simpleClient.htm session_footer.htm > $@

include ../../quickbuild.make
