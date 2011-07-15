#!/bin/sh


case "$1" in
	start)
		export QWS_KEYBOARD=chumbyirkb

		# Temp fix for "Semop lock/unlock failure Identifier removed" flood
		rm -rf /tmp/qtembedded-0

		# Start in the background so we don't hog the console
		NeTVBrowser -qws -nomouse SetUrl /usr/share/netvserver/docroot/index.html 2>&1 > /dev/null &
		;;

	stop)
		killall NeTVBrowser 2> /dev/null
		;;

	restart)
		$0 stop
		$0 start
		;;

	*)

		echo "Usage: $0 {start|stop|restart}"
		;;
esac

