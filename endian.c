#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main(void)
{
  union
  {
    uint8_t byte[2];
    uint16_t word;
  }word;

  word.word = 1;

  if(word.byte[0])
    printf("#undef NHALE_BIG_ENDIAN\n");
  else
    printf("#define NHALE_BIG_ENDIAN\n");

  return 0;
}
