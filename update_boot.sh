#! /bin/bash

##requires mdadm grep basename gawk lsblk grub2 sort

#look at devices mounted as "/" and "/boot"
boot_dev="$(grep " /boot " /proc/mounts | gawk '{print $1}')"
root_dev="$(grep " / " /proc/mounts | gawk '{print $1}')"

#determine if there is a seperate boot device, if not use device housing root
if [ "$boot_dev" == "" ]; then
   boot_dev="$root_dev"
fi

#determine if boot device is an md array
is_md="$(grep -c "$(basename "$boot_dev") :" /proc/mdstat)"

#if so add all the component devices as bootloader targets.
if [ "$is_md" == "1" ]; then
   boot_dev="$(mdadm --detail $boot_dev | grep "/dev/" | gawk '{print $7}' | sort)"
fi

##determine the disk associated with the partion and add to bootloader target
temp_dev=""
for part in $boot_dev
do
     dev_name="$(lsblk -dprn -o PKNAME "$part")"
      if [ "$dev_name" != "" ]; then
         temp_dev="$temp_dev $dev_name"
      else
         temp_dev="$temp_dev $part"
      fi
done
boot_dev="$temp_dev"

update-grub

# install the latest grub config on each disk associated with /boot
for dev in $boot_dev
do
   echo "$dev:"
   grub-install "$dev"
done
