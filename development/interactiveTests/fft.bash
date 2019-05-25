#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../../bin/crts_radio"

source usrp_config

./termRun

./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3
