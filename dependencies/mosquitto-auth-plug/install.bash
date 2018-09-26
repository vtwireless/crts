#!/bin/bash

set -e

source ../common.bash

set -x

cd src

plugin_dir=$MOSQUITTO_PREFIX/share/mosquitto/plugins

mkdir -p $plugin_dir 

cp auth-plug.so $plugin_dir
