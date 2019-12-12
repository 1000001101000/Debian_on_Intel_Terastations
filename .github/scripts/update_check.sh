#!/bin/bash

distros="Stretch Buster"

for distro in $distros
do
  curl http://ftp.nl.debian.org/debian/dists/${distro,,}/main/installer-amd64/current/images/netboot/mini.iso 2>/dev/null > /tmp/latest.txt
  diff /tmp/latest.txt installer-image/$distro/build/last_build.txt 2>/dev/null
  if [ $? -ne 0 ]; then
    echo "trigger build of $distro images"
  else
    cd installer-image/$distro/build/
    ./generate_images.sh
  fi

done
exit 0
