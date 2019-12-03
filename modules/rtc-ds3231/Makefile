MODULE_NAME=rtc
KO=$(MODULE_NAME).ko

SCRIPTS=../../scripts
KROOT=/lib/modules/$(shell uname -r)/build

OBJ = rtc_main.o rtc_fops.o

obj-m += ds3231_rtc.o
ds3231_rtc-objs := $(OBJ)

all:
	sudo make -C $(KROOT) M=$(shell pwd) modules

install:
	sudo make -C $(KROOT) M=$(shell pwd) modules_install

clean:
	sudo make -C $(KROOT)  M=$(shell pwd) clean

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
