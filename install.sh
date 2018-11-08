#!/bin/bash

PATH=$(pwd)

sudo dependencies/depend.sh
./bootstrap

#NODE install
cd dependencies/node  
make download  
make  
sudo make install  
cd $PATH
node --version

echo "NODE installed"

#UHD install
cd dependencies/uhd  
make download  
make  
sudo make install  
cd $PATH
pkg-config --modversion uhd

echo "UHD installed"

#GNU Radio install
cd dependencies/gnuradio
make download
make
sudo make install
cd $PATH
pkg-config --modversion gnuradio-blocks

echo "GNURadio installed"

#CRTS Install
cd $PATH
./configure
make download
make
sudo make install

echo "CRTS installed!"
