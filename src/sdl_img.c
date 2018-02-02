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

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>

//POSIX works with MinGW64
#include <sys/stat.h>
#include <dirent.h>
#include <curl/curl.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

enum { QUIT, REDRAW, NOCHANGE };
enum { NOTHING, MODE2 = 2, MODE4 = 4, MODE8 = 8, LEFT, RIGHT };

#ifdef _WIN32
#define mkdir(A, B) mkdir(A)
#endif

#define PATH_SEPARATOR '/'
#define ZOOM_RATE 0.05
#define PAN_RATE 0.05

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SET_MODE1_SCR_RECT()                     \
	do {                                         \
	g->img[0].scr_rect.x = 0;                    \
	g->img[0].scr_rect.y = 0;                    \
	g->img[0].scr_rect.w = g->scr_w;             \
	g->img[0].scr_rect.h = g->scr_h;             \
	set_rect_bestfit(&g->img[0], g->fullscreen); \
	} while (0)

#define SET_MODE2_SCR_RECTS()                    \
	do {                                         \
	g->img[0].scr_rect.x = 0;                    \
	g->img[0].scr_rect.y = 0;                    \
	g->img[0].scr_rect.w = g->scr_w/2;           \
	g->img[0].scr_rect.h = g->scr_h;             \
	g->img[1].scr_rect.x = g->scr_w/2;           \
	g->img[1].scr_rect.y = 0;                    \
	g->img[1].scr_rect.w = g->scr_w/2;           \
	g->img[1].scr_rect.h = g->scr_h;             \
	set_rect_bestfit(&g->img[0], g->fullscreen); \
	set_rect_bestfit(&g->img[1], g->fullscreen); \
	} while (0)

#define SET_MODE4_SCR_RECTS()                         \
	do {                                              \
	for (int i=0; i<4; ++i) {                         \
		g->img[i].scr_rect.x = (i%2)*g->scr_w/2;      \
		g->img[i].scr_rect.y = (i/2)*g->scr_h/2;      \
		g->img[i].scr_rect.w = g->scr_w/2;            \
		g->img[i].scr_rect.h = g->scr_h/2;            \
		set_rect_bestfit(&g->img[i], g->fullscreen);  \
	}                                                 \
	} while (0)

#define SET_MODE8_SCR_RECTS()                         \
	do {                                              \
	for (int i=0; i<8; ++i) {                         \
		g->img[i].scr_rect.x = (i%4)*g->scr_w/4;      \
		g->img[i].scr_rect.y = (i/4)*g->scr_h/2;      \
		g->img[i].scr_rect.w = g->scr_w/4;            \
		g->img[i].scr_rect.h = g->scr_h/2;            \
		set_rect_bestfit(&g->img[i], g->fullscreen);  \
	}                                                 \
	} while (0)

typedef struct img_state
{
	unsigned char* pixels;
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

	cvector_str files;

	int fullscreen;
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
int loading;

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
		puts("1");
	} else {
		dirpath[0] = '.';
		dirpath[1] = 0;
		puts("2");
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

	// don't need the pixels anymore (don't plan on adding editting features)
	stbi_image_free(img->pixels);
	img->pixels = NULL;

	return 1;
}

void cleanup(int ret)
{
	for (int i=0; i<g->n_imgs; ++i) {
		//stbi_image_free(g->img[i].pixels);

		for (int j=0; j<g->img[i].frames; ++j)
			SDL_DestroyTexture(g->img[i].tex[j]);

		free(g->img[i].tex);
	}

	cvec_free_str(&g->files);

	SDL_DestroyRenderer(g->ren);
	SDL_DestroyWindow(g->win);

	SDL_Quit();

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

int load_image(const char* img_name, img_state* img, int make_textures)
{
	int frames, n;

	// img->frames should always be 0 and there should be no allocated textures
	// in tex because clear_img(img) should always have been called before

	char fullpath[4096];
	int ret;

	if (g->dirpath)
		ret = snprintf(fullpath, 4096, "%s/%s", g->dirpath, img_name);
	else
		ret = snprintf(fullpath, 4096, "%s", img_name);
	if (ret >= 4096) {
		// TODO add messagebox here?
		printf("path too long, exiting\n");
		cleanup(0);
	}

	printf("loading %s\n", fullpath);

	img->pixels = stbi_xload(fullpath, &img->w, &img->h, &n, 4, &frames);
	if (!img->pixels) {
		printf("failed to load %s: %s\n", fullpath, stbi_failure_reason());
		return 0;
	}

	if (frames > img->frame_capacity) {
		// img->tex is either NULL or previously alloced
		img->tex = realloc(img->tex, frames*sizeof(SDL_Texture*));
		img->frame_capacity = frames;
	}

	int size = img->w * img->h * 4;
	//gif delay is in 100ths, ticks are 1000ths
	//assume that the delay is the same for all frames (never seen anything else anyway)
	//and if delay is 0, default to 10 fps
	img->looped = 1;
	if (frames > 1) {
		img->looped = 0;
		img->delay = *(short*)(&img->pixels[size]) * 10;
		if (!img->delay)
			img->delay = 100;
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

int load_imgs_dir(DIR* dir, char* initial_image, int milliseconds)
{
	struct dirent* entry;
	struct stat file_stat;
	char fullpath[4096] = { 0 };
	int ret, i;

	char* names[8] = { 0 };
	for (i=0; i<g->n_imgs; ++i) {
		names[i] = g->files.a[g->img[i].index];
		printf("names[%d] = %s\n", i, names[i]);
	}

	int ticks, start;
	ticks = start = SDL_GetTicks();

	while (ticks - start < milliseconds && (entry = readdir(dir))) {
		ret = snprintf(fullpath, 4096, "%s/%s", g->dirpath, entry->d_name);
		if (ret >= 4096) {
			printf("path too long\n");
			cleanup(0);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}

		// if it's a regular file and an image stb_image recognizes an not the initial image
		if (S_ISREG(file_stat.st_mode) && stbi_info(fullpath, NULL, NULL, NULL) && strcmp(initial_image, entry->d_name)) {
			cvec_push_str(&g->files, entry->d_name);
		}

		ticks = SDL_GetTicks();
	}

	// could do it manually since I maintain sorted
	//printf("sorting images\n");
	qsort(g->files.a, g->files.size, sizeof(char*), cmp_string_lt);

	printf("finding current images to update indices\n");
	char** res;
	for (i=0; i<g->n_imgs; ++i) {
		res = bsearch(&names[i], g->files.a, g->files.size, sizeof(char*), cmp_string_lt);
		if (!res) {
			cleanup(0);
		}
		g->img[i].index = res - g->files.a;
	}

	if (!entry) {
		printf("Loaded all %"PRIuMAX" filenames\n", g->files.size);
		closedir(dir);
		return 1;
	} else {
		printf("loaded %"PRIuMAX" filenames\n", g->files.size);
	}


	return 0;
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
	char title_buf[1024];
	int ret;
	int load_what;
	
	while (1) {
		SDL_LockMutex(g->mtx);
		while (!g->loading) {
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
				tmp = (load_what == RIGHT) ? g->n_imgs : -g->n_imgs;
				for (int i=0; i<g->n_imgs; ++i) {
					img[i].index = g->img[i].index;
					do {
						img[i].index = wrap(img[i].index + tmp);
					} while (!(ret = load_image(g->files.a[img[i].index], &img[i], SDL_FALSE)));
					set_rect_bestfit(&img[i], g->fullscreen);
				}
				// just set title to upper left image when !img_focus
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[img[0].index], title_buf));

			} else {
				tmp = (load_what == RIGHT) ? 1 : -1;
				img[0].index = g->img_focus->index;
				do {
					img[0].index = wrap(img[0].index + tmp);
				} while (!(ret = load_image(g->files.a[img[0].index], &img[0], SDL_FALSE)));
				img[0].scr_rect = g->img_focus->scr_rect;
				set_rect_bestfit(&img[0], g->fullscreen);
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[img[0].index], title_buf));
			}
		} else {
			for (int i=g->n_imgs; i<load_what; ++i) {
				g->img[i].index = g->img[i-1].index;
				do {
					g->img[i].index = wrap(g->img[i].index + 1);
				} while (!(ret = load_image(g->files.a[g->img[i].index], &g->img[i], SDL_FALSE)));
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
	char error_str[1024] = { 0 };
	char title_buf[1024] = { 0 };

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, 1024, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
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
		snprintf(error_str, 1024, "Couldn't create window: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		exit(1);
	}

	g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!g->ren) {
		snprintf(error_str, 1024, "%s, falling back to software renderer.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning: No HW Acceleration", error_str, g->win);
		puts(error_str);
		g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_SOFTWARE);
	}

	if (!g->ren) {
		snprintf(error_str, 1024, "Software rendering failed: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		cleanup(1);
	}
	// get black screen while loading (big gif could take a few seconds)
	SDL_SetRenderDrawColor(g->ren, 0, 0, 0, 255);
	SDL_RenderClear(g->ren);
	SDL_RenderPresent(g->ren);
	
	g->n_imgs = 1;
	g->img = g->img1;

	g->img[0].tex = malloc(100*sizeof(SDL_Texture*));
	g->img[0].frame_capacity = 100;

	g->fullscreen = 0;

	if (!load_image(img_name, &g->img[0], SDL_TRUE)) {
		cleanup(0);
	}
	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
		printf("Error getting display mode: %s\n", SDL_GetError());
	} else {
		// don't know a way to get window border/title bar dimensions cross platform,
		// but apparently it's not necessary, if the the dimensions are too big it'll get
		// set to the max windowed size (at least on Linux still need to test windows)
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

	SET_MODE1_SCR_RECT();
	SDL_RenderSetClipRect(g->ren, &g->img[0].scr_rect);
	SDL_RenderCopy(g->ren, g->img[0].tex[g->img[0].frame_i], NULL, &g->img[0].disp_rect);
	SDL_RenderPresent(g->ren);

	if (!(g->cnd = SDL_CreateCond())) {
		printf("Error: %s", SDL_GetError());
		cleanup(0);
	}

	if (!(g->mtx = SDL_CreateMutex())) {
		printf("Error: %s", SDL_GetError());
		cleanup(0);
	}

	SDL_Thread* loading_thrd;
	if (!(loading_thrd = SDL_CreateThread(load_new_images, "loading_thrd", NULL))) {
		puts("couldn't create thread");
	}
	SDL_DetachThread(loading_thrd);

}

void clear_img(img_state* img)
{
	//stbi_image_free(img->pixels);
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}

	//could clear everything else but these are the important
	//ones logic is based on
	//img->pixels = NULL;
	img->frames = 0;
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

/*
int any_zoomed()
{
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			img = &g->img[i];
			if (img->disp_rect.h > img->scr_rect.h || img->disp_rect.w > img->scr_rect.w) {
				img->disp_rect.y += PAN_RATE * img->disp_rect.h;
				fix_rect(img);
				zoomed = 1;
			}
		}
	} else {
		img = g->img_focus;
		if (img->disp_rect.h > img->scr_rect.h) {
			img->disp_rect.y += PAN_RATE * img->disp_rect.h;
			fix_rect(img);
			zoomed = 1;
		}
	}
	return 0;
}
*/

int handle_events()
{
	SDL_Event e;
	int sc;
	int zoomed;
	char title_buf[1024];
	img_state* img;

	g->status = NOCHANGE;

	SDL_Keymod mod_state = SDL_GetModState();

	SDL_Event right;
	right.type = SDL_KEYDOWN;
	right.key.keysym.scancode = SDL_SCANCODE_RIGHT;

	int ticks = SDL_GetTicks();
	int set_slide_timer = 0;

	if (g->slideshow && !loading && ticks - g->slide_timer > g->slideshow) {
		int i;
		// make sure all current gifs have gotten to the end
		// at least once
		for (i=0; i<g->n_imgs; ++i) {
			if (!g->img[i].looped)
				break;
		}
		if (i == g->n_imgs) {
			SDL_PushEvent(&right);
		}
	}

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
		loading = 0;
		if (g->slideshow)
			set_slide_timer = 1;
	}
	SDL_UnlockMutex(g->mtx);

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
						g->slideshow = 0;
					} else if (g->fullscreen) {
						g->status = REDRAW;
						SDL_SetWindowFullscreen(g->win, 0);
						g->fullscreen = 0;
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
				set_slide_timer = 1;
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
					set_slide_timer = 1;
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
				set_slide_timer = 1;
				if (!loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
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
				set_slide_timer = 1;
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
				set_slide_timer = 1;
				if (!loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
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
					if (!g->img_focus) {
						for (int i=0; i<g->n_imgs; ++i)
							set_rect_bestfit(&g->img[i], 1);
					} else {
						set_rect_bestfit(g->img_focus, 1);
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

			case SDL_SCANCODE_RIGHT:
				zoomed = 0;
				g->status = REDRAW;
				if (loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
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
				if (!loading && !zoomed) {
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
				if (loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
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
				if (!loading && !zoomed) {
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
				if (loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
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
				if (!loading && !zoomed) {
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
				if (loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
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
				if (!loading && !zoomed) {
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
								g->img[i].delay = MAX(20, g->img[i].delay);
							}
						}
					} else {
						if (g->img_focus->frames > 1) {
							g->img_focus->delay -= 10;
							g->img_focus->delay = MAX(20, g->img_focus->delay);
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

	if (set_slide_timer) {
		g->slide_timer =  SDL_GetTicks();
	}

	return 0;
}

size_t write_data(void* buf, size_t size, size_t num, void* userp)
{
	return fwrite(buf, 1, size*num, (FILE*)userp);
}

//stupid windows
void normalize_path(char* path)
{
	for (int i=0; i<strlen(path); ++i) {
		if (path[i] == '\\')
			path[i] = '/';
	}
}

void convert_path(char* path);

int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("usage: %s image_name\n", argv[0]);
		printf("usage: %s -f text_list_of_images\n", argv[0]);
		printf("usage: %s -u text_list_of_image_urls\n", argv[0]);
		printf("Or any combination of those uses, ie:\n");
		printf("usage: %s image1 -f list -u url_list img2 img3\n", argv[0]);
		exit(0);
	}

	char* path = NULL;
	char dirpath[4096] = { 0 };
	char img_name[1024] = { 0 };
	int done_loading = 1;
	int ticks;
	DIR* dir;

	curl_global_init(CURL_GLOBAL_ALL);
	cvec_str(&g->files, 0, 100);

	char* exepath = SDL_GetBasePath();
	// TODO think of a company name
	char* prefpath = SDL_GetPrefPath("", "sdl_img");
	printf("%s\n%s\n\n", exepath, prefpath);

	if (argc == 2) {
		path = argv[1];
		normalize_path(path);
		mydirname(path, dirpath);
		mybasename(path, img_name);

		g->dirpath = dirpath;

		ticks = SDL_GetTicks();
		setup(img_name);

		dir = opendir(dirpath);
		if (!dir) {
			perror("opendir");
			cleanup(1);
		}
		cvec_push_str(&g->files, img_name);
		printf("reading file names\n");

		done_loading = load_imgs_dir(dir, img_name, 2500-(SDL_GetTicks()-ticks));
	} else {
		g->dirpath = NULL;
		for (int i=1; i<argc; ++i) {
			if (!strcmp(argv[i], "-f")) {
				FILE* file = fopen(argv[++i], "r");
				if (!file) {
					perror("fopen");
					return 1; // make cleanup() more flexible
				}
				char* s;
				int len;
				while ((s = fgets(dirpath, 4096, file))) {
					len = strlen(s);
					s[len-1] = 0;
					if (stbi_info(s, NULL, NULL, NULL)) {
						normalize_path(s);
						cvec_push_str(&g->files, s);
					}
				}
				fclose(file);
			} else if (!strcmp(argv[i], "-u")) {
				FILE* file = fopen(argv[++i], "r");
				FILE* imgfile;
				if (!file) {
					perror("fopen");
					return 1; // make cleanup() more flexible
				}
				char* s;
				CURL* curl = curl_easy_init();
				CURLcode res;
				int len;
				char filename[1024];
				char cachedir[1024];
				char curlerror[CURL_ERROR_SIZE];
				sprintf(cachedir, "%scache", prefpath);
				mkdir(cachedir, 0700);
				while ((s = fgets(dirpath, 4096, file))) {
					len = strlen(s);
					s[len-1] = 0;

					char* slash = strrchr(s, '/');
					if (!slash)
						continue;
					snprintf(filename, sizeof(filename), "%s/%s", cachedir, slash+1);

					printf("Getting %s\n%s\n", s, filename);
					if (!(imgfile = fopen(filename, "wb"))) {
						perror("fopen");
						return 0;
					}

					// TODO Do I have to set all of these every time or just the ones that change
					curl_easy_setopt(curl, CURLOPT_URL, s);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, imgfile);
					curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
					curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");
					curl_easy_setopt(curl, CURLOPT_CAPATH, exepath);

					if ((res = curl_easy_perform(curl)) != CURLE_OK) {
						printf("curl: %s\n", curlerror);
					}
					fclose(imgfile);
					if (stbi_info(filename, NULL, NULL, NULL))
						cvec_push_str(&g->files, filename);
					else
						printf("%s\n", stbi_failure_reason());
				}
				fclose(file);
				curl_easy_cleanup(curl);
			} else {
				path = argv[i];
				normalize_path(path);
				cvec_push_str(&g->files, path);
			}
		}

		path = g->files.a[0];
		printf("starting with\n%s\n", path);
		setup(path);
	}

	printf("done with setup\n");


	int is_a_gif;
	while (1) {
		if (handle_events())
			break;

		ticks = SDL_GetTicks();

		if (g->mouse_state && ticks - g->mouse_timer > 5000) {
			SDL_ShowCursor(SDL_DISABLE);
			g->mouse_state = 0;
		}

		if (!done_loading && argc == 2) {
			done_loading = load_imgs_dir(dir, img_name, 300);
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

		//"sleep" save CPU cycles/battery when not viewing animated gifs
		if (done_loading && !is_a_gif && !loading)
			SDL_Delay(50);
	}

	cleanup(0);

	//never get here
	return 0;
}


