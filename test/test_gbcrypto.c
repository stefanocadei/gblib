#include "../h/gb_crypto.h"
#include "stdio.h"
#include "string.h"

#define SUCCESS 0

/* Test Vector 1 */
void testvec(char *text)
{
  char result[21];
  char hexresult[41];
  size_t offset;

  /* calculate hash */
  gb_SHA1( result, text, strlen(text) );

  /* format the hash for comparison */
  for( offset = 0; offset < 20; offset++) {
    sprintf( ( hexresult + (2*offset)), "%02x", result[offset]&0xff);
  }
  printf("\nResVec1=%s",hexresult);
}

int main(int argc,char* argv[])
{
	if(argc<2)
	{
		printf("\nUsage: ./test string\n");
	}
	else
	{
		printf("\e[36m\n*********\n* HASHING %s *\n***********\e[0m",argv[1]);
		testvec(argv[1]);
  	}
	return 1;
}
