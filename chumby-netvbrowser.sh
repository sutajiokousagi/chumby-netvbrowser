#!/bin/sh


case "$1" in
	start)
		# Start in the background so we don't hog the console
		NeTVBrowser -qws -nomouse SetUrl http://localhost 2>&1 > /dev/null &

		# Temp fix for "Semop lock/unlock failure Identifier removed" flood
		sleep 2
		rm -rf /tmp/qtembedded-0

		;;

	stop)
		killall NeTVBrowser 2> /dev/null
		;;

	*)

		echo "Usage: $0 {start|stop}"
		;;
esac

