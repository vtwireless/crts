#!/bin/bash

arr=$(pgrep python)
for i in ${arr}
do
    perf record -F 99 -p $i -g
    #perf script > p-$i.perf
    perf script | stackcollapse-perf.pl | flamegraph.pl > graph-$i.svg
done
