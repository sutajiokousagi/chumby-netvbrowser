#!/bin/sh


case "$1" in
	start)
		# Start in the background so we don't hog the console
		NeTVBrowser -qws -nomouse SetUrl http://localhost 2>&1 > /dev/null &
		;;

	stop)
		killall NeTVBrowser 2> /dev/null
		;;

	*)

		echo "Usage: $0 {start|stop}"
		;;
esac

