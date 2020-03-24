#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>

#define MYDEV_NAME "mychrdev"
#define MYCLASS_NAME "mychrdev_class"
#define MYDEV_MAX_COUNT 1
#define KBUF_SIZE ((10) * PAGE_SIZE)

#define MY_MAJOR 511
#define MY_MINOR 0

struct chrdev {
	struct cdev *my_cdev;
	struct device *my_dev;
};

struct chrdev_kbuf {
	char *kbuf;
	int cur_len;
	int max;
	int is_open;

	/*
	 * Need to sync r/w operation:
	 * If user's app wants to read n bytes, but kbuf has less, then n,
	 * we must sleep and wait write operation untill kbuf will have >= n bytes
	 */
	struct semaphore *sem;
};

static int mychrdev_open(struct inode *inode, struct file *file)
{
	struct chrdev_kbuf *kbuf_str;

	/* check that file has been already opened */
	kbuf_str = file->private_data;
	if (kbuf_str) {
		if (kbuf_str->is_open) {
			printk(KERN_ERR
				"%s: file has been already opened\n",
				__func__);
			return 0;
		}
	}

	kbuf_str = kmalloc(sizeof(struct chrdev_kbuf), GFP_KERNEL);
	if (!kbuf_str) {
		printk(KERN_ERR "%s: kmalloc for kbuf_str error\n", __func__);
		goto open_err;
	}

	kbuf_str->kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);
	if (!kbuf_str->kbuf) {
		printk(KERN_ERR "%s: kmalloc for kbuf error\n", __func__);
		goto open_err;
	}

	kbuf_str->cur_len = 0;
	kbuf_str->max = KBUF_SIZE;
	kbuf_str->is_open = 1;

	/*
	 * Save buffer pointer for each application
	 * which has opened the chrdev file
	 */
	file->private_data = kbuf_str;

	printk(KERN_INFO "%s\n", __func__);
	return 0;

open_err:
	if (kbuf_str->kbuf)
		kfree(kbuf_str->kbuf);
	if (kbuf_str)
		kfree(kbuf_str);

	return -1;
}

static int mychrdev_release(struct inode *inode, struct file *file)
{
	struct chrdev_kbuf *kbuf_str = file->private_data;

	if ((kbuf_str) && (kbuf_str->kbuf)) {
		kfree(kbuf_str->kbuf);
		kbuf_str->kbuf = NULL;
	}

	if (kbuf_str) {
		kbuf_str->is_open = 0;
		kfree(kbuf_str);
	}

	kbuf_str = NULL;

	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

static ssize_t mychrdev_read(struct file *file, char __user *buf,
	size_t lbuf, loff_t *ppos)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

static ssize_t mychrdev_write(struct file *file, const char __user *buf,
	size_t lbuf, loff_t *ppos)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

static struct chrdev mychrdev[MYDEV_MAX_COUNT];
static dev_t dev_no;
static struct class *my_class;

static struct file_operations mychrdev_fops = {
	.owner = THIS_MODULE,
	.open = mychrdev_open,
	.release = mychrdev_release,
	.read = mychrdev_read,
	.write = mychrdev_write,
};

/*
 * This is the starting point of the kernel module's code execution.
 * There you can allocate some memory, init global variables and so on.
 */
static int __init init_chrdev(void)
{
	int ret = -1, res, i;
	dev_t curr_dev;

	/* Request the kernel for MYDEV_MAX_COUNT devices */
	res = alloc_chrdev_region(&dev_no, MY_MINOR,
		MYDEV_MAX_COUNT, MYDEV_NAME);
	if (res < 0) {
		printk(KERN_ERR
			"%s: alloc_chrdev_region() allocate error\n",
			__func__);
		goto init_error;
	}

	/* Create a class: appears at /sys/class/<name> */
	my_class = class_create(THIS_MODULE, MYCLASS_NAME);
	if (!my_class) {
		printk(KERN_ERR "%s: class_create() error\n", __func__);
		goto init_error;
	}

	/* Initialize and create each of the device(cdev) */
	for (i = 0; i < MYDEV_MAX_COUNT; ++i) {
		mychrdev[i].my_cdev = cdev_alloc();
		if (!mychrdev[i].my_cdev) {
			printk(KERN_ERR "%s: cdev_alloc() error\n", __func__);
			goto init_error;
		}

		/* Associate the cdev with a set of file_operations */
		cdev_init(mychrdev[i].my_cdev, &mychrdev_fops);

		/* Build up the current device number. To be used further */
		curr_dev = MKDEV(MAJOR(dev_no), MINOR(dev_no) + i);

		/* Create a device node for this device. Look, the class is
		 * being used here. The same class is associated with MYDEV_MAX_COUNT
		 * devices. Once the function returns, device nodes will be
		 * created as /dev/my_dev0, /dev/my_dev1,... You can also view
		 * the devices under /sys/class/my_driver_class.
		 */
		mychrdev[i].my_dev = device_create(my_class, NULL,
			curr_dev, NULL, "%s%d", MYDEV_NAME, i);
		if (!mychrdev[i].my_dev) {
			printk(KERN_ERR "%s: device_create() error\n", __func__);
			goto init_error;
		}

		res = cdev_add(mychrdev[i].my_cdev, curr_dev, 1);
		if (res < 0) {
			printk(KERN_ERR "%s: cdev_add() error\n", __func__);
			goto init_error;
		}
	}

	printk(KERN_INFO "Hello, chrdev!\n");

	ret = 0;
	goto init_ok;

init_error:
	for (i = 0; i < MYDEV_MAX_COUNT; ++i) {
		if (mychrdev[i].my_dev && my_class) {
			curr_dev = MKDEV(MAJOR(dev_no), MINOR(dev_no) + i);
			device_destroy(my_class, curr_dev);
		}
	}

	if (my_class)
		class_destroy(my_class);

	for (i = 0; i < MYDEV_MAX_COUNT; ++i) {
		if (mychrdev[i].my_cdev)
			cdev_del(mychrdev[i].my_cdev);
	}

init_ok:
	return ret;
}

/*
 * This is the exit point of the kernel module. When the module is removed 
 * this function is called to do any sort of cleanup activities and other such
 * stuff.
 * 
 * For example you have made a tree where you keep some information - you would 
 * like to place the code for removing the nodes of the tree here.
 */
static void __exit exit_chrdev(void)
{
	dev_t curr_dev;
	int i;

	for (i = 0; i < MYDEV_MAX_COUNT; ++i) {
		curr_dev = MKDEV(MAJOR(dev_no), MINOR(dev_no) + i);
		device_destroy(my_class, curr_dev);
	}

	class_destroy(my_class);

	for (i = 0; i < MYDEV_MAX_COUNT; ++i) {
		if (mychrdev[i].my_cdev)
			cdev_del(mychrdev[i].my_cdev);
	}

	unregister_chrdev_region(dev_no, MYDEV_MAX_COUNT);

	printk(KERN_INFO "Goodbuy, chrdev!\n");
}

module_init(init_chrdev);
module_exit(exit_chrdev);

MODULE_DESCRIPTION("chardev node for learning purpose");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
