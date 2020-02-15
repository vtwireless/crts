#!/bin/bash

for i in {0..50}:
do
    ./threads.bash
    sleep 40
    echo "collecting data ........."
    for i in $(pgrep crts);
    do
        cat /proc/$i/status > proc_t/proc-$i.txt
        top -p $i -n 20 -b > top_t/top-$i.txt
    done
    echo "does top keep running?"
    sleep 602
    echo "sleep"
    for pid in $(pgrep xterm); do kill -9 $pid; done
    echo "xterm killed" 
done
