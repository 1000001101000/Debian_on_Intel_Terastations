#!/bin/bash

##requires mdadm grep basename gawk lsblk grub-install sort lvm2

# 1. Find where /boot or / is mounted
boot_dev="$(grep " /boot " /proc/mounts | gawk '{print $1}')"
root_dev="$(grep " / " /proc/mounts | gawk '{print $1}')"

if [ "$boot_dev" == "" ]; then
   boot_dev="$root_dev"
fi

# 2. Function to recursively resolve down to the actual physical hard drives
resolve_to_physical_disks() {
    local current_dev="$1"

    # If LVM: Find the underlying Physical Volumes (which are your RAID devices)
    if [[ "$current_dev" == *"/dev/mapper/"* ]] || [[ "$current_dev" == *"/dev/dm-"* ]]; then
        local vg_name
        vg_name="$(lvs --noheadings -o vg_name "$current_dev" 2>/dev/null | xargs)"
        if [ "$vg_name" != "" ]; then
            current_dev="$(pvs --noheadings -o pv_name --select vg_name=$vg_name 2>/dev/null)"
        fi
    fi

    local final_disks=""

    # Loop through each device (in case the LVM spans multiple RAID arrays)
    for dev in $current_dev
    do
        local base_name="$(basename "$dev")"

        # If the device is an MD RAID array: Find the partitions that build it
        if grep -q "^${base_name} *:" /proc/mdstat 2>/dev/null; then
            local raid_parts="$(mdadm --detail "$dev" 2>/dev/null | grep "/dev/" | gawk '{print $7}')"

            # For each partition in the RAID, find its physical parent disk (e.g., /dev/sda1 -> /dev/sda)
            for part in $raid_parts
            do
                local parent="$(lsblk -dprn -o PKNAME "$part" 2>/dev/null)"
                if [ "$parent" != "" ]; then
                    final_disks="$final_disks $parent"
                else
                    final_disks="$final_disks $part"
                fi
            done
        else
            # If it is not a RAID array, run a standard lsblk lookup on the device
            local parent="$(lsblk -dprn -o PKNAME "$dev" 2>/dev/null)"
            if [ "$parent" != "" ]; then
                final_disks="$final_disks $parent"
            else
                final_disks="$final_disks $dev"
            fi
        fi
    done

    # Sort and remove duplicates to return only unique physical disks (e.g., /dev/sda /dev/sdb)
    echo "$final_disks" | tr ' ' '\n' | sort -u | xargs
}

# 3. Run the resolution function on your boot device
target_disks=$(resolve_to_physical_disks "$boot_dev")

# 4. Update the GRUB configuration
update-grub

# 5. Install GRUB to the MBR/boot sector of ALL physical disks backing the LVM/RAID structure
for disk in $target_disks
do
   echo "Found physical disk underlying LVM/RAID. Installing GRUB on: $disk"
   # --force is required because the ext2/LVM structure blocks normal blocklist embedding
   grub-install --force "$disk"
done
