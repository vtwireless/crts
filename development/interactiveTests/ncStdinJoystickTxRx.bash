#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../../bin/crts_radio"

source usrp_config

./termRun

./termRun uhd_fft -f 915.0e6 -s 2.6e6 --args $USRP3

./termRun "nc -ulp 6666 |\
 $crts_radio\
 -f fileIn [ --file - ]\
 -f sink [ --control joystick ]\
 -c\
 -f fileIn [ --file /dev/urandom ] \
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 915.5 --rate 0.2 --gain 15 ]\
 -c\
 -f rx [ --uhd $USRP2 --freq 915.5 --rate 0.2 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -c\
 -C tests/joystickTxRx\
 -D | hexdump -v"
