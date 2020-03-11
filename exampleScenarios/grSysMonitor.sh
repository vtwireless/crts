arr=$(pgrep python)
#printf "${arr}"
for i in ${arr}
    do
#        top -p $i -n 2 -b > top-$i.txt
#        cat /proc/$i/status > proc.txt
        top -p $i -n 40 -b > top-$i.txt
        cat /proc/$i/status > proc-$i.txt
    done

