#!/bin/sh


case "$1" in
	start)
		# Start in the background so we don't hog the console
		# This app uses QtGui hence requires -qws option,
		# but does not render anything to the framebuffer
		NeTVBrowser -qws 2>&1 2> /dev/null&
		;;

	stop)
		killall NeTVBrowser 2> /dev/null
		;;

	*)

		echo "Usage: $0 {start|stop}"
		;;
esac

