#!/bin/bash

usage(){
cat <<EOF
usage $0 options

Send various parameters to the proc file for the ratelimit kernel module.
There is only minimal checking done on the parameters, so make sure you get them right!

OPTIONS:
  -h   show this message
  -p   port to monitor
  -l   overflow limit
  -w   wait time after limit exceeded
EOF
}


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

if [ "$#" -eq 0 ]; then
    usage
    exit
fi

if [ ! -e "/proc/ratelimit" ]; then
    echo "Module ratelimit is not running. Use \"insmod ratelimit.ko\" to load it."
    exit
fi

PORT=
LIMIT=
WAIT=
while getopts “hp:l:w:” OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
             ;;
         p)
	     PORT=$OPTARG
             ;;
         l)
	     LIMIT=$OPTARG
             ;;
	 w)
	     WAIT=$OPTARG
	     ;;
         ?)
             usage
             exit
             ;;
     esac
done

if [[ -n $PORT ]]; then
    echo "P $PORT" > /proc/ratelimit
    echo "Sent port number"
fi

if [[ -n "$LIMIT" ]]; then
    echo "L $LIMIT" > /proc/ratelimit
    echo "Sent limit"
fi

if [[ -n "$WAIT" ]]; then
    echo "W $WAIT" > /proc/ratelimit
    echo "Sent wait time"
fi