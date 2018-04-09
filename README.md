## Ports

Currently developing on: 
  - Debian GNU/Linux 9.3 (stretch)
  - Debian GNU/Linux 8.10 (jessie)
  - Ubuntu 16.04

## Dependencies


Building and installing CRTS requires: 

```
apt-get install\
 build-essential\
 libreadline-dev\
 graphviz\
 imagemagick\
 joystick\
 doxygen
```

Included in CRTS are scripts that build and install the libuhd and
GNUradio from the latest tagged GitHub source files.  To change the
repository release tags you can edit uhd/uhd.tar.gz.dl and
gnuradio/gnuradio.tar.gz.dl.


If you are not building and installing libuhd and GNUradio from the CRTS
scripts and you wish to use the versions installed by the Debian system
software package manager run: 

```
apt-get install\
 libuhd-dev\
 gnuradio-dev
```


If we do not use package deb package "libuhd-dev" and you want to build
libuhd from the GitHub source using scripts included in this CRTS package
in uhd/, you need in addition:

```
apt-get install\
 libboost-all-dev\
 python-mako\
 libqt4-dev\
 qt4-dev-tools\
 libqwt5-qt4-dev\
 swig\
 libfftw3-dev\
 texlive
```


If we do not use package deb package "gnuradio-dev" and you want to build
GNUradio from the GitHub source using scripts included in this CRTS package
in gnuradio/, you need in addition:

```
apt-get install\
 python-bs4\
 python-cheetah\
 python-cycler\
 python-dateutil\
 python-decorator\
 python-functools32\
 python-glade2\
 python-html5lib\
 python-imaging\
 python-lxml\
 python-matplotlib\
 python-matplotlib-data\
 python-networkx\
 python-opengl\
 python-pygraphviz\
 python-pyparsing\
 python-qt4\
 python-qwt5-qt4\
 python-scipy\
 python-sip\
 python-subprocess32\
 python-tz\
 python-wxgtk3.0\
 python-wxversion\
 python-yaml\
 python-zmq
```

and on Debain systems:

```
apt-get install python-webencodings
```

and on Ubuntu systems:

```
apt-get install python-weblib
```



## Installing CRTS from git cloned source

In the top source directory run

```
./bootstrap
```
which will download the file quickbuild.make which is a GNU makefile.


Then edit the file Devel_Configure, making sure you set PREFIX (the
installation prefix) to the directory that you would like.  If you are
building and installing libuhd from GitHub source using the included
scripts edit the value of UHD_PREFIX.  If you are building and installing
GNUradio from GitHub source using the included scripts edit the value of
GNURADIO__PREFIX.

```
./Devel_Configure
```
which will generate the one file config.make.

Then run
```
make download
```
which will download the liquid-dsp and libfec package tarballs
and maybe one or more files.


If you are building and installing libUHD from GitHub source using
the scripts provided by CRTS run:
```
cd uhd
make download
make
make install
cd ..
```


If you are building and installing GNUradio from GitHub source using
the scripts provided by CRTS run:
```
cd gnuradio
make download
make
make install
cd ..
```


To build and install CRTS, then run
```
make
```
and then run
```
make install
```

You could run all these commands in one line, but things can
happen, like not having a dependency installed, or a server
is not serving a downloaded file.


## Tests

```
cd interactiveTests
```

and look at and run test programs in that directory.

