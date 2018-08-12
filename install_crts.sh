#!/usr/bin/env bash

set -e

git clone git@github.com:vtwireless/crts.git
#: << 'END'

printf "\nCRTS dependencies...\n"

declare -a crts_pkg=(
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


printf "\nDependencies for libuhd...\n"
declare -a uhd_pkg=(
 "libboost-all-dev"
 "python-mako"
 "libqt4-dev"
 "qt4-dev-tools"
 "libqwt5-qt4-dev"
 "swig"
 "libfftw3-dev"
 "texlive"
 "cmake")

printf "\nDependencies for gnuradio...\n"
declare -a gnuradio_pkg=( 
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
    gnuradio_pkg+=('python-webencodings')
else
    printf "is ubuntu\n"
    gnuradio_pkg+=('python-weblib');
fi

printf "\nInstalling crts packages\n"
for i in "${crts_pkg[@]}"
do
    trap "rm -rf crts; exit" INT TERM EXIT
    sudo apt-get install "$i"
    #echo "$i"
    trap - INT TERM EXIT
done

printf "\nInstalling UHD dependencies to install UHD from source"
for i in "${uhd_pkg[@]}"
do
    trap "rm -rf crts; exit" INT TERM EXIT
    sudo apt-get install "$i"
    trap - INT TERM EXIT
    #echo "$i"
done

printf "\nInstalling gnuradio dependencies to install from source"
for i in "${gnuradio_pkg[@]}"
do
    trap "rm -rf crts; exit" INT TERM EXIT
    sudo apt-get install "$i"
    #echo "$i"
    trap - INT TERM EXIT
done
#END

trap "rm quickbuild.make; exit" INT TERM EXIT
cd crts
./bootstrap
./Devel_Configure
./Install_NodeJS
make download
cd uhd
make download
make
make install
cd ../gnuradio
make download
make 
make install
cd ..
make
make install
trap - INT TERM EXIT

exit 0
