# CRTS

The Cognitive Radio Test System (CRTS) provides a framework for software
defined radios (SDRs) with interfaces from the web.


# Development Status

This software package is not ready for general public use.


## Ports

Currently developing on: 
  - Debian GNU/Linux 9.3 (stretch)
  - Ubuntu 16.04

## Dependencies

You can run the script
```
sudo dependencies/apt-getDependencies.bash
```
In this script we keep a running list of dependencies that can be
installed via apt-get.  You can install them yourself or run the script
which will run apt-get install for you.

At the time of this writing the debian/ubuntu packages "node",
"libuhd-dev" and "gnuradio-dev" are not current enough to be used.  Also
the node JS package is not current enough, so we provide scripts to build
and install them from git tagged github.com repositories.

These installation scripts depend on running
```
./bootstrap
```
This bootstrap downloads the GNU make file, quickbuild.make, which is used
in downloading, building, and installing these dependencies, and is used in
building and installing CRTS.  quickbuild.make is just one small make file.

The downloading, building and installing of Node JS, UHD (libUHD), and GNU
Radio may be done using scripts in this directory tree in the
sub-directory.  All of these packages we download, build and install using
particular tagged releases from github.com.  You may for-go using these
scripts and install them yourself.  The building and installing of CRTS
will use pkg-conf and the installed programs in your path to find the
installed files on your system.  Where these scripts install Node JS,
UHD, and GNU Radio can be changed by
```
cp dependencies/default_prefixes dependencies/prefixes
gedit dependencies/prefixes # of other editor
```
or used another test editor (gedit) then the values you put in prefixes
will be used when the installation scripts install node JS, UHD, and GNU
Radio.  Also this file has the git tags (version tag) of the packages that
are will be installed via these scripts.  This prefixes (or
default_prefix) file is only accessed by the optional node JS, UHD, and
GNU Radio installation scripts, and is not used by this CRTS package.


### Building and Installing NodeJS

If you choose to you may have the following script install nodeJS from
the github.com tagged source

```
cd depenences/node
make download
make
make install
cd -
```
Now you need to put node in your PATH.

Test that node is installed in your path:
```
node --version
```


### Building and Installing UHD

If you choose to you may have the following script install UHD from
the github.com tagged source:

```
cd depenences/uhd
make download
make
make install
cd -
```

Now you may want to test that this UHD installation is accessible via the
pkg-config system:
```
pkg-config --modversion uhd
```


### Building and Installing GNU Radio

If you choose to you may have the following script install GNU Radio from
the github.com tagged source:

```
cd depenences/gnuradio
make download
make
make install
cd -
```

Now you may want to test that this GNU Radio installation is accessible
via the pkg-config system:
```
pkg-config --modversion gnuradio-blocks
```


## Building and Installing CRTS (this package)


Then run
```
./configure
make download
make
make install
```

You may want to run the configure script with the --prefix option like so:
```
./configure --prefix /usr/local/my_CRTS
make download
make
make install
```
where you pick a better prefix than /usr/local/my_CRTS.



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

$UHD_PREFIX/utils/uhd_images_downloader.py

$UHD_PREFIX//bin/uhd_image_loader" --args="type=x300,addr=192.168.12.2"


## Tests

```
cd interactiveTests
```

and look at and run test programs in that directory.


## Running the CRTS server

```
crts_scenarioWebServer
```

