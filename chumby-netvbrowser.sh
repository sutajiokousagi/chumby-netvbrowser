#!/bin/sh


case "$1" in
	start)
		export QWS_KEYBOARD=chumbyirkb

		# Temp fix for "Semop lock/unlock failure Identifier removed" flood
		rm -rf /tmp/qtembedded-0

		# Start in the background so we don't hog the console
		# if NeTVServer is not up, we have to use absolute path, else http://localhost returns nothing
		# when server is up, we should use http://localhost for POST/GET to work
		netv_started=$(ps ax | grep 'NeTVServer')
		if [ ${#netv_started} -gt 10 ]; then
			NeTVBrowser -qws -nomouse SetUrl http://localhost 2>&1 > /dev/null &
		else
			NeTVBrowser -qws -nomouse SetUrl /usr/share/netvserver/docroot/index.html 2>&1 > /dev/null &
		fi
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

