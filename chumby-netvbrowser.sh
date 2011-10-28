#!/bin/sh


case "$1" in
	start)
		# enable chroma key
		fpga_ctl w 0xc 2

		# [Temp] bottom pink line
		regutil -w LCD_SPU_V_PORCH=0x50005

		# Use irkb and resetkb driver as a normal keyboard		
		export QWS_KEYBOARD="chumbyirkb:repeat-delay=600:repeat-rate=50 chumbyresetkb"

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
		$0 start
		;;

	*)

		echo "Usage: $0 {start|stop|restart}"
		;;
esac

