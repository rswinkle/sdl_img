
// TODO maybe I should move these recents related functions to their own file
char* uri_decode(const char* str)
{
	int len = strlen(str);
	char* dst = malloc(len+1);
	int x, a, b;
	char hex[3] = {0};
	int j = 0;
	
	for (int i=0; str[i]; ++i) {
		if (str[i] == '%') {
			a = str[i+1];
			b = str[i+2];
			if (!a) {
				// copy %?
				return dst;
			}
			if (isxdigit(a) && isxdigit(b)) {
				hex[0] = a;
				hex[1] = b;
				x = strtol(hex, NULL, 16);
				dst[j++] = x;
				i += 2;
			}
		} else if (str[i] == '&') {
			if (!strncmp(str+i+1, "apos;", 5)) {
				dst[j++] = '\'';
				i += 5;
			} else if (!strncmp(str+i+1, "amp;", 4)) {
				dst[j++] = '&';
				i += 4;
			} else {
				dst[j++] = str[i];
			}
		} else {
			dst[j++] = str[i];
		}
	}
	dst = realloc(dst, j+1);
	dst[j] = 0;

	return dst;
}

/** Read file into into allocated string, return in *out
 * Data is NULL terminated.  file is closed before returning (since you
 * just read the entire file ...). */
int file_read(FILE* file, char** out)
{
	assert(file);
	assert(out);

	char* data = NULL;
	long size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (size <= 0) {
		if (size == -1)
			perror("ftell failure");
		fclose(file);
		return 0;
	}

	data = (char*)malloc(size+1);
	if (!data) {
		fclose(file);
		return 0;
	}

	rewind(file);
	if (!fread(data, size, 1, file)) {
		perror("fread failure");
		fclose(file);
		free(data);
		return 0;
	}

	data[size] = 0; /* null terminate in all cases even if reading binary data */

	*out = data;

	fclose(file);
	return size;
}

int linux_recents(cvector_str* recents, void* userdata)
{
	assert(recents);

	char path[MAX_PATH_LEN];
	snprintf(path, MAX_PATH_LEN, "%s/.local/share/recently-used.xbel", get_homedir());
	FILE* recents_file = fopen(path, "r");
	if (!recents_file) {
		perror("Could not open recents file");
		return 0;
	}

	// TODO cvector_str* or char***, obviously leaning toward former
	cvec_reserve_str(recents, 200);

	char* text = NULL;
	int file_len = 0;

	if (!(file_len = file_read(recents_file, &text))) {
		// empty file
		return 0;
	}

	const char needle[] = "bookmark href=\"file://";
	int needle_len = strlen(needle);

	char* start_search = text;

	char* result = NULL;
	char* end;
	char* dst;
	int i = 0;
	while ((result = strstr(start_search, needle))) {
		result += needle_len;
		end = strchr(result, '\"');

		*end = 0;

		dst = uri_decode(result);
		//printf("%d %s\n%d %s\n", i, result, i, dst);
		cvec_pushm_str(recents, dst);

		// continue search just past the end of last result
		start_search = end+1;
		i++;
	}

	free(text);

	// unnecessary with size included in cvector_str...
	return i;
}

#define CLNK_IMPLEMENTATION
#include "clnk.h"

int windows_recents(cvector_str* recents, void* userdata)
{
	assert(recents);

	char recents_dir_buf[STRBUF_SZ];
	int ret = snprintf(recents_dir_buf, STRBUF_SZ, "%s\\Microsoft\\Windows\\Recent", getenv("APPDATA"));
	if (ret >= STRBUF_SZ) {
		return 0;
	}

	// TODO could just use scandir or inline opendir/readdir since all the extra file work is wasted...
	// Or have windows and lnk files be a special case and just switch to the directory but when they
	// click on a lnk file extract the path right then...?
	cvector_file links = {0};
	const char* exts[] = { ".lnk" }; // shouldn't be anything else in the directory but jic

	fb_scandir(&links, recents_dir_buf, exts, 1, SDL_FALSE, SDL_FALSE);

	char* tmp;
	for (int i=0; i<links.size; ++i) {
		if ((tmp = clnk_get_path(links.a[i].path))) {
			normalize_path(tmp);
			cvec_pushm_str(recents, tmp);
		}
	}
	cvec_free_file(&links);
	return recents->size;
}
