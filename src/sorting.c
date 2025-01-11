// The MIT License (MIT)
// 
// Copyright (c) 2017-2025 Robert Winkler
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

// plain sort
int cmp_string_lt(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}


typedef int (*compare_func)(const void* a, const void* b);

// TODO use quick sort or generic quicksort
//
//Mirrored Insertion sort, sorts a and b based on a
void sort(file* a, thumb_state* b, i64 n, compare_func cmp)
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
int generic_partition(void* array, i64 p, i64 r, i64 size, int(*compare)(const void*, const void*), int count, void** arrays, int* sizes)
{
	char* a = (char*)array;
	char* x = &a[r*size];
	i64 i = p-1;
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

	for (i64 j=p; j<r; ++j) {
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

void generic_qsort_recurse(void* a, i64 p, i64 r, i64 size, int(*compare)(const void*, const void*), int count, void** arrays, int* sizes)
{
	if (p < r && ~r) {
		int q = generic_partition(a, p, r, size, compare, count, arrays, sizes);
		generic_qsort_recurse(a, p, q-1, size, compare, count, arrays, sizes);
		generic_qsort_recurse(a, q+1, r, size, compare, count, arrays, sizes);
	}
}

void generic_qsort(void* a, i64 n, i64 size, int(*compare)(const void* , const void*), int count, void** arrays, int* sizes)
{
	generic_qsort_recurse(a, 0, n-1, size, compare, count, arrays, sizes);
}

void mirrored_qsort(void* a, i64 n, i64 size, int(*compare)(const void*, const void*), int count, ...)
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

