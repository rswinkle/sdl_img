//gcc test_scandir.c -o scandir -I../src 

#include "myinttypes.h"

#define CVECTOR_IMPLEMENTATION
#define CVEC_ONLY_INT
#define CVEC_ONLY_STR
#define CVEC_SIZE_T i64
#define PRIcv_sz PRIiMAX
#include "cvector.h"

#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define PATH_SEPARATOR '/'
#define STRBUF_SZ 1024
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

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

int bytes2str(int bytes, char* buf, int len);
int myscandir(const char* dirpath, const char** exts, int num_exts, int recurse);

typedef struct global_state
{
	i64 path_mem;
	cvector_file files;

} global_state;

static global_state state = { 0 };
global_state* g = &state;


int main(int argc, char** argv)
{
	const char* exts[] =
	{
		".jpg",
		".jpeg",
		".gif",
		".png",
		".bmp",

		".ppm",
		".pgm",

		".tga",
		".hdr",
		".pic",
		".psd"
	};
	int num_exts = sizeof(exts)/sizeof(*exts);

	if (argc != 2) {
		printf("usage: %s directory_to_scan\n", argv[0]);
		return 0;
	}
	
	int n = myscandir(argv[1], exts, num_exts, 1);

	printf("paths = %lu\n", g->path_mem);

	getchar();


	return 0;
}



// renamed to not conflict with <dirent.h>'s scandir
// which I could probably use to accomplish  most of this...
int myscandir(const char* dirpath, const char** exts, int num_exts, int recurse)
{
	char fullpath[STRBUF_SZ] = { 0 };
	struct stat file_stat;
	struct dirent* entry;
	int ret, i=0;;
	DIR* dir;
	struct tm* tmp_tm;
	int start_size = g->files.size;

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		exit(1);
	}

	char* sep;
	char* ext = NULL;
	file f;

	//SDL_Log("Scanning %s for images...\n", dirpath);
	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			//SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "path too long\n");
			exit(1);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}

		// S_ISLNK() doesn't seem to work but d_type works, though the man page
		// says it's not supported on all filesystems... or windows TODO?
		// aggh I hate windows
#ifndef _WIN32
		if (recurse && S_ISDIR(file_stat.st_mode) && entry->d_type != DT_LNK)
#else
		if (recurse && S_ISDIR(file_stat.st_mode))
#endif
		{
			myscandir(fullpath, exts, num_exts, recurse);
			continue;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			continue;
		}

		// only add supported extensions
		ext = strrchr(entry->d_name, '.');
		if (!ext)
			continue;

		for (i=0; i<num_exts; ++i) {
			if (!strcasecmp(ext, exts[i]))
				break;
		}
		if (i == num_exts)
			continue;

		// have to use fullpath not d_name in case we're in a recursive call
#ifndef _WIN32
		f.path = realpath(fullpath, NULL);
#else
		f.path = CVEC_STRDUP(fullpath);
#endif
		g->path_mem += strlen(f.path)+1;
		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;

		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(&g->files, &f);
	}

	//SDL_Log("Found %"PRIcv_sz" images in %s\n", g->files.size-start_size, dirpath);

	closedir(dir);
	return 1;
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
