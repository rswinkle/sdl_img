// The MIT License (MIT)
// 
// Copyright (c) 2017-2024 Robert Winkler
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
//
// TODO struct packing?  save a few bytes?
typedef struct file
{
	char* path;   // could be url;

	// time_t is a long int ...
	time_t modified;
	int size;     // in bytes (hard to believe it'd be bigger than ~2.1 GB)

	//  caching for list mode
	char mod_str[MOD_STR_BUF];
	char size_str[SIZE_STR_BUF];
	char* name;  // pointing at filename in path
} file;

CVEC_NEW_DECLS2(file)

CVEC_NEW_DEFS2(file, RESIZE)

void free_file(void* f)
{
	free(((file*)f)->path);
}

// comparison functions
int filename_cmp_lt(const void* a, const void* b)
{
	return StringCompare(((file*)a)->name, ((file*)b)->name);
}

int filename_cmp_gt(const void* a, const void* b)
{
	return StringCompare(((file*)b)->name, ((file*)a)->name);
}

int filepath_cmp_lt(const void* a, const void* b)
{
	return StringCompare(((file*)a)->path, ((file*)b)->path);
}

int filepath_cmp_gt(const void* a, const void* b)
{
	return StringCompare(((file*)b)->path, ((file*)a)->path);
}

int filesize_cmp_lt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->size < f2->size)
		return -1;
	if (f1->size > f2->size)
		return 1;

	return 0;
}

int filesize_cmp_gt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->size > f2->size)
		return -1;
	if (f1->size < f2->size)
		return 1;

	return 0;
}

int filemodified_cmp_lt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->modified < f2->modified)
		return -1;
	if (f1->modified > f2->modified)
		return 1;

	return 0;
}

int filemodified_cmp_gt(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->modified > f2->modified)
		return -1;
	if (f1->modified < f2->modified)
		return 1;

	return 0;
}

