/*
 * Module to demostrate how to do some calculations.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>

static int __init calc_init(void)
{
    printk(KERN_INFO "Hello from Anisyanka");

    return 0;
}

static void __exit calc_exit(void)
{
    return;
}

module_init(calc_init);
module_exit(calc_exit);

MODULE_DESCRIPTION("Module to demostrate how to do some calculations.");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
