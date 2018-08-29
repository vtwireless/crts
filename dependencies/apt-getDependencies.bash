#!/bin/bash

# This script installs system managed packages that CRTS depends on.
# This script runs "sudo apt-get install ..."

set -ex

#Dependencies for CRTS

declare -a crts_dep=(
 "build-essential"
 "patchelf"
 "libreadline-dev"
 "graphviz"
 "imagemagick"
 "joystick"
 "doxygen"
 "dia"
 "wget"
 "libgtk-3-dev"
 "git"
 "yui-compressor"
 "patchelf"
 "python-netifaces")

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
 "python-imaging"
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

if(grep -q 'Debian' /etc/os-release); then
    printf "is debian\n"
    gnuradio_dep+=('python-webencodings')
else
    printf "assuming that your system is like Ubuntu\n"
    gnuradio_dep+=('python-weblib')
fi


printf "\nInstalling crts packages\n"
sudo apt-get install ${crts_dep[@]}

printf "\nInstalling UHD dependencies\n"
sudo apt-get install ${uhd_dep[@]}

printf "\nInstalling GNU Radio dependencies\n"
sudo apt-get install ${gnuradio_dep[@]}
