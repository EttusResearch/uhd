#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int rgc, char *argv[])
{
	int fp, i, cnt;
	unsigned short buf[1024];
	unsigned short buf_rb[1024];

	fp = open("/dev/usrp1_e0", O_RDWR);
	printf("fp = %d\n", fp);

	for (i=0; i<1024; i++)
		buf[i] = i*256; 
	write(fp, buf, 2048);
	read(fp, buf_rb, 2048);

	printf("Read back %hX %hX\n", buf_rb[0], buf_rb[1]);

	for (i=0; i<1024; i++) {
		if (buf[i] != buf_rb[i])
			printf("Read - %hX, expected - %hX\n", buf_rb[i], buf[i]);
	}
}
