# This is sourced by other files in the build process

function Download()
{
    if [ -z "$2" ] ; then
        echo "Usage: Download target url [sha512sum]"
        exit 1
    fi

    # fail on error and verbose
    #trap "rm -f $1" ERR

    set -x

    local ckopts=
    [ -n "$3" ] && ckopts="--no-check-certificate"

    if ! wget $2 -O $1 --no-use-server-timestamps $ckopts ; then
        rm -f $1
        return 1
    fi

    if [ -n "$3" ] ; then
        # Check that the sha512 hash is correct for this downloaded file:
        if ! echo "$3  $1" | sha512sum -c ; then
            rm -f $1
            return 1
        fi
    fi
    set +x

    return 0 # success
}
