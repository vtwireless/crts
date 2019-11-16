#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

USRP2="addr=192.168.40.206"

crts_radio="../bin/crts_radio"

exec $crts_radio\
     -f rx [ --uhd $USRP2 --freq 105.3 --rate 0.4 --gain 0 ]\
     -f liquidFMDemod\
     -f stdout\
     #-C logger [ --file op.txt rx freq ]\ 
     -D |\
    hexdump -v


