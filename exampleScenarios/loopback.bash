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

while [ -e "rxLogs_l/$fname" ]; do
    printf -v fname -- '%s-%02d.txt' "$today" "$(( ++number ))"
done

printf 'Will use "%s" as filename\n' "$fname"

touch "stdinLogs_l/$fname"
touch "frameLogs_l/$fname"
touch "txLogs_l/$fname"
touch "rxLogs_l/$fname"
touch "syncLogs_l/$fname"
touch "stdoutLogs_l/$fname"
#touch "top/$fname"
#touch "proc/$fname"

./termRun " timeout 1000 cat /dev/urandom |\
$crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 915.5 --rate 0.4 --gain 15 ]\
 -C logger [ --file stdinLogs_l/$fname stdin totalBytesOut \
 --file frameLogs_l/$fname liquidFrame totalBytesIn totalBytesOut \
 --file txLogs_l/$fname tx totalBytesIn ]\
 | $crts_radio\
 -f rx [ --uhd $USRP2 --freq 915.5 --rate 0.4 --gain 15 ]\
 -f liquidSync\
 -f stdout\
 -C logger [ --file rxLogs_l/$fname rx freq totalBytesOut \
 --file syncLogs_l/$fname liquidSync totalBytesIn totalBytesOut \
 --file stdoutLogs_l/$fname stdout totalBytesIn ]\
 -D |\
 hexdump -v"


