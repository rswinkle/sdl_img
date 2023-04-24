
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int cmp_char_lt(const void* a, const void* b)
{
	char a_ = *(char*)a;
	char b_ = *(char*)b;

	if (a_ < b_)
		return -1;
	if (a_ > b_)
		return 1;

	return 0;
}

int cmp_int_lt(const void* a, const void* b)
{
	int a_ = *(int*)a;
	int b_ = *(int*)b;

	if (a_ < b_)
		return -1;
	if (a_ > b_)
		return 1;

	return 0;
}

int cmp_float_lt(const void* a, const void* b)
{
	float a_ = *(float*)a;
	float b_ = *(float*)b;

	if (a_ < b_)
		return -1;
	if (a_ > b_)
		return 1;

	return 0;
}

int cmp_double_lt(const void* a, const void* b)
{
	double a_ = *(double*)a;
	double b_ = *(double*)b;

	if (a_ < b_)
		return -1;
	if (a_ > b_)
		return 1;

	return 0;
}

/* greater than */
int cmp_char_gt(const void* a, const void* b)
{
	char a_ = *(char*)a;
	char b_ = *(char*)b;

	if (a_ > b_)
		return -1;
	if (a_ < b_)
		return 1;

	return 0;
}

int cmp_int_gt(const void* a, const void* b)
{
	int a_ = *(int*)a;
	int b_ = *(int*)b;

	if (a_ > b_)
		return -1;
	if (a_ < b_)
		return 1;

	return 0;
}

int cmp_float_gt(const void* a, const void* b)
{
	float a_ = *(float*)a;
	float b_ = *(float*)b;

	if (a_ > b_)
		return -1;
	if (a_ < b_)
		return 1;

	return 0;
}

int cmp_double_gt(const void* a, const void* b)
{
	double a_ = *(double*)a;
	double b_ = *(double*)b;

	if (a_ > b_)
		return -1;
	if (a_ < b_)
		return 1;

	return 0;
}


void mirrored_qsort(void* a, size_t n, size_t size, int(*compare)(const void*, const void*), int count, ...);


int main()
{
#define LEN 26
	int ai[LEN];
	float af[LEN];
	double ad[LEN];
	char ac[LEN];

	for (int i=0; i<LEN; ++i) {
		ai[i] = i;
		af[i] = i*2.0;
		ad[i] = i*3.0;
		ac[i] = 'z' - i;
	}

	//mirrored_qsort(ai, LEN, sizeof(int), cmp_int_gt, 3, ad, sizeof(double), af, sizeof(float), ac, 1);
	//mirrored_qsort(af, LEN, sizeof(float), cmp_float_gt, 3, ad, sizeof(double), ai, sizeof(int), ac, 1);
	//mirrored_qsort(ad, LEN, sizeof(double), cmp_char_lt, 2, af, sizeof(float), ai, sizeof(int), ac, 1);
	mirrored_qsort(ac, LEN, 1, cmp_char_lt, 3, ad, sizeof(double), af, sizeof(float), ai, sizeof(int));

	for (int i=0; i<LEN; ++i) {
		printf("%5d %5.1f %5.1f %5c\n", ai[i], af[i], ad[i], ac[i]);
	}



	return 0;
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

