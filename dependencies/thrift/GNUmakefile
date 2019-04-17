# This is a GNU make file that uses GNU make make extensions

# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src


# After we install nothing here, we do this to install thrift using the build
# system in the thrift package.
POST_INSTALL_COMMAND = ./install.bash


include ../../quickbuild.make
