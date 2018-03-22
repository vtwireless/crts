#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

./termRun

./termRun uhd_fft -f 915.0e6 -s 2.6e6 --args addr=192.168.10.3

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f joystick\
 -f sink [ --control joystick ]\
 -c\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd addr=192.168.10.2 --freq 915.5 --rate 0.2 --gain 0 ]\
 -C joystickTx\
 -D > /dev/null"


# 915.5 MHz receiver
./termRun "$crts_radio\
 -f rx [ --uhd addr=192.168.10.4 --freq 915.5 --rate 0.2 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -D |\
 hexdump -v"


exit

# If we had more USRPs

# 914.5 MHz receiver
./termRun "$crts_radio\
 -f rx [ --uhd addr=192.168.10.5 --freq 914.5 --rate 0.2 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -D |\
 hexdump -v"




