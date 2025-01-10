
// removed for unity build
// pulls in file and cvector
//#include "filebrowser.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

//POSIX (mostly) works with MinGW64
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#ifndef _WIN32
// for getpwuid and getuid
#include <pwd.h>
#else
#include <windows.h>
#endif


// inline edited filebrowser.h starting here

#ifndef FILE_TYPE_STR
#define FILE_TYPE_STR "Match Exts"
#endif

#define TRUE 1
#define FALSE 0

#ifndef FB_LOG
#define FB_LOG(A, ...) printf(A, __VA_ARGS__)
#endif

#define FILE_LIST_SZ 20
//#define STRBUF_SZ 512
#define MAX_PATH_LEN STRBUF_SZ
//#define PATH_SEPARATOR '/'

typedef int (*recents_func)(cvector_str* recents, void * userdata);
typedef int (*cmp_func)(const void* a, const void* b);
//enum { NAME_UP, NAME_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };


// TODO name? file_explorer? selector?
typedef struct file_browser
{
	char dir[MAX_PATH_LEN];   // cur location
	char file[MAX_PATH_LEN];  // return "value" ie file selected 

	// special bookmarked locations
	char home[MAX_PATH_LEN];
	char desktop[MAX_PATH_LEN];

	// searching
	char text_buf[STRBUF_SZ];
	int text_len;

	recents_func get_recents;
	void* userdata;

	// bools
	int is_recents;
	int is_search_results;
	int is_text_path; // could change to flag if I add a third option
	int list_setscroll;

	// does not own memory
	const char** exts;
	int num_exts;
	int ignore_exts; // if true, show all files, not just matching exts

	// list of files in cur directory
	cvector_file files;

	
	cvector_i search_results;
	int selection;

	int begin;
	int end;
	//int done; // true if a selection was made ie file[0] != 0

	int sorted_state;
	cmp_func c_func;

} file_browser;

// TODO re-order defs so I don't need prototypes?
int init_file_browser(file_browser* browser, const char** exts, int num_exts, const char* start_dir, recents_func r_func, void* userdata);
void free_file_browser(file_browser* fb);
void switch_dir(file_browser* fb, const char* dir);
void handle_recents(file_browser* fb);

void fb_search_filenames(file_browser* fb);
const char* get_homedir();
int fb_scandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts);
char* mydirname(const char* path, char* dirpath);
char* mybasename(const char* path, char* base);
void normalize_path(char* path);
int bytes2str(int bytes, char* buf, int len);

// end filebrowser.h


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
int init_file_browser(file_browser* browser, const char** exts, int num_exts, const char* start_dir, recents_func r_func, void* userdata)
{
	memset(browser, 0, sizeof(file_browser));
	
	const char* home = get_homedir();

	size_t l = 0;
	strncpy(browser->home, home, MAX_PATH_LEN);
#ifdef _WIN32
	normalize_path(browser->home);
#endif
	browser->home[MAX_PATH_LEN - 1] = 0;
	printf("home = %s\n", browser->home);

	home = browser->home;
	const char* sd = home;
	if (start_dir) {
		l = strlen(start_dir);
		struct stat file_stat;
		if (stat(start_dir, &file_stat)) {
			perror("Could not stat start_dir, will use home directory");
		} else if (l >= MAX_PATH_LEN) {
			fprintf(stderr, "start_dir path too long, will use home directory\n");
		} else {
			sd = start_dir;
		}
	}
	snprintf(browser->dir, MAX_PATH_LEN, "%s", sd);
	// cut off trailing '/'
	if (l > 1 && sd[l-1] == '/') {
		browser->dir[l-1] = 0;
	}

	// TODO snprintf instead of strncpy everywhere?
	strcpy(browser->desktop, browser->home);
	l = strlen(browser->desktop);
	strcpy(browser->desktop + l, "/Desktop");

	browser->files.elem_free = free_file;

	browser->end = 20;

	browser->exts = exts;
	browser->num_exts = num_exts;

	fb_scandir(&browser->files, browser->dir, exts, num_exts);

	qsort(browser->files.a, browser->files.size, sizeof(file), filename_cmp_lt);
	browser->sorted_state = NAME_UP;
	browser->c_func = filename_cmp_lt;

	browser->get_recents = r_func;
	browser->userdata = userdata;

	return 1;
}

void reset_file_browser(file_browser* fb, char* start_dir)
{
	// clear vectors and prior selection
	cvec_clear_file(&fb->files);
	cvec_clear_i(&fb->search_results);
	fb->is_search_results = FALSE;
	fb->file[0] = 0;
	fb->text_len = 0;
	fb->text_buf[0] = 0;

	// set start dir
	size_t l = 0;
	const char* sd = fb->home;
	if (start_dir) {
		l = strlen(start_dir);
		struct stat file_stat;
		if (stat(start_dir, &file_stat)) {
			perror("Could not stat start_dir, will use home directory");
		} else if (l >= MAX_PATH_LEN) {
			fprintf(stderr, "start_dir path too long, will use home directory\n");
		} else {
			sd = start_dir;
		}
	}
	snprintf(fb->dir, MAX_PATH_LEN, "%s", sd);
	// cut off trailing '/'
	if (l > 1 && sd[l-1] == '/') {
		fb->dir[l-1] = 0;
	}

	// scan and sort
	fb_scandir(&fb->files, fb->dir, fb->exts, fb->num_exts);

	qsort(fb->files.a, fb->files.size, sizeof(file), filename_cmp_lt);
	fb->sorted_state = NAME_UP;
	fb->c_func = filename_cmp_lt;
}

void free_file_browser(file_browser* fb)
{
	cvec_free_file(&fb->files);
	cvec_free_i(&fb->search_results);
	memset(fb, 0, sizeof(file_browser));
}

void handle_recents(file_browser* fb)
{
	file f;
	struct stat file_stat;
	struct tm* tmp_tm;
	char* sep;
	char* ext = NULL;

	cvector_str recents = {0};
	int n = fb->get_recents(&recents, fb->userdata);

	fb->is_recents = TRUE;
	fb->dir[0] = 0;
	cvec_clear_file(&fb->files);
	fb->selection = 0;

	const char** exts = fb->exts;
	const int num_exts = fb->num_exts;

	char* p;
	int i, j;
	for (i=0; i<n; i++) {
		p = recents.a[i];

		if (!fb->ignore_exts && num_exts) {
			if ((ext = strrchr(p, '.'))) {
				for (j=0; j<num_exts; ++j) {
					if (!strcasecmp(ext, exts[j]))
						break;
				}
				if (j == num_exts) {
					//free(p);
					continue;
				}
			}
		}
		if (stat(p, &file_stat)) {
			perror("stat");
			//free(p);
		} else {
			f.size = S_ISREG(file_stat.st_mode) ? file_stat.st_size : -1;
			f.path = p;
			f.modified = file_stat.st_mtime;

			bytes2str(f.size, f.size_str, SIZE_STR_BUF);
			tmp_tm = localtime(&f.modified);
			strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T


			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;

			cvec_pushm_file(&fb->files, &f);

			// NULL out pointer in recents string since we moved ownership of
			// the path string to fb->files and don't want a heap use after free
			recents.a[i] = 0;
		}
	}

	qsort(fb->files.a, fb->files.size, sizeof(file), fb->c_func);
	fb->list_setscroll = TRUE;

	FB_LOG("Found %"PRIcv_sz" recent files\n", fb->files.size);

	cvec_free_str(&recents);
}


void fb_search_filenames(file_browser* fb)
{
	// fast enough to do here?  I do it in events?
	char* text = fb->text_buf;
	text[fb->text_len] = 0;
	
	FB_LOG("Final text = \"%s\"\n", text);

	// strcasestr is causing problems on windows
	// so just convert to lower before using strstr
	char lowertext[STRBUF_SZ] = { 0 };
	char lowername[STRBUF_SZ] = { 0 };

	// start at 1 to cut off '/'
	for (int i=0; text[i]; ++i) {
		lowertext[i] = tolower(text[i]);
	}

	cvector_file* files = &fb->files;

	// it'd be kind of cool to add results of multiple searches together if we leave this out
	// of course there might be duplicates.  Or we could make it search within the existing
	// search results, so consecutive searches are && together...
	fb->search_results.size = 0;
	
	int j;
	for (int i=0; i<files->size; ++i) {

		for (j=0; files->a[i].name[j]; ++j) {
			lowername[j] = tolower(files->a[i].name[j]);
		}
		lowername[j] = 0;

		// searching name since I'm showing names not paths in the list
		if (strstr(lowername, lowertext)) {
			FB_LOG("Adding %s\n", files->a[i].path);
			cvec_push_i(&fb->search_results, i);
		}
	}
	FB_LOG("found %d matches\n", (int)fb->search_results.size);
}

// TODO would it be better to just use scandir + an extra pass to fill cvector of files?
// How portable would that be?  Windows? etc.
int fb_scandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts)
{
	assert(!num_exts || exts);

	char fullpath[STRBUF_SZ] = { 0 };
	struct stat file_stat;
	struct dirent* entry;
	int ret, i=0;
	DIR* dir;
	struct tm* tmp_tm;

	cvec_clear_file(files);

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		return 0;
	}

	char* sep;
	char* ext = NULL;
	file f;
	
	// This is to turn windows drives like C:/ into C: so the fullpath below doesn't become C://subdir
	// Can't remove the / before caling opendir or it won't work
	int l = strlen(dirpath);
	const char* fmt_strs[] = { "%s/%s", "%s%s" };
	int has_ts = dirpath[l-1] == '/';

	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, fmt_strs[has_ts], dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			// path too long
			assert(ret >= STRBUF_SZ);
			return 0;
		}
		if (stat(fullpath, &file_stat)) {
			printf("%s\n", fullpath);
			perror("stat");
			continue;
		}

		if (!S_ISREG(file_stat.st_mode) && !S_ISDIR(file_stat.st_mode)) {
			continue;
		}

		if (S_ISREG(file_stat.st_mode)) {
			f.size = file_stat.st_size;

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
		} else {
			f.size = -1;
		}

		// have to use fullpath not d_name in case we're in a recursive call
#ifndef _WIN32
		// resize to exact length to save memory, reduce internal
		// fragmentation.  This dropped memory use by 80% in certain
		// extreme cases.
		//f.path = realpath(fullpath, NULL);
		char* tmp = realpath(fullpath, NULL);
		f.path = realloc(tmp, strlen(tmp)+1);
#else
		f.path = CVEC_STRDUP(fullpath);
#endif

		f.modified = file_stat.st_mtime;

		// f.size set above separately for files vs directories
		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(files, &f);
	}

	FB_LOG("Found %"PRIcv_sz" files in %s\n", files->size, dirpath);

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

void normalize_path(char* path)
{
	if (path) {
		for (int i=0; path[i]; ++i) {
			if (path[i] == '\\') {
				path[i] = '/';
			}
		}
	}
}

int bytes2str(int bytes, char* buf, int len)
{
	// empty string for negative numbers
	if (bytes < 0) {
		buf[0] = 0;
		return 1;
	}

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


void switch_dir(file_browser* fb, const char* dir)
{
	if (dir) {
		if (!strncmp(fb->dir, dir, MAX_PATH_LEN)) {
			return;
		}
		strncpy(fb->dir, dir, MAX_PATH_LEN);
	}

	fb->is_recents = FALSE;
	fb->is_search_results = FALSE;
	fb->text_buf[0] = 0;
	fb->text_len = 0;

	FB_LOG("switching to '%s'\n", fb->dir);
#ifndef _WIN32
	fb_scandir(&fb->files, fb->dir, fb->exts, (fb->ignore_exts) ? 0 : fb->num_exts);
#else
	if (fb->dir[1]) {
		fb_scandir(&fb->files, fb->dir, fb->exts, (fb->ignore_exts) ? 0 : fb->num_exts);
	} else {
		// have to handle "root" special on windows since it doesn't have a unified filesystem
		// like *nix
		char buf[STRBUF_SZ];
		cvec_clear_file(&fb->files);
		int sz = GetLogicalDriveStrings(sizeof(buf), buf);
		file f = {0};
		f.size = -1;
		if (sz > 0) {
			char* p = buf, *p2;
			while (*p && (p2 = strchr(p, 0))) {
				p[2] = '/'; // change \ to / so "C:/" instead of "C:\"
				printf("%s\n", p);

				f.path = strdup(p);
				f.name = f.path;
				cvec_push_file(&fb->files, &f);

				p = p2+1;
			}
		} else {
			DWORD err = GetLastError();
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buf, sizeof(buf), 0);
			SDL_Log("Error getting drive names: %s\n", buf);
		}
	}
#endif
	qsort(fb->files.a, fb->files.size, sizeof(file), fb->c_func);
	fb->list_setscroll = TRUE;
	fb->selection = 0;

	fb->begin = 0;
}
