#!/bin/sh

# Absolute path to this script. /home/user/bin/foo.sh
SCRIPT=$(readlink -f $0)

# Copy this script to /tmp and execute from there
if [[ ${SCRIPT:0:8} == "/usr/bin" ]]
then
	rm -f /tmp/upgrade-script.sh
	cp $SCRIPT /tmp/upgrade-script.sh
	exec /tmp/upgrade-script.sh
fi

echo "Executing upgrade script from ${SCRIPT}..."

UPDATE_PROGRESS_FILE=/tmp/netvbrowser_temp_upgrade

# Wait for CPanel to hide & new html_page to load (1650ms x 2)
sleep 4

# Start the upgrade, pipe to fifo (blocking)
# Might restart the browser half way
mount -o remount,rw /
opkg-chumby-upgrade > /tmp/upgrade.$$.log 2>&1

# XXX HACK XXX
# We're having problems where / is not remounting as ro.
# This hack forces the two chumby processes to restart.
if ! mount -o remount,ro /
then
	logger -t update "Unable to remount / as ro.  Stopping netvbrowser and netvserver and trying again..."
	/etc/init.d/chumby-netvbrowser stop
	/etc/init.d/chumby-netvserver stop
	sync
	if ! mount -oremount,ro /
	then
		logger -t update "Critical error!  Root still mounted rw!"
	else
		logger -t update "Managed to mount / as ro"
	fi
	/etc/init.d/chumby-netvbrowser start
	/etc/init.d/chumby-netvserver start
fi

# Wait for NeTVBrowser in case it got killed
if [ ! -z "$(pidof NeTVBrowser)" ];
then
	echo "NeTVBrowser is not dead"
else
	sleep 4
fi

# Wait for NeTVBrowser (2nd chance)
if [ ! -z "$(pidof NeTVBrowser)" ];
then
	echo "NeTVBrowser is still alive"
else
	sleep 4
fi


# Tell the browser that we are done here
# The browser should have reloaded the html_update page &
# load the upgraded package list from /tmp/
NeTVBrowser -qws UpdateDone &

# Wait for browser to perform cleaning up & slide out update page (1650ms)
sleep 2

# Wipe opkg cache folder
rm -rf /var/lib/opkg/tmp/*

# Cleaning up
# Should have been done by NeTVBrowser anyway
if [ -e ${UPDATE_PROGRESS_FILE} ]; then
	rm ${UPDATE_PROGRESS_FILE}
fi
