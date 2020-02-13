#!/bin/ash

# iterator for all cycles
i=0
i1=0
i2=0
cnt_time_read=5

echo "Make sure that you have already loaded driver to kernel or compiled kernel"
echo "with embedded RTC ds3231 driver and press something..."

read
echo

echo "===> READ FROM RTC:"
cur_time=`hwclock -r`
echo "Current RTC ds3231: $cur_time"
while [ $i -ne $cnt_time_read ]; do

    let "i=i+1"
    hwclock -r
    sleep 1
    echo
done

echo 
echo "===> SET TIME TO RTC: 1995-08-29 22:15:30"
date -s "1995-08-29 22:15:30"
hwclock -w
sleep 1
while [ $i1 -ne $cnt_time_read ]; do

    let "i1=i1+1"
    hwclock -r
    sleep 1
    echo
done

echo
echo "===> SET TIME TO RTC: 2020-02-14 01:24:10"
date -s "2020-02-14 01:24:10"
hwclock -w
sleep 1
while [ $i2 -ne $cnt_time_read ]; do

    let "i2=i2+1"
    hwclock -r
    sleep 1
    echo
done

