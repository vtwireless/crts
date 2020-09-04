#!/bin/bash


set -ex
set -o pipefail

# This gets a tarball from github.

#Usage: GetSrcFromGithub user package tag tarname [sha512]

function GetSrcFromGithub()
{
    [ -n "$4" ] || exit 1

    local path="$1/$2"
    local tag="$3"
    local tarfile=$4.tar.gz

    # This gets a tarball file from github for the package
    wget --no-check-certificate -O $tarfile https://github.com/$path/tarball/$tag

    if [ -n "$5" ] ; then
        set +e
        # Check that the file has a good SHA512 sum
        if ! echo "$5  $tarfile" | sha512sum -c ; then
            mv $tarfile $tarfile.BAD_SHA512
            exit 1 # FAIL
        fi
    else
        # Just report the SHA512 sum
        sha512sum $tarfile
    fi
    # Success
}
