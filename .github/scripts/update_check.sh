#!/bin/bash

distros="Stretch Buster"
svpwd="$(pwd)"

for distro in $distros
do
  curl http://ftp.nl.debian.org/debian/dists/${distro,,}/main/installer-amd64/current/images/netboot/mini.iso 2>/dev/null | md5sum > /tmp/latest.txt
  diff /tmp/latest.txt installer-image/$distro/build/last_build.txt 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "image is up to date"
    continue
  else
    cd installer-image/$distro/build/
    sudo ./generate_images.sh
  fi
  if [ $? -eq 0 ] && [ -f "output/ts-${distro,,}-installer.iso" ]; then
    cp "output/ts-${distro,,}-installer.iso" ..
    cp "/tmp/latest.txt" "last_build.txt"
    git commit -a -m "generate images based on latest debian installer" 
    echo "::set-output name=commit_needed::yes"
  fi
  cd "$svpwd"
done
exit 0
