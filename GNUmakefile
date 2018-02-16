
# TODO: liquid-dsp is a software dependency that we download when 'make
# download' is run, which 'make build' and 'make' depend on.  It would be
# best to not have this built and installed with this package, and just
# have it be an pre-installed prerequisite package.  The problem is that
# liquid-dsp does have a (OS or other-wise) managed or automated method to
# be installed with.  It's not a mature package at the time of writing
# this.

# TODO: In looking at the liquid-dsp ofdmflexframegen and
# ofdmflexframesync code we see it hammers the libc memory allocator
# (realloc, malloc, and free) and does an lot of unnecessary memory copies
# when it is running at steady state.  That's just poor sub-optimal
# coding.  It is happening in the inner most loop.  Just a simple size
# change flag check may speed it up this code 10 or 100 fold, so it checks
# to see if it needs to reallocate memory before just reallocating memory
# for no reason at every frame.

# TODO: libuhd is not so good.  Ya, rewrite it...  Also get rid of the
# libBOOST dependency in libuhd.



# We wish to have things accessed/built/installed in this order:
SUBDIRS := liquid-dsp include lib bin share etc interactiveTests



# The program crts_mkTUN need a special installation so it can startup as
# effective user root and create a TUN device and route it and then it
# goes back to being a regular user.  We could have added this as the last
# part of the "install" target but we wanted to keep the basic install as
# a non-root thing, with no root access.
#
# You can run 'make sudo_install' or 'make root_install'
#

crts_mkTUN = $(PREFIX)/bin/crts_mkTUN


sudo_install:
	sudo chown root:root $(crts_mkTUN)
	sudo chmod u+s $(crts_mkTUN)

# if you do not have sudo install you can run 'make root_install' as root
root_install:
	chown root:root $(crts_mkTUN)
	chmod u+s $(crts_mkTUN)


include ./quickbuild.make
