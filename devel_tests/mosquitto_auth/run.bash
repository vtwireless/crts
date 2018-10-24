#!/bin/bash

set -ex

dir=$(dirname ${BASH_SOURCE[0]})
cd $dir
dir=$PWD

cd $(dirname\
 $(dirname $(readlink\
 $(which mosquitto))))/share/mosquitto/plugins


function catcher()
{
    [ -n "$pids" ] && kill $pids
    echo "finished via catcher"
    exit
}

trap catcher SIGINT SIGQUIT SIGTERM

redis-server &
pids=$!

# Wait for redis-server to bind to port
while ! nc -z localhost 6379 ; do
    sleep 0.4
done

mosquitto -c $dir/mosquitto.conf &
pids="$pids $!"

wait

echo "Finished - end of script"
