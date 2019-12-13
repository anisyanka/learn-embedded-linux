# Learn embedded Linux

I have started to learn embedded Linux and driver devepompemt with the [BeagleBone Black board](https://www.elinux.org/Beagleboard:BeagleBoneBlack). There I will write some usefull notes or commands about work with the BBB or about driver development.

## My study plan

##### 1. Preparation
  - Build set of tools for Cortex A8 ARM
  - Build and flash U-Boot in eMMC memory on board
  - Build the Linux Kernel 
  - Build root file system

##### 2. Learning
  - Learn how to boot the Linux kernel
  - Learn what is device tree table
  - Learn how to write kernel modules

##### 3. Laboratory works from [Bootlin](https://bootlin.com/doc/training/linux-kernel/) about Linux kernel course
  - Make "Hello world" kernel module
  - Make I2C-driver for external I2C-device
  - Make driver avaliable in user-space through `/dev/devname`
  - Realize `fops` struct for i2c-device
  - Realize interrupt handler from the device

##### 4. Try to do some drivers for own soldered board
  - [SPI OLED](https://components101.com/oled-display-ssd1306) display based on ssd1306 controller
  - [Rotary](https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/)
  - Temperature and humidity sensor [DHT11](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
  - RTC I2C based on [DS3231](https://www.maximintegrated.com/en/products/analog/real-time-clocks/DS3231.html) controller
  - Buttons/Leds
  - Ambient light I2C-sensor based on [BH1750](https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf) controller
  - Barometric pressure I2C-sensor based on [BMP180](https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf) controller
  - Ultrasonic ranging module [HC-SR04](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf)

## Few words about homemade development board
| ![Front of development board](tools/pictures/front-back.png) |
|:--:|
| *My Frankenstein* |
 > 1. Servomotor, that allows for precise control of angular position
 > 2. Mechanical rotary encoder that converts the angular position to digital output signals
 > 3. Ultrasonic ranging module
 > 4. Power LED
 > 5. SPI OLED display
 > 6. User's GPIO LED
 > 7. Temperature and humidity sensor
 > 8. Seven-segment display
 > 9. User's button
 > 10. Interface to BBB
 > 11. CR2032 batary for RTC
 > 12. RTC
 > 13. Ambient light sensor
 > 14. Barometric pressure sensor

| ![The BBB and UART-USB module](tools/pictures/bbb.png) |
|:--:|
| *The BBB and UART-USB module* |

## Few words about schematic
I don't know why I have decided to make this ugly board, but I think it doesn't matter for driver development and learning the Linux kernel :)

*in progress*

## Toolchain
Toolchain is a collection of different tools set up to tightly work together.
First of all, in embedded systems case, toolchain is needed to build a bootloader, a kernel and a root file system.
After this actions we need to use the toolchain to build kernel modules, system drivers and user-space applications.

Every toolchain contains of:
 - Compiler. It is need to translate source code to assemler mnemonics.
 - Binutils. It is need to use `as` and `ld`. Assembler translates the mnemonics to binary codes. Linker makes one executable file.
 - C library and inludes of the Linux kernel(for development).

We need a cross-toolchain, because we need to compile code for machine different from the machine it runs on.
Build machine in my case is `x86-64 platform`.
Target machine (where code will be executed) in my case is `ARM Cortex-A8 32 bit platform`.

Toolchain choice depends on target platform:
 - Processor architecture (ARM / MIPS / x86_64 and so on)
 - Little endian or big endian
 - Is there hard-float arithmetic?
 - Should we use ABI with or without `hf`?

If a processor has hardware hard-float block, we need to pick toolchain which will generate code for hf-block.
It will be more faster, than chip without hf, because without it compiler will use soft-float for double and float operations.
The Beagle Bone Black has ARMv7 arch and hardware float block.

There are some ways to get needed toolchain:
 - Download already pre-built toolchain, for example, from [Linaro](https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/arm-linux-gnueabihf/).
 - To make the toolchain itself.

There are some methods for the last way. We can build all set of tools by hand with help [this guide](https://clfs.org/view/clfs-embedded/arm/).
It's very long and hard way. Or we can use some frameworks for creating Linux distributions for embedded devices.
It is can be [Buildroot](https://en.wikipedia.org/wiki/Buildroot), [Yocto Project](https://en.wikipedia.org/wiki/Yocto_Project) and other Linux build systems.
There is [comparing embedded Linux build systems](https://elinux.org/images/0/0a/Embedded_Linux_Build_Systems.pdf) document, which can help to pick needed system.

I have decided to download alredy pre-built toolchain:
```sh
wget -c https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/arm-linux-gnueabihf/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz
tar xf gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz
*use path to this toolchain during building modules, drivers and user-space*
```
