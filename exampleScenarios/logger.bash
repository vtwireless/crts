#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

source usrp_config

crts_radio="../bin/crts_radio"

#./termRun

#./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 915.5 --rate 0.4 --gain 15 ]\
 -C logger [ --file lgStdin.txt stdin totalBytesOut \
 --file lgFrame.txt liquidFrame totalBytesIn totalBytesOut \
 --file lgTx.txt tx totalBytesIn ]\
 -D"

# 915.5 MHz receiver
./termRun "$crts_radio\
 -f rx [ --uhd $USRP2 --freq 915.5 --rate 0.4 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -C logger [ --file lgRx.txt rx freq totalBytesOut \
 --file lgSync.txt liquidSync totalBytesIn totalBytesOut \
 --file lgStdout.txt stdout totalBytesIn ]\
 -D |\
 hexdump -v"

