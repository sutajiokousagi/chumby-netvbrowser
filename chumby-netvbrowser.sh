#!/bin/sh


case "$1" in
	start)
		# prefix chroma key
		fpga_ctl w 0xd 240
		fpga_ctl w 0xe 0
		fpga_ctl w 0xf 240
		fpga_ctl w 0xc 2

		# [Temp] bottom pink line
		regutil -w LCD_SPU_V_PORCH=0x50005

		# [Temp] Hide the flash player
		if [ ! -z $(pidof chumbyflashplayer.x) ];
		then
			setplayer c 0 0 1 1
			setplayer p
		fi

		# Use irkb and resetkb driver as a normal keyboard		
		export QWS_KEYBOARD="chumbyirkb chumbyresetkb"

		# Start in the background so we don't hog the console
		NeTVBrowser -qws > /dev/null 2>&1 &
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

