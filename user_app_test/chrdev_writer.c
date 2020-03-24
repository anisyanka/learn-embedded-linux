#include <stdio.h>

#define DBG_WORD "WRITER: "
#define CHRDEV_FILE_NAME "/dev/mychrdev0"

int main(int argc, char const *argv[])
{
	FILE *fp;
	int c;

	printf(DBG_WORD "Hello, from chardev writer\n");

	fp = fopen(CHRDEV_FILE_NAME, "w");
	if (!fp) {
		printf(DBG_WORD "fopen() error\n");
		return 1;
	}
	setbuf(fp, NULL);

	while ((c = getchar()) != EOF) {
		if (c == '\n')
			continue;

		fwrite((char *)&c, 1, 1, fp);
		fflush(fp);
	}

	fclose(fp);

	return 0;
}
