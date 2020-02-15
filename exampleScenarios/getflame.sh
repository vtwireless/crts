#!/bin/bash

arr=$(pgrep crts)
for i in ${arr}
do
    echo "$i"
    perf record -F 99 -p $i -g
    #perf script > p-$i.perf
    perf script | stackcollapse-perf.pl | flamegraph.pl > graph-$i.svg
done
