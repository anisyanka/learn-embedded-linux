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

static const char d[] = "Hello world";
static int dlen = sizeof(d) - 1;

static int strlen_test(void)
{
	int my_len;

	my_len = mystring_find_length(d);

	if (my_len == dlen)
		return 1;

	return 0;
}

static int reverse_test(void)
{
	static char test[50] = { 0 };
	static char dreverse[] = "dlrow olleH";

	mystring_get_reverse(d, test);

	if (strcmp(dreverse, test) == 0)
		return 1;

	return 0;
}

static int rotate_test(void)
{
	#define RIGTH_ROT_DIR 1
	#define LEFT_ROT_DIR 0

	static char test1[50] = { 0 };
	static char test2[50] = { 0 };
	static char drotate_right_by_3[] = "rldHello wo";
	static char drotate_left_by_3[] = "lo worldHel";

	mystring_get_rotated(d, test1, 3, RIGTH_ROT_DIR);
	mystring_get_rotated(d, test2, 3, LEFT_ROT_DIR);

	if (strcmp(drotate_right_by_3, test1) != 0)
		return 0;

	if (strcmp(drotate_left_by_3, test2) != 0)
		return 0;

	return 1;
}

static int palindrom_test(void)
{
	static const char palindrom[] = "Hello olleH";
	static const char not_palindrom[] = "12345789";

	if (mystring_is_palindrome(palindrom) == 0)
		return 0;

	if (mystring_is_palindrome(not_palindrom) == 1)
		return 0;

	return 1;
}

static int even_test(void)
{
	static char test[50] = { 0 };
	static const char d[] = "Hello world";
	static const char d_even_indx[] = "Hlowrd";

	mystring_save_even_indx(d, test);

	if (strcmp(d_even_indx, test) == 0)
		return 1;

	return 0;
}

static int lowercase_test(void)
{
	static char test[50] = { 0 };
	static const char D[] = "HELLOWORLD";
	static char Dlower[] = "helloworld";

	mystring_go_lowercase(D, test);

	if (strcmp(Dlower, test) == 0)
		return 1;

	return 0;
}

static int __init strtest_init(void)
{
	printk(KERN_INFO "strlen_test() state = %s",
			strlen_test() ? "OK" : "FAIL");

	printk(KERN_INFO "reverse_test() state = %s",
			reverse_test() ? "OK" : "FAIL");

	printk(KERN_INFO "rotate_test() state = %s",
			rotate_test() ? "OK" : "FAIL");

	printk(KERN_INFO "palindrom_test() state = %s",
			palindrom_test() ? "OK" : "FAIL");

	printk(KERN_INFO "even_test() state = %s",
			even_test() ? "OK" : "FAIL");

	printk(KERN_INFO "lowercase_test() state = %s",
			lowercase_test() ? "OK\n" : "FAIL\n");

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
