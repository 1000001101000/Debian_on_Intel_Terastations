##requires gzip, wget, cpio, syslinux, xorriso

distro="bookworm"

mkdir debian-files output
rm -r payload/
mkdir -p payload/source

cd debian-files
if [ -d "tmp" ]; then
   rm -r "tmp/"
fi

wget -N "https://deb.debian.org/debian/dists/$distro/main/installer-amd64/current/images/netboot/debian-installer/amd64/initrd.gz"
wget -N "https://deb.debian.org/debian/dists/$distro/main/installer-amd64/current/images/netboot/debian-installer/amd64/linux"
cd ..

cp preseed.cfg payload/
cp ../../../*.sh payload/source/
cp ../../../*.service payload/source/
cp ../../../it8721.conf payload/source/
cp -r ../../../micon_scripts payload/source/
cp ../../../micro-evtd payload/source/
cp -r ../../../Tools/modules payload/source/

cp debian-files/initrd.gz .
if [ $? -ne 0 ]; then
        echo "failed to retrieve initrd.gz, quitting"
        exit
fi

gunzip initrd.gz
if [ $? -ne 0 ]; then
        echo "failed to unpack initrd.gz, quitting"
        exit
fi
cd payload
find . | cpio -v -H newc -o -A -F ../initrd
if [ $? -ne 0 ]; then
        echo "failed to patch initrd.gz, quitting"
        exit
fi
cd ..
#gzip initrd
cat initrd | xz --check=crc32 -9e > initrd.xz
if [ $? -ne 0 ]; then
        echo "failed to pack initrd, quitting"
        exit
fi

rm output/*
mkdir img
extlinuximg="output/ts-$distro-installer.img"

dd if=/dev/zero of="$extlinuximg" bs=1M count=45
mkfs.vfat "$extlinuximg"
mount -o loop "$extlinuximg" ./img/
syslinuxdir="/usr/lib/syslinux/modules/bios"
for x in "initrd.xz" "debian-files/linux" "$syslinuxdir/vesamenu.c32" "$syslinuxdir/libcom32.c32" "$syslinuxdir/libutil.c32"
do
  cp "$x" ./img/
done

cfg="./img/syslinux.cfg"
echo "ui vesamenu.c32" 				> "$cfg"
echo "TIMEOUT 20"				>> "$cfg"
echo "label debian-installer"			>> "$cfg"
echo "      menu label Debian $distro Installer">> "$cfg"
echo "      menu default"			>> "$cfg"
echo "      kernel /linux"			>> "$cfg"
echo "      initrd /initrd.xz"			>> "$cfg"
echo "Modify message"				>> "$cfg"

umount ./img/
syslinux --install "$extlinuximg"

rm -r img/
rm initrd*