## Ports

Currently developing on: 
  - Debian GNU/Linux 9.3 (stretch)
  - Debian GNU/Linux 8.10 (jessie)
  - Ubuntu 16.04

## Dependencies

WARNING: At this point this is just developer notes.  This is all in flux.

Uninstalling libuhd-dev shows:

The following packages were automatically installed and are no longer required:
  fonts-lyx gnuradio gnuradio-dev libcodec2-0.4 libcomedi0 libcppunit-1.13-0v5 libcppunit-dev libglade2-0 libgnuradio-analog3.7.10 libgnuradio-atsc3.7.10
  libgnuradio-audio3.7.10 libgnuradio-blocks3.7.10 libgnuradio-channels3.7.10 libgnuradio-comedi3.7.10 libgnuradio-digital3.7.10 libgnuradio-dtv3.7.10
  libgnuradio-fcd3.7.10 libgnuradio-fec3.7.10 libgnuradio-fft3.7.10 libgnuradio-filter3.7.10 libgnuradio-noaa3.7.10 libgnuradio-pager3.7.10
  libgnuradio-pmt3.7.10 libgnuradio-qtgui3.7.10 libgnuradio-runtime3.7.10 libgnuradio-trellis3.7.10 libgnuradio-uhd3.7.10 libgnuradio-video-sdl3.7.10
  libgnuradio-vocoder3.7.10 libgnuradio-wavelet3.7.10 libgnuradio-wxgui3.7.10 libgnuradio-zeromq3.7.10 libgsl2 libjs-jquery-ui liblog4cpp5-dev liblog4cpp5v5
  libqt4-declarative libqt4-scripttools libqt4-test libqtassistantclient4 libqwt5-qt4 librtlsdr0 libuhd003 libvolk1-bin libvolk1-dev libvolk1.3 python-bs4
  python-cheetah python-cycler python-dateutil python-decorator python-functools32 python-glade2 python-html5lib python-imaging python-lxml
  python-matplotlib python-matplotlib-data python-networkx python-opengl python-pygraphviz python-pyparsing python-qt4 python-qwt5-qt4 python-scipy
  python-sip python-subprocess32 python-tz python-webencodings python-wxgtk3.0 python-wxversion python-yaml python-zmq rtl-sdr uhd-host


```
apt-get install build-essential libreadline-dev graphviz imagemagick joystick doxygen
```

If we do not use package "libuhd-dev" 

In addition 
```
apt-get install libboost-all-dev python-mako libqt4-dev qt4-dev-tools libqwt5-qt4-dev swig libfftw3-dev texlive
```



We need this to build gnuradio in /gnuradio

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
 python-webencodings\
 python-wxgtk3.0\
 python-wxversion\
 python-yaml\
 python-zmq
```

Maybe more...


## Installing from git clone source

In the top source directory run

```
./bootstrap
```
which will download the file quickbuild.make.

Then edit the file Devel_Configure, making sure you set
PREFIX (the installation prefix) to the directory that you
would like, and then run it:
```
./Devel_Configure
```
which will generate the one file config.make.

Then run
```
make download
```
which will download the liquid-dsp and libfec package tarballs.
At this point you have all the needed files "non-system installed"
files.  So ya, no more downloads unless you need to install a
system dependency.

Then run
```
make
```
and then run
```
make install
```

You could run all these commands in one line, but things can
happen, like not having a dependency install, or a server
is not serving a downloaded file.


## Tests

```
cd interactiveTests
```

and look at and run test programs in that directory.

