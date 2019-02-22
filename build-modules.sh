## intel gpio drivers
opts="CONFIG_GPIO_ICH=m"
modules="gpio-ich"

num_cpu="$(cat /proc/cpuinfo | grep -e ^processor  | wc -l)"
kernels="$(ls /lib/modules)"

#quick and dirty check that internet is up so that apt calls can succeed
for x in {1..10}
do
    ping -c 4 google.com >/dev/null 2>&1
    if [ $? -eq 0 ]; then
      break
    fi
    sleep 10
done

for kernel in $kernels
do
    k_ver="$(echo $kernel | cut -d'.' -f1-2)"
    k_ver_long="$(echo $kernel | cut -d'-' -f1-2)"
    for module in $modules
    do
        find /lib/modules/$kernel/ | grep $module.ko > /dev/null
        if [ $? -eq 0 ]; then
            continue
        fi
        apt-get install -yq linux-headers-$kernel linux-source-$k_ver linux-headers-$k_ver_long-common ##> /dev/null
        cd /usr/src
        cp -rf linux-headers-$kernel build-temp-$kernel
        cd build-temp-$kernel
        cp -rf ../linux-headers-$k_ver_long-common/* .
        src_path="$(tar tf ../linux-source-$k_ver.tar.xz | grep $module)"
	src_dir="$(dirname $src_path)"
        tar xvf ../linux-source-$k_ver.tar.xz --wildcards --strip-components=1 $src_dir/*
        src_dir="${src_dir#*/}"
        make -j $num_cpu M="$src_dir" $opts
        cp -v $src_dir/$module.ko /lib/modules/$kernel/kernel/
        insmod /lib/modules/$kernel/kernel/$module.ko
    done
    dpkg --purge linux-source-$k_ver
    if [ -d "/usr/src/build-temp-$kernel/" ]; then
       rm -r /usr/src/build-temp-$kernel/
    fi
    depmod -a
done
