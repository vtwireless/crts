#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

./termRun

./termRun "$crts_radio -f joystick -f stdout -D | hexdump"
