#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int rgc, char *argv[])
{
	int fp, cnt, n;
	short buf[1024];

	n = 0;

	fp = open("/dev/usrp1_e0", O_RDONLY);
	printf("fp = %d\n", fp);

	do {
		cnt = read(fp, buf, 2048);
		n++;
//		printf("Bytes read - %d\n", cnt);
	} while(n < 10*512);
	printf("Data - %hX\n", buf[0]);
}
