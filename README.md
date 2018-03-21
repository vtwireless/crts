## Ports

Currently developing on: 
  - Debian GNU/Linux 9.3 (stretch)
  - Debian GNU/Linux 8.10 (jessie)
  - Ubuntu 16.04

## Dependencies


```
apt-get install build-essential libuhd-dev libreadline-dev graphviz imagemagick joystick
```

In addition, this may not be needed, on broken Ubuntu systems that mess up
the libuhd-dev installation:
```
apt-get install libboost-all-dev
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

