
#include "myinttypes.h"
#include "c_utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

//POSIX (mostly) works with MinGW64
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// for getpwuid and getuid
#include <pwd.h>
#include <unistd.h>

#define CVECTOR_IMPLEMENTATION
#define CVEC_ONLY_INT
#define CVEC_ONLY_STR
#define CVEC_SIZE_T i64
#define PRIcv_sz PRIiMAX
#include "cvector.h"

#define STRBUF_SZ 1024
#define FONT_SIZE 24
#define PATH_SEPARATOR '/'
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

#define TRUE 1
#define FALSE 0

enum { SORT_NAME, SORT_PATH, SORT_SIZE, SORT_MODIFIED, NUM_USEREVENTS };

#define RESIZE(x) ((x+1)*2)

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


#define FILE_LIST_SZ 20
#define MAX_PATH_LEN 512
// TODO name? file_explorer? selector?
typedef struct file_browser
{
	char dir[MAX_PATH_LEN];   // cur location
	char file[MAX_PATH_LEN];  // return "value" ie file selected 

	// special bookmarked locations
	char home[MAX_PATH_LEN];
	char desktop[MAX_PATH_LEN];

	// list of files in cur directory
	cvector_file files;
	int up_to_date;

	int begin;
	int end;

} file_browser;

enum { NONE, NAME_UP, NAME_DOWN, PATH_UP, PATH_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };

int running;



cvector_str list1;
//cvector_i selected;
cvector_file files;

void print_browser(file_browser* fb);
void search_filenames(cvector_i* search_results, cvector_file* files, char* text);
int init_file_browser(file_browser* fb, const char* start_dir);

const char* get_homedir();
int myscandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts, int recurse);
char* mydirname(const char* path, char* dirpath);
char* mybasename(const char* path, char* base);
void normalize_path(char* path);
int bytes2str(int bytes, char* buf, int len);


int main(int argc, char** argv)
{
	printf("sizeof(file) == %d\n", (int)sizeof(file));
	printf("sizeof(time_t) == %d\n", (int)sizeof(time_t));
	printf("sizeof(long) == %d\n", (int)sizeof(long));

	printf("homedir = '%s'\n", get_homedir());

	const char* exts[] = { ".jpg" };
	//myscandir(&files, "/home/robert/", NULL, 0, 0);
	//myscandir(&files, "/home/robert/Pictures/Monica/", NULL, 0, 0);
	/*
	myscandir(&files, "/home/robert/Pictures/Monica", exts, 1, 0);

	printf("n_results = %ld\n", files.size);
	for (int i=0; i<files.size; i++) {
		printf("%-20s%20s%30s\n", files.a[i].name, files.a[i].size_str, files.a[i].mod_str);
	}

	*/


	running = 1;

	file_browser browser = { 0 };

	char* start_dir = NULL;
	if (argc == 2) {
		start_dir = argv[1];
	}

	init_file_browser(&browser, start_dir);

	while (!browser.file[0]) {
		print_browser(&browser);
	}

	printf("You selected %s\n", browser.file);





	return 0;
}

void print_browser(file_browser* fb)
{
	cvector_file* f = &fb->files;

	printf("H. home = %s\n", fb->home);
	printf("D. desktop = %s\n", fb->desktop);
	printf("P. To parent directory\n");

	printf("dir = %s\n", fb->dir);
	
	printf("There are %ld files\n", f->size);

	int invalid = 0;
	static int pos;

	for (int i=fb->begin, j=0; i<fb->end; i++, j++) {
		printf("%2d %-40s%20s%30s\n", j, f->a[i].name, f->a[i].size_str, f->a[i].mod_str);
	}

	printf("S. down\nW. up\nF. choose file\n");
	printf("Enter selection: ");

	char line_buf[STRBUF_SZ];

	char choice = read_char(stdin, SPACE_SET, 0, 1);
	if (choice == 'f' || choice == 'F') {
		int fn = 0;
		do {
			printf("Enter file number: ");
			
			// TODO properly
			freadstring_into_str(stdin, '\n', line_buf, STRBUF_SZ);
		} while (sscanf(line_buf, "%d", &fn) != 1 || fn < 0 || fn >= fb->end - fb->begin);

		int idx = fb->begin + fn;
		printf("fn = %d, idx = %d\n", fn, idx);


		// it's a directory, switch to it
		if (f->a[idx].size == -1) {
			printf("switching to '%s'\n", f->a[idx].path);
			strncpy(fb->dir, f->a[idx].path, MAX_PATH_LEN);
			myscandir(&fb->files, fb->dir, NULL, 0, FALSE);
			fb->begin = 0;
		} else {
			strncpy(fb->file, f->a[idx].path, MAX_PATH_LEN);
		}
	} else {
		int len;
		char* c;
		invalid = 0;
		switch (choice) {
			case 'H':
			case 'h':
				if (strncmp(fb->dir, fb->home, MAX_PATH_LEN)) {
					strcpy(fb->dir, fb->home);
					myscandir(&fb->files, fb->dir, NULL, 0, FALSE);
					fb->begin = 0;
				}
				break;
			case 'D':
			case 'd':
				if (strncmp(fb->dir, fb->desktop, MAX_PATH_LEN)) {
					strcpy(fb->dir, fb->desktop);
					myscandir(&fb->files, fb->dir, NULL, 0, FALSE);
					fb->begin = 0;
				}
				break;
			case 'P':
			case 'p':
					len = strlen(fb->dir);
					fb->dir[len-1] = 0; // erase trailing '/'

					// find next '/'
					c = strrchr(fb->dir, '/');
					c[1] = 0;   // erase everything after last /
					myscandir(&fb->files, fb->dir, NULL, 0, FALSE);
					fb->begin = 0;
				break;
			case 'S':
			case 's':
				fb->begin += FILE_LIST_SZ;
				if (fb->begin >= fb->files.size) {
					fb->begin = 0;
				}
				break;
			case 'W':
			case 'w':
				// TODO should I wrap around the top like I do the bottom?
				fb->begin -= FILE_LIST_SZ;
				if (fb->begin < 0) {
					// wrap if you were already at the top
					if (fb->begin == -FILE_LIST_SZ && fb->files.size > FILE_LIST_SZ) {
						fb->begin = fb->files.size - FILE_LIST_SZ;
					} else {
						fb->begin = 0;  // just go all the way to the top
					}
				}
				break;

			default:
				invalid = 1;
				puts("Invalid choice.");
		}
	}

	if (!invalid) {
		fb->end = fb->begin + FILE_LIST_SZ;
		if (fb->end > fb->files.size) {
			fb->end = fb->files.size;
		}
	}


}




// renamed to not conflict with <dirent.h>'s scandir
// which I could probably use to accomplish  most of this...
int myscandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts, int recurse)
{
	char fullpath[STRBUF_SZ] = { 0 };
	struct stat file_stat;
	struct dirent* entry;
	int ret, i=0;
	DIR* dir;
	struct tm* tmp_tm;

	// can be used if I call stbi_info for files without extensions
	// or I could just pass NULLs
	int x, y, n;

	// empty files if not recursing, otherwise assume user knows what they're doing and
	// emptied it before top level call if they wanted to
	if (!recurse) {
		cvec_clear_file(files);
	}
	int start_size = files->size;

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		return 0;
	}

	char* tmp;
	char* sep;
	char* ext = NULL;
	file f;

	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			// path too long
			assert(ret >= STRBUF_SZ);
			return 0;
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}


		// S_ISLNK() doesn't seem to work but d_type works, though the man page
		// says it's not supported on all filesystems... or windows TODO?
		// aggh I hate windows
#ifndef _WIN32
		if (S_ISDIR(file_stat.st_mode) && entry->d_type != DT_LNK)
#else
		if (S_ISDIR(file_stat.st_mode))
#endif
		{
			if (recurse) {
				myscandir(files, fullpath, exts, num_exts, recurse);
				continue;
			}

			memset(&f, 0, sizeof(file));
			tmp = realpath(fullpath, NULL);
			f.path = realloc(tmp, strlen(tmp)+1);
			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;
			f.size = -1;
			cvec_push_file(files, &f);
			continue;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			continue;
		}

		ext = strrchr(entry->d_name, '.');

		// TODO
		if (ext && num_exts)
		{
			for (i=0; i<num_exts; ++i) {
				if (!strcasecmp(ext, exts[i]))
					break;
			}
			if (i == num_exts)
				continue;
		}

		// have to use fullpath not d_name in case we're in a recursive call
#ifndef _WIN32
		// resize to exact length to save memory, reduce internal
		// fragmentation.  This dropped memory use by 80% in certain
		// extreme cases.
		//f.path = realpath(fullpath, NULL);
		tmp = realpath(fullpath, NULL);
		f.path = realloc(tmp, strlen(tmp)+1);
#else
		f.path = CVEC_STRDUP(fullpath);
#endif

		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;

		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(files, &f);
	}

	printf("Found %"PRIcv_sz" files in %s\n", files->size-start_size, dirpath);

	closedir(dir);
	return 1;
}

// works same as SUSv2 libgen.h dirname except that
// dirpath is user provided output buffer, assumed large
// enough, return value is dirpath
char* mydirname(const char* path, char* dirpath)
{
	if (!path || !path[0]) {
		dirpath[0] = '.';
		dirpath[1] = 0;
		return dirpath;
	}

	char* last_slash = strrchr(path, PATH_SEPARATOR);
	if (last_slash) {
		strncpy(dirpath, path, last_slash-path);
		dirpath[last_slash-path] = 0;
	} else {
		dirpath[0] = '.';
		dirpath[1] = 0;
	}

	return dirpath;
}

// same as SUSv2 basename in libgen.h except base is output
// buffer
char* mybasename(const char* path, char* base)
{
	if (!path || !path[0]) {
		base[0] = '.';
		base[1] = 0;
		return base;
	}

	int end = strlen(path) - 1;

	if (path[end] == PATH_SEPARATOR)
		end--;

	int start = end;
	while (path[start] != PATH_SEPARATOR && start != 0)
		start--;
	if (path[start] == PATH_SEPARATOR)
		start++;

	memcpy(base, &path[start], end-start+1);
	base[end-start+1] = 0;

	return base;
}

//stupid windows
void normalize_path(char* path)
{
	for (int i=0; path[i]; ++i) {
		if (path[i] == '\\')
			path[i] = '/';
	}
}

int bytes2str(int bytes, char* buf, int len)
{
	// MiB KiB? 2^10, 2^20?
	// char* iec_sizes[3] = { "bytes", "KiB", "MiB" };
	char* si_sizes[3] = { "bytes", "KB", "MB" }; // GB?  no way

	char** sizes = si_sizes;
	int i = 0;
	double sz = bytes;
	if (sz >= 1000000) {
		sz /= 1000000;
		i = 2;
	} else if (sz >= 1000) {
		sz /= 1000;
		i = 1;
	} else {
		i = 0;
	}

	int ret = snprintf(buf, len, ((i) ? "%.1f %s" : "%.0f %s") , sz, sizes[i]);
	if (ret >= len)
		return 0;

	return 1;
}


void search_filenames(cvector_i* search_results, cvector_file* files, char* text)
{
	// strcasestr is causing problems on windows
	// so just convert to lower before using strstr
	char lowertext[STRBUF_SZ] = { 0 };
	char lowername[STRBUF_SZ] = { 0 };

	// start at 1 to cut off '/'
	for (int i=0; text[i]; ++i) {
		lowertext[i] = tolower(text[i]);
	}

	// it'd be kind of cool to add results of multiple searches together if we leave this out
	// of course there might be duplicates.  Or we could make it search within the existing
	// search results, so consecutive searches are && together...
	search_results->size = 0;
	
	int j;
	for (int i=0; i<files->size; ++i) {

		for (j=0; files->a[i].name[j]; ++j) {
			lowername[j] = tolower(files->a[i].name[j]);
		}
		lowername[j] = 0;

		// searching name since I'm showing names not paths in the list
		if (strstr(lowername, lowertext)) {
			printf("Adding %s\n", files->a[i].path);
			cvec_push_i(search_results, i);
		}
	}
	printf("found %d matches\n", (int)search_results->size);
}

const char* get_homedir()
{
	const char* home = getenv("HOME");
#ifdef _WIN32
	if (!home) home = getenv("USERPROFILE");
#else
	if (!home) home = getpwuid(getuid())->pw_dir;
#endif
	return home;
}

// TODO pass extensions?
int init_file_browser(file_browser* browser, const char* start_dir)
{
	//if (fb->up_to_date) return
	
	memset(browser, 0, sizeof(file_browser));
	
	const char* home = get_homedir();

	size_t l;
	strncpy(browser->home, home, MAX_PATH_LEN);
	browser->home[MAX_PATH_LEN - 1] = 0;
	l = strlen(browser->home);
	strcpy(browser->home + l, "/");

	const char* sd = home;
	if (start_dir) {
		struct stat file_stat;
		if (stat(start_dir, &file_stat)) {
			perror("Could not stat start_dir, will use home directory");
		} else {
			sd = start_dir;
		}
	}
	strcpy(browser->dir, sd);


	strcpy(browser->desktop, browser->home);
	l = strlen(browser->desktop);
	strcpy(browser->desktop + l, "desktop/");

	browser->files.elem_free = free_file;

	browser->end = 20;

	myscandir(&browser->files, browser->dir, NULL, 0, 0);

	return 1;
}





