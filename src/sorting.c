
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

int filename_cmp(const void* a, const void* b)
{
	char* pa = ((file*)a)->path;
	char* pb = ((file*)b)->path;
	char* tmp = strrchr(pa, '/'); // TODO test on windows but I think I normalize
	pa = (tmp) ? tmp+1 : pa;
	tmp = strrchr(pb, '/');
	pb = (tmp) ? tmp+1 : pb;
	return StringCompare(pa, pb);
}

int filepath_cmp(const void* a, const void* b)
{
	return StringCompare(((file*)a)->path, ((file*)b)->path);
}

// TODO add reverse sorts?  largest to smallest and newest to oldest
int filesize_cmp(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->size < f2->size)
		return -1;
	if (f1->size > f2->size)
		return 1;

	return 0;
}

int filemodified_cmp(const void* a, const void* b)
{
	file* f1 = (file*)a;
	file* f2 = (file*)b;
	if (f1->modified < f2->modified)
		return -1;
	if (f1->modified > f2->modified)
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

// Generic Quicksort.  Partion pivots on last element
int generic_partition(void* array, size_t p, size_t r, size_t size, int(*compare)(const void*, const void*))
{
	unsigned char* a = (unsigned char*)array;
	unsigned char* x = &a[r*size];
	size_t i = p-1;
	int temp, k;
	unsigned char* ptr1, *ptr2;

	for (size_t j=p; j<r; ++j) {
		if (compare(&a[j*size], x) <= 0) {
			++i;

			k = size;
			ptr1 = &a[j*size];
			ptr2 = &a[i*size];
			while (k >= sizeof(int)) {
				temp = *(int*)ptr1;
				*(int*)ptr1 = *(int*)ptr2;
				*(int*)ptr2 = temp;
				k -= sizeof(int);
				ptr1 += sizeof(int);
				ptr2 += sizeof(int);
			}
			while (k) {
				temp = *ptr1;
				*ptr1 = *ptr2;
				*ptr2 = temp;
				--k;
				++ptr1;
				++ptr2;
			}
		}
	}

	++i;
	k = size;
	ptr1 = &a[i*size];
	while (k >= sizeof(int)) {
		temp = *(int*)ptr1;
		*(int*)ptr1 = *(int*)x;
		*(int*)x = temp;
		k -= sizeof(int);
		ptr1 += sizeof(int);
		x += sizeof(int);
	}
	while (k) {
		temp = *ptr1;
		*ptr1 = *x;
		*x = temp;
		--k;
		++ptr1;
		++x;
	}
	return i;
}

void generic_qsort_recurse(void* a, size_t p, size_t r, size_t size, int(*compare)(const void*, const void*))
{
	if (p < r && ~r) {
		int q = generic_partition(a, p, r, size, compare);
		generic_qsort_recurse(a, p, q-1, size, compare);
		generic_qsort_recurse(a, q+1, r, size, compare);
	}
}

void generic_qsort(void* a, size_t n, size_t size, int(*compare)(const void* , const void*))
{
	generic_qsort_recurse(a, 0, n-1, size, compare);
}


