#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

source usrp_config

crts_radio="../bin/crts_radio"

#./termRun

#./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

#checks if file for today's date exists in rxLogs only.
#assuming that all logs will be updated for all folders
#if exists, adds number at the end and creates new file

today="$( date +"%Y%m%d" )"
number=0

fname=$today.txt

while [ -e "$fname" ]; do
    printf -v fname -- '%s-%02d.txt' "$today" "$(( ++number ))"
done

printf 'Will use "%s" as filename\n' "$fname"

touch "stdinLogs/$fname"
touch "frameLogs/$fname"
touch "txLogs/$fname"
touch "rxLogs/$fname"
touch "syncLogs/$fname"
touch "stdoutLogs/$fname"

./termRun "cat /dev/urandom |\
 $crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 915.5 --rate 0.4 --gain 15 ]\
 -C logger [ --file stdinLogs/$fname stdin totalBytesOut \
 --file frameLogs/$fname liquidFrame totalBytesIn totalBytesOut \
 --file txLogs/$fname tx totalBytesIn ]\
 -D"

# 915.5 MHz receiver
./termRun "$crts_radio\
 -f rx [ --uhd $USRP2 --freq 915.5 --rate 0.4 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -C logger [ --file rxLogs/$fname rx freq totalBytesOut \
 --file syncLogs/$fname liquidSync totalBytesIn totalBytesOut \
 --file stdoutLogs/$fname stdout totalBytesIn ]\
 -D |\
 hexdump -v"

