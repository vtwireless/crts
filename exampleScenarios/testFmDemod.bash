#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

source usrp_config

crts_radio="../bin/crts_radio"

./termRun

#./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f tx [ --uhd $USRP1 --freq 915.5 --rate 0.4 --gain 15 ]\
 -C dualTx \
 -D"

# 915.5 MHz receiver
./termRun "$crts_radio\
 -f rx [ --uhd $USRP2 --freq 915.5 --rate 0.4 --gain 0 ]\
 -f liquidFMDemod\
 -f stdout\
 -D |\
 hexdump -v"
