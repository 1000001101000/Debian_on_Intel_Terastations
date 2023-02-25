##requires gzip, wget, cpio, syslinux, xorriso


mkdir img
extlinuximg="stock_recovery.img"

dd if=/dev/zero of="$extlinuximg" bs=1M count=50
mkfs.vfat "$extlinuximg"
mount -o loop "$extlinuximg" ./img/
syslinuxdir="/usr/lib/syslinux/modules/bios"
for x in "syslinux.cfg" "$syslinuxdir/vesamenu.c32" "$syslinuxdir/libcom32.c32" "$syslinuxdir/libutil.c32" "$syslinuxdir/ldlinux.c32"
do
  cp "$x" ./img/
done

umount ./img/
syslinux --install "$extlinuximg"

rm -r img/
