#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

./termRun

./termRun "$crts_radio -f stdin -f stdout -D < /dev/urandom | hexdump"
./termRun "$crts_radio -f stdin -f stdout -t 0 -t 1 -D < /dev/urandom | hexdump"
