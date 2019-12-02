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

#define DEV_NAME "ds3231_rtc"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

#define RTC_CHR_MAJOR 500
#define RTC_CHR_MINOR 0

#define RTC_MAX_DEVICE_CNT 1

/* Buffer for device data (read/write work with this buffer) */
static char *rtc_kbuf;

/*
 * Identificate device in system
 * dev_t - is just identificator for device
 * It will be in file struct
 */
static dev_t rtc_first_dev;

/*
 * This struct is device in the kernel.
 * It dev_t identificator and file_operations structures.
 */
static struct cdev *rtc_cdevice;

static int rtc_open(struct inode *inode, struct file *file)
{
	return 0;
}

int rtc_release(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t rtc_read(struct file *file, char __user *user_buffer,
			size_t luser_buffer, loff_t *ppos)
{
	return 0;
}

ssize_t rtc_write(struct file *file, const char __user *user_buffer,
			size_t luser_buffer, loff_t *ppos)
{
	return 0;
}

/*
 * This functions will call afted user program will work
 * with RTC device. F. e. `int df = open("/dev/ds3231-rtc");`
 */
static const struct file_operations rtc_fops = {
	.owner = THIS_MODULE,
	.read = rtc_read,
	.write = rtc_write,
	.open = rtc_open,
	.release = rtc_release
};

static int __init rtc_init(void)
{
	int err;

	printk(KERN_INFO "RTC module is being load\n");

	rtc_kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);
	if (!rtc_kbuf) {
		printk(KERN_ERR "kmalloc() didn't allocate memory\n");

		return MODULE_INIT_ERROR;
	}

	/*
	 * It need calls device counts times.
	 * One times in this case.
	*/
	rtc_cdevice = cdev_alloc();
	if (!rtc_cdevice) {
		printk(KERN_ERR "cdev_alloc() didn't allocate memory\n");

		return MODULE_INIT_ERROR;
	}

	/* Create device number with <majot:minor> pair */
	rtc_first_dev = MKDEV(RTC_CHR_MAJOR, RTC_CHR_MINOR);
	err = register_chrdev_region(rtc_first_dev, RTC_MAX_DEVICE_CNT, DEV_NAME);
	if (err != 0) {
		printk(KERN_ERR "can't register chrdev region\n");

		return err;
	}

	/*
	 * Init and add device to /dev/ file system 
	 * It need calls device counts times
	 */
	cdev_init(rtc_cdevice, &rtc_fops);
	cdev_add(rtc_cdevice, rtc_first_dev, RTC_MAX_DEVICE_CNT);

	printk(KERN_INFO "RTC module was loaded\n");

	return MODULE_INIT_SUCCESS;
}

static void __exit rtc_exit(void)
{
	printk(KERN_INFO "RTC module exit\n");

	if (rtc_kbuf)
		kfree(rtc_kbuf);

	/*
	 * Init and add device to /dev/ file system 
	 * It need calls device counts times
	 */
	cdev_del(rtc_cdevice);
	unregister_chrdev_region(rtc_first_dev, RTC_MAX_DEVICE_CNT);
}

module_init(rtc_init);
module_exit(rtc_exit);

MODULE_DESCRIPTION("Driver for RTC based on DS3231 chip");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
