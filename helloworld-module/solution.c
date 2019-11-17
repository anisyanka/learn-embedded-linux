#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h> 
#include <linux/device.h> 
#include <linux/init.h>

/* external module file */
#include <checker.h>

static void *p1 = NULL;
static int *p2 = NULL;
static struct device *p3 = NULL;

/*
 * This is the starting point of the kernel module's code execution.
 * There you can allocate some memory, init global variables and so on.
 */
static int solution_init(void)
{
    /* allocate any pointer type */
    p1 = kmalloc((size_t)get_void_size(), GFP_KERNEL);
    submit_void_ptr(p1);

    /* allocate array of ints */
    p2 = (int *)kmalloc(get_int_array_size(), GFP_KERNEL);
    submit_int_array_ptr(p2);

    /* go mem for some device */
    p3 = (struct device *)kmalloc(sizeof(struct device), GFP_KERNEL);
    submit_struct_ptr(p3);

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
static void solution_exit(void)
{
    if (p1)
        checker_kfree(p1);

    if (p2)
        checker_kfree(p2);

    if (p3)
        checker_kfree(p3);
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_DESCRIPTION("Learn Linux Kernel with stepik.org");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
