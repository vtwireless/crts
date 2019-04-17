#!/bin/bash

# This script installs system managed packages that CRTS depends on.
# This script runs "sudo apt-get install ..."

set -ex

#Dependencies for CRTS in addition to 

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


#Dependencies for installing apache thrift
#Some modules in GNU radio depend on thrift
declare -a thrift_dep=(
 "bison"
 "flex"
 "libevent-dev"
 "python-twisted"
 "libcrypto++-dev")


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

OS="$(lsb_release -is)"

if [ "$OS" = "Ubuntu" ]; then
	if(uname -r |grep 4.15); then
		printf "Ubuntu 18.04 OR kernel version 4.15 detected, where python-imaging pkg is not supported, hence we add python-pil\n"
		gnuradio_dep+=('python-pil')
		printf "webencodings is required for debian as well"
		gnuradio_dep+=('python-webencodings')
	else
		printf "for ubuntu systems 16.04 or below"
		gnuradio_dep+=('python-imaging')
		gnuradio_dep+=('python-weblib')
	fi
elif [ "$OS" = "Debian" ]; then
	printf "Debian detected, add python-imaging and python-webencodings"
	gnuradio_dep+=('python-imaging')
	gnuradio_dep+=('python-webencodings')
	thrift_dep+=('libssl1.0-dev')
else
	printf "$OS detected, support will be added for this in the future"
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

printf "\nInstalling apache thrift dependencies\n"
apt-get -y install ${thrift_dep[@]}

printf "\nInstalling UHD dependencies\n"
apt-get -y install ${uhd_dep[@]}

printf "\nInstalling GNU Radio dependencies\n"
apt-get -y install ${gnuradio_dep[@]}

printf "\nInstalled all dependencies successfully!\n"
