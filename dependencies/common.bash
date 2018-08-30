# This file is sourced from subdirectories uhd/, gnuradio/, and node/.

currentDir=$(dirname ${BASH_SOURCE[0]})

source $currentDir/default_prefixes

if [ -f $currentDir/prefixes ] ; then
    source $currentDir/prefixes
fi

[ -z "$*" ] && return

for i in "$*" ; do
    echo "${!i}"
done
