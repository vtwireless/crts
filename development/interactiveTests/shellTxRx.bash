#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

#source usrp_config
USRP3="addr=192.168.40.203"
USRP1="addr=192.168.40.101"
USRP2="addr=192.168.40.201"

crts_radio="../../bin/crts_radio"

./termRun

./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

./termRun "cat /dev/urandom | $crts_radio -f stdin -f liquidFrame -f tx [\
 --control tx --uhd $USRP1 --freq 915.5 --rate 0.4 --gain 15 ]\
 -c -f rx [ --uhd $USRP2  --freq 915.5 --rate 0.4 --gain 0 ]\
 -f liquidSync -f stdout -c    -C shell  -D | hexdump -v"
