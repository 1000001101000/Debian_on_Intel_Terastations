mount -t proc none /target/proc/
mount -t sysfs none /target/sys/
cp /source/*.sh /target/usr/local/bin/
cp /source/micro-evtd /target/usr/local/bin/

modules="gpio_dnv gpio_it87 gpio_ich it87ts"

for mod in $modules
do
  cp -r "/source/modules/$mod" "/target/usr/src/$mod-1.0/"
done

cp /source/*.service /target/etc/systemd/system/
cp /source/it8721.conf /target/etc/sensors.d/
cp -r /source/micon_scripts /target/usr/local/bin/
cp /source/micon_scripts/*.service /target/etc/systemd/system/
echo it87ts >> /target/etc/modules
echo gpio-it87 >> /target/etc/modules
echo gpio-ich >> /target/etc/modules
echo pinctrl-dnv >> /target/etc/modules
mkdir -p /target/etc/initramfs/post-update.d/

exit 0
