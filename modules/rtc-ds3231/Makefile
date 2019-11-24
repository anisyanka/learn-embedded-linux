MODULE_NAME=rtc
KO=$(MODULE_NAME).ko

obj-m += rtc.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

install:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules_install

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) clean

insert:
	sudo dmesg -c
	sudo insmod $(KO)
	dmesg

remove:
	sudo dmesg -c
	sudo rmmod $(KO)
	dmesg
