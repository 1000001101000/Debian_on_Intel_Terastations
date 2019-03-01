#!/bin/bash

#sata0 -> GPIO2
#sata1 -> GPIO3
#sata2 -> GPIO4
#sata3 -> GPIO5
#sata4 -> GPIO15
#sata5 -> GPIO9
#sata6 -> GPIO13
#sata7 -> GPIO10

modprobe gpio-ich

get_gpio_value()
{
    local export_folder="/sys/class/gpio/gpio$1"
    if [ ! -d "$export_folder" ]; then
       echo $1 > /sys/class/gpio/export 2>/dev/null
    fi
    if [ ! -d "$export_folder" ]; then
       return ""
    fi
    if [ "$(cat $export_folder/direction)" != "in" ]; then
       echo "in" > "$export_folder/direction"
    fi
    echo "$(cat $export_folder/value)"
    return 0
}

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

gpio_offsets="2 3 4 5 15 9 13 10"
bay_gpios=""

for gpio in $gpio_offsets
do
    bay_gpios="$bay_gpios $(($gpio_base+$gpio))"
done

new_array=""

while true
do
    for gpio in $bay_gpios
    do
        tmp_val="$(get_gpio_value $gpio)"
        if [ "${new_array[$gpio]}" != "$tmp_val" ] && [ "${new_array[$gpio]}" != "" ]; then
            echo "$gpio" "=" "${new_array[$gpio]}""->""$tmp_val"
        fi
        new_array[$gpio]=$tmp_val
    done
    sleep 1
done
