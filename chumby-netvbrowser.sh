#!/bin/sh


case "$1" in
	start)
		# [Temp] Hide the flash player
		setplayer c 0 0 1 1
		setplayer p

		# Start in the background so we don't hog the console
		NeTVBrowser SetUrl http://localhost 2>&1 > /dev/null &
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

