# This is sourced by bash scripts in this directory

set -ex


# Usage: DownloadFile FILENAME TAG [SHA512SUM]
#
function DownloadFile() {

    if [ -z "$2" ] ; then
        echo "Usage: DownloadFile FILENAME TAG [SHA512SUM]"
        exit 1
    fi


    URL=https://raw.githubusercontent.com/vtwireless/HLSI/$2/$1

    if [ ! -f "$1" ] ; then
        wget -O $1 $URL
    fi

    if [ -n "$3" ] ; then
        echo "$3  $1" | sha512sum -c
    else
        sha512sum $1
    fi

    touch $1
}
