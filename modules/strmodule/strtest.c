#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

extern int mystring_find_length(const char *s);
extern char *mystring_get_reverse(const char *srcstr, char *deststr);
extern char *mystring_get_rotated(const char *srcstr, char *deststr,
		int rotations, int direction);
extern int mystring_is_palindrome(const char *srcstr);
extern char *mystring_save_even_indx(const char *srcstr, char *deststr);
extern char *mystring_go_lowercase(const char *srcstr, char *deststr);

static int __init strtest_init(void)
{
	return 0;
}

static void __exit strtest_exit(void)
{
	return;
}

module_init(strtest_init);
module_exit(strtest_exit);

MODULE_DESCRIPTION("Module to test mystr implementation");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
