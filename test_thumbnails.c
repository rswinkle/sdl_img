
#include "WjCryptLib_Md5.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef unsigned char u8;

#define STRBUF_SZ 1024

//int myscandir(const char* dirpath, const char** exts, int num_exts, int recurse);
void cleanup(int a, int b);
int mkdir_p(const char* path, mode_t mode);

int filter_imgs(const struct dirent* entry)
{
	static const char* exts[] =
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

	char* ext = strrchr(entry->d_name, '.');
	if (!ext)
		return 0;

	for (int i=0; i<sizeof(exts)/sizeof(exts[0]); ++i) {
		if (!strcasecmp(ext, exts[i]))
			return 1;
	}

	return 0;
}

void hash2str(char* str, MD5_HASH* h)
{
	char buf[3];

	for (int i=0; i<MD5_HASH_SIZE; ++i) {
		sprintf(buf, "%02x", h->bytes[i]);
		strcat(str, buf);
	}
}


int main(int argc, char** argv)
{

	if (argc != 2) {
		printf("usage: %s dir_of_images\n", argv[0]);
		return 0;
	}

	struct dirent** img_list = NULL;

	int n_imgs = scandir(argv[1], &img_list, filter_imgs, alphasort);

	char fullpath[STRBUF_SZ] = { 0 };
	char cachepath[STRBUF_SZ] = { 0 };
	char thumbpath[STRBUF_SZ] = { 0 };
	char hash_str[MD5_HASH_SIZE*2+1] = { 0 };
	int ret;

	int len = strlen(argv[1]);
	if (argv[1][len-1] == '/')
		argv[1][len-1] = 0;


	ret = snprintf(cachepath, STRBUF_SZ, "%s/.sdl_img_thumbnails", argv[1]);
	if (mkdir_p(cachepath, S_IRWXU) && errno != EEXIST) {
		perror("Failed to make cache directory");
		cleanup(1, 0);
	}

	int w, h, channels;
	int out_w, out_h;

	u8* pix;
	u8* outpix;
	MD5_HASH hash;



	for (int i=0; i<n_imgs; ++i) {
		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", argv[1], img_list[i]->d_name);
		if (ret >= STRBUF_SZ) {
			printf("path too long\n");
			cleanup(0, 1);
		}

		pix = stbi_load(fullpath, &w, &h, &channels, 4);
		if (!pix)
			continue;

		Md5Calculate(fullpath, ret, &hash);
		hash_str[0] = 0;
		hash2str(hash_str, &hash);

		if (w > h) {
			out_w = 128;
			out_h = 128.0 * h/w;
		} else {
			out_h = 128;
			out_w = 128.0 * w/h;
		}

		if (!(outpix = malloc(out_h*out_w*4))) {
			return 0;
		}

		if (!stbir_resize_uint8(pix, w, h, 0, outpix, out_w, out_h, 0, 4)) {
			free(pix);
			free(outpix);
			continue;
		}

		// could just do the %02x%02x etc. here but that'd be a long format string and 16 extra parameters
		ret = snprintf(thumbpath, STRBUF_SZ, "%s/%s.png", cachepath, hash_str);

		stbi_write_png(thumbpath, out_w, out_h, 4, outpix, out_w*4);

		free(pix);
		free(outpix);
		printf("generated thumb %d for %s\n", i, fullpath);//img_list[i]->d_name);
	}
	printf("found %d images\n", n_imgs);


}

int mkdir_p(const char* path, mode_t mode)
{
	char path_buf[STRBUF_SZ] = { 0 };

	strncpy(path_buf, path, STRBUF_SZ);
	if (path_buf[STRBUF_SZ-1]) {
		errno = ENAMETOOLONG;
		return -1;
	}

	char* p = path_buf;

	// minor optimization, and lets us do p[-1] below
	if (*p == '/')
		p++;

	for (; *p; ++p) {
		// no need to handle two / in a row
		if (*p == '/' && p[-1] != '/') {
			*p = 0;
			if (mkdir(path_buf, mode) && errno != EEXIST) {
				return -1;
			}
			*p = '/';
		}
	}

	if (mkdir(path_buf, mode) && errno != EEXIST) {
		return -1;
	}

	return 0;
}

void cleanup(int a, int b)
{
	exit(0);
}


/*
int myscandir(const char* dirpath, const char** exts, int num_exts, int recurse)
{
	char fullpath[STRBUF_SZ] = { 0 };
	struct stat file_stat;
	struct dirent* entry;
	int ret, i=0;;
	DIR* dir;

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1, 1);
	}

	char* ext = NULL;

	//printf("Scanning %s for images...\n", dirpath);
	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			printf("path too long\n");
			cleanup(0, 1);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}

		// S_ISLNK() doesn't seem to work but d_type works, though the man page
		// says it's not supported on all filesystems... or windows TODO?
		// and having this breaks my code folding ... aggh I hate windows
#ifndef _WIN32
		if (recurse && S_ISDIR(file_stat.st_mode) && entry->d_type != DT_LNK) {
#else
		if (recurse && S_ISDIR(file_stat.st_mode)) {
#endif
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
		cvec_push_str(&g->files, fullpath);
	}

	closedir(dir);
	g->loading = 0;
	return 1;
}
*/
