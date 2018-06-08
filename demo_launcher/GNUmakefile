# This is a GNU make file that uses GNU make make extensions

# We are not developing demo_launcher in our CRTS package so we do not need
# make dependencies to reach into the source of demo_launcher.
#
# We have two automatic quickbuild suffix targets: 'download' target from
# demo_launcher.tar.gz.dl and a 'build' target from src.bl and we make the
# whole demo_launcher source in src/ or we fail.
#

# We don't want the demo_launcher software to be part of the CRTS
# software, so it's not a sub-directory.
SUBDIRS := .

# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src

-include ../config.make

# After we install nothing here, we do this to install demo_launcher using
# the build system in the demo_launcher package.
POST_INSTALL_COMMAND = cd src && $(MAKE) install PREFIX=$(PREFIX)


include ../quickbuild.make
