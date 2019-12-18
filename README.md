# Learn embedded Linux

I have started to learn embedded Linux and driver development with the [BeagleBone Black board](https://www.elinux.org/Beagleboard:BeagleBoneBlack). There I will write some usefull notes or commands about work with the BBB or about driver development.

# Table of contents
  - [Study plan](#Study-plan)
  - [Few words about homemade development board](#Few-words-about-homemade-development-board)
  - [Few words about schematic](#Few-words-about-schematic)
  - [Toolchain](#Toolchain)
  - [Bootloader](#Bootloader)
    - [Stages of loading OS](#What-are-there-stages-of-loading-OS)
    - [Building uboot](#Building-uboot)
  - [Installing all images to the BBB](#Installing-all-images-to-the-BBB)

## Study plan

**Preparation**
  - Build set of tools for Cortex-A8 ARM
  - Build and flash U-Boot in eMMC memory on board
  - Build the Linux Kernel
  - Build root file system

**Learning**
  - Learn how to boot the Linux kernel
  - Learn what is device tree table
  - Learn how to write kernel modules

**[Bootlin's](https://bootlin.com/doc/training/linux-kernel/) laboratory works about Linux kernel course**
  - Make "Hello world" kernel module
  - Make I2C-driver for external I2C-device
  - Make driver avaliable in user-space through `/dev/devname`
  - Realize `fops` struct for i2c-device
  - Realize interrupt handler from the device

**Write drivers for homemade soldered board**
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
1. Servomotor, that allows for precise control of angular position
2. Mechanical rotary encoder that converts the angular position to digital output signals
3. Ultrasonic ranging module
4. Power LED
5. SPI OLED display
6. User's GPIO LED
7. Temperature and humidity sensor
8. Seven-segment display
9. User's button
10. Interface to BBB
11. CR2032 batary for RTC
12. RTC
13. Ambient light sensor
14. Barometric pressure sensor

| ![The BBB and UART-USB module](tools/pictures/bbb.png) |
|:--:|
| *The BBB and UART-USB module* |

## Few words about schematic
I don't know why I have decided to make this ugly board, but I think it doesn't matter for driver development and learning the Linux kernel :)

---in progress---

## Toolchain
Toolchain is a collection of different tools set up to tightly work together.
First of all, in embedded systems case, toolchain is needed to build a bootloader, a kernel and a root file system.
After this actions we need to use the toolchain to build kernel modules, system drivers and user-space applications.

**Every toolchain contains of:**
 - Compiler. It is need to translate source code to assemler mnemonics.
 - Binutils. It is need to use `as` and `ld`. Assembler translates the mnemonics to binary codes. Linker makes one executable file.
 - C library and inludes of the Linux kernel(for development).

We need a cross-toolchain, because we need to compile code for machine different from the machine it runs on.
Build machine in my case is `x86-64 platform`.
Target machine (where code will be executed) in my case is `ARM Cortex-A8 32 bit platform`.

**Toolchain choice depends on target platform:**
 - Processor architecture (ARM / MIPS / x86_64 and so on)
 - Little endian or big endian
 - Is there hard-float arithmetic?
 - Should we use ABI with or without `hf`?

If a processor has hardware hard-float block, we need to pick toolchain which will generate code for hf-block.
It will be more faster, than chip without hf, because without it compiler will use soft-float for double and float operations.
The Beagle Bone Black has ARMv7 arch and hardware float block.

**There are some ways to get needed toolchain:**
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
## Bootloader
The main role of a bootloader is base initialisation of processor peripherals and loading a kernel of operating system to RAM memory.

### What are there stages of loading OS?

OS is loaded with help several stages
```
Power-reset ---> ROM-code ---> Preloader or Secondary-Program-Loader ---> u-boot/barebox or Third-Program-Loader ---> kernel-start
```

**ROM code**

A lot of modern SoCs have on-chip ROM memory. Code into the ROM is flashed on factory and it will executed by the CPU from power-up.
Immediately аfter power-up CPU has no access to the peripheral devices, such as RAM, storage and so on, because the CPU doesn't know something about its board.
Only the next stages can know about board's chipset, because we itself have configured, built and flashed right code(uboot/device-tree) to flash/eMMC card.
That's why the one can use only on-chip RAM memory. It is a SRAM usually.

This bootloader initializes a minimal amount of CPU and board hardware, then accesses the first partition of the SD or MMC card, which must be in FAT format,
and loads a file called "MLO", and executes it. "MLO" is the second-stage bootloader.

If thare aren't MMC, ROM-code tries to load the preloader into SRAM with help this interfaces:
 - SPI flash card
 - Ethernet/USB/UART

In the end of this stage SPL-code is loaded into SRAM and starts to execute.
If SoC-system has enough RAM memory, ROM code can load and TPL(`uboot.bin`) too.

**SPL**

SPL-code must configure DRAM-chip, using SoC DRAM controller, to load the TPL to it. 
This bootloader apparently also just reads the first partition of the SD card, and loads a file called "u-boot.bin", and executes it.
"u-boot.bin" is the third-stage bootloader.

**TPL: uboot or barebox**

u-boot code loads OS kernel from MMC/SD to DRAM, flattened device tree and ramdisk.

### Building uboot
```sh
git clone https://github.com/u-boot/u-boot.git uboot
cd uboot
git checkout v2019.10
```

We need to build the SPL and TPL. Other words we need to get two binary files:
 - MLO as the SPL
 - u-boot.bin as the TPL

To compile this code, we should configure the uboot sources with help `make <config-file-for-your-board>`.
There are a lot of ready config files in the `./uboot/configs/`. We can take ready file for a board or write own file.
Config file must specify a device tree file for a board and define some constants.
See `./scripts/build_bbb_uboot.sh` which builds uboot for the BeagleBoneBlack board or you may use Makefile:
`make bbb_uboot`

After building you can find this files into `./out/uboot/` directory:
 - MLO is secodary program loader
 - u-boot is itself bootloader in ELF format for debugging
 - u-boot.map is table of symbols
 - u-boot.bin is itself bootloader with dtb ready for executing in a target

## Installing all images to the BBB
The BBB has a JTAG-connector on the board, that's why we can flash the uboot to RAM, run it and, with help uboot command line,
copy its code to flash card. But I have no JTAG for armv7. I will use a SD-card and my work station to prepare the uboot for the BBB.

The AM335x chip has internal ROM memory, in which ROM-code was saved. This code can access to SD card during boot process.
We just need to make the first part of a SD card as booting part and format it to FAT.
Use [this](https://ragnyll.gitlab.io/2018/05/22/format-a-sd-card-to-fat-32linux.html) guide to format SD-card

Then we need to mount the sd-card to work station OS and copy our bootloaders to SD:
```sh
cp ./out/uboot/MLO ./out/uboot/u-boot.bin -t /media/[user]/[dir]
```

After this manipulations we may disable the bbb, insert sd-card, press `uSD button` on the board
(that will be booting from SD, not MMC) and turn on power.
