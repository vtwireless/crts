# This is a GNU make file that uses GNU make extensions

SHELL := /bin/bash
.ONESHELL:

SUBDIRS := Controllers Filters General

build clean cleaner:
	for dir in $(SUBDIRS) ; do\
	    if ! $(MAKE) $@ --directory=$$dir ; then exit 1; fi;\
	done


