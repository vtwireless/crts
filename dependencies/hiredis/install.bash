#!/bin/bash

set -e

source ../common.bash

set -x

cd src

make PREFIX=$HIREDIS_PREFIX install
