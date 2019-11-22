obj-m += solution.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) clean

insert:
	sudo dmesg -c
	sudo insmod solution.ko
	dmesg

remove:
	sudo dmesg -c
	sudo rmmod solution.ko
	dmesg
