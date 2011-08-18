#!/bin/sh

OPKG_FIFO=/tmp/opkg_upgrade_fifo
UPDATE_PROGRESS_FILE=/tmp/netvbrowser_temp_upgrade

# Create the fifo if it's not already there
# Should have been done by NeTVBrowser anyway
if [ ! -e ${OPKG_FIFO} ]; then
	mkfifo ${OPKG_FIFO}
fi

# Wait for CPanel to hide & new html_page to load (1650ms x 2)
sleep 4

# Start the upgrade, pipe to fifo (blocking)
# Might restart the browser half way
mount -o remount,rw /
opkg --cache /var/lib/opkg/tmp upgrade > ${OPKG_FIFO}

# NOTE: right here there are 2 opened file handles to /tmp to OPKG_FIFO & UPDATE_PROGRESS_FILE
# Does it matter?
mount -o remount,ro /

# Wait for NeTVBrowser in case it got killed
if [ ! -z $(pidof NeTVBrowser) ];
then
	echo "NeTVBrowser is not dead"
else
	sleep 4
fi

# Wait for NeTVBrowser (2nd chance)
if [ ! -z $(pidof NeTVBrowser) ];
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
if [ -e ${OPKG_FIFO} ]; then
	rm ${OPKG_FIFO}
fi
if [ -e ${UPDATE_PROGRESS_FILE} ]; then
	rm ${UPDATE_PROGRESS_FILE}
fi

