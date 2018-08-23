
# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src


# After we install nothing here, we do this to install gnuradio using the build
# system in the gnuradio package.
POST_INSTALL_COMMAND = cd src && $(MAKE) install


include ../quickbuild.make
