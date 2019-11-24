/**
 * Module realizes high-level driver RTC chip
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h> /* file operations */
#include <linux/slab.h> /* slab allocator */
#include <linux/uaccess.h> /* data from user-space to kernel */
#include <linux/cdev.h> /* need for char devices */

static int __init rtc_init(void)
{
	return 0;
}

static void __exit rtc_exit(void)
{
	return;
}

module_init(rtc_init);
module_exit(rtc_exit);

MODULE_DESCRIPTION("Driver for RTC based on DS3231 chip");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
