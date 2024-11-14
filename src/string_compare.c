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

extern inline i64 parsenum(char **a_p)
{
	char *a = *a_p;
	i64 result = *a - '0';
	++a;

	while (isnum(*a)) {
		// signed integer overflow undefined behavior on very large numbers
		// do I care about that here?
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

		i64 a0, b0; // will contain either a number or a letter

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

