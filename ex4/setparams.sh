#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

if [ "$1" == "-h" ] || [ "$1" == "-help" ]; then
    echo "This script sends parameters to the proc file /proc/ratelimit. Not all parameters are required. There is no checking done on the parameters, so make sure you get them right!"
    echo "usage: `basename $0` portno packetlimit waittime"
    exit
fi

if [[ -n "$1" ]]; then
    echo "P $1" > /proc/ratelimit
fi

if [[ -n "$2" ]]; then
    echo "P $2" > /proc/ratelimit
fi

if [[ -n "$3" ]]; then
    echo "P $3" > /proc/ratelimit
fi