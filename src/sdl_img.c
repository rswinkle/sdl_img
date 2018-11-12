// The MIT License (MIT)
// 
// Copyright (c) 2017-2018 Robert Winkler
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

#define CVECTOR_IMPLEMENTATION
#include "cvector.h"

//#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

//POSIX works with MinGW64
#include <sys/stat.h>
#include <dirent.h>
#include <curl/curl.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

enum { QUIT, REDRAW, NOCHANGE };
enum { NOTHING = 0, SCANNING = 1, MODE2 = 2, MODE4 = 4, MODE8 = 8, LEFT, RIGHT };

typedef uint8_t u8;
typedef int16_t i16;
typedef int32_t i32;

#ifdef _WIN32
#define mkdir(A, B) mkdir(A)
#endif

#define PATH_SEPARATOR '/'
#define ZOOM_RATE 0.05
#define PAN_RATE 0.05
#define MIN_GIF_DELAY 10
#define HIDE_CURSOR_TIMER 5000
#define MAX_STARTUP_TIME 2500
#define SLEEP_TIME 50
#define STRBUF_SZ 1024

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SET_MODE1_SCR_RECT()                                    \
	do {                                                        \
	g->img[0].scr_rect.x = 0;                                   \
	g->img[0].scr_rect.y = 0;                                   \
	g->img[0].scr_rect.w = g->scr_w;                            \
	g->img[0].scr_rect.h = g->scr_h;                            \
	set_rect_bestfit(&g->img[0], g->fullscreen | g->fill_mode); \
	} while (0)

#define SET_MODE2_SCR_RECTS()                                   \
	do {                                                        \
	g->img[0].scr_rect.x = 0;                                   \
	g->img[0].scr_rect.y = 0;                                   \
	g->img[0].scr_rect.w = g->scr_w/2;                          \
	g->img[0].scr_rect.h = g->scr_h;                            \
	g->img[1].scr_rect.x = g->scr_w/2;                          \
	g->img[1].scr_rect.y = 0;                                   \
	g->img[1].scr_rect.w = g->scr_w/2;                          \
	g->img[1].scr_rect.h = g->scr_h;                            \
	set_rect_bestfit(&g->img[0], g->fullscreen | g->fill_mode); \
	set_rect_bestfit(&g->img[1], g->fullscreen | g->fill_mode); \
	} while (0)

#define SET_MODE4_SCR_RECTS()                                        \
	do {                                                             \
	for (int i=0; i<4; ++i) {                                        \
		g->img[i].scr_rect.x = (i%2)*g->scr_w/2;                     \
		g->img[i].scr_rect.y = (i/2)*g->scr_h/2;                     \
		g->img[i].scr_rect.w = g->scr_w/2;                           \
		g->img[i].scr_rect.h = g->scr_h/2;                           \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->fill_mode);  \
	}                                                                \
	} while (0)

#define SET_MODE8_SCR_RECTS()                                        \
	do {                                                             \
	for (int i=0; i<8; ++i) {                                        \
		g->img[i].scr_rect.x = (i%4)*g->scr_w/4;                     \
		g->img[i].scr_rect.y = (i/4)*g->scr_h/2;                     \
		g->img[i].scr_rect.w = g->scr_w/4;                           \
		g->img[i].scr_rect.h = g->scr_h/2;                           \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->fill_mode);  \
	}                                                                \
	} while (0)

typedef struct img_state
{
	u8* pixels;
	int w;
	int h;

	int index;
	int is_dup;

	int frame_i;
	int delay; // for now just use the same delay for every frame
	int frames;
	int frame_capacity;
	int frame_timer;
	int looped;
	int rotated;
	
	SDL_Texture** tex;

	SDL_Rect scr_rect;  // rect describing available space
	SDL_Rect disp_rect; // rect image is actually rendered to
} img_state;

typedef struct global_state
{
	SDL_Window* win;
	SDL_Renderer* ren;

	int scr_w;
	int scr_h;

	img_state* img_focus;
	int n_imgs;
	img_state* img;

	img_state img1[8];
	img_state img2[8];

	int status;

	char* dirpath;
	char* cachedir;

	cvector_str files;

	int fullscreen;
	int fill_mode;
	int mouse_timer;
	int mouse_state;

	int slideshow;
	int slide_timer;

	// threading
	int loading;
	int done_loading;
	SDL_cond* cnd;
	SDL_mutex* mtx;

} global_state;

// Use a pointer in case I ever move this to another TU, though it's unlikely
static global_state state = { 0 };
global_state* g = &state;

// TODO hmmm
//int loading;


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

// TODO use http://stereopsis.com/strcmp4humans.html
// or slightly modified in stb-imv?
int cmp_string_lt(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}

size_t write_data(void* buf, size_t size, size_t num, void* userp)
{
	return fwrite(buf, 1, size*num, (FILE*)userp);
}

//need to think about best way to organize following 4 functions' functionality
void adjust_rect(img_state* img, int w, int h)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	
	int l = img->scr_rect.x;
	int r = l + img->scr_rect.w;
	int t = img->scr_rect.y;
	int b = t + img->scr_rect.h;
	
	x = MAX(x, l);
	x = MIN(x, r);
	y = MAX(y, t);
	y = MIN(y, b);

	float px = (x-img->disp_rect.x)/(float)img->disp_rect.w;
	float py = (y-img->disp_rect.y)/(float)img->disp_rect.h;

	// There might be a slightly better way to organize/calculate this but this works
	int final_x, final_y;
	if ((g->n_imgs == 1 || g->img_focus == img) && w > img->scr_rect.w) {
		final_x = x-px*w;
		
		if (final_x + w < r)
			final_x += r-(final_x+w);
		if (final_x > l)
			final_x -= final_x-l;
	} else {
		final_x = img->scr_rect.x + (img->scr_rect.w-w)/2;
	}

	if ((g->n_imgs == 1 || g->img_focus == img) && h > img->scr_rect.h) {
		final_y = y-py*h;

		if (final_y + h < b)
			final_y += b-(final_y+h);
		if (final_y > t)
			final_y -= final_y-t;
	} else {
		final_y = img->scr_rect.y + (img->scr_rect.h-h)/2;
	}
	img->disp_rect.x = final_x;
	img->disp_rect.y = final_y;
	img->disp_rect.w = w;
	img->disp_rect.h = h;
}

void set_rect_bestfit(img_state* img, int fill_screen)
{
	float aspect = img->w/(float)img->h;
	int h, w;
	
	// TODO macro evaluates division twice
	int tmp = MIN(img->scr_rect.h, img->scr_rect.w/aspect);
	if (fill_screen)
		h = tmp;
	else
		h = MIN(tmp, img->h); //show actual size if smaller than viewport

	w = h * aspect;

	adjust_rect(img, w, h);
}

void set_rect_zoom(img_state* img, int zoom)
{
	float aspect = img->w/(float)img->h;
	int h, w;
	
	h = img->disp_rect.h * (1.0 + zoom*ZOOM_RATE);
	w = h * aspect;

	adjust_rect(img, w, h);
}

// don't waste space even when drag/panning
void fix_rect(img_state* img)
{
	int w = img->disp_rect.w;
	int h = img->disp_rect.h;
	int scr_right = img->scr_rect.x + img->scr_rect.w;
	int scr_bottom = img->scr_rect.y + img->scr_rect.h;
	if (w > img->scr_rect.w) {
		if (img->disp_rect.x > img->scr_rect.x) {
			img->disp_rect.x = img->scr_rect.x;
		} else if (img->disp_rect.x + w < scr_right) {
			img->disp_rect.x += scr_right - (img->disp_rect.x+w);
		}
	}
	if (h > img->scr_rect.h) {
		if (img->disp_rect.y > img->scr_rect.y) {
			img->disp_rect.y = img->scr_rect.y;
		} else if (img->disp_rect.y + h < scr_bottom) {
			img->disp_rect.y += scr_bottom - (img->disp_rect.y+h);
		}
	}
}

int create_textures(img_state* img)
{
	int size = img->w * img->h * 4;

	for (int i=0; i<img->frames; ++i) {
		img->tex[i] = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, img->w, img->h);
		if (!img->tex[i]) {
			printf("Error: %s\n", SDL_GetError());
			return 0;
		}
		if (SDL_UpdateTexture(img->tex[i], NULL, img->pixels+(size+2)*i, img->w*4)) {
			printf("Error updating texture: %s\n", SDL_GetError());
			return 0;
		}
	}

	// can't do this anymore, need it for rotations
	//stbi_image_free(img->pixels);
	//img->pixels = NULL;

	return 1;
}

void clear_img(img_state* img)
{
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}

	if (img->rotated && img->frames == 1) {
		char msgbox_prompt[STRBUF_SZ];
		char full_img_path[STRBUF_SZ];
		int buttonid;

		SDL_MessageBoxButtonData buttons[] = {
			//{ /* .flags, .buttonid, .text */        0, 0, "no" },
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "yes" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "no" },
		};

		SDL_MessageBoxData messageboxdata = {
			SDL_MESSAGEBOX_WARNING, /* .flags */
			NULL, /* .window */
			"Warning", /* .title */
			NULL, /* .message to be set later */
			SDL_arraysize(buttons), /* .numbuttons */
			buttons, /* .buttons */
			NULL /* .colorScheme, NULL = system default */
		};

		if (g->dirpath)
			sprintf(full_img_path, "%s/%s", g->dirpath, g->files.a[img->index]);
		else
			strncpy(full_img_path, g->files.a[img->index], STRBUF_SZ-1);


		snprintf(msgbox_prompt, STRBUF_SZ, "Do you want to save changes to '%s'?", full_img_path);
		messageboxdata.message = msgbox_prompt;
		SDL_ShowMessageBox(&messageboxdata, &buttonid);

		if (buttonid == 1) {
			char* ext = strrchr(full_img_path, '.');
			if (!ext) {
				ext = ".jpg";
				strcat(full_img_path, ext);
				// TODO should I delete the original? If so, I need
				// to update g->files.a[img->index] with the new name
			}
			if (!strcasecmp(ext, ".png"))
				stbi_write_png(full_img_path, img->w, img->h, 4, img->pixels, img->w*4);
			else if (!strcasecmp(ext, ".bmp"))
				stbi_write_bmp(full_img_path, img->w, img->h, 4, img->pixels);
			else if (!strcasecmp(ext, ".tga"))
				stbi_write_tga(full_img_path, img->w, img->h, 4, img->pixels);
			else
				stbi_write_jpg(full_img_path, img->w, img->h, 4, img->pixels, 100);
		}
	}
	//could clear everything else but these are the important
	//ones logic is based on
	//stbi_image_free(img->pixels);
	free(img->pixels);
	img->pixels = NULL;
	img->frames = 0;
	img->rotated = 0;
}

void cleanup(int ret, int called_setup)
{
	if (called_setup) {
		for (int i=0; i<g->n_imgs; ++i) {
			clear_img(&g->img[i]);
			free(g->img[i].tex);
		}

		SDL_DestroyRenderer(g->ren);
		SDL_DestroyWindow(g->win);
		SDL_Quit();
	}

	cvec_free_str(&g->files);
	curl_global_cleanup();
	exit(ret);
}

// debug
void print_img_state(img_state* img)
{
	printf("{\nimg = %p\n", img);
	printf("pixels = %p\n", img->pixels);
	printf("WxH = %dx%d\n", img->w, img->h);
	printf("index = %d\n", img->index);
	printf("frame_i = %d\ndelay = %d\nframes = %d\nframe_cap = %d\n", img->frame_i, img->delay, img->frames, img->frame_capacity);
	printf("frame_timer = %d\nlooped = %d\n", img->frame_timer, img->looped);

	printf("tex = %p\n", img->tex);
	for (int i=0; i<img->frames; ++i) {
		printf("tex[%d] = %p\n", i, img->tex[i]);
	}

	printf("scr_rect = %d %d %d %d\n", img->scr_rect.x, img->scr_rect.y, img->scr_rect.w, img->scr_rect.h);
	printf("disp_rect = %d %d %d %d\n}\n", img->disp_rect.x, img->disp_rect.y, img->disp_rect.w, img->disp_rect.h);
}

int curl_image(int img_idx)
{
	CURL* curl = curl_easy_init();
	CURLcode res;
	char filename[STRBUF_SZ];
	char curlerror[CURL_ERROR_SIZE];
	char* s = g->files.a[img_idx];
	FILE* imgfile;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
	#ifdef _WIN32
	curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");
	curl_easy_setopt(curl, CURLOPT_CAPATH, exepath);
	#endif

	char* slash = strrchr(s, '/');
	if (!slash) {
		puts("invalid url");
		goto exit_cleanup;
	}
	int len = snprintf(filename, STRBUF_SZ, "%s/%s", g->cachedir, slash+1);
	if (len >= STRBUF_SZ) {
		puts("url too long");
		goto exit_cleanup;
	}

	printf("Getting %s\n%s\n", s, filename);
	if (!(imgfile = fopen(filename, "wb"))) {
		perror("fopen");
		goto exit_cleanup;
	}

	curl_easy_setopt(curl, CURLOPT_URL, s);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, imgfile);

	if ((res = curl_easy_perform(curl)) != CURLE_OK) {
		printf("curl error: %s\n", curlerror);
		goto exit_cleanup;
	}
	fclose(imgfile);
	cvec_replace_str(&g->files, img_idx, filename, NULL);

	curl_easy_cleanup(curl);
	return 1;

exit_cleanup:
	curl_easy_cleanup(curl);
	return 0;
}

int load_image(const char* img_name, img_state* img, int make_textures)
{
	int frames, n;

	// img->frames should always be 0 and there should be no allocated textures
	// in tex because clear_img(img) should always have been called before

	char fullpath[STRBUF_SZ];
	int ret;

	if (g->dirpath)
		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", g->dirpath, img_name);
	else
		ret = snprintf(fullpath, STRBUF_SZ, "%s", img_name); // TODO strncpy?
	if (ret >= STRBUF_SZ) {
		// TODO add messagebox here?
		puts("path too long");
		return 0;
	}

	printf("loading %s\n", fullpath);

	img->pixels = stbi_xload(fullpath, &img->w, &img->h, &n, 4, &frames);
	if (!img->pixels) {
		printf("failed to load %s: %s\n", fullpath, stbi_failure_reason());
		return 0;
	}

	if (frames > img->frame_capacity) {
		// img->tex is either NULL or previously alloced
		if (!(img->tex = realloc(img->tex, frames*sizeof(SDL_Texture*)))) {
			perror("Couldn't allocate tex array");
			return 0;
		}
		img->frame_capacity = frames;
	}

	int size = img->w * img->h * 4;
	//gif delay is in 100ths, ticks are 1000ths, but newer stb_image converts for us
	//assume that the delay is the same for all frames (never seen anything else anyway)
	//and if delay is 0, default to 10 fps
	img->looped = 1;
	if (frames > 1) {
		img->looped = 0;
		img->delay = *(short*)(&img->pixels[size]); // * 10;
		if (!img->delay)
			img->delay = 100;
		img->delay = MAX(MIN_GIF_DELAY, img->delay);
		printf("%d frames %d delay\n", frames, img->delay);
	}

	img->frames = frames;
	img->frame_i = 0;

	if (make_textures) {
		if (!create_textures(img))
			return 0;
	}

	return 1;
}

int scandir(void* data)
{
	char fullpath[STRBUF_SZ] = { 0 };
	struct stat file_stat;
	struct dirent* entry;
	int ret, i=0;;
	DIR* dir;

	char* initial_image = data;

	dir = opendir(g->dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1, 1);
	}

	printf("Scanning %s for images...\n", g->dirpath);
	while ((entry = readdir(dir))) {
		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", g->dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			printf("path too long\n");
			cleanup(0, 1);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}

		// if it's a regular file (checking for valid image makes startup too slow)
		if (S_ISREG(file_stat.st_mode)) { // && stbi_info(fullpath, NULL, NULL, NULL)) {
			cvec_push_str(&g->files, entry->d_name);
		}
		i++;
		if (i % 400 == 0)
			printf("scanned %d\n", i);
	}

	printf("sorting images\n");
	qsort(g->files.a, g->files.size, sizeof(char*), cmp_string_lt);

	printf("finding current image to update index\n");
	char** res;
	res = bsearch(&initial_image, g->files.a, g->files.size, sizeof(char*), cmp_string_lt);
	if (!res) {
		cleanup(0, 1);
	}
	g->img[0].index = res - g->files.a;

	printf("Loaded all %"PRIuMAX" filenames\n", g->files.size);
	closedir(dir);
	g->loading = 0;
	return 1;
}

int wrap(int z)
{
   int n = g->files.size;
   if (z < 0) return z + n;
   while (z >= n) z = z - n;
   return z;
}

int load_new_images(void* data)
{
	int tmp;
	char title_buf[STRBUF_SZ];
	int ret;
	int load_what;
	int last;
	
	while (1) {
		SDL_LockMutex(g->mtx);
		while (g->loading < 2) {
			SDL_CondWait(g->cnd, g->mtx);
		}
		load_what = g->loading;
		SDL_UnlockMutex(g->mtx);

		//printf("loading %p = %d\n", &g->loading, g->loading);
		if (load_what >= LEFT) {
			img_state* img;
			if (g->img == g->img1)
				img = g->img2;
			else
				img = g->img1;

			for (int i=0; i<g->n_imgs; ++i)
				img[i].scr_rect = g->img[i].scr_rect;
			
			if (!g->img_focus) {
				if (load_what == RIGHT) {
					last = g->img[g->n_imgs-1].index;
					for (int i=0; i<g->n_imgs; ++i) {
						do {
							last = wrap(last + 1);
							ret = load_image(g->files.a[last], &img[i], SDL_FALSE);
							if (!ret)
								if (curl_image(last))
									ret = load_image(g->files.a[last], &img[i], SDL_FALSE);
						} while (!ret);
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						img[i].index = last;
					}
				} else {
					last = g->img[0].index;
					for (int i=g->n_imgs-1; i>=0; --i) {
						do {
							last = wrap(last - 1);
							ret = load_image(g->files.a[last], &img[i], SDL_FALSE);
							if (!ret)
							if (!ret)
								if (curl_image(last))
									ret = load_image(g->files.a[last], &img[i], SDL_FALSE);
						} while (!ret);
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						img[i].index = last;
					}
				}

				// just set title to upper left image when !img_focus
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[img[0].index], title_buf));
			} else {
				tmp = (load_what == RIGHT) ? 1 : -1;
				last = g->img_focus->index;
				do {
					last = wrap(last + tmp);
					ret = load_image(g->files.a[last], &img[0], SDL_FALSE);
					if (!ret)
						if (curl_image(last))
							ret = load_image(g->files.a[last], &img[0], SDL_FALSE);
				} while (!ret);
				img[0].index = last;
				img[0].scr_rect = g->img_focus->scr_rect;
				set_rect_bestfit(&img[0], g->fullscreen | g->slideshow | g->fill_mode);
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[img[0].index], title_buf));
			}
		} else {
			last = g->img[g->n_imgs-1].index;
			for (int i=g->n_imgs; i<load_what; ++i) {
				do {
					last = wrap(last + 1);
					ret = load_image(g->files.a[last], &g->img[i], SDL_FALSE);
					if (!ret)
						if (curl_image(last))
							ret = load_image(g->files.a[last], &g->img[i], SDL_FALSE);
				} while (!ret);
				g->img[i].index = last;
			}

		}

		SDL_LockMutex(g->mtx);
		g->done_loading = load_what;
		g->loading = 0;
		SDL_UnlockMutex(g->mtx);
	}
}

void setup(const char* img_name)
{
	g->win = NULL;
	g->ren = NULL;
	char error_str[STRBUF_SZ] = { 0 };
	char title_buf[STRBUF_SZ] = { 0 };

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, STRBUF_SZ, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		exit(1);
	}

	// could use stbi_info to get the img dimensions here without fully decoding...
	g->scr_w = 640;
	g->scr_h = 480;
	mybasename(img_name, title_buf);
	
	g->win = SDL_CreateWindow(title_buf, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g->scr_w, g->scr_h, SDL_WINDOW_RESIZABLE);
	if (!g->win) {
		snprintf(error_str, STRBUF_SZ, "Couldn't create window: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		exit(1);
	}

	g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!g->ren) {
		snprintf(error_str, STRBUF_SZ, "%s, falling back to software renderer.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning: No HW Acceleration", error_str, g->win);
		puts(error_str);
		g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_SOFTWARE);
	}

	if (!g->ren) {
		snprintf(error_str, STRBUF_SZ, "Software rendering failed: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		cleanup(1, 1);
	}
	// get black screen while loading (big gif could take a few seconds)
	SDL_SetRenderDrawColor(g->ren, 0, 0, 0, 255);
	SDL_RenderClear(g->ren);
	SDL_RenderPresent(g->ren);
	
	g->n_imgs = 1;
	g->img = g->img1;

	if (!(g->img[0].tex = malloc(100*sizeof(SDL_Texture*)))) {
		perror("Couldn't allocate tex array");
		cleanup(0, 1);
	}
	g->img[0].frame_capacity = 100;

	g->fullscreen = 0;
	g->fill_mode = 0;

	// TODO best way to structure this?
	int ret = load_image(img_name, &g->img[0], SDL_TRUE);
	if (!ret) {
		if (g->files.size && curl_image(0)) {
			ret = load_image(g->files.a[0], &g->img[0], SDL_TRUE);
			img_name = g->files.a[0];
		}
	}
	if (!ret) {
		cleanup(0, 1);
	}

	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
		printf("Error getting display mode: %s\n", SDL_GetError());
	} else {
		// don't know a way to get window border/title bar dimensions cross platform,
		// but apparently it's not necessary, if the the dimensions are too big it'll get
		// set to the max windowed size (at least on Linux)
		//
		// UPDATE: Windows is *not* smart enough to limit window size but will happily
		// create a "window" where the edges and titlebar are waaaay off the screen
		// so, just subtracting arbitrary amount from screen dimensions for now
		printf("screen WxH = %d x %d\n", dm.w, dm.h);
		g->scr_w = MAX(g->img[0].w, 640);
		g->scr_h = MAX(g->img[0].h, 480);
		g->scr_w = MIN(g->scr_w, dm.w-20);
		g->scr_h = MIN(g->scr_h, dm.h-80);
		SDL_SetWindowSize(g->win, g->scr_w, g->scr_h);
		SDL_SetWindowPosition(g->win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}


	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		;  // apparently we have to "handle" the window change event for it to fully process
	}

	SET_MODE1_SCR_RECT();
	SDL_RenderClear(g->ren);
	SDL_RenderCopy(g->ren, g->img[0].tex[g->img[0].frame_i], NULL, &g->img[0].disp_rect);
	SDL_RenderPresent(g->ren);

	if (!(g->cnd = SDL_CreateCond())) {
		printf("Error: %s", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->mtx = SDL_CreateMutex())) {
		printf("Error: %s", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* loading_thrd;
	if (!(loading_thrd = SDL_CreateThread(load_new_images, "loading_thrd", NULL))) {
		puts("couldn't create thread");
	}
	SDL_DetachThread(loading_thrd);

	printf("Done with setup\nStarting with %s\n", img_name);

}

void toggle_fullscreen()
{
	g->status = REDRAW;
	if (g->fullscreen) {
		SDL_SetWindowFullscreen(g->win, 0);
		g->fullscreen = 0;
	} else {
		SDL_SetWindowFullscreen(g->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
		g->fullscreen = 1;
	}
}

void replace_img(img_state* i1, img_state* i2)
{
	SDL_Texture** tmptex = i1->tex;
	int tmp = i1->frame_capacity;
	memcpy(i1, i2, sizeof(img_state));
	i2->tex = tmptex;
	i2->frame_capacity = tmp;
	i2->frames = 0;
}

void rotate_img(img_state* img, int left)
{
	int w = img->w, h = img->h;
	int sz = w*h;
	int frames = img->frames;
	u8* rotated = NULL;
	if (frames > 1) {
		if (!(rotated = malloc(frames*(sz*4+2)))) {
			perror("Couldn't allocate rotated");
			cleanup(0, 1);
		}
		*(i16*)(&rotated[sz*4]) = img->delay;
	} else {
		if (!(rotated = malloc(sz*4))) {
			perror("Couldn't allocate rotated");
			cleanup(0, 1);
		}
	}
	u8* pix = img->pixels;
	i32 *p, *rot;
	for (int k=0; k<frames; ++k) {
		rot = (i32*)&rotated[k*(sz*4+2)];
		p = (i32*)&pix[k*(sz*4+2)];
		for (int i=0; i<h; ++i) {
			for (int j=0; j<w; ++j) {
				if (left)
					rot[(w-j-1)*h+i] = p[i*w+j];
				else
					rot[(j+1)*h-i-1] = p[i*w+j];
			}
		}
	}

	// don't call clear_img here because could do multiple
	// rotations and don't want to prompt to save for each one
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}
	free(pix);

	img->w = h;
	img->h = w;
	img->frames = frames;
	img->pixels = rotated;
	img->rotated = 1;
}

int handle_events()
{
	SDL_Event e;
	int sc;
	int zoomed;
	char title_buf[STRBUF_SZ];
	img_state* img;
	int loading = g->loading;

	g->status = NOCHANGE;

	SDL_Keymod mod_state = SDL_GetModState();

	// use space to move to next image(s) even if zoomed in, ie during slideshow
	SDL_Event space;
	space.type = SDL_KEYDOWN;
	space.key.keysym.scancode = SDL_SCANCODE_SPACE;

	int ticks = SDL_GetTicks();

	SDL_MessageBoxButtonData buttons[] = {
		//{ /* .flags, .buttonid, .text */        0, 0, "no" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "cancel" },
	};

	SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_WARNING, /* .flags */
		NULL, /* .window */
		"Warning", /* .title */
		NULL, /* .message to be set later */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		NULL /* .colorScheme, NULL = system default */
	};

	char msgbox_prompt[STRBUF_SZ];
	char full_img_path[STRBUF_SZ];

	int buttonid;

	SDL_LockMutex(g->mtx);
	if (g->done_loading) {
		if (g->done_loading >= LEFT) {
			img = (g->img == g->img1) ? g->img2 : g->img1;
			if (g->img_focus) {
				clear_img(g->img_focus);
				replace_img(g->img_focus, &img[0]);
				create_textures(g->img_focus);
			} else {
				for (int i=0; i<g->n_imgs; ++i) {
					create_textures(&img[i]);
					clear_img(&g->img[i]);
				}
				g->img = img;
			}
		} else {
			for (int i=g->n_imgs; i<g->done_loading; ++i)
				create_textures(&g->img[i]);

			if (g->done_loading == MODE2) {
				SET_MODE2_SCR_RECTS();
				g->n_imgs = 2;
				g->img_focus = NULL;
			} else if (g->done_loading == MODE4) {
				SET_MODE4_SCR_RECTS();
				g->n_imgs = 4;
				g->img_focus = NULL;
			} else {
				SET_MODE8_SCR_RECTS();
				g->n_imgs = 8;
				g->img_focus = NULL;
			}
		}
		g->done_loading = 0;
		g->status = REDRAW;
		if (g->slideshow)
			g->slide_timer =  SDL_GetTicks();
	}
	SDL_UnlockMutex(g->mtx);

	if (g->slideshow && !g->loading && ticks - g->slide_timer > g->slideshow) {
		int i;
		// make sure all current gifs have gotten to the end
		// at least once
		for (i=0; i<g->n_imgs; ++i) {
			if (!g->img[i].looped)
				break;
		}
		if (i == g->n_imgs) {
			SDL_PushEvent(&space);
		}
	}


	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			return 1;
		case SDL_KEYUP:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				if (!g->fullscreen && !g->slideshow) {
					return 1;
				} else {
					if (g->slideshow) {
						puts("Ending slideshow");
						g->slideshow = 0;
					} else if (g->fullscreen) {
						g->status = REDRAW;
						SDL_SetWindowFullscreen(g->win, 0);
						g->fullscreen = 0;
					}
				}
				break;

			case SDL_SCANCODE_DELETE:
				// only delete in single image mode to avoid confusion and complication
				if (!g->loading && g->n_imgs == 1) {
					if (g->dirpath)
						sprintf(full_img_path, "%s/%s", g->dirpath, g->files.a[g->img[0].index]);
					else
						strncpy(full_img_path, g->files.a[g->img[0].index], STRBUF_SZ-1);
					snprintf(msgbox_prompt, STRBUF_SZ, "Are you sure you want to delete '%s'?", full_img_path);
					messageboxdata.message = msgbox_prompt;
					SDL_ShowMessageBox(&messageboxdata, &buttonid);
					if (buttonid == 1) {
						if (remove(full_img_path)) {
							perror("Failed to delete image");
						} else {
							printf("Deleted %s\n", full_img_path);
							cvec_erase_str(&g->files, g->img[0].index, g->img[0].index);
							g->img[0].index--; // since everything shifted left, we need to pre-decrement to not skip an image
							SDL_PushEvent(&space);
						}
					}
				}
				break;

			// Rotate
			case SDL_SCANCODE_L:
			case SDL_SCANCODE_R:
				if (!g->loading) {
					img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
					if (img) {
						rotate_img(img, sc == SDL_SCANCODE_L);
						create_textures(img);
						if (g->n_imgs == 1)
							SET_MODE1_SCR_RECT();
						else if (g->n_imgs == 2)
							SET_MODE2_SCR_RECTS();
						else if (g->n_imgs == 4)
							SET_MODE4_SCR_RECTS();
						else
							SET_MODE8_SCR_RECTS();
						g->status = REDRAW;
					}
				}
				break;

			// CAPSLOCK comes right before F1 and F1-F12 are contiguous
			case SDL_SCANCODE_F1:
			case SDL_SCANCODE_F2:
			case SDL_SCANCODE_F3:
			case SDL_SCANCODE_F4:
			case SDL_SCANCODE_F5:
			case SDL_SCANCODE_F6:
			case SDL_SCANCODE_F7:
			case SDL_SCANCODE_F8:
			case SDL_SCANCODE_F9:
			case SDL_SCANCODE_F10:
				g->slideshow = (sc - SDL_SCANCODE_CAPSLOCK)*1000;
				g->slide_timer =  SDL_GetTicks();
				puts("Starting slideshow");
				break;

			case SDL_SCANCODE_F11:
				toggle_fullscreen();
				break;

			case SDL_SCANCODE_0:
				g->img_focus = NULL;
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index], title_buf));
				break;
			case SDL_SCANCODE_1:
				if (g->n_imgs != 1 && mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->status = REDRAW;
					g->slide_timer =  SDL_GetTicks();
					// TODO refactor into function?  don't free everything everytime
					// make load_image smarter
					if (g->img_focus && g->img_focus != &g->img[0]) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img_focus == &g->img[i]) {
								continue;
							}

							clear_img(&g->img[i]);
						}
						replace_img(&g->img[0], g->img_focus);

					} else {
						for (int i=1; i<g->n_imgs; ++i) {
							clear_img(&g->img[i]);
						}
					}
					SET_MODE1_SCR_RECT();
					g->n_imgs = 1;
					g->img_focus = NULL;

				} else if (g->n_imgs >= 2) {
					g->img_focus = &g->img[0];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_2:
				g->status = REDRAW;
				g->slide_timer =  SDL_GetTicks();
				if (!g->loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (g->n_imgs != 2 && g->files.size >= 2) {

						// TODO hmm
						if (g->n_imgs == 1) {
							SDL_LockMutex(g->mtx);
							g->loading = MODE2;
							loading = 1;
							SDL_CondSignal(g->cnd);
							SDL_UnlockMutex(g->mtx);
						} else {
							for (int i=g->n_imgs-1; i>1; --i)
								clear_img(&g->img[i]);

							SET_MODE2_SCR_RECTS();
							g->n_imgs = 2;
							g->img_focus = NULL;
						}
					}

				} else if (g->n_imgs >= 2) {
					g->img_focus = &g->img[1];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_3:
				g->status = REDRAW;
				if (g->n_imgs >= 3) {
					g->img_focus = &g->img[2];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_4:
				g->status = REDRAW;
				g->slide_timer =  SDL_GetTicks();
				if (!g->loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (g->n_imgs != 4 && g->files.size >= 4) {
						
						if (g->n_imgs < 4) {
							SDL_LockMutex(g->mtx);
							g->loading = MODE4;
							loading = 1;
							SDL_CondSignal(g->cnd);
							SDL_UnlockMutex(g->mtx);
						} else {
							for (int i=g->n_imgs-1; i>3; --i) {
								clear_img(&g->img[i]);
							}
							SET_MODE4_SCR_RECTS();
							g->n_imgs = 4;
							g->img_focus = NULL;
						}
					}

				} else if (g->n_imgs >= 4) {
					g->img_focus = &g->img[3];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_5:
				g->status = REDRAW;
				if (g->n_imgs >= 5) {
					g->img_focus = &g->img[4];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_6:
				g->status = REDRAW;
				if (g->n_imgs >= 6) {
					g->img_focus = &g->img[5];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_7:
				g->status = REDRAW;
				if (g->n_imgs >= 7) {
					g->img_focus = &g->img[6];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_8:
				g->status = REDRAW;
				g->slide_timer =  SDL_GetTicks();
				if (!g->loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					if (g->n_imgs != 8 && g->files.size >= 8) {
						if (g->n_imgs < 8) {
							SDL_LockMutex(g->mtx);
							g->loading = MODE8;
							loading = 1;
							SDL_CondSignal(g->cnd);
							SDL_UnlockMutex(g->mtx);
						} else {
							// This loop will never run unless I add a higher number somehow like 12 or 16
							for (int i=g->n_imgs-1; i>7; --i) {
								clear_img(&g->img[i]);
							}
							SET_MODE8_SCR_RECTS();
							g->n_imgs = 8;
							g->img_focus = NULL;
						}
					}

				} else if (g->n_imgs >= 8) {
					g->img_focus = &g->img[7];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index], title_buf));
				}
				break;

			case SDL_SCANCODE_A:
				g->status = REDRAW;
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i) {
						adjust_rect(&g->img[i], g->img[i].w, g->img[i].h);
					}
				} else {
					adjust_rect(g->img_focus, g->img_focus->w, g->img_focus->h);
				}
				break;

			case SDL_SCANCODE_F: {
				g->status = REDRAW;
				if (mod_state & (KMOD_LALT | KMOD_RALT)) {
					toggle_fullscreen();
				} else {
					g->fill_mode = !g->fill_mode;
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);
					} else {
						set_rect_bestfit(g->img_focus, g->fullscreen | g->slideshow | g->fill_mode);
					}
				}
			}
				break;
		}
			break;  //end SDL_KEYUP

		case SDL_KEYDOWN:
			// TODO use symcodes?
			sc = e.key.keysym.scancode;
			switch (sc) {

			case SDL_SCANCODE_SPACE:
				if (!g->loading) {
					SDL_LockMutex(g->mtx);
					g->loading = RIGHT;
					loading = 1;
					SDL_CondSignal(g->cnd);
					SDL_UnlockMutex(g->mtx);
				}
				break;

			// TODO merge RIGHT/DOWN and LEFT/UP?
			case SDL_SCANCODE_RIGHT:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x -= PAN_RATE * img->disp_rect.w;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x -= PAN_RATE * img->disp_rect.w;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
					}
				}
				if (!g->loading && !zoomed) {
					SDL_LockMutex(g->mtx);
					g->loading = RIGHT;
					loading = 1;
					SDL_CondSignal(g->cnd);
					SDL_UnlockMutex(g->mtx);
				}
				break;
			case SDL_SCANCODE_DOWN:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y -= PAN_RATE * img->disp_rect.h;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y -= PAN_RATE * img->disp_rect.h;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
					}
				}
				if (!g->loading && !zoomed) {
					SDL_LockMutex(g->mtx);
					g->loading = RIGHT;
					loading = 1;
					SDL_CondSignal(g->cnd);
					SDL_UnlockMutex(g->mtx);
				}
				break;

			case SDL_SCANCODE_LEFT:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x += PAN_RATE * img->disp_rect.w;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += PAN_RATE * img->disp_rect.w;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.h > img->scr_rect.h;
					}
				}
				if (!g->loading && !zoomed) {
					SDL_LockMutex(g->mtx);
					g->loading = LEFT;
					loading = 1;
					SDL_CondSignal(g->cnd);
					SDL_UnlockMutex(g->mtx);
				}
				break;
			case SDL_SCANCODE_UP:
				zoomed = 0;
				g->status = REDRAW;
				if (g->loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							img = &g->img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y += PAN_RATE * img->disp_rect.h;
								fix_rect(img);
								zoomed = 1;
							}
							zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
						}
					} else {
						img = g->img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += PAN_RATE * img->disp_rect.h;
							fix_rect(img);
							zoomed = 1;
						}
						zoomed = zoomed || img->disp_rect.w > img->scr_rect.w;
					}
				}
				if (!g->loading && !zoomed) {
					SDL_LockMutex(g->mtx);
					g->loading = LEFT;
					loading = 1;
					SDL_CondSignal(g->cnd);
					SDL_UnlockMutex(g->mtx);
				}
				break;

			case SDL_SCANCODE_MINUS:
				g->status = REDRAW;
				if (!(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_zoom(&g->img[i], -1);
					} else {
						set_rect_zoom(g->img_focus, -1);
					}
				} else {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].frames > 1)
								g->img[i].delay += 10;
						}
					} else {
						if (g->img_focus->frames > 1)
							g->img_focus->delay += 10;
					}
				}
				break;
			case SDL_SCANCODE_EQUALS:
				g->status = REDRAW;
				if (!(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_zoom(&g->img[i], 1);
					} else {
						set_rect_zoom(g->img_focus, 1);
					}
				} else {
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].frames > 1) {
								g->img[i].delay -= 10;
								g->img[i].delay = MAX(MIN_GIF_DELAY, g->img[i].delay);
							}
						}
					} else {
						if (g->img_focus->frames > 1) {
							g->img_focus->delay -= 10;
							g->img_focus->delay = MAX(MIN_GIF_DELAY, g->img_focus->delay);
						}
					}
				}
				break;
			default:
				;
			}

			break;

		case SDL_MOUSEMOTION:
			if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				img = NULL;
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i) {
						img = &g->img[i];
						if (e.motion.xrel != 0 && img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += e.motion.xrel;
						}
						if (e.motion.yrel != 0 && img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += e.motion.yrel;
						}
						fix_rect(img);
					}
				} else {
					img = g->img_focus;
					if (e.motion.xrel != 0 && img->disp_rect.w > img->scr_rect.w) {
						img->disp_rect.x += e.motion.xrel;
					}
					if (e.motion.yrel != 0 && img->disp_rect.h > img->scr_rect.h) {
						img->disp_rect.y += e.motion.yrel;
					}
					fix_rect(img);
				}
				g->status = REDRAW;
			}

		// fall through intentional (mouse movement or button clicks should unhide cursor)
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			SDL_ShowCursor(SDL_ENABLE);
			g->mouse_timer = SDL_GetTicks();
			g->mouse_state = 1;
			break;

		case SDL_MOUSEWHEEL:
			g->status = REDRAW;
			if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i)
						set_rect_zoom(&g->img[i], e.wheel.y);
				} else {
					set_rect_zoom(g->img_focus, e.wheel.y);
				}
			} else {
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i)
						set_rect_zoom(&g->img[i], -e.wheel.y);
				} else {
				set_rect_zoom(g->img_focus, -e.wheel.y);
				}
			}
			break;

		case SDL_WINDOWEVENT:
			g->status = REDRAW;
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;

				// TODO how/where to reset all the "subscreens" rects
				if (g->n_imgs == 1) {
					SET_MODE1_SCR_RECT();
				} else if (g->n_imgs == 2) {
					SET_MODE2_SCR_RECTS();
				} else if (g->n_imgs == 4) {
					SET_MODE4_SCR_RECTS();
				} else if (g->n_imgs == 8) {
					SET_MODE8_SCR_RECTS();
				}
				break;
			case SDL_WINDOWEVENT_EXPOSED: {
				/*
				int x, y;
				SDL_GetWindowSize(g->win, &x, &y);
				printf("windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
				puts("exposed event");
				*/
			}
				break;
			default:
				;
			}
			break;
		}
	}

	return 0;
}

//stupid windows
void normalize_path(char* path)
{
	for (int i=0; i<strlen(path); ++i) {
		if (path[i] == '\\')
			path[i] = '/';
	}
}

int main(int argc, char** argv)
{
	// TODO add --version option
	if (argc < 2) {
		printf("usage: %s image_name\n", argv[0]);
		printf("usage: %s -f text_list_of_image_paths/urls\n", argv[0]);
		printf("Or any combination of those uses, ie:\n");
		printf("usage: %s image.jpg -f list1 example.com/image.jpg -f list3 image4.gif\n", argv[0]);
		exit(0);
	}

	char* path = NULL;
	char dirpath[STRBUF_SZ] = { 0 };
	char img_name[STRBUF_SZ] = { 0 };
	char cachedir[STRBUF_SZ] = { 0 };
	int ticks;

	if (curl_global_init(CURL_GLOBAL_ALL)) {
		puts("Failed to initialize libcurl");
		//cleanup(1, 0);
	}
	cvec_str(&g->files, 0, 100);

	char* exepath = SDL_GetBasePath();
	// TODO think of a company/org name
	char* prefpath = SDL_GetPrefPath("", "sdl_img");
	printf("%s\n%s\n\n", exepath, prefpath);

	int len = snprintf(cachedir, STRBUF_SZ, "%scache", prefpath);
	if (len >= STRBUF_SZ) {
		puts("cache path too long");
		cleanup(1, 0);
	}
	if (mkdir(cachedir, 0700) && errno != EEXIST) {
		perror("Failed to make cache directory");
		cleanup(1, 0);
	}
	g->cachedir = cachedir;

	if (argc == 2) {
		path = argv[1];
		normalize_path(path);
		mydirname(path, dirpath);
		mybasename(path, img_name);

		g->dirpath = dirpath;

		setup(img_name);

		//SDL_Thread* scandir_thrd;
		g->loading = SCANNING;
		scandir(img_name);
		//if (!(scandir_thrd = SDL_CreateThread(scandir, "scandir_thrd", img_name))) {
		//	puts("couldn't create thread");
		//}
		//SDL_DetachThread(scandir_thrd);

	} else {
		g->dirpath = NULL;
		for (int i=1; i<argc; ++i) {
			if (!strcmp(argv[i], "-f")) {
				FILE* file = fopen(argv[++i], "r");
				if (!file) {
					perror("fopen");
					cleanup(1, 0);
				}
				char* s;
				int len;
				while ((s = fgets(dirpath, STRBUF_SZ, file))) {
					len = strlen(s);
					s[len-1] = 0;  // get rid of '\n'
					// handle quoted paths
					if ((s[len-2] == '"' || s[len-2] == '\'') && s[len-2] == s[0]) {
						s[len-2] = 0;
						memmove(s, &s[1], len-2);
					}
					normalize_path(s);
					cvec_push_str(&g->files, s);
				}
				fclose(file);
			} else {
				normalize_path(argv[i]);
				cvec_push_str(&g->files, argv[i]);
			}
		}
		printf("loaded all %ld filenames\n", g->files.size);

		setup(g->files.a[0]);
	}

	printf("done with arguments\n");


	int is_a_gif;
	while (1) {
		if (handle_events())
			break;

		ticks = SDL_GetTicks();

		if (g->mouse_state && ticks - g->mouse_timer > HIDE_CURSOR_TIMER) {
			SDL_ShowCursor(SDL_DISABLE);
			g->mouse_state = 0;
		}

		is_a_gif = 0;
		for (int i=0; i<g->n_imgs; ++i) {
			if (g->img[i].frames > 1) {
				if (ticks - g->img[i].frame_timer > g->img[i].delay) {
					g->img[i].frame_i = (g->img[i].frame_i + 1) % g->img[i].frames;
					if (g->img[i].frame_i == 0)
						g->img[i].looped = 1;
					g->img[i].frame_timer = ticks; // should be set after present ...
					g->status = REDRAW;
				}
				is_a_gif = 1;
			}
		}

		if (g->status == REDRAW) {
			SDL_RenderSetClipRect(g->ren, NULL);
			SDL_RenderClear(g->ren);
			for (int i=0; i<g->n_imgs; ++i) {
				SDL_RenderSetClipRect(g->ren, &g->img[i].scr_rect);
				SDL_RenderCopy(g->ren, g->img[i].tex[g->img[i].frame_i], NULL, &g->img[i].disp_rect);
			}
			SDL_RenderPresent(g->ren);
		}

		//"sleep" save CPU cycles/battery especially when not viewing animated gifs
		if (!is_a_gif && !g->loading)
			SDL_Delay(SLEEP_TIME);
		else
			SDL_Delay(MIN_GIF_DELAY/2);
	}

	cleanup(0, 1);

	//never get here
	return 0;
}


