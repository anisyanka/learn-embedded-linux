obj-m += solution.o

all: checker.h
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) clean

load:
	sudo insmod solution.ko
	sudo dmesg -c

unload:
	sudo rmmod solution.ko
	sudo dmesg -c
