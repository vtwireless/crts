#!/bin/bash

dir="$(dirname ${BASH_SOURCE[0]})"

set -e
cd $dir
set +e


function catcher()
{
    kill $mosq_pid $client_pid
}

passcode="$(openssl rand -hex 3)"


trap catcher SIGINT SIGQUIT SIGTERM


mosquitto -c mosquitto.conf &
mosq_pid=$!


# Wait 1 second for mosquitto to bind to port.
#
# This is okay for a stupid interactive test.
sleep 1

./client &
client_pid=$!


sleep 5
catcher

wait

echo "$0 finished"
