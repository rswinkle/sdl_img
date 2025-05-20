
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("Usage: %s text_file\n", argv[0]);
		return 0;
	}


	FILE* f = fopen(argv[1], "r");
	if (!f) {
		perror("Failed to open file");
		exit(1);
	}

	char linebuf[1024];
	int len;

	while (fgets(linebuf, sizeof(linebuf), f)) {
		len = strlen(linebuf);
		linebuf[len-1] = 0; // chop off \n
		len--;

		putchar('\"');
		for (int i=0; i<len; i++) {
			if (linebuf[i] == '\"') {
				putchar('\\');
			}
			putchar(linebuf[i]);
		}
		puts("\\n\"");
	}

	return 0;
}
