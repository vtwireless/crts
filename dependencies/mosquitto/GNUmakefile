# This is a GNU make file that uses GNU make make extensions

# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src

MOSQUITTO_PREFIX := $(shell /bin/bash ../common.bash MOSQUITTO_PREFIX)


# After we install nothing here, we do this to install mosquitto using the build
# system in the mosquitto package.
#
# Note: we put mosquitto as a relative symlink in the bin/
#
POST_INSTALL_COMMAND =\
 cd src &&\
 $(MAKE) install &&\
 ln -sr $(MOSQUITTO_PREFIX)/sbin/mosquitto $(MOSQUITTO_PREFIX)/bin


include ../../quickbuild.make
