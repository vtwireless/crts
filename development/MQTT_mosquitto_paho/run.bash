#!/bin/bash

dir="$(dirname ${BASH_SOURCE[0]})"

set -e
cd $dir
set +e

MOSQUITTO=mosquitto

HTTP_PORT=8180


function catcher()
{
    kill $mosq_pid $crts_pid $firefox_pid
    echo "finished"
}

passcode="$(openssl rand -hex 3)"


trap catcher SIGINT SIGQUIT SIGTERM


../../lib/crts_server.js --http_port $HTTP_PORT &
crts_pid=$!


$MOSQUITTO -c mosquitto.conf &
mosq_pid=$!

URL="http://localhost:$HTTP_PORT/tests/MQTT_mosquitto_paho"

echo "Going to: $URL"

firefox $URL &
firefox_pid=$!

while wait ; do
    echo "waiting"
done

echo "finished"
