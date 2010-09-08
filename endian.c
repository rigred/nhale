#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifdef NHALE_GET_ENDIANNESS

// NOTE: I only consider big endian and little endian as possible byte orders

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

#endif
