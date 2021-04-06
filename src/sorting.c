// The MIT License (MIT)
// 
// Copyright (c) 2017-2021 Robert Winkler
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

// NOTE(rswinkle): string sorting taken from
// https://github.com/nothings/stb-imv/blob/master/imv.c
//
// derived from michael herf's code: http://www.stereopsis.com/strcmp4humans.html
//
// Also see GNU strverscmp for similar functionality

// sorts like this:
//     foo.jpg
//     foo1.jpg
//     foo2.jpg
//     foo10.jpg
//     foo_1.jpg
//     food.jpg

// use upper, not lower, to get better sorting versus '_'
// no, use lower not upper, to get sorting that matches explorer
extern inline char tupper(char b)
{
	if (b >= 'A' && b <= 'Z') return b - 'A' + 'a';
	//if (b >= 'a' && b <= 'z') return b - 'a' + 'A';
	return b;
}

extern inline char isnum(char b)
{
	if (b >= '0' && b <= '9') return 1;
	return 0;
}

extern inline int parsenum(char **a_p)
{
	char *a = *a_p;
	int result = *a - '0';
	++a;

	while (isnum(*a)) {
		result *= 10;
		result += *a - '0';
		++a;
	}

	*a_p = a-1;
	return result;
}

int StringCompare(char *a, char *b)
{
   char *orig_a = a, *orig_b = b;

	if (a == b) return 0;

	if (a == NULL) return -1;
	if (b == NULL) return 1;

	while (*a && *b) {

		int a0, b0;	// will contain either a number or a letter

		if (isnum(*a) && isnum(*b)) {
			a0 = parsenum(&a);
			b0 = parsenum(&b);
		} else {
		// if they are mixed number and character, use ASCII comparison
		// order between them (number before character), not herf's
		// approach (numbers after everything else). this produces the order:
		//     foo.jpg
		//     foo1.jpg
		//     food.jpg
		//     foo_.jpg
		// which I think looks better than having foo_ before food (but
		// I could be wrong, given how a blank space sorts)

			a0 = tupper(*a);
			b0 = tupper(*b);
		}

		if (a0 < b0) return -1;
		if (a0 > b0) return 1;

		++a;
		++b;
	}

	if (*a) return 1;
	if (*b) return -1;

	{
		// if strings differ only by leading 0s, use case-insensitive ASCII sort
		// (note, we should work this out more efficiently by noticing which one changes length first)
		int z = strcasecmp(orig_a, orig_b);
		if (z) return z;
		// if identical case-insensitive, return ASCII sort
		return strcmp(orig_a, orig_b);
	}
}

int StringCompareSort(const void *p, const void *q)
{
   return StringCompare(*(char **) p, *(char **) q);
}

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

// plain sort
int cmp_string_lt(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}


typedef int (*compare_func)(const void* a, const void* b);

// TODO use quick sort or generic quicksort
//
//Mirrored Insertion sort, sorts a and b based on a
void sort(file* a, thumb_state* b, size_t n, compare_func cmp)
{
	int j;
	file temp;
	thumb_state tmp_thumb;

	if (b) {
		for (int i=1; i<n; ++i) {
			j = i-1;
			temp = a[i];
			tmp_thumb = b[i];
			
			while (j >= 0 && cmp(&a[j], &temp) > 0) {
				a[j+1] = a[j];
				b[j+1] = b[j];
				--j;
			}
			a[j+1] = temp;
			b[j+1] = tmp_thumb;
		}
	} else {
		for (int i=1; i<n; ++i) {
			j = i-1;
			temp = a[i];
			
			while (j >= 0 && cmp(&a[j], &temp) > 0) {
				a[j+1] = a[j];
				--j;
			}
			a[j+1] = temp;
		}
	}

	return;
}


#define MAX_MIRRORS 10

// ptr1, ptr2 are char*, temp and counter are int
// all except size are modified
#define inline_swap(ptr1, ptr2, temp, size, counter) \
	counter = size;                               \
	while (counter >= sizeof(int)) {            \
		temp = *(int*)ptr1;                     \
		*(int*)ptr1 = *(int*)ptr2;              \
		*(int*)ptr2 = temp;                     \
		counter -= sizeof(int);                 \
		ptr1 += sizeof(int);                    \
		ptr2 += sizeof(int);                    \
	}                                           \
	while (counter) {                           \
		temp = *ptr1;                           \
		*ptr1 = *ptr2;                          \
		*ptr2 = temp;                           \
		--counter;                              \
		++ptr1;                                 \
		++ptr2;                                 \
	}



// Generic Quicksort.  Partion pivots on last element
int generic_partition(void* array, size_t p, size_t r, size_t size, int(*compare)(const void*, const void*), int count, void** arrays, int* sizes)
{
	char* a = (char*)array;
	char* x = &a[r*size];
	size_t i = p-1;
	int temp, k;
	char* p1, *p2;
	int sz;

	// pick random pivot, and swap with last element
	int pivot = rand() % (r-p+1) + p;
	p1 = &a[pivot * size];
	inline_swap(x, p1, temp, size, k)

	x -= size; // inline_swap modifies x

	for (int m=0; m<count; ++m) {
		sz = sizes[m];
		p1 = &((char*)arrays[m])[r*sz];
		p2 = &((char*)arrays[m])[pivot*sz];
		inline_swap(p1, p2, temp, sz, k)
	}

	for (size_t j=p; j<r; ++j) {
		if (compare(&a[j*size], x) <= 0) {
			++i;

			// TODO use sizeof(long long)? memcopy/memmove?
			p1 = &a[j*size];
			p2 = &a[i*size];
			inline_swap(p1, p2, temp, size, k)

			for (int m=0; m<count; ++m) {
				sz = sizes[m];
				p1 = &((char*)arrays[m])[j*sz];
				p2 = &((char*)arrays[m])[i*sz];
				inline_swap(p1, p2, temp, sz, k)
			}


		}
	}

	++i;
	p1 = &a[i*size];
	p2 = x;
	inline_swap(p1, p2, temp, size, k)

	for (int m=0; m<count; ++m) {
		sz = sizes[m];
		p1 = &((char*)arrays[m])[i*sz];
		p2 = &((char*)arrays[m])[r*sz];
		inline_swap(p1, p2, temp, sz, k)
	}
	return i;
}

void generic_qsort_recurse(void* a, size_t p, size_t r, size_t size, int(*compare)(const void*, const void*), int count, void** arrays, int* sizes)
{
	if (p < r && ~r) {
		int q = generic_partition(a, p, r, size, compare, count, arrays, sizes);
		generic_qsort_recurse(a, p, q-1, size, compare, count, arrays, sizes);
		generic_qsort_recurse(a, q+1, r, size, compare, count, arrays, sizes);
	}
}

void generic_qsort(void* a, size_t n, size_t size, int(*compare)(const void* , const void*), int count, void** arrays, int* sizes)
{
	generic_qsort_recurse(a, 0, n-1, size, compare, count, arrays, sizes);
}

void mirrored_qsort(void* a, size_t n, size_t size, int(*compare)(const void*, const void*), int count, ...)
{
	va_list args;
	va_start(args, count);
	if (count > MAX_MIRRORS) {
		//puts("Too many mirrored arrays");
		return;
	}

	void* arrays[MAX_MIRRORS];
	int type_sizes[MAX_MIRRORS];
	for (int i=0; i<count; ++i) {
		arrays[i] = va_arg(args, void*);
		type_sizes[i] = va_arg(args, int);
	}

	generic_qsort(a, n, size, compare, count, arrays, type_sizes);
	va_end(args);


}
#undef MAX_MIRRORS

