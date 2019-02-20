# Debian_on_Intel_Terastations
Tools for running Debian effectively on Intel-based Buffalo Terastations (TS/WS-WVHL/QVHL/6VHL/8VHL/RVHL, TS4000,TS5000)

Buffalo's Intel-based Terastations are very similar to a fairly typical PC with their built in USB and VGA ports. This allows you to Install any operating system you want on them though without some of the features provided by Buffalo's firmware. The goal of this project is to provide tools which can be used to handle some of the features not automatically dealt with by a normal Debian install.

The specific issues I'm currently addressing are:

### Hotswap bays (hotswap_enable.sh, hotswap.service and build-modules.sh)
When these devices startup they scan each drive bay to determine which have drives and enable power for just those bays. Normally the ability to add drives to an empty bay is handled by Buffalo's software. To replicate this behaviour under Debian it is necessary to add a kernel module which allows us to access the GPIO pins used to detect drives and enable power to the bays.

* build-modules.sh - A script which automatically builds and installs the missing GPIO driver needed to enable/disable drive bays and detect drive insert/removal. This is from my other project (https://github.com/1000001101000/Debian_Module_Builder)
* hotswap_enable.sh - A script which calls build-modules.sh to ensure the GPIO driver is loaded then makes sure all the device's drive bays are enabled. This also serves as an example of how to enable/disable bays from userspace.
* hotswap.service - A systemd service which runs hotswap_enable.sh at startup.

To install these manually:
* Copy hotswap_enable.sh and build-modules.sh to /usr/local/bin/
* Copy hotswap.service to /etc/systemd/system/
* Run systemctl enable hotswap.service

### Boot loader
Typically you'll want to have your boot/root partitions replicated across each disk in the device so that the device can still boot in the event that the first drive fails. Although Debian will automatically update your Grub configuration for you when a configuration change is needed it will only install the updated Grub to one drive. I've put together a script which autmations scanning for drives with Grub setup and updates all of them with the latest configuration.

### Headless installer
Since you can hook up a keyboard and monitor directly to these devices you can use one of the installer images provided by Debian, but I'm planning to provide a custom image which allows headless installs over ssh and automatically includes the scripts from this project.
