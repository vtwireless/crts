# This is a GNU make file that uses GNU make make extensions

# We are not developing liquid-dsp in our CRTS package so we do not need
# make dependencies to reach into the source of liquid-dsp.
#
# We have two automatic quickbuild suffix targets: 'download' target from
# liquid-dsp.tar.gz.dl and a 'build' target from src.bl and we make the
# whole liquid-dsp source in src/ or we fail.
#

# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src


# After we install nothing here, we do this to install liquid-dsp using
# the build system in the liquid-dsp package.
POST_INSTALL_COMMAND = cd src && $(MAKE) -f makefile install


include ../quickbuild.make
