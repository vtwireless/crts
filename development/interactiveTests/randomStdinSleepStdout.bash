#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../../bin/crts_radio"

./termRun

./termRun "$crts_radio -l sleepTest\
 -f stdin\
 -f sleepTest\
 -f sleepTest\
 -f sleepTest\
 -f stdout -D < /dev/urandom | hexdump"
./termRun "$crts_radio -l sleepTest\
 -f stdin\
 -f sleepTest\
 -f sleepTest\
 -f sleepTest\
 -f stdout\
 -t 0 -t 1 -t 2 -t 3 -t 4\
 -D < /dev/urandom | hexdump"
