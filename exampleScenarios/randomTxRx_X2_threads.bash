#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

source usrp_config

#./termRun

#./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 914.5 --rate 0.4 --gain 15 ]\
 -t 0 -t 1 -t 2\
 -c -D"
# -f rx [ --uhd $USRP1 --freq 915.5 --rate 0.2 --gain 0 ]\
# -f liquidSync\
# -f stdout\
# -t 0 -t 1 -t 2\
# -D |\
# hexdump -v"

