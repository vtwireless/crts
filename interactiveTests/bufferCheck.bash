#!/bin/bash

set -e
cd $(dirname ${BASH_SOURCE[0]})

crts_radio="../bin/crts_radio"

./termRun

tmp=$(mktemp --suffix=CRTS_test)
out_tmp=$(mktemp --suffix=CRTS_test)


dd if=/dev/urandom count=100K of=$tmp 


function waitForStart()
{
    # This can totally fail due to race condition.
    #
    while ! killall -CONT crts_radio 2> /dev/null  ; do
        sleep 0.01
    done &
}

function waitForFinish()
{
    # This can totally fail due to race condition.
    #
    while killall -CONT crts_radio 2> /dev/null ; do
        sleep 0.01
    done
    if ! diff $tmp $out_tmp ; then
    echo "It failed to produce the same output"
        ret=1
        exit 1
    fi
}

ret=0

waitForStart

./termRun "cat $tmp |\
 $crts_radio\
 -f stdin\
 -f stdout\
 -D > $out_tmp"

wait
waitForFinish
waitForStart


./termRun "cat $tmp |\
 $crts_radio\
 -f stdin\
 -f copy\
 -f stdout\
 -D > $out_tmp"

wait
waitForFinish
waitForStart

./termRun "cat $tmp |\
 $crts_radio\
 -f stdin\
 -f passThrough\
 -f stdout\
 -D > $out_tmp"

wait
waitForFinish
waitForStart



./termRun "cat $tmp |\
 $crts_radio\
 -f stdin\
 -f copy\
 -f passThrough\
 -f copy\
 -f passThrough\
 -f stdout\
 -D > $out_tmp"

wait
waitForFinish


set -x

wc $tmp

rm $tmp $out_tmp

set +x

if [ $ret = 0 ] ; then
    echo -e "\nSUCCESS\n"
else
    echo -e "\nSomething failed\n"
fi

exit $ret
