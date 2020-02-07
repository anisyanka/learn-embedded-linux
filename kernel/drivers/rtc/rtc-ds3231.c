/*
 * RTC client/driver for the Maxim/Dallas DS3231 Real-Time Clock
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>

static const struct of_device_id ds1307_of_match[] = {
	{
		.compatible = "maxim-integrated,ds3231",
		.data = NULL
	},
	{ }
};

static int __init ds3231_init(void)
{
	printk(KERN_INFO "ds3231 driver is being init\n");

	return i2c_add_driver();
}

static void __init ds3231_exit(void)
{
	return;
}

module_init(ds3231_init);
module_exit(ds3231_exit);

MODULE_AUTHOR("Anisimov Aleksandr <anisimov.alexander.s@gmail.com>");
MODULE_DESCRIPTION("Maxim-Integrated DS3231");
MODULE_LICENSE("GPL");
