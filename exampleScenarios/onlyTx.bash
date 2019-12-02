#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

source usrp_config

crts_radio="../bin/crts_radio"

start=$SECONDS

sudo tcpdump -i eth1 dst 192.168.40.203 -nn -B 8192 &
exec cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 915.5 --rate 0.4 --gain 15 ]\
 -C dualTx \
 -D
duration=$(( SECONDS - start ))
echo $duration 1>&2
