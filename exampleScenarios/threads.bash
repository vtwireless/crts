#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

source usrp_config

#./termRun

#./termRun uhd_fft -f 915.0e6 -s 10.5e6 --args $USRP3

today="$( date +"%Y%m%d" )"
number=0

fname=$today.txt

while [ -e "txLogs_t/$fname" ]; do
    printf -v fname -- '%s-%02d.txt' "$today" "$(( ++number ))"
done

printf 'Will use "%s" as filename\n' "$fname"

touch "stdinLogs_t/$fname"
touch "frameLogs_t/$fname"
touch "txLogs_t/$fname"
touch "rxLogs_t/$fname"
touch "syncLogs_t/$fname"
touch "stdoutLogs_t/$fname"

./termRun "timeout 600 cat /dev/urandom |\
$crts_radio\
 -f stdin\
 -f liquidFrame\
 -f tx [ --uhd $USRP1 --freq 914.5 --rate 0.4 --gain 15 ]\
 -t 0 -t 1 -t 2\
 -c\
 -C logger [ --file stdinLogs_t/$fname stdin totalBytesOut \
 --file frameLogs_t/$fname liquidFrame totalBytesIn totalBytesOut \
 --file txLogs_t/$fname tx totalBytesIn ]\
  -D"

./termRun "timeout 600 $crts_radio\
 -f rx [ --uhd $USRP2 --freq 915.5 --rate 0.4 --gain 0 ]\
 -f liquidSync\
 -f stdout\
 -t 0 -t 1 -t 2\
 -c\
 -C logger [ --file rxLogs_t/$fname rx freq totalBytesOut \
 --file syncLogs_t/$fname liquidSync totalBytesIn totalBytesOut \
 --file stdoutLogs_t/$fname stdout totalBytesIn ]\
 -D"
