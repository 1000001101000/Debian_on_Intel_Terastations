#!/bin/bash

###ICH GPIO for V-Series, 5000 series and TS7000/TS-2RZ
#HDD_Power0 --> GPIO17
#HDD_Power1 --> GPIO1
#HDD_Power2 --> GPIO6
#HDD_Power3 --> GPIO7
#HDD_Power4 --> GPIO25 or GPIO56
#HDD_Power5 --> GPIO32
#HDD_Power6 --> GPIO33
#HDD_Power7 --> GPIO34

### Denverton GPIO for TS5020 and TS6000
#HDD_Power0 --> GPIO31
#HDD_Power1 --> GPIO32
#HDD_Power2 --> GPIO22
#HDD_Power3 --> GPIO23
#HDD_Power4 --> GPIO92
#HDD_Power5 --> GPIO90

ich_gpios="1 6 7 17 25 32 33 34 56"
dnv_gpios="31 32 22 23 92 90"

gpio_set()
{
    ## $1 = GPIO#
    ## $2 = value
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
    if [ "$(cat $export_folder/value)" != "$2" ]; then
       echo "$2" > "$export_folder/value"
    fi
}

modprobe gpio-ich
modprobe pinctrl_dnv

gpio_chips="$(ls /sys/class/gpio/ | grep gpiochip)"
gpio_base=""
bay_gpios=""
for chip in $gpio_chips
do
    dir="/sys/class/gpio/$chip"
    label="$(cat $dir/label)"

    if [ "$label" == "gpio_ich" ]; then
        bay_gpios="$ich_gpios"
        value="1"
        gpio_base="$(cat $dir/base)"
    fi

    if [ "$label" == "gpio_dnv.0" ]; then
        bay_gpios="$dnv_gpios"
        value="1"
        gpio_base="$(cat $dir/base)"
    fi

done

if [ "$gpio_base" == "" ]; then
    echo "gpio chip not found"
    exit
fi

for gpio in $bay_gpios
do
    gpio_set "$(($gpio_base+$gpio))" "$value"
done
