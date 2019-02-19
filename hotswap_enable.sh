#!/bin/bash

#452 ata2
#457 ata3
#458 ata4
#468 ata1
#476/507 ata5
#483 ata6
#484 ata7
#485 ata8

/usr/local/bin/build-modules.sh

gpio_list="452 457 458 468 476 483 484 485 507"
modprobe gpio-ich

for gpio in $gpio_list
do
    echo $gpio > /sys/class/gpio/export
    export_folder="/sys/class/gpio/gpio$gpio"

    echo "out" > "$export_folder/direction"
    echo "1" > "$export_folder/value"
done



