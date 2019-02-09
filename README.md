# Debian_on_Intel_Terastations
Tools for running Debian effectively on Intel-based Buffalo Terastations (TS/WS-WVHL/QVHL/6VHL/8VHL/RVHL, TS4000,TS5000)

Buffalo's Intel-based Terastations are very similar to a fairly typical PC with their built in USB and VGA ports. This allows you to Install any operating system you want on them though without some of the features provided by Buffalo's firmware. The goal of this project is to provide tools which can be used to handle some of the features not automatically dealt with by a normal Debian install.

The specific issues I'm currently addressing are:

### Hotswap bays
When these devices startup they scan each drive bay to determine which have drives and enable power for just those bays. Normally the ability to add drives to an empty bay is handled by Buffalo's software. To replicate this behaviour under Debian it is necessary to add a kernel module which allows us to access the GPIO pins used to detect drives and enable power to the bays.

I've put together a script to automate builing that kernel module and another script which enables power to all the bays at startup. 

### Boot loader
Typically you'll want to have your boot/root partitions replicated across each disk in the device so that the device can still boot in the event that the first drive fails. Although Debian will automatically update your Grub configuration for you when a configuration change is needed it will only install the updated Grub to one drive. I've put together a script which autmations scanning for drives with Grub setup and updates all of them with the latest configuration.

### Headless installer
Since you can hook up a keyboard and monitor directly to these devices you can use one of the installer images provided by Debian, but I'm planning to provide a custom image which allows headless installs over ssh and automatically includes the scripts from this project.
