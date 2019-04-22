# This is a GNU make file that uses GNU make make extensions

# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src


# After we install nothing here, we do this to install uhd using the build
# system in the uhd package.
POST_INSTALL_COMMAND = cd src/host/BUILD && $(MAKE) install 

# TODO:

# 1.
#/usr/local/encap/uhd-v3.13.0.1/lib/uhd/utils/uhd_images_downloader.py

# 2.
# https://files.ettus.com/manual/page_transport.html#transport_usb_udev
#
#b_series:




include ../../quickbuild.make
