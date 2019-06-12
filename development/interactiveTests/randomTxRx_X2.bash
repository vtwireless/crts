#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../../bin/crts_radio"

source usrp_config

./termRun

./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 913.5 --rate 1 --gain 1 ]\
 -c\
 -f rx [ --uhd $USRP1 --freq 916.5 --rate 1 --gain 1 ]\
 -f liquidSync\
 -f stdout\
 -D |\
 hexdump -v"


./termRun "cat /dev/urandom | $crts_radio\
 -f rx [ --uhd $USRP2 --freq 913.5 --rate 1 --gain 1 ]\
 -f liquidSync\
 -f stdout\
 -c\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP2 --freq 916.5 --rate 1 --gain 1 ]\
 -D |\
 hexdump -v"

