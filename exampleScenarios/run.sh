#!/bin/bash

i=0
while [ $i -lt 10 ]
do
    ./logger.bash
     sleep $[ ( $RANDOM % 20 )  + 1 ]s
    ((i++))
done

echo "Done"
