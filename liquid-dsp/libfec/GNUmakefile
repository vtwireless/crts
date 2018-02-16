# This is a GNU make file that uses GNU make make extensions
#
# We are not developing libfec in our CRTS package so we do not need
# make dependencies to reach into the source of libfec.
#
# We have two automatic quickbuild suffix targets: 'download' target from
# libfec.tar.gz.dl and a 'build' target from src.bl and we make the
# whole libfec source in src/ or we fail.
#

# src.bl makes a clean file named src, but src is a directory so
# we need to tell quickbuild that it is a directory so it can clean it.
CLEANDIRS := src



# After we install nothing, we install this using the build system in the
# libfec package.
POST_INSTALL_COMMAND = cd src && $(MAKE) -f makefile install


include ../../quickbuild.make
