#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h> 
#include <linux/device.h> 
#include <linux/init.h>

/*
 * This is the starting point of the kernel module's code execution.
 * There you can allocate some memory, init global variables and so on.
 */
static int __init solution_init(void)
{
	printk(KERN_INFO "Hello, BBB\n");

	return 0;
}
 
/*
 * This is the exit point of the kernel module. When the module is removed 
 * this function is called to do any sort of cleanup activities and other such
 * stuff.
 * 
 * For example you have made a tree where you keep some information - you would 
 * like to place the code for removing the nodes of the tree here.
 */
static void __exit solution_exit(void)
{
	printk(KERN_INFO "Goodbuy, BBB\n");
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_DESCRIPTION("Learn Linux Kernel with Bootlin");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
