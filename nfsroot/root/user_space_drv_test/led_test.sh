#!/bin/ash

while true
do
	echo "1"
        echo 1 > /sys/class/leds/anisyan_led/brightness
        sleep 1
	echo "0"
        echo 0 > /sys/class/leds/anisyan_led/brightness
        sleep 1

done
