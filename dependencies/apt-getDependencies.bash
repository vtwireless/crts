#!/bin/bash

# This script installs system managed packages that CRTS depends on.
# This script runs "sudo apt-get install ..."

set -ex

#Dependencies for CRTS

declare -a crts_dep=(
 "build-essential"
 "libreadline-dev"
 "graphviz"
 "imagemagick"
 "joystick"
 "doxygen"
 "dia"
 "wget"
 "libgtk-3-dev"
 "git"
 "libjansson-dev"
 "yui-compressor"
 "python-netifaces"
 "libev-dev"
 "libuv1-dev"
 "libmunge-dev"
 "libc-ares-dev")

# libwebsockets is needed for mosquitto

# netloc lua man2html munge libcurl-dev may be needed by slurm

# Wed Sep 19 2018, libwebsockets is broken
# and lws_service_fd_tsi () from /usr/lib/x86_64-linux-gnu/libwebsockets.so.8
# just seg-faults, so we add a build and install of libwebsockets
# from source.  This adds a need for libev-dev and libuv1-dev which
# automatically came with libwebsockets.


#Dependencies for installing libuhd from source

declare -a uhd_dep=(
 "libboost-all-dev"
 "python-mako"
 "libqt4-dev"
 "qt4-dev-tools"
 "libqwt5-qt4-dev"
 "swig"
 "libfftw3-dev"
 "texlive"
 "cmake")

#Dependencies for installing gnuradio from source

declare -a gnuradio_dep=( 
 "python-bs4"
 "python-cheetah"
 "python-cycler"
 "python-dateutil"
 "python-decorator"
 "python-functools32"
 "python-glade2"
 "python-html5lib"
 "python-lxml"
 "python-matplotlib"
 "python-matplotlib-data"
 "python-networkx"
 "python-opengl"
 "python-pygraphviz"
 "python-pyparsing"
 "python-qt4"
 "python-qwt5-qt4"
 "python-scipy"
 "python-sip"
 "python-subprocess32"
 "python-tz"
 "python-wxgtk3.0"
 "python-wxversion"
 "python-yaml"
 "python-zmq"
)

if(uname -r |grep 4.15); then
	printf "Ubuntu 18.04 OR kernel version 4.15 detected, where python-imaging pkg is not supported. hence we add python-pil\n"
	gnuradio_dep+=('python-pil')
	printf "webencodings is required for debian as well"
	gnuradio_dep+=('python-webencodings')
else
	printf "for ubuntu systems 16.04 or below"
	gnuradio_dep+=('python-imaging')
	gnuradio_dep+=('python-weblib')
fi

#if(grep -q 'Debian' /etc/os-release); then
#    printf "is debian\n"
#    gnuradio_dep+=('python-webencodings')
#else
#    printf "assuming that your system is like Ubuntu\n"
#    gnuradio_dep+=('python-weblib')
#fi


printf "\nInstalling crts packages\n"
apt-get -y install ${crts_dep[@]}

printf "\nInstalling UHD dependencies\n"
apt-get -y install ${uhd_dep[@]}

printf "\nInstalling GNU Radio dependencies\n"
apt-get -y install ${gnuradio_dep[@]}

printf "Installed all dependencies successfully!"
