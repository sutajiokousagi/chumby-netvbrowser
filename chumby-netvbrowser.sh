#!/bin/sh


case "$1" in
	start)
		# enable chroma key
		fpga_ctl x i

		# [Temp] bottom pink line
		regutil -w LCD_SPU_V_PORCH=0x50005

		# Custom keyboard driver for infrared remote & on-board Setup button
		inputDrivers="chumbyirkb:repeat-delay=600:repeat-rate=50 chumbyresetkb"

		# If external keyboard & mouse is connnected, use it as well
		[ -e /dev/input/event2 ] && inputDrivers="${inputDrivers} linuxinput:/dev/input/event2"
		[ -e /dev/input/event3 ] && inputDrivers="${inputDrivers} linuxinput:/dev/input/event3"
		[ -e /dev/input/event4 ] && inputDrivers="${inputDrivers} linuxinput:/dev/input/event4"
		[ -e /dev/input/event5 ] && inputDrivers="${inputDrivers} linuxinput:/dev/input/event5"
		export QWS_KEYBOARD="${inputDrivers}"

		# Transformed screen driver supporting screen rotation
		export QWS_DISPLAY="transformed"

		# Start in the background so we don't hog the console
		NeTVBrowser -qws -nomouse > /dev/null 2>&1 &

		# To use a different default URL
		# Remember to implement a JavaScript function "fCheckAlive();" to return 'true' to keep the page alive
		# See http://wiki.chumby.com/index.php/NeTV_web_services#KeepAlive
		#NeTVBrowser -qws -nomouse SetUrl http://www.yoururl.com > /dev/null 2>&1 &

		;;

	stop)
		killall NeTVBrowser > /dev/null 2>&1 &
		;;

	restart)
		$0 stop
		sleep 2
		$0 start
		;;

	*)

		echo "Usage: $0 {start|stop|restart}"
		;;
esac

