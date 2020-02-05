.PHONY: all clean compile link

CC=$(CC_PREFX)gcc
LD=CC=$(CC_PREFX)ld
AS=$(CC_PREFX)as
AR=$(CC_PREFX)ar
STRIP=$(CC_PREFX)strip

# Compiling/linking flags
USER_APPS_CCPFLAGS=-std=gnu99 -Wall -g
USER_APPS_LDFLAGS=-lc -lm

# tests
LED_C=led_test.c
LED_O=led_test

all:
	$(CC) $(USER_APPS_CCPFLAGS) $(LED_C) -o $(LED_O)
	sudo cp $(LED_O) --target-directory=$(NFS)/root

clean:
	rm -rf *.o $(LED_O)
