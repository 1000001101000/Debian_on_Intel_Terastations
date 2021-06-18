ln -s /usr/local/bin/update_boot.sh /etc/initramfs/post-update.d/update_boot
ln -s /usr/local/bin/micon_scripts/micon_shutdown.py /lib/systemd/system-shutdown/micon_shutdown
systemctl enable hotswap.service

## do we need logic to ensure only the needed modules for a given device are installed?

modules="gpio_dnv gpio_it87 gpio_ich it87ts"

for mod in $modules
do
  dkms add -m "$mod" -v 1.0
  dkms build -m "$mod" -v 1.0
  dkms install -m "$mod" -v 1.0
done

systemctl enable micon_boot.service
udevadm trigger
/usr/local/bin/update_boot.sh
exit 0
