#!/bin/bash

modprobe gpio-ich
modprobe gpio-it87

## it87_gp34 = function button
## it87_gp35 = display button

#### ich gpios:

#boot switch -> GPIO12

#sata0 -> GPIO2
#sata1 -> GPIO3
#sata2 -> GPIO4
#sata3 -> GPIO5
#sata4 -> GPIO15
#sata5 -> GPIO9
#sata6 -> GPIO13
#sata7 -> GPIO10

#also possibly:
#sata5 -> GPIO16
#sata6 -> GPIO18
#sata7 -> GPIO11
#sata7 -> GPIO20

## board id:
#      GPIO31   GPIO30   GPIO29        SKU
#      ------   ------   ------     ----------
#        0        0        0         2HDD box
#        0        0        1         4HDD box
#        0        1        0         6HDD box
#        0        1        1         8HDD box
#        1        0        0         1U


get_gpio_value()
{
    local export_folder="/sys/class/gpio/$1"
    if [ ! -d "$export_folder" ]; then
       return 0
    fi
    if [ "$(cat $export_folder/direction)" != "in" ]; then
       echo "in" > "$export_folder/direction"
    fi
    echo "$(cat $export_folder/value)"
    return 0
}

gpio_chips="$(ls /sys/class/gpio/ | grep gpiochip)"
it87_gpio_base=""
ich_gpio_base=""
for chip in $gpio_chips
do
    dir="/sys/class/gpio/$chip"
    if [ "$(cat $dir/label)" == "gpio_it87" ]; then
        it87_gpio_base="$(cat $dir/base)"
    fi
    if [ "$(cat $dir/label)" == "gpio_ich" ]; then
        ich_gpio_base="$(cat $dir/base)"
    fi
done

##offsets corresponding to 34/35
it87_offsets="20 21"

ich_offsets="2 3 4 5 9 10 11 12 13 15 16 18 20 29 30 31"
gpio_list=""
export_list=""
sku=""

if [ "$it87_gpio_base" != "" ]; then
for gpio in $it87_offsets
    do
        gpio_list="$gpio_list $(($it87_gpio_base+$gpio))"
    done
fi

if [ "$ich_gpio_base" != "" ]; then
for gpio in $ich_offsets
    do
        gpio_list="$gpio_list $(($ich_gpio_base+$gpio))"
        export_list="$export_list gpio$(($ich_gpio_base+$gpio))"
    done
    sku+="$(get_gpio_value gpio$(($ich_gpio_base+31)))"
    sku+="$(get_gpio_value gpio$(($ich_gpio_base+30)))"
    sku+="$(get_gpio_value gpio$(($ich_gpio_base+29)))"
fi
echo "$(dmidecode --type 11 | grep String\ | cut -d":" -f2)""$(dmidecode --type 0 | grep Version)"
if [ "$sku" == "000" ]; then
    echo "BoardID: $sku = 2HDD box"
fi
if [ "$sku" == "001" ]; then
    echo "BoardID: $sku = 4HDD box"
fi
if [ "$sku" == "010" ]; then
    echo "BoardID: $sku = 6HDD box"
fi
if [ "$sku" == "011" ]; then
    echo "BoardID: $sku = 8HDD box"
fi
if [ "$sku" == "100" ]; then
    echo "BoardID: $sku = 1U box"
fi

new_array=""
for gpio in $gpio_list
do
    echo $gpio > /sys/class/gpio/export 2>/dev/null
done

##assumes all it87 gpio where exported by this script
export_list="$export_list $(ls /sys/class/gpio | grep it87)"

while true
do
x=0
    for gpio in $export_list
    do
        x=$x+1
        tmp_val="$(get_gpio_value $gpio)"
        if [ "${new_array[$x]}" != "$tmp_val" ] && [ "${new_array[$x]}" != "" ]; then
            echo "$gpio" "=" "${new_array[$x]}""->""$tmp_val"
        fi
        new_array[$x]=$tmp_val
    done
    sleep 1
done
