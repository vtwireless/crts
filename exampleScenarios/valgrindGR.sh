#!/bin/bash

echo "Provide the name of GNU Radio's generated python file"
read grpath

timeout 10 valgrind --leak-check=full --show-leak-kinds=all --xtree-leak=yes
python $grpath
memcheck=$!
echo "Your memcheck output was stored as xtleak-$!"

timeout 50 valgrind --tool=massif --xtree-memory=allocs python $grpath
massif=$!
echo "Your massif output is stored as massif-$!"

timeout 50 valgrind --tool=cachegrind python $grpath
cache=$!
echo"Your cachegrind output is stored as cachegrind.$! "

timeout 50 valgrind --tool=callgrind python $grpath
call=$!
echo "callgrind.$!"

printf "\n\nSUMMARY\n\n memcheck output xtleak.$memcheck \n\n
massif.$massif \n\n cachegrind.$cache \n\n callgrind.$call"
