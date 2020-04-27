#!/bin/bash

set -ex

cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../../bin/crts_radio"

USRP1="addr=192.168.40.41"
USRP2="addr=192.168.40.42"
USRP3="addr=192.168.40.44"




uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3 &


 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 913.5 --rate 1 --gain 15 ]\
 -c\
 -f rx [ --uhd $USRP1 --freq 916.5 --rate 1 --gain 15 ]\
 -f liquidSync\
 -f stdout\
 -D > /dev/null < /dev/urandom &
# hexdump -v < /dev/urandom &


$crts_radio\
 -f rx [ --uhd $USRP2 --freq 913.5 --rate 1 --gain 15 ]\
 -f liquidSync\
 -f stdout\
 -c\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP2 --freq 916.5 --rate 1 --gain 15 ]\
 -D |\
 hexdump -v < /dev/urandom &

while wait ; do
    sleep 1
done

