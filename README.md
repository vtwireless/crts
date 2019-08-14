# CRTS

The Cognitive Radio Test System (CRTS) provides a framework for software
defined radios (SDRs) with interfaces from the web.


# Development Status

This software package is not ready for general public use.


## Ports

Currently developing on: 
  - Debian GNU/Linux 9.3 (stretch)
  - Ubuntu 16.04, 18.04

## Dependencies

See README in dependencies/ subdirectory.  Installing the dependencies is
more involved than installing CRTS, so we provide a separate dependencies
directory.

If you are using this source code from a git repository there is a git
submodule, webtop, https://github.com/vtwireless/webtop.git, that is
software depends on.  Webtop can be downloaded by running the included
*bootstrap* script.


## "make download" for code re-usability

Not unique to CRTS, we introduce the idea of downloading all the files
needed, before starting to build the software, so we have a "download"
make target as in running *make download* as you'll see below.


## Building and Installing CRTS (this package)


Then run
```
./bootstrap
./configure --prefix /usr/local/my_CRTS
make download
make
make install
```

where you pick a better prefix than */usr/local/my_CRTS*.


## Up grading Firmware and FPGA Images with Pre-built Images

With a browser go to relative to your UHD installation prefix
installation directory, UHD_PREFIX

```
$UHD_PREFIX/share/doc/uhd/doxygen/html/page_images.html
```

Download files by running:

```
$UHD_PREFIX/bin/uhd_images_downloader
```

This will download files to:
```
$UHD_PREFIX/share/uhd/images/
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

$UHD_PREFIX/bin/uhd_image_loader" --args="type=x300,addr=192.168.12.2"


For USB USRP devices
https://files.ettus.com/manual/page_transport.html#transport_usb_udev

## Development notes

If you are developing using a git repository, run

*git add htdocs/webtop && git commit -m "upgraded version of webtop"*

to upgrade the webtop git submodule to your current checkout of
crts.

### on git submodules
How can I specify a branch/tag when adding a Git submodule?
https://stackoverflow.com/questions/1777854/how-can-i-specify-a-branch-tag-when-adding-a-git-submodule

If you want to move the submodule to a particular tag:
```
  cd submodule_directory
  git checkout v1.0
  cd ..
  git add submodule_directory
  git commit -m "moved submodule to v1.0"
  git push
```
Then, another developer who wants to have submodule_directory changed to that tag, does this
```
  git pull
  git submodule update --init
```
