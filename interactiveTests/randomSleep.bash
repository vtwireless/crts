#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

./termRun

./termRun "$crts_radio -l sleepTest -f stdin -f stdout -D < /dev/urandom | hexdump"
                           
./termRun "$crts_radio -l sleepTest\
 -f stdin -f sleepTest -f sleepTest -f stdout\
 -c 0 1 0 2 1 3 2 3\
 -D < /dev/urandom | hexdump"

./termRun "$crts_radio -l sleepTest\
 -f stdin -f sleepTest -f sleepTest -f stdout\
 -t 0 -t 1 -t 2 -t 3\
 -c 0 1 0 2 1 3 2 3\
 -D < /dev/urandom | hexdump"
