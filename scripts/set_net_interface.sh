#!/bin/bash

#
# This interface will be UP when device with MAC=f8dc7a000001 was plugged
#
nmcli con add type ethernet ifname enxf8dc7a000001 ip4 192.168.0.1/24

#
# To install NFS server:
# - Disable v4 into /etc/default/nfs-kernel-server with help `--no-nfs-version 4`
# - edit the /etc/exports file as root to add the following lines
#   /home/<user>/linux-kernel-labs/modules/nfsroot 192.168.0.100(rw,no_root_squash,no_subtree_check)
# - sudo systemctl restart nfs-config
# - sudo /etc/init.d/nfs-kernel-server restart
#
# - sudo chown nobody:nogroup /mnt/sharedfolder
# - sudo chmod 777 /mnt/sharedfolder
#

