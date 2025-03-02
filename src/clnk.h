/*
clnk 0.90 MIT licensed Windows .lnk shortct file parser
robertwinkler.com

Do this:
    #define CLNK_IMPLEMENTATION
before you include this file in *one* C or C++ file to create the implementation.

LICENSE

    See end of file for license information.

QUICK NOTES:

    For now all it does is extract the path to the file the shortcut points at.
    That seems the most relevant data and all I needed for my project.  If
    someone requests more I'd consider adding it but for the life of me I can't
    figure out what all that extra information is for let alone why they made the
    format so complicated.

	There are currently only 2 main functions with 2 variants each.  They
	either allocate the buffer for you or accept a buffer and size parameter
	and they either take a lnk file path or a FILE* to one already open.

	The program below demonstrates:

    int main(int argc, char** argv)
    {
        if (argc != 2) {
            printf("Usage: %s some_shortcut.lnk\n", argv[0]);
            return 0;
        }

        char path[1024] = {0};
        if (clnk_get_path_buf(argv[1], path, sizeof(path))) {
            printf("Path: %s\n", path);
        }

        char* path2 = clnk_get_path(argv[1]);
        if (path2) {
            printf("Path: %s\n", path2);
            free(path2);
        }

        return 0;
    }



*/

#ifndef CLNK_H
#define CLNK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// TODO remove, make C89 compliant
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char* clnk_get_path_buf(const char* lnk_file, char* buf, int size);
char* clnk_get_path_buf_from_file(FILE* f, char* buf, int size);

char* clnk_get_path_from_file(FILE* f);
char* clnk_get_path(const char* lnk_file);

#ifdef __cplusplus
}
#endif

// end CLNK_H
#endif

#ifdef CLNK_IMPLEMENTATION

int validate_lnk_file(FILE* f)
{
	uint8_t sig[4];
	uint8_t guid[16] = {0};

	rewind(f);
	fread(sig, 4, 1, f);

	if (memcmp(sig, "L\0\0\0", 4)) {
		puts("This is not a .lnk file!");
		return 0;
	}
	fread(guid, 16, 1, f);

	char guid_valid[16] = "\x01\x14\x02\x00\x00\x00\x00\x00\xc0\x00\x00\x00\x00\x00\x00" "F";
	if (memcmp(guid, guid_valid, 16)) {
		puts("Cannot read this kind of .lnk file!");
		return 0;
	}
	return 1;
}

#ifdef __BIG_ENDIAN__
void make_native_u16(char buf[2])
{
	char tmp = buf[0];
	buf[0] = buf[1];
	buf[1] = tmp;
}
void make_native_u32(char buf[4])
{
	char tmp = buf[0];
	buf[0] = buf[3];
	buf[3] = tmp;

	tmp = buf[2];
	buf[2] = buf[1];
	buf[1] = tmp;
}
#else
#define make_native_u16(x)
#define make_native_u32(x)
#endif


int get_path_offset(FILE* f)
{
	fseek(f, 76, SEEK_SET);

	// assume LSB machine for now
	uint16_t items;
	fread(&items, 2, 1, f);
	make_native_u16((char*)&items);

	int struct_start = 78 + items;
	int base_path_off_off = struct_start + 16;

	uint32_t base_path_off;
	fseek(f, base_path_off_off, SEEK_SET);
	fread(&base_path_off, 4, 1, f);
	make_native_u32((char*)&base_path_off);

	base_path_off += struct_start;
	return base_path_off;
}

char* extract_cstring_buf(FILE* f, int offset, char* out_buf, int size)
{
	fseek(f, offset, SEEK_SET);

	char* s = out_buf;
	int i=0;
	do {
		if (!fread(&s[i], 1, 1, f)) {
			s[i] = 0;
			puts("fread failed");
			return NULL; // TODO hmm
		}
		i++;
	} while (s[i-1] && i < size);

	if (s[i-1]) {
		s[i] = 0;
		puts("ran out of space");
		return NULL;
	}
	return s;
}

char* extract_cstring(FILE* f, int offset)
{
	fseek(f, offset, SEEK_SET);

	int cap = 1024;
	char* s = (char*)malloc(cap); assert(s);
	char* tmp;

	// Could read in chunks rather than bytes but the path would
	// have to be impractically long for that to make a perf difference
	int i=0;
	do {
		if (i == cap) {
			tmp = (char*)realloc(s, cap*2);
			if (!tmp) {
				free(s);
				return NULL;
			}
			cap *= 2;
			s = tmp;
		}

		if (!fread(&s[i], 1, 1, f)) {
			s[i] = 0;
			puts("fread failed");
			free(s);
			return NULL;
		}
		i++;
	} while (s[i-1]);

	// resize to fit, shrinking should never fail imo but...
	tmp = realloc(s, i);
	if (!tmp) {
		// oh well better to have the string with the extra space
		return s;
	}

	return tmp;
}


char* clnk_get_path_buf(const char* lnk_file, char* buf, int size)
{
	FILE* f = fopen(lnk_file, "rb");
	if (!f) {
		perror("Couldn't open file");
		return NULL;
	}
	char* ret = clnk_get_path_buf_from_file(f, buf, size);
	fclose(f);
	return ret;
}

char* clnk_get_path_buf_from_file(FILE* f, char* buf, int size)
{
	long pos = ftell(f);
	if (!validate_lnk_file(f)) {
		return NULL;
	}
	int base_path_off = get_path_offset(f);
	char* ret = extract_cstring_buf(f, base_path_off, buf, size);
	fseek(f, pos, SEEK_SET);
	return ret;
}

char* clnk_get_path(const char* lnk_file)
{
	FILE* f = fopen(lnk_file, "rb");
	if (!f) {
		perror("Couldn't open file");
		return NULL;
	}
	char* ret = clnk_get_path_from_file(f);
	fclose(f);
	return ret;
}

char* clnk_get_path_from_file(FILE* f)
{
	long pos = ftell(f);
	if (!validate_lnk_file(f)) {
		return NULL;
	}
	int base_path_off = get_path_offset(f);
	char* ret = extract_cstring(f, base_path_off);
	fseek(f, pos, SEEK_SET);
	return ret;
}

#undef CLNK_IMPLEMENTATION
#endif


// The MIT License (MIT)
// 
// Copyright (c) 2025 Robert Winkler
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
// to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
