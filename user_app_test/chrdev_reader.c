#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DBG_WORD "READER: "
#define CHRDEV_FILE_NAME "/dev/mychrdev0"
#define FROM_CHRDEV_BUF_SIZE 1
#define EXIT_SYMBOL '!'

int main(int argc, char const *argv[])
{
	FILE *fp;
	char from_chrdev[FROM_CHRDEV_BUF_SIZE] = { 0 };

	printf(DBG_WORD "Hello, from chardev reader\n");

	fp = fopen(CHRDEV_FILE_NAME, "r");
	if (!fp) {
		printf(DBG_WORD "fopen() error\n");
		return 1;
	}
	setbuf(fp, NULL);

	while (1) {
		fread(from_chrdev, 1, FROM_CHRDEV_BUF_SIZE, fp);

		printf(DBG_WORD "read from " CHRDEV_FILE_NAME ": ");
		for (int i = 0; i < FROM_CHRDEV_BUF_SIZE; ++i)
			printf("%c", from_chrdev[i]);
		printf("\n");

		/* exit flag */
		for (int i = 0; i < FROM_CHRDEV_BUF_SIZE; ++i)
			if (from_chrdev[i] == EXIT_SYMBOL)
				break;
	}

	return 0;
}
