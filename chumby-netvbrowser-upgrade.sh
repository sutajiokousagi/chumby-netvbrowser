#!/bin/sh

OPKG_FIFO=/tmp/opkg_upgrade_fifo
UPDATE_PROGRESS_FILE=/tmp/netvbrowser_temp_upgrade

# Create the fifo if it's not already there
# Should have been done by NeTVBrowser anyway
if [ ! -e ${OPKG_FIFO} ]; then
	mkfifo ${OPKG_FIFO}
fi

# Wait for CPanel to hide & new html_page to load
sleep 4

# Start the upgrade, pipe to fifo (blocking)
# Might restart the browser half way
mount -o remount,rw /
opkg --cache /var/lib/opkg/tmp upgrade > ${OPKG_FIFO}
mount -o remount,ro /

# Wait for NeTVBrowser
if ps ax | grep -v grep | grep NeTVBrowser > /dev/null
then
	echo "NeTVBrowser is not dead"
else
	sleep 4
fi

# Wait for NeTVBrowser (2nd chance)
if ps ax | grep -v grep | grep NeTVBrowser > /dev/null
then
	echo "NeTVBrowser is alive"
else
	sleep 7
fi

# Tell the browser that we are done here
# The browser should have reloaded the html_update page &
# load the upgraded package list from /tmp/
NeTVBrowser -qws UpdateDone &

# Wait for browser to perform cleaning up (1650ms)
sleep 2

# Wipe opkg cache folder
rm -rf /var/lib/opkg/tmp/*

# Cleaning up
# Should have been done by NeTVBrowser anyway
if [ -e ${OPKG_FIFO} ]; then
	rm ${OPKG_FIFO}
fi
if [ -e ${UPDATE_PROGRESS_FILE} ]; then
	rm ${UPDATE_PROGRESS_FILE}
fi

