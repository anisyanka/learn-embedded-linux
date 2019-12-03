#include "rtc_fops.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/export.h>

static int rtc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int rtc_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t rtc_read(struct file *file, char __user *user_buffer,
			size_t luser_buffer, loff_t *ppos)
{
	return 0;
}

static ssize_t rtc_write(struct file *file, const char __user *user_buffer,
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

const struct file_operations *rtc_get_fops_implementation(void)
{
	return &rtc_fops;
}
