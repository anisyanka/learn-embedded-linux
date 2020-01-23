MODULE_NAME=rtc
KO=$(MODULE_NAME).ko

OBJ = rtc_main.o rtc_fops.o

obj-m += rtc_ds3231.o
rtc_ds3231-objs := $(OBJ)

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
