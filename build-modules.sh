#!/bin/bash
## intel gpio drivers
opts="CONFIG_GPIO_IT87=m CONFIG_GPIO_ICH=m CONFIG_SENSORS_IT87=m"
modules="gpio-it87 gpio-ich it87"
patches="it87-gpio.patch it87-hwmon.patch"

num_cpu="$(cat /proc/cpuinfo | grep -e ^processor  | wc -l)"
kernels="$(ls /lib/modules)"

exclusions="$(cat /usr/local/etc/module_exclude.txt)"


#quick and dirty check that internet is up so that apt calls can succeed
for x in {1..10}
do
    ping -c 2 google.com >/dev/null 2>&1
    if [ $? -eq 0 ]; then
      break
    fi
    sleep 10
done

for kernel in $kernels
do
    k_ver="$(echo $kernel | cut -d'.' -f1-2)"
    k_ver_long="$(echo $kernel | cut -d'-' -f1-2)"
    tmpmods=""
    for module in $modules
    do
        if [ -f "/lib/modules/$kernel/kernel/$module.ko" ]; then
           continue
        fi
        find /lib/modules/$kernel/ | grep $module.ko | xargs rm 2>/dev/null
        tmpmods="$tmpmods $module"
    done
    if [ "$tmpmods" == "" ]; then
        continue
    fi
    apt-get install -yq linux-headers-$kernel linux-source-$k_ver linux-headers-$k_ver_long-common > /dev/null
    cd /usr/src
    cp -rf linux-headers-$kernel build-temp-$kernel
    cd build-temp-$kernel
    cp -rf ../linux-headers-$k_ver_long-common/* .
    dirlist=()
    tar tf ../linux-source-$k_ver.tar.xz > tmpfilelist
    for module in $tmpmods
    do
        src_path="$(cat tmpfilelist | grep $module.c)"
	src_dir="$(dirname $src_path)"
        dirlist+=($src_dir)
    done
    dirlist=$(echo "${dirlist[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' ')

    for src_dir in $dirlist
    do
        tar xvf ../linux-source-$k_ver.tar.xz --wildcards --strip-components=1 $src_dir/*
    done
    for patch in $patches
    do
        patch -t -p1 < /usr/src/$patch
    done
    for src_dir in $dirlist
    do
        src_dir="${src_dir#*/}"
        make -j $num_cpu M="$src_dir" $opts $exclusions
    done
    for module in $tmpmods
    do
        echo "$(find | grep /$module.ko)"
        cp -v "$(find | grep /$module.ko)" /lib/modules/$kernel/kernel/
        rmmod $module
        insmod /lib/modules/$kernel/kernel/$module.ko
    done
    dpkg --purge linux-source-$k_ver
    if [ -d "/usr/src/build-temp-$kernel/" ]; then
       rm -r /usr/src/build-temp-$kernel/
    fi
    depmod -a
done
