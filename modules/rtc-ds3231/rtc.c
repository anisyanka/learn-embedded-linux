/**
 * Module realizes high-level driver RTC chip
 */
#include <linux/init.h> /* __init/__exit macros */
#include <linux/module.h>
#include <linux/moduleparam.h> /* pass parameters to module */
#include <linux/export.h> /* export symbols or functions */
#include <linux/fs.h> /* file operations */
#include <linux/slab.h> /* slab allocator */
#include <linux/uaccess.h> /* data from user-space to kernel */
#include <linux/cdev.h> /* need for char devices */

#define MODULE_INIT_ERROR -1
#define MODULE_INIT_SUCCESS 0

#define DEV_NAME "ds3231-rtc"
#define KBUF_SIZE size_t ((10) * PAGE_SIZE)

#define RTC_CHR_MAJOR 700
#define RTC_CHR_MINOR 0

/* Buffer for device data (read/write work with this buffer) */
static char *rtc_kbuf;

/* Identificate device in system */
/* dev_t - is just identificator for device*/
/* It will be in file struct */
static dev_t rtc_first_dev;

static struct cdev *rtc_cdevice;
static struct file_operations rtc_fops;

static int __init rtc_init(void)
{
	int err;

	printk(KERN_INFO "RTC module is being load");

	rtc_kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);
	if (!rtc_kbuf) {
		printk(KERN_ERR "kmalloc() didn't allocate memory");

		return MODULE_INIT_ERROR;
	}

	rtc_cdevice = cdev_alloc();
	if (!rtc_cdevice) {
		printk(KERN_ERR "cdev_alloc() didn't allocate memory");

		return MODULE_INIT_ERROR;
	}

	/* create device number with <majot:minor> pair */
	rtc_first_dev = MKDEV(RTC_CHR_MAJOR, RTC_CHR_MINOR);
	err = register_chrdev_region(rtc_first_dev, rtc_dev_count, DEV_NAME);

	if (err != 0) {
		printk(KERN_ERR "can't register chrdev region");

		return err;
	}

	return MODULE_INIT_SUCCESS;
}

static void __exit rtc_exit(void)
{
	if (rtc_kbuf)
		kfree(rtc_kbuf);
}

module_init(rtc_init);
module_exit(rtc_exit);

MODULE_DESCRIPTION("Driver for RTC based on DS3231 chip");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
