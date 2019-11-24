obj-m += strmodule.o
obj-m += strtest.o

# environmental for build modules
KROOT=/lib/modules/$(shell uname -r)/build

all:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) modules

install:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) modules_install

clean:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) clean

insert:
	sudo dmesg -c
	sudo insmod strmodule.ko
	sudo insmod strtest.ko
	dmesg

remove:
	sudo dmesg -c
	sudo rmmod strtest.ko
	sudo rmmod strmodule.ko
	dmesg
