#!/bin/bash

#HDD_Power0 --> GPIO17
#HDD_Power1 --> GPIO1
#HDD_Power2 --> GPIO6
#HDD_Power3 --> GPIO7
#HDD_Power4 --> GPIO25 or GPIO56
#HDD_Power5 --> GPIO32
#HDD_Power6 --> GPIO33
#HDD_Power7 --> GPIO34

gpio_enable()
{
    local export_folder="/sys/class/gpio/gpio$1"
    if [ ! -d "$export_folder" ]; then
       echo $1 > /sys/class/gpio/export 2>/dev/null
    fi
    if [ ! -d "$export_folder" ]; then
       return 0
    fi
    if [ "$(cat $export_folder/direction)" != "out" ]; then
       echo "out" > "$export_folder/direction"
    fi
    if [ "$(cat $export_folder/value)" != "1" ]; then
       echo "1" > "$export_folder/value"
    fi
}

modprobe gpio-ich

bay_gpios="1 6 7 17 25 32 33 34 56"

gpio_chips="$(ls /sys/class/gpio/ | grep gpiochip)"
gpio_base=""
for chip in $gpio_chips
do
    dir="/sys/class/gpio/$chip"
    if [ "$(cat $dir/label)" == "gpio_ich" ]; then
        gpio_base="$(cat $dir/base)"
    fi
done

if [ "$gpio_base" == "" ]; then
    echo "ich gpio chip not found"
    exit
fi

for gpio in $bay_gpios
do
    gpio_enable "$(($gpio_base+$gpio))"
done
