#!/bin/bash

set -ex

source ../common.bash

export PY_PREFIX=$THRIFT_PREFIX

cd src
make install
