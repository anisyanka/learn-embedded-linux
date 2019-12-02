MODULE_NAME=rtc
KO=$(MODULE_NAME).ko

SCRIPTS=../../scripts

obj-m += rtc.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

install:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules_install

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) clean

insert: create_node
	sudo dmesg -c
	sudo insmod $(KO)
	dmesg

remove:
	sudo dmesg -c
	sudo rmmod $(KO)
	dmesg

create_node:
	@./$(SCRIPTS)/create_rtc_nod.sh
