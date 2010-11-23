#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int rgc, char *argv[])
{
	int fp, i, cnt;
	short buf[1024];

	fp = open("/dev/usrp1_e0", O_WRONLY);
	printf("fp = %d\n", fp);

	for (i=0; i<1024; i++) {
		buf[i] = i;
	}

//	do {
		cnt = write(fp, buf, 2048);
		printf("Bytes written - %d\n", cnt);
//	} while (1);
}
