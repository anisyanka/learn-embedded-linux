/*
 * Module to demostrate how to do some calculations.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

struct sample {
    int x, y;
};

static void calculator(void) {
    int *ptrx;

    struct sample s;
    s.x = 15;
    s.y = 3;
    ptrx = &s.x;

    printk (KERN_INFO "x is %d y is %d\n", s.x, s.y);
    printk (KERN_INFO "Sum is           %d\n", s.x + s.y);
    printk (KERN_INFO "Difference is    %d\n", s.x - s.y);
    printk (KERN_INFO "Product is       %d\n", s.x * s.y);
    printk (KERN_INFO "Divison is       %d\n", s.x / s.y);
    printk (KERN_INFO "Mod is           %d\n", s.x % s.y);
    printk (KERN_INFO "Bitwise NOT of %d Is   %d\n", s.x, ~s.x);
    printk (KERN_INFO "Bitwise OR is    %d\n", s.x | s.y);
    printk (KERN_INFO "Bitwise AND is   %d\n", s.x & s.y);
    printk (KERN_INFO "Bitwise XOR Is   %d\n", s.x ^ s.y);
    printk (KERN_INFO "Logical OR Is    %d\n", s.x || s.y);
    printk (KERN_INFO "Logical AND Is   %d\n", s.x && s.y);

    if (s.x > s.y) {
        printk (KERN_INFO "%d is greater than %d\n", s.x, s.y);
    } else if (s.y > s.x) {
        printk (KERN_INFO "%d is greater than %d\n", s.y, s.x);
    } else {
        printk (KERN_INFO "%d is equal to %d\n", s.y, s.x);
    }

    printk (KERN_INFO "Address of a is %p\n", ptrx);
    printk (KERN_INFO "Value of ptrx is %d\n", *ptrx);
}

static int __init calc_init(void)
{
    printk(KERN_INFO "Hello from Anisyanka\n");

    calculator();

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
