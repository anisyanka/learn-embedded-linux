#!/bin/bash
#
# A TFTP (Trivial File Transfer Protocol) server can be used to test kernel 
# images on your embedded target and speed up development.
#
# See https://wiki.beyondlogic.org/index.php?title=Setup_TFTP_Server
#

echo "# description: The tftp server serves files using the trivial file transfer" >> /etc/xinetd.d/tftp
echo "#       protocol.  The tftp protocol is often used to boot diskless" >> /etc/xinetd.d/tftp
echo "#       workstations, download configuration files to network-aware printers," >> /etc/xinetd.d/tftp
echo "#       and to start the installation process for some operating systems." >> /etc/xinetd.d/tftp
echo "service tftp" >> /etc/xinetd.d/tftp
echo "{" >> /etc/xinetd.d/tftp
echo "        socket_type             = dgram" >> /etc/xinetd.d/tftp
echo "        protocol                = udp" >> /etc/xinetd.d/tftp
echo "        wait                    = yes" >> /etc/xinetd.d/tftp
echo "        user                    = root" >> /etc/xinetd.d/tftp
echo "        server                  = /usr/sbin/in.tftpd" >> /etc/xinetd.d/tftp
echo "        server_args             = -s /home/cpeacock/export" >> /etc/xinetd.d/tftp
echo "#       disable                 = yes" >> /etc/xinetd.d/tftp
echo "        per_source              = 11" >> /etc/xinetd.d/tftp
echo "        cps                     = 100 2" >> /etc/xinetd.d/tftp
echo "}" >> /etc/xinetd.d/tftp

sudo service xinetd restart
