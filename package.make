# Define CRTS package specific GNU make variables in here.

IN_VARS :=\
 SPEW_LEVEL\
 DEBUG\
 PREFIX\
 VERSION\
 USRP_HOSTS\
 DEFAULT_RADIO_PORT

# What to call a tarball file name prefix TAR_NAME-VERSION.tar.gz
TAR_NAME := crts

# package release version.  May differ from interface (library) versions.
VERSION := 0.3

# PACKAGE CXXFLAGS is appended to the users CXXFLAGS
CXXFLAGS := -std=c++11

# Default JSON array list of USRP host computers.
# Empty string is localhost.
USRP_HOSTS ?= [ \"\"]

# Default TCP/IP PORT used to connect crts_radio controller module to the
# crts_server (web server).
#
DEFAULT_RADIO_PORT ?= 9383


# Export some additional make variables to the spawned make scripts
export\
 PREFIX
