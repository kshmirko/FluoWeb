#include <stdio.h>

int
main(int argc, char *argv[])
{
  int k, mode, range;
  if(scanf("A[%d] = (mode=%d, range=%d)", &k, &mode, &range)==3)
    {
      printf("A[%d]=(mode=%d, range=%d)", k, mode, range);
    }
}

