#!/bin/bash

set -ex

mapfile -t dep < installed.txt

for i in "${dep[@]}"
do
	printf "\nremoving $i\n"
	sudo apt-get --assume-yes remove $i
done
sudo apt-get autoremove
sudo apt-get autoclean
