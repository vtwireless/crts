#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

source usrp_config

crts_radio="../bin/crts_radio"

./termRun "$crts_radio\
 -f count\
 -f stdout\
 -D > countCorrect.txt"


