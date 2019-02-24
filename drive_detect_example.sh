#!/bin/bash

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

bay_gpios="453 454 455 456 460 461 462 464 467 469 471"
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
