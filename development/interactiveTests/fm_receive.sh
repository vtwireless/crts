#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

USRP2="addr=192.168.40.206"

crts_radio="../../bin/crts_radio"

exec $crts_radio\
     -f rx [ --uhd $USRP2 --freq 89.1 --rate 0.2 --gain 0 ]\
     -f liquidFMDemod\
     -f stdout\
     -D |\
    hexdump -v


