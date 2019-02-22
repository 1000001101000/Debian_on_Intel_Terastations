#!/bin/bash

#452 ata2
#457 ata3
#458 ata4
#468 ata1
#476/507 ata5
#483 ata6
#484 ata7
#485 ata8

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

model_gpios="480 481 482"
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
gpio_enable 468
gpio_enable 452

## if device reports as 2-bay exit
if [ "$model" == "000" ]; then
    exit
fi

# enable next 2 bays
gpio_enable 457
gpio_enable 458

## if device reports as 4-bay exit
if [ "$model" == "100" ] || [ "$model" == "001" ]; then
    exit
fi

# enable next 2 bays
gpio_enable 476
gpio_enable 507
gpio_enable 483

## if device reports as 6-bay exit
if [ "$model" == "010" ]; then
    exit
fi

gpio_enable 484
gpio_enable 485

