#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H


// pulls in cvector
#include "file.h"

#ifndef FILE_TYPE_STR
#define FILE_TYPE_STR "Match Exts"
#endif

#define TRUE 1
#define FALSE 0

#ifndef FB_LOG
#define FB_LOG(A, ...) printf(A, __VA_ARGS__)
#endif

#define FILE_LIST_SZ 20
#define STRBUF_SZ 512
#define MAX_PATH_LEN STRBUF_SZ
#define PATH_SEPARATOR '/'

typedef int (*recents_func)(cvector_str* recents, void * userdata);
typedef int (*cmp_func)(const void* a, const void* b);
enum { NAME_UP, NAME_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };

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

	int sorted_state;
	cmp_func c_func;

} file_browser;

int init_file_browser(file_browser* browser, const char** exts, int num_exts, const char* start_dir, recents_func r_func, void* userdata);
void free_file_browser(file_browser* fb);
void switch_dir(file_browser* fb, const char* dir);
void handle_recents(file_browser* fb);

void search_filenames(file_browser* fb);
const char* get_homedir();
int fb_scandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts);
char* mydirname(const char* path, char* dirpath);
char* mybasename(const char* path, char* base);
void normalize_path(char* path);
int bytes2str(int bytes, char* buf, int len);


#endif
