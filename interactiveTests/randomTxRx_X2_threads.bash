#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

./termRun

./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args addr=192.168.10.3

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd addr=192.168.10.2 --freq 914.5 --rate 0.2 --gain 0 ]\
 -t 0 -t 1 -t 2\
 -c\
 -f rx [ --uhd addr=192.168.10.2 --freq 915.5 --rate 0.2 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -t 0 -t 1 -t 2\
 -d |\
 hexdump -v"


./termRun "cat /dev/urandom | $crts_radio\
 -f rx [ --uhd addr=192.168.10.4 --freq 914.5 --rate 0.2 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -t 0 -t 1 -t 2\
 -c\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd addr=192.168.10.4 --freq 915.5 --rate 0.2 --gain 0 ]\
 -t 0 -t 1 -t 2\
 -d |\
 hexdump -v"

