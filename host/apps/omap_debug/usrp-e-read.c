#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int rgc, char *argv[])
{
	int fp, cnt;
	short buf[1024];

	fp = open("/dev/usrp1_e0", O_RDONLY);
	printf("fp = %d\n", fp);

	do {
	cnt = read(fp, buf, 2048);
//	printf("Bytes read - %d\n", cnt);
	} while(1);
	printf("Data - %hX\n", buf[0]);
}
