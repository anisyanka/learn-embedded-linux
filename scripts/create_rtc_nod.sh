#!/bin/bash

RTC_DEVICE_NAME=/dev/ds3231_rtc
RTC_DEVICE_MAJOR=500
RTC_DEVICE_MINOR=0

sudo mknod $RTC_DEVICE_NAME c $RTC_DEVICE_MAJOR $RTC_DEVICE_MINOR 2> /dev/null
sudo chmod 666 $RTC_DEVICE_NAME

echo "RTC DS3231 device nod has created"
