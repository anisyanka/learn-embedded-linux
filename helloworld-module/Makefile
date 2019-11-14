obj-m += my_module.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) clean

load:
	sudo insmod my_module.ko
	sudo dmesg -c

unload:
	sudo rmmod my_module.ko
	sudo dmesg -c
