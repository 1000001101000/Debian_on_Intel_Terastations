micon_version="$(/source/micro-evtd -s 8083)"

## diable startup watchdog
/source/micro-evtd -s 0003

##clear alert leds if any set power led
/source/micro-evtd -s 0250090f96,02520000ac,02510d00a0

## set lcd display to installer message
/source/micro-evtd -s 20905465726173746174696f6e207838362044656269616e20496e7374616c6c6572,0025,013220,013aff

##clear alert leds if any set power led
/source/micro-evtd -s 0250090f96,02520000ac,02510d00a0

##try changing the lcd color
/source/micro-evtd -s 02500007,02510006


exit 0
