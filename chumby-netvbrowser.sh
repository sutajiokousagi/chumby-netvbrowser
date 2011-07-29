#!/bin/sh


case "$1" in
	start)
		# hide the ugly initialization sequence
		setbox 0 0 1 1

		# prefix chroma key
		fpga_ctl w 0xc 2
		fpga_ctl w 0xd 240
		fpga_ctl w 0xe 0
		fpga_ctl w 0xf 240

		# use irkb driver as a normal keyboard		
		export QWS_KEYBOARD=chumbyirkb

		# [Temp] Hide the flash player
		setplayer c 0 0 1 1
		setplayer p

		# [Temp] Mute the browser alignment complains
		echo 2 > /proc/cpu/alignment

		# Start in the background so we don't hog the console
		NeTVBrowser -qws SetUrl http://localhost 2>&1 > /dev/null &

		# fullscreen
		setbox 0 0 1279 719
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

