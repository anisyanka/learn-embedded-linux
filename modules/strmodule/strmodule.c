#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

int mystring_find_length(const char *s)
{
	return 0;
}

char *mystring_get_reverse(const char *srcstr, char *deststr)
{
	return NULL;
}

char *mystring_get_rotated(const char *srcstr, char *deststr,
		int rotations, int direction)
{
	return NULL;
}

int mystring_is_palindrome(const char *srcstr)
{
	return 0;
}

char *mystring_save_even_indx(const char *srcstr, char *deststr)
{
	return NULL;
}

char *mystring_go_lowercase(const char *srcstr, char *deststr)
{
	return NULL;
}

EXPORT_SYMBOL(mystring_find_length);
EXPORT_SYMBOL(mystring_get_reverse);
EXPORT_SYMBOL(mystring_get_rotated);
EXPORT_SYMBOL(mystring_is_palindrome);
EXPORT_SYMBOL(mystring_save_even_indx);
EXPORT_SYMBOL(mystring_go_lowercase);

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

MODULE_DESCRIPTION("Module presents mystr implementation");
MODULE_AUTHOR("Aleksandr Anisimov <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
