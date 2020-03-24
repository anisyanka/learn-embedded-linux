.PHONY: all clean compile link

CC=$(CC_PREFX)gcc
LD=CC=$(CC_PREFX)ld
AS=$(CC_PREFX)as
AR=$(CC_PREFX)ar
STRIP=$(CC_PREFX)strip

# Compiling/linking flags
USER_APPS_CCPFLAGS=-std=gnu99 -Wall -g -static
USER_APPS_LDFLAGS=-lc -lm

# tests
LED_C=led_test.c
LED_O=led_test

CHRDEV_WRITER_C=chrdev_writer.c
CHRDEV_WRITER_O=chrdev_writer

CHRDEV_READER_C=chrdev_reader.c
CHRDEV_READER_O=chrdev_reader

all:
	$(CC) $(USER_APPS_CCPFLAGS) $(LED_C) -o $(LED_O)
	$(CC) $(USER_APPS_CCPFLAGS) $(CHRDEV_WRITER_C) -o $(CHRDEV_WRITER_O)
	$(CC) $(USER_APPS_CCPFLAGS) $(CHRDEV_READER_C) -o $(CHRDEV_READER_O)

	sudo cp $(LED_O) --target-directory=$(NFS)/root/user_space_drv_test
	sudo cp $(CHRDEV_WRITER_O) --target-directory=$(NFS)/root/user_space_drv_test
	sudo cp $(CHRDEV_READER_O) --target-directory=$(NFS)/root/user_space_drv_test

clean:
	rm -rf *.o $(LED_O) $(CHRDEV_WRITER_O) $(CHRDEV_READER_O
