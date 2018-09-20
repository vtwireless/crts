# This is a GNU make file that uses GNU make make extensions

# src.bl makes a clean file named src, but src is a directory, so we must
# tell quickbuild that src is a directory:
CLEANDIRS := src


# After we install nothing here, we do this to install slurm using the
# build system in the slurm package.
POST_INSTALL_COMMAND = cd src && $(MAKE) install


include ../../quickbuild.make
