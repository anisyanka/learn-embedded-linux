#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

int mystring_find_length(const char *s)
{
	int l = 0;

	while (*s++ != '\0')
		++l;

	return l;
}

char *mystring_get_reverse(const char *srcstr, char *deststr)
{
	int i;
	int src_len = strlen(srcstr) - 1; /* for last element */

	for (i = src_len; i >= 0; i--)
		deststr[src_len - i] = srcstr[i];

	return deststr;
}

char *mystring_get_rotated(const char *srcstr, char *deststr,
		int rotations, int direction)
{
	#define RIGTH_ROT_DIR 1
	#define LEFT_ROT_DIR 0

	int i;
	int src_len = strlen(srcstr);

	while (rotations >= src_len)
		rotations -= src_len;

	switch (direction) {
	case RIGTH_ROT_DIR:
		for (i = 0; i < rotations; ++i)
			deststr[i] = srcstr[src_len - rotations + i];

		for (; i < src_len; ++i)
			deststr[i] = srcstr[i - rotations];
		break;
	case LEFT_ROT_DIR:
		for (i = rotations; i < src_len; ++i)
			deststr[i - rotations] = srcstr[i];

		for (i = src_len - rotations; i < src_len; ++i)
			deststr[i] = srcstr[i + rotations -src_len];
		break;
	default:
		break;
	}

	return deststr;
}

int mystring_is_palindrome(const char *srcstr)
{
	int ret;
	int i;
	int src_len = strlen(srcstr);
	char *p = (char *)kmalloc(src_len + 1, GFP_KERNEL);

	for (i = 0; i < src_len + 1; ++i)
		p[i] = 0;

	if (!p)
		return 0;

	mystring_get_reverse(srcstr, p);

	if (strcmp(srcstr, p) == 0)
		ret = 1;
	else
		ret = 0;

	kfree(p);

	return ret;
}

char *mystring_save_even_indx(const char *srcstr, char *deststr)
{
	int i, j;
	int src_len = strlen(srcstr);

	i = 0;
	j = 0;
	for (; i < src_len; ++i)
		if (i % 2 == 0)
			deststr[j++] = srcstr[i];

	return deststr;
}

char *mystring_go_lowercase(const char *srcstr, char *deststr)
{
	int i = 0;
	int src_len = strlen(srcstr);

	for (; i < src_len; ++i) {
		if (srcstr[i] < 65 || srcstr[i] > 90)
			deststr[i] = srcstr[i];
		else
			deststr[i] = srcstr[i] + 32;	
	}

	return deststr;
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
