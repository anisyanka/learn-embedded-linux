obj-m += chrdev.o

all:
	sudo $(MAKE) -C $(BUILD_DIR)            \
		M=$(shell pwd)                  \
		CROSS_COMPILE=$(CROSS_COMPILE)  \
		ARCH=$(ARCH) modules

clean:
	sudo $(MAKE) -C $(BUILD_DIR)            \
		M=$(shell pwd)                  \
		ARCH=$(ARCH) clean

insert:
	@echo "Need ssh to 192.168.0.100"

remove:
	@echo "Need ssh to 192.168.0.100"
