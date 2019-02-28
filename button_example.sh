#!/bin/bash

modprobe gpio-it87

get_gpio_value()
{
    local export_folder="/sys/class/gpio/$1"
    if [ ! -d "$export_folder" ]; then
       return ""
    fi
    #if [ "$(cat $export_folder/direction)" != "in" ]; then
    #   echo "in" > "$export_folder/direction"
    #fi
    echo "$(cat $export_folder/value)"
    return 0
}

gpio_chips="$(ls /sys/class/gpio/ | grep gpiochip)"
gpio_base=""
for chip in $gpio_chips
do
    dir="/sys/class/gpio/$chip"
    if [ "$(cat $dir/label)" == "gpio_it87" ]; then
        gpio_base="$(cat $dir/base)"
    fi
done

if [ "$gpio_base" == "" ]; then
    echo "it87 gpio chip not found"
    exit
fi

gpio_offsets="20 21"
gpio_list=""

for gpio in $gpio_offsets
do
    gpio_list="$gpio_list $(($gpio_base+$gpio))"
done

new_array=""
for gpio in $gpio_list
do
    echo "export gpio $gpio -> $(ls -lrt /sys/class/gpio | tail -n 3 | head -n 1 | gawk '{print $9}')"
    echo $gpio > /sys/class/gpio/export 2>/dev/null
done

clear
export_list="$(ls /sys/class/gpio | grep it87)"

while true
do
x=0
    for gpio in $export_list
    do
        echo "$exclusion" | grep -c $gpio > /dev/null
        if [ $? -eq 0 ]; then
            continue
        fi
        x=$x+1
        tmp_val="$(get_gpio_value $gpio)"
        #echo $tmp_val
        if [ "${new_array[$x]}" != "$tmp_val" ] && [ "${new_array[$x]}" != "" ]; then
            echo "$gpio" "=" "${new_array[$x]}""->""$tmp_val"
        fi
        new_array[$x]=$tmp_val
    done
    sleep 1
done
