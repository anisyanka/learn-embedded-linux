#!/bin/sh

echo "Make sure that you have already loaded driver to kernel or have compiled kernel"
echo "with embedded RTC ds3231 driver and press something..."

read
echo

echo
echo "===> SET TIME TO RTC: 2020-03-16 00:03:50"
date -s "2020-03-16 00:03:50"
hwclock -w

echo
echo "===> SET ALARM TIME: 2020-03-16 00:03:55"
echo 0 > /sys/class/rtc/rtc0/device/alarm_int_enabled
echo "Mon 16 00:03:55 2020" > /sys/class/rtc/rtc0/device/alarm_time
echo 1 > /sys/class/rtc/rtc0/device/alarm_int_enabled
echo
echo
echo "WAIT DS3231 INTERRUPT LOG THROUGH .... 5 SECONDS"
sleep 1
echo "4..."
sleep 1
echo "3..."
sleep 1
echo "2..."
sleep 1
echo "1..."
