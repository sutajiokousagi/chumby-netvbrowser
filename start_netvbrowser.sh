#!/bin/sh

# Absolute path to this script. /home/user/bin/foo.sh
SCRIPT=$(readlink -f $0)
# Absolute path this script is in. /home/user/bin
SCRIPTPATH=`dirname $SCRIPT`

# Process GET/POST variables and export to environment
if [ ! -z "$1" ]; then
	echo "Starting NeTVBrowser with $1"
	${SCRIPTPATH}/NeTVBrowser -qws SetUrl $1
else
	echo "Starting NeTVBrowser with default page"
	${SCRIPTPATH}/NeTVBrowser -qws
fi
