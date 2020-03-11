#today="$( date +"%Y%m%d" )"
#number=0

#fname=rxLogs/$today.txt

#while [ -e "$fname" ]; do
#    printf -v fname -- '%s-%02d.txt' "$today" "$(( ++number ))"
#done

#printf 'Will use "%s" as filename\n' "$fname"
#touch "$fname"
#################################

#./logger.bash
#ID=$!
#echo $ID

arr=$(pgrep crts)
#printf "${arr}"
for i in ${arr}
    do
#        top -p $i -n 2 -b > top-$i.txt
#        cat /proc/$i/status > proc.txt
        top -p $i -n 200 -b > top/top-$i.txt
        cat /proc/$i/status > proc/proc-$i.txt
    done

