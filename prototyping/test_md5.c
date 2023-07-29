
#include <WjCryptLib_Md5.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_md5(MD5_HASH* h)
{
	for (int i=0; i<MD5_HASH_SIZE; ++i) {
		printf("%02x", h->bytes[i]);
	}
	putchar('\n');
}


int main(int argc, char** argv)
{

	const char str1[] = "Hello";



	MD5_HASH result;

	Md5Calculate(str1, strlen(str1), &result);

	print_md5(&result);

	for (int i=1; i<argc; ++i) {
		Md5Calculate(argv[i], strlen(argv[i]), &result);
		print_md5(&result);
	}

	return 0;
}


