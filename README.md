# CRTS

The Cognitive Radio Test System (CRTS) provides a framework for software
defined radios (SDRs).


# Development Status

This software package is not ready for general public use.


## Ports

Currently developing on: 
  - Debian GNU/Linux 9.3 (stretch)
  - Ubuntu 16.04

## Dependencies

Included in CRTS are scripts that build and install libwebsocket, libuhd
and GNUradio from the latest tagged GitHub source files (as of Aug 2018).
At this time debian APT packages of libwebsocket, libuhd and GNUradio
there not new enough.

Building and installing CRTS requires: 

```
apt-get install\
 build-essential\
 patchelf\
 libreadline-dev\
 libjansson-dev\
 graphviz\
 imagemagick\
 joystick\
 doxygen\
 dia\
 wget\
 libgtk-3-dev\
 git\
 yui-compressor\
 patchelf\
 python-netifaces
```


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
 texlive\
 cmake
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
./Install_NodeJS
```

or


```
./Install_NodeJS -prefix MY_PREFIX
```
where MY_PREFIX is the node JS installation prefix directory,
or install node JS your own way.


Then run
```
make download
```
which will download some files using node JS's npm, the liquid-dsp and
libfec package tarballs and a few more files.


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


## Up grading Firmware and FPGA Images with Pre-built Images

With a browser go to relative to your UHD installation prefix
installation directory, UHD_PREFIX

```
UHD_PREFIX/share/doc/uhd/doxygen/html/page_images.html
```

Download files by running:

```
UHD_PREFIX/bin/uhd_images_downloader
```

This will download files to:
```
UHD_PREFIX/share/uhd/images/
```

Read some more on the web pages.  In my case I had a USRP N210 and I ran:
```
uhd_image_loader --args="type=usrp2,addr=192.168.10.2"
```

In my case the output on the terminal where I ran that was:
```
[INFO] [UHD] linux; GNU C++ version 6.3.0 20170516; Boost_106200; UHD_3.11.0.1-0-4e672fab
Unit: USRP N210 r4 (F44901, 192.168.10.2)
Firmware image: /usr/local/encap/uhd/share/uhd/images/usrp_n210_fw.bin
-- Erasing firmware image...successful.
-- Writing firmware image...successful.
-- Verifying firmware image...successful.
FPGA image: /usr/local/encap/uhd/share/uhd/images/usrp_n210_r4_fpga.bin
-- Erasing FPGA image...successful.
-- Writing FPGA image...successful.
-- Verifying FPGA image...successful.
```

For the X310

/usr/local/encap/uhd/lib/uhd/utils/uhd_images_downloader.py

/usr/local/encap/uhd/bin/uhd_image_loader" --args="type=x300,addr=192.168.12.2"


## Tests

```
cd interactiveTests
```

and look at and run test programs in that directory.


## Running the CRTS server

```
DEBUG=express:* crts_server
```

