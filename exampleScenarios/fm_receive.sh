#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})


crts_radio="../bin/crts_radio"

./termRun "$crts_radio\
     -f rx [ --uhd $USRP2 --freq 105.3 --rate 0.4 --gain 0 ]\
     -f liquidFMDemod\
     -f stdout\
     -D |\
    hexdump -v"


