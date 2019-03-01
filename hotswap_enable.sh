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
       exit
    fi
    if [ "$(cat $export_folder/direction)" != "out" ]; then
       echo "out" > "$export_folder/direction"
    fi
    if [ "$(cat $export_folder/value)" != "1" ]; then
       echo "1" > "$export_folder/value"
    fi
}

/usr/local/bin/build-modules.sh
modprobe gpio-ich

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

model_gpios="$(($gpio_base+29)) $(($gpio_base+30)) $(($gpio_base+31))"
model=""
for gpio in $model_gpios
do
    export_folder="/sys/class/gpio/gpio$gpio"
    if [ ! -d "$export_folder" ]; then
      echo $gpio > /sys/class/gpio/export
    fi
    model="$model$(cat $export_folder/value)"
done

# enable first 2 bays
gpio_enable "$(($gpio_base+17))"
gpio_enable "$(($gpio_base+1))"

## if device reports as 2-bay exit
if [ "$model" == "000" ]; then
    exit
fi

# enable next 2 bays
gpio_enable "$(($gpio_base+6))"
gpio_enable "$(($gpio_base+7))"

## if device reports as 4-bay exit
if [ "$model" == "100" ] || [ "$model" == "001" ]; then
    exit
fi

# enable next 2 bays
gpio_enable "$(($gpio_base+25))"
gpio_enable "$(($gpio_base+56))"
gpio_enable "$(($gpio_base+32))"

## if device reports as 6-bay exit
if [ "$model" == "010" ]; then
    exit
fi

gpio_enable "$(($gpio_base+33))"
gpio_enable "$(($gpio_base+34))"
