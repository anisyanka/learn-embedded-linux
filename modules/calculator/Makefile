MODULE_FILENAME=calc

X=0
Y=0

# kernel object
KO_FILE=$(MODULE_FILENAME).ko

# environmental for build modules
export KROOT=/lib/modules/$(shell uname -r)/build

# common object file after compilation
obj-m += calc.o

compile:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) modules_install

clean:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) clean

# `sudo dmesg -c` clears buffer
insert:
	sudo dmesg -c
	sudo insmod $(KO_FILE) x=$(X) y=$(Y)
	dmesg

remove:
	sudo dmesg -c
	sudo rmmod $(KO_FILE)
	dmesg
