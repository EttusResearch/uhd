#include <stdio.h>

void
main()
{
  int x;
  char *y;
  long long z;

  x = 0x01020304;
  z = 0x0102030405060708LL;

  printf("%x\n",x);
  y = (char *)&x;
  printf("%x\n",y[0]);
  printf("%x\n",y[1]);
  printf("%x\n",y[2]);
  printf("%x\n",y[3]);

  printf("Printing z ...\n");
  printf("%llx\n",z);
  printf("Printing z done\n");

  y = (char *)&z;
  printf("%x\n",y[0]);
  printf("%x\n",y[1]);
  printf("%x\n",y[2]);
  printf("%x\n",y[3]);
  printf("%x\n",y[4]);
  printf("%x\n",y[5]);
  printf("%x\n",y[6]);
  printf("%x\n",y[7]);
}

