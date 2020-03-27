// The MIT License (MIT)
// 
// Copyright (c) 2017-2020 Robert Winkler
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


// for strcasestr
#define _GNU_SOURCE

#define CVECTOR_IMPLEMENTATION
#define CVEC_ONLY_INT
#define CVEC_ONLY_STR
#include "cvector.h"

#include "WjCryptLib_Md5.c"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

// was messing with tcc
//#define STBI_NO_SIMD
//#define SDL_DISABLE_IMMINTRIN_H
//
//#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// TODO sin, cos, sqrt etc.
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
//#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_IMPLEMENTATION
#include "nuklear.h"

#define SDL_MAIN_HANDLED
#include "nuklear_sdl.h"

// for rotozoomSurfaceSize()
#include <SDL2_rotozoom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

//POSIX (mostly) works with MinGW64
#include <sys/stat.h>
#include <dirent.h>
#include <curl/curl.h>

enum { QUIT, REDRAW, NOCHANGE };
enum { NOTHING = 0, MODE1 = 1, MODE2 = 2, MODE4 = 4, MODE8 = 8, LEFT, RIGHT, SELECTION, EXIT };
enum { NOT_EDITED, ROTATED, TO_ROTATE, FLIPPED};
enum { DELAY, ALWAYS, NEVER };
enum { NONE, NAME_UP, NAME_DOWN, PATH_UP, PATH_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };
enum { NEXT, PREV, ZOOM_PLUS, ZOOM_MINUS, ROT_LEFT, ROT_RIGHT, FLIP_H, FLIP_V,
       MODE_CHANGE, THUMB_MODE, LIST_MODE, DELETE_IMG, ACTUAL_SIZE, ROT360, SHUFFLE,
       SORT_NAME, SORT_PATH, SORT_SIZE, SORT_MODIFIED, NUM_USEREVENTS };

// off on visual search results
enum {
	NORMAL           = 0x1,
	THUMB_DFLT       = 0x2,
	THUMB_VISUAL     = 0x4,
	THUMB_SEARCH     = 0x8,
	THUMB_RESULTS    = 0x10,
	LIST_DFLT        = 0x20,
	LIST_RESULTS     = 0x40,
	VIEW_RESULTS     = 0x80
};

#define THUMB_MASK (THUMB_DFLT | THUMB_VISUAL | THUMB_SEARCH | THUMB_RESULTS)
#define LIST_MASK (LIST_DFLT | LIST_RESULTS)

#define IS_THUMB_MODE() (g->state & THUMB_MASK)
#define IS_LIST_MODE() (g->state & LIST_MASK)
#define IS_VIEW_RESULTS() (g->state & VIEW_RESULTS)

typedef uint8_t u8;
typedef uint32_t u32;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#ifdef _WIN32
#define mkdir(A, B) mkdir(A)
#endif

// TODO hmm
#define VERSION 0.98
#define VERSION_STR "sdl_img 0.98"

#define PATH_SEPARATOR '/'
#define PAN_RATE 0.05
#define MIN_GIF_DELAY 10
#define HIDE_GUI_DELAY 2
#define SLEEP_TIME 50
#define STRBUF_SZ 1024
#define START_WIDTH 1200
#define START_HEIGHT 800
#define THUMBSIZE 128
#define THUMB_ROWS 8
#define THUMB_COLS 15
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

// zoom is calculated
// h = old_h * (1.0 + zoom*ZOOM_RATE)
// zoom is divided by GIF_ZOOM_DIV if any
// current image is gif because fps is higher
// which speeds up GUI button repeat
//
// It doesn't affect mouse/keyboard so I should
// probably change do_zoom to only divide in the
// gif/gui case...
#define ZOOM_RATE 0.01
#define GUI_ZOOM 5
#define SCROLL_ZOOM 12
#define KEY_ZOOM 12
#define GIF_ZOOM_DIV 3

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// TODO not used currently, would be useful if
// I used SDL_Surfaces
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xFF000000
#define GMASK 0x00FF0000
#define BMASK 0x0000FF00
#define AMASK 0x000000FF
#else
#define RMASK 0x000000FF;
#define GMASK 0x0000FF00;
#define BMASK 0x00FF0000;
#define AMASK 0xFF000000;
#endif


#define SET_MODE1_SCR_RECT()                                                   \
	do {                                                                       \
	g->img[0].scr_rect.x = 0;                                                  \
	g->img[0].scr_rect.y = 0;                                                  \
	g->img[0].scr_rect.w = g->scr_w;                                           \
	g->img[0].scr_rect.h = g->scr_h;                                           \
	set_rect_bestfit(&g->img[0], g->fullscreen | g->slideshow | g->fill_mode); \
	} while (0)

#define SET_MODE2_SCR_RECTS()                                                  \
	do {                                                                       \
	g->img[0].scr_rect.x = 0;                                                  \
	g->img[0].scr_rect.y = 0;                                                  \
	g->img[0].scr_rect.w = g->scr_w/2;                                         \
	g->img[0].scr_rect.h = g->scr_h;                                           \
	g->img[1].scr_rect.x = g->scr_w/2;                                         \
	g->img[1].scr_rect.y = 0;                                                  \
	g->img[1].scr_rect.w = g->scr_w/2;                                         \
	g->img[1].scr_rect.h = g->scr_h;                                           \
	set_rect_bestfit(&g->img[0], g->fullscreen | g->slideshow | g->fill_mode); \
	set_rect_bestfit(&g->img[1], g->fullscreen | g->slideshow | g->fill_mode); \
	} while (0)

#define SET_MODE4_SCR_RECTS()                                                       \
	do {                                                                            \
	for (int i=0; i<4; ++i) {                                                       \
		g->img[i].scr_rect.x = (i%2)*g->scr_w/2;                                    \
		g->img[i].scr_rect.y = (i/2)*g->scr_h/2;                                    \
		g->img[i].scr_rect.w = g->scr_w/2;                                          \
		g->img[i].scr_rect.h = g->scr_h/2;                                          \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);  \
	}                                                                               \
	} while (0)

#define SET_MODE8_SCR_RECTS()                                                       \
	do {                                                                            \
	for (int i=0; i<8; ++i) {                                                       \
		g->img[i].scr_rect.x = (i%4)*g->scr_w/4;                                    \
		g->img[i].scr_rect.y = (i/4)*g->scr_h/2;                                    \
		g->img[i].scr_rect.w = g->scr_w/4;                                          \
		g->img[i].scr_rect.h = g->scr_h/2;                                          \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);  \
	}                                                                               \
	} while (0)

#define SET_THUMB_SCR_RECTS()                                                       \
	do {                                                                            \
	for (int i=0; i<120; ++i) {                                                     \
		g->img[i].scr_rect.x = (i%15)*g->scr_w/15;                                  \
		g->img[i].scr_rect.y = (i/15)*g->scr_h/8;                                   \
		g->img[i].scr_rect.w = g->scr_w/15;                                         \
		g->img[i].scr_rect.h = g->scr_h/8;                                          \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);  \
	}                                                                               \
	} while (0)

typedef struct thumb_state
{
	int w;
	int h;
	SDL_Texture* tex;
} thumb_state;

#define RESIZE(x) ((x+1)*2)

CVEC_NEW_DECLS2(thumb_state)

CVEC_NEW_DEFS2(thumb_state, RESIZE)

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

typedef struct img_state
{
	u8* pixels;
	int w;
	int h;
	int file_size;
	char* fullpath;  // allocated by realpath() needs to be freed

	int index;
	int is_dup;  // TODO not used

	int frame_i;
	int delay; // for now just use the same delay for every frame
	int frames;
	int frame_capacity;
	int frame_timer;
	int looped;
	int paused;
	int edited;
	int rotdegs;
	
	SDL_Texture** tex;

	SDL_Rect scr_rect;  // rect describing available space (ie clip space)
	SDL_Rect disp_rect; // rect image is actually rendered to
} img_state;

typedef struct global_state
{
	SDL_Window* win;
	SDL_Renderer* ren;
	struct nk_context* ctx;

	// scaling is for GUI only
	float x_scale;
	float y_scale;
	u32 userevent;

	int scr_w;
	int scr_h;

	// stupid hack for arbitrary rotation
	u8* orig_pix;
	int orig_w;
	int orig_h;

	img_state* img_focus;
	int n_imgs;
	img_state* img;

	img_state img1[8];
	img_state img2[8];

	cvector_thumb_state thumbs;
	cvector_i search_results;
	int cur_result;

	int status;

	char* cachedir;
	char* thumbdir;
	//char* config_dir;

	cvector_file files;
	cvector_str favs;

	int state; // better name?

	int fullscreen;
	int fill_mode;
	int gui_delay;
	int gui_timer;
	int show_gui;
	int fullscreen_gui;
	int show_infobar;

	int list_setscroll;

	int thumbs_done;
	int thumb_start_row;
	int thumb_rows;
	int thumb_cols;

	int thumb_sel;  // current image
	int thumb_sel_end; // start or end of visual selection (_sel is other side)
	int selection;  // actual selection made (switching to normal mode)

	int show_about;
	int show_prefs;
	int show_rotate;
	int menu_state;
	int sorted_state;

	struct nk_color bg;

	int slide_delay;
	int slideshow;
	int slide_timer;

	// threading
	int generating_thumbs;
	int loading;
	int done_loading;
	SDL_cond* cnd;
	SDL_mutex* mtx;

} global_state;

// Use a pointer in case I ever move this to another TU, though it's unlikely
// Also I know initializing a global to 0 is redundant but meh
static global_state state = { 0 };
global_state* g = &state;

char text[STRBUF_SZ];
int text_len;
char* composition;
Sint32 cursor;
Sint32 selection_len;

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
	// char* iec_sizes[3] = { "bytes", "KiB", "MiB" };
	char* si_sizes[3] = { "bytes", "KB", "MB" }; // GB?  no way

	char** sizes = si_sizes;
	int i = 0;
	double sz = bytes;
	// TODO MiB KiB? 2^10, 2^20?
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


// has to come after all the enums/macros/struct defs and bytes2str 
#include "gui.c"

#include "sorting.c"

size_t write_data(void* buf, size_t size, size_t num, void* userp)
{
	return fwrite(buf, 1, size*num, (FILE*)userp);
}

//need to think about best way to organize following 4 functions' functionality
void adjust_rect(img_state* img, int w, int h, int use_mouse)
{
	// default is just zoom in/out with the image centered in it screen space
	int final_x = img->scr_rect.x + (img->scr_rect.w-w)/2;
	int final_y = img->scr_rect.y + (img->scr_rect.h-h)/2;

	// if the mouse scrollwheel was used and there's a single image focused *and* the image is bigger
	// than its space in at least one dimension, zoom in on where the mouse is hovering
	if (use_mouse && (g->n_imgs == 1 || g->img_focus) && (w > img->scr_rect.w || h > img->scr_rect.h)) {
		int x, y;

		// I know r and b aren't actually valid pixels since it's [x, x+w) etc.
		// but I'm just using it as an intermediate step and I want the full 0-100%
		int l = img->scr_rect.x;
		int r = l + img->scr_rect.w;
		int t = img->scr_rect.y;
		int b = t + img->scr_rect.h;

		SDL_GetMouseState(&x, &y);

		// clip mouse position to image available screen space
		x = MAX(x, l);
		x = MIN(x, r);
		y = MAX(y, t);
		y = MIN(y, b);

		float px = (x-img->disp_rect.x)/(float)img->disp_rect.w;
		float py = (y-img->disp_rect.y)/(float)img->disp_rect.h;

		// There might be a slightly better way to organize/calculate this but this works
		if (w > img->scr_rect.w) {
			// adjust based on mouse position % relative to current display width * new width
			final_x = x-px*w;

			// don't allow blank space on a side if the dimension is greater
			// than the screen space, that would be a waste
			if (final_x + w < r)
				final_x += r-(final_x+w);
			if (final_x > l)
				final_x = l;
		}
		if (h > img->scr_rect.h) {
			final_y = y-py*h;

			if (final_y + h < b)
				final_y += b-(final_y+h);
			if (final_y > t)
				final_y = t;
		}
	}

	// in either case update the display rect
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

	adjust_rect(img, w, h, SDL_FALSE);
}

void set_rect_zoom(img_state* img, int zoom, int use_mouse)
{
	float aspect = img->w/(float)img->h;
	int h, w;
	
	h = img->disp_rect.h * (1.0 + zoom*ZOOM_RATE);
	if (zoom > 0 && h == img->disp_rect.h) {
		h = img->disp_rect.h + 10; // 10 is arbitrary
	} else if (h < 0.025 * img->h) { // so is 2.5%
		h = 0.025 * img->h;
	}

	if (h > 20 * img->h) {
		h = 20 * img->h;
	}

	w = h * aspect;

	adjust_rect(img, w, h, use_mouse);
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
			SDL_Log("Error creating texture: %s\n", SDL_GetError());
			return 0;
		}
		if (SDL_UpdateTexture(img->tex[i], NULL, img->pixels+(size+2)*i, img->w*4)) {
			SDL_Log("Error updating texture: %s\n", SDL_GetError());
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

	if (img->edited && img->frames == 1) {
		char msgbox_prompt[STRBUF_SZ];
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

		char* full_img_path = g->files.a[img->index].path;

		snprintf(msgbox_prompt, STRBUF_SZ, "Do you want to save changes to '%s'?", full_img_path);
		messageboxdata.message = msgbox_prompt;
		SDL_ShowMessageBox(&messageboxdata, &buttonid);

		if (buttonid == 1) {
			char* ext = strrchr(full_img_path, '.');
			if (!ext) {
				ext = ".jpg";
				strcat(full_img_path, ext);
				// TODO should I overwrite the original? If not, I need
				// to update g->files.a[img->index].path with the new name
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
	free(img->fullpath);
	img->pixels = NULL;
	img->frames = 0;
	img->rotdegs = 0;
	img->edited = NOT_EDITED;
	img->file_size = 0;
}

void cleanup(int ret, int called_setup)
{
	if (called_setup) {

		// not really necessary to exit thread but
		// valgrind reports it as possibly lost if not
		SDL_LockMutex(g->mtx);
		g->loading = EXIT;
		SDL_CondSignal(g->cnd);
		SDL_UnlockMutex(g->mtx);

		for (int i=0; i<g->n_imgs; ++i) {
			clear_img(&g->img[i]);
			free(g->img[i].tex);
		}

		SDL_DestroyRenderer(g->ren);
		SDL_DestroyWindow(g->win);
		SDL_Quit();
	}

	cvec_free_thumb_state(&g->thumbs);
	cvec_free_file(&g->files);
	curl_global_cleanup();
	exit(ret);
}

extern inline void hash2str(char* str, MD5_HASH* h)
{
	char buf[3];

	for (int i=0; i<MD5_HASH_SIZE; ++i) {
		sprintf(buf, "%02x", h->bytes[i]);
		strcat(str, buf);
	}
}

void get_thumbpath(const char* path, char* thumbpath, size_t thumbpath_len)
{
	MD5_HASH hash;
	char hash_str[MD5_HASH_SIZE*2+1] = { 0 };

	Md5Calculate(path, strlen(path), &hash);
	hash_str[0] = 0;
	hash2str(hash_str, &hash);
	// could just do the %02x%02x etc. here but that'd be a long format string and 16 extra parameters
	int ret = snprintf(thumbpath, thumbpath_len, "%s/%s.png", g->thumbdir, hash_str);
	if (ret >= thumbpath_len) {
		SDL_Log("path too long\n");
		cleanup(0, 1);
	}
}

void make_thumb_tex(int i, int w, int h, u8* pix)
{
	if (!pix)
		return;

	g->thumbs.a[i].tex = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
	if (!g->thumbs.a[i].tex) {
		SDL_Log("Error creating texture: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	if (SDL_UpdateTexture(g->thumbs.a[i].tex, NULL, pix, w*4)) {
		SDL_Log("Error updating texture: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	g->thumbs.a[i].w = w;
	g->thumbs.a[i].h = h;
}

int thumb_thread(void* data)
{
	int w, h, channels;
	int out_w, out_h;
	char thumbpath[STRBUF_SZ] = { 0 };

	// seems so much more efficient but PATH_MAX isn't really accurate
	// or portable... I'm tempted to just pick an arbitrary number like 4*STRBUF_SZ
	// or something
	// char fullpath[PATH_MAX] = { 0 };
	char* fullpath;

	struct stat thumb_stat, orig_stat;

	intptr_t do_load = (intptr_t)data;

	int start = SDL_GetTicks();
	u8* pix;
	u8* outpix;
	for (int i=0; i<g->files.size; ++i) {
		// TODO better to stat orig here and error out early for a url?

		// Windows will just generate duplicate thumbnails most of the time
#ifndef _WIN32
		if (!(fullpath = realpath(g->files.a[i].path, NULL)))
			continue;
#else
		fullpath = g->files.a[i].path;
#endif
		get_thumbpath(fullpath, thumbpath, sizeof(thumbpath));
#ifndef _WIN32
		free(fullpath); // keep for use below? not worth it imo
#endif


		if (!stat(thumbpath, &thumb_stat)) {
			// someone has deleted the original since we made the thumb or it's a url
			if (stat(g->files.a[i].path, &orig_stat)) {
				continue;
			}

			// make sure original hasn't been modified since thumb was made
			// don't think it's necessary to check nanoseconds
			if (orig_stat.st_mtime < thumb_stat.st_mtime) {
				if (do_load) {
					outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
					make_thumb_tex(i, w, h, outpix);
				}
				continue;
			}
		}

		pix = stbi_load(g->files.a[i].path, &w, &h, &channels, 4);
		if (!pix) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s for thumbnail generation\n", g->files.a[i].path);
			continue;
		}

		if (w > h) {
			out_w = THUMBSIZE;
			out_h = THUMBSIZE * h/(float)w;
		} else {
			out_h = THUMBSIZE;
			out_w = THUMBSIZE * w/(float)h;
		}

		if (!(outpix = malloc(out_h*out_w*4))) {
			cleanup(0, 1);
		}

		if (!stbir_resize_uint8(pix, w, h, 0, outpix, out_w, out_h, 0, 4)) {
			free(pix);
			free(outpix);
			continue;
		}

		stbi_write_png(thumbpath, out_w, out_h, 4, outpix, out_w*4);

		if (do_load) {
			make_thumb_tex(i, out_w, out_h, outpix);
		}
		free(pix);
		free(outpix);
		SDL_Log("generated thumb %d for %s\n", i, g->files.a[i].path);
	}

	g->generating_thumbs = SDL_FALSE;
	g->thumbs_done = SDL_TRUE;
	SDL_Log("Done generating thumbs in %d, exiting thread.\n", SDL_GetTicks()-start);
	return 0;
}

void free_thumb(void* t)
{
	SDL_DestroyTexture(t);
}

void generate_thumbs(intptr_t do_load)
{
	if (g->thumbs.a || (g->thumbs_done && !do_load))
		return;

	// still using separate calloc because calling vec constructor uses
	// malloc and I want them 0'd
	thumb_state* tmp;
	if (!(tmp = calloc(g->files.size, sizeof(thumb_state)))) {
		cleanup(0, 1);
	}
	g->thumbs.a = tmp;
	g->thumbs.size = g->files.size;
	g->thumbs.capacity = g->files.size;
	g->thumbs.elem_free = free_thumb;
	// elem_init already NULL

	g->generating_thumbs = SDL_TRUE;
	SDL_Log("Starting thread to generate thumbs and create thumb textures...\n");
	SDL_Thread* thumb_thrd;
	if (!(thumb_thrd = SDL_CreateThread(thumb_thread, "thumb_thrd", (void*)do_load))) {
		// TODO warning?
		SDL_Log("couldn't create thumb thread\n");
	}
	// passing NULL is a no-op like free
	SDL_DetachThread(thumb_thrd);
}

// debug
#if 0
void print_img_state(img_state* img)
{
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "{\nimg = %p\n", img);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "pixels = %p\n", img->pixels);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "WxH = %dx%d\n", img->w, img->h);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "index = %d\n", img->index);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "rotdegs = %d\n", img->rotdegs);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "frame_i = %d\ndelay = %d\nframes = %d\nframe_cap = %d\n", img->frame_i, img->delay, img->frames, img->frame_capacity);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "frame_timer = %d\nlooped = %d\n", img->frame_timer, img->looped);

	("tex = %p\n", img->tex);
	for (int i=0; i<img->frames; ++i) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "tex[%d] = %p\n", i, img->tex[i]);
	}

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "scr_rect = %d %d %d %d\n", img->scr_rect.x, img->scr_rect.y, img->scr_rect.w, img->scr_rect.h);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "disp_rect = %d %d %d %d\n}\n", img->disp_rect.x, img->disp_rect.y, img->disp_rect.w, img->disp_rect.h);
}
#else
#define print_img_state(x)
#endif

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

int curl_image(int img_idx)
{
	CURL* curl = curl_easy_init();
	CURLcode res;
	char filename[STRBUF_SZ];
	char curlerror[CURL_ERROR_SIZE];
	char* s = g->files.a[img_idx].path;
	FILE* imgfile;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
	#ifdef _WIN32
	curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");
	curl_easy_setopt(curl, CURLOPT_CAPATH, SDL_GetBasePath());
	#endif

	char* slash = strrchr(s, PATH_SEPARATOR);
	if (!slash) {
		SDL_Log("invalid url\n");
		goto exit_cleanup;
	}
	int len = snprintf(filename, STRBUF_SZ, "%s/%s", g->cachedir, slash+1);
	if (len >= STRBUF_SZ) {
		SDL_Log("url too long\n");
		goto exit_cleanup;
	}

	SDL_Log("Getting %s\n%s\n", s, filename);
	if (!(imgfile = fopen(filename, "wb"))) {
		// TODO Log?
		perror("fopen");
		goto exit_cleanup;
	}

	curl_easy_setopt(curl, CURLOPT_URL, s);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, imgfile);

	if ((res = curl_easy_perform(curl)) != CURLE_OK) {
		SDL_Log("curl error: %s\n", curlerror);
		goto exit_cleanup;
	}
	fclose(imgfile);


	struct stat file_stat;
	stat(filename, &file_stat);

	file* f = &g->files.a[img_idx];
	free(f->path);
	f->path = mystrdup(filename);
	f->size = file_stat.st_size;
	f->modified = file_stat.st_mtime;

	bytes2str(f->size, f->size_str, SIZE_STR_BUF);
	struct tm* tmp_tm = localtime(&f->modified);
	strftime(f->mod_str, MOD_STR_BUF, "%F %T", tmp_tm);
	char* sep = strrchr(f->path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
	f->name = (sep) ? sep+1 : f->path;


	curl_easy_cleanup(curl);
	return 1;

exit_cleanup:
	curl_easy_cleanup(curl);
	return 0;
}

int load_image(const char* fullpath, img_state* img, int make_textures)
{
	int frames, n;

	// img->frames should always be 0 and there should be no allocated textures
	// in tex because clear_img(img) should always have been called before

#ifndef _WIN32
	img->fullpath = realpath(fullpath, NULL);
	SDL_Log("loading %s\n", fullpath);
#endif

	img->pixels = stbi_xload(fullpath, &img->w, &img->h, &n, STBI_rgb_alpha, &frames);
	if (!img->pixels) {
		SDL_Log("failed to load %s: %s\n", fullpath, stbi_failure_reason());
		return 0;
	}

	struct stat file_stat;
	if (!stat(fullpath, &file_stat)) {
		img->file_size = file_stat.st_size;
	}

	SDL_Texture** tmp;
	if (frames > img->frame_capacity) {
		// img->tex is either NULL or previously alloced
		if (!(tmp = realloc(img->tex, frames*sizeof(SDL_Texture*)))) {
			perror("Couldn't allocate tex array");
			return 0;
		}
		img->tex = tmp;

		img->frame_capacity = frames;
	}

	int size = img->w * img->h * 4;
	//gif delay is in 100ths, ticks are 1000ths, but newer stb_image converts for us
	//assume that the delay is the same for all frames (never seen anything else anyway)
	//and if delay is 0, default to 10 fps
	img->looped = 1;
	img->paused = 0;
	if (frames > 1) {
		img->looped = 0;
		img->delay = *(short*)(&img->pixels[size]); // * 10;
		if (!img->delay)
			img->delay = 100;
		img->delay = MAX(MIN_GIF_DELAY, img->delay);
		SDL_Log("%d frames %d delay\n", frames, img->delay);
	}

	img->frames = frames;
	img->frame_i = 0;

	if (make_textures) {
		if (!create_textures(img))
			return 0;
	}

	return 1;
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

	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1, 1);
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
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "path too long\n");
			cleanup(0, 1);
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
		f.path = mystrdup(fullpath);
		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;

		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%F %T", tmp_tm);
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(&g->files, &f);
	}

	closedir(dir);
	g->loading = 0;
	return 1;
}

int wrap(int z)
{
	int n = g->files.size;
	if (IS_VIEW_RESULTS()) {
		n = g->search_results.size;
	}
	while (z < 0) z += n;
	while (z >= n) z -= n;

	return z;
}

int attempt_image_load(int last, img_state* img)
{
	char *path;
	if (IS_VIEW_RESULTS()) {
		path = g->files.a[g->search_results.a[last]].path;
	} else {
		path = g->files.a[last].path;
	}
	int ret = load_image(path, img, SDL_FALSE);
	if (!ret)
		if (curl_image(last)) //TODO results
			ret = load_image(path, img, SDL_FALSE);
	return ret;
}

int load_new_images(void* data)
{
	int tmp;
	char title_buf[STRBUF_SZ];
	int load_what;
	int last;
	
	while (1) {
		SDL_LockMutex(g->mtx);
		while (g->loading < 2) {
			SDL_CondWait(g->cnd, g->mtx);
		}
		load_what = g->loading;
		SDL_UnlockMutex(g->mtx);

		if (load_what == EXIT)
			break;

		//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "loading %p = %d\n", &g->loading, g->loading);
		if (load_what >= LEFT) {
			img_state* img;
			if (g->img == g->img1)
				img = g->img2;
			else
				img = g->img1;

			for (int i=0; i<g->n_imgs; ++i)
				img[i].scr_rect = g->img[i].scr_rect;
			
			// TODO possible (very unlikely) infinite loop if there
			// are allocation failures for every valid image in the list
			if (!g->img_focus) {
				if (load_what >= RIGHT) {
					last = (load_what == RIGHT) ? g->img[g->n_imgs-1].index : g->selection;
					for (int i=0; i<g->n_imgs; ++i) {
						do {
							last = wrap(last + 1);
						} while (!attempt_image_load(last, &img[i]));
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						SDL_Log("Loaded %d\n", last);
						img[i].index = last;
					}
				} else if (load_what == LEFT) {
					last = g->img[0].index;
					for (int i=g->n_imgs-1; i>=0; --i) {
						do {
							last = wrap(last - 1);
						} while (!attempt_image_load(last, &img[i]));
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						img[i].index = last;
					}
				}

				// just set title to upper left image when !img_focus
				// TODO use file.name for all of these
				int index = (g->state & VIEW_RESULTS) ? g->search_results.a[img[0].index] : img[0].index;
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[index].path, title_buf));
			} else {
				tmp = (load_what == RIGHT) ? 1 : -1;
				last = g->img_focus->index;
				do {
					last = wrap(last + tmp);
				} while (!attempt_image_load(last, &img[0]));
				img[0].index = last;
				img[0].scr_rect = g->img_focus->scr_rect;
				set_rect_bestfit(&img[0], g->fullscreen | g->slideshow | g->fill_mode);
				
				int index = (g->state & VIEW_RESULTS) ? g->search_results.a[img[0].index] : img[0].index;
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[index].path, title_buf));
			}
		} else {
			last = g->img[g->n_imgs-1].index;
			for (int i=g->n_imgs; i<load_what; ++i) {
				do {
					last = wrap(last + 1);
				} while (!attempt_image_load(last, &g->img[i]));
				g->img[i].index = last;
			}
		}

		SDL_LockMutex(g->mtx);
		g->done_loading = load_what;
		g->loading = 0;
		SDL_UnlockMutex(g->mtx);
	}

	return 0;
}

void free_file(void* f)
{
	free(((file*)f)->path);
}

void setup(int start_idx)
{
	g->win = NULL;
	g->ren = NULL;
	char error_str[STRBUF_SZ] = { 0 };
	char title_buf[STRBUF_SZ] = { 0 };

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, STRBUF_SZ, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s\n", error_str);
		exit(1);
	}

	g->n_imgs = 1;
	g->img = g->img1;
	g->slide_delay = 3;
	g->gui_delay = HIDE_GUI_DELAY;
	g->show_infobar = nk_true;
	g->bg = nk_rgb(0,0,0);
	g->fill_mode = 0;
	g->thumb_rows = THUMB_ROWS;
	g->thumb_cols = THUMB_COLS;
	g->sorted_state = NAME_UP;  // ie by name ascending
	g->state = NORMAL;

	if (!(g->img[0].tex = malloc(100*sizeof(SDL_Texture*)))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Couldn't allocate tex array: %s\n", strerror(errno));
		cleanup(0, 1);
	}
	g->img[0].frame_capacity = 100;

	// TODO handle when first image (say in a list that's out of date) is gone/invalid
	// loop through till valid
	// TODO can I reuse NEXT code somehow?
	int i = start_idx;
	char* img_name;
	int ret;
	do {
		g->img[0].index = i;
		img_name = g->files.a[i].path;
		// TODO best way to structure this and use in main()?
		ret = load_image(img_name, &g->img[0], SDL_FALSE);
		if (!ret) {
			if (curl_image(i)) {
				ret = load_image(g->files.a[i].path, &g->img[0], SDL_FALSE);
				img_name = g->files.a[i].path;
			}
		}
		i = (i+1) % g->files.size;
	} while (!ret && i != start_idx);

	if (!ret) {
		cleanup(0, 1);
	}

	SDL_Rect r;
	if (SDL_GetDisplayUsableBounds(0, &r)) {
		SDL_Log("Error getting usable bounds: %s\n", SDL_GetError());
		r.w = START_WIDTH;
		r.h = START_HEIGHT;
	} else {
		SDL_Log("Usable Bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);
	}
	g->scr_w = MAX(g->img[0].w, START_WIDTH);
	g->scr_h = MAX(g->img[0].h, START_HEIGHT);
	g->scr_w = MIN(g->scr_w, r.w - 20);  // to account for window borders/titlebar on non-X11 platforms
	g->scr_h = MIN(g->scr_h, r.h - 40);

	int max_w, max_h;
	float hdpi = 0, vdpi = 0, ddpi = 0;
	if (!SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi))
		SDL_Log("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);
	if (!SDL_GetDisplayBounds(0, &r)) {
		SDL_Log("Display Bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);
		if (hdpi && vdpi && ddpi)
			SDL_Log("Physical Screen size: %f %f %f\n", r.w / hdpi, r.h / vdpi, sqrt(r.w*r.w + r.h*r.h) / ddpi);
	}

	u32 win_flags = (g->fullscreen) ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;

	// just experimenting
	//win_flags |= SDL_WINDOW_BORDERLESS;

	// TODO do I need to update scr_w and src_h if it's fullscreen?  is there an initial window event?

	mybasename(img_name, title_buf);
	
	g->win = SDL_CreateWindow(title_buf, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g->scr_w, g->scr_h, win_flags);
	if (!g->win) {
		snprintf(error_str, STRBUF_SZ, "Couldn't create window: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		exit(1);
	}

	// GetWindowBorderSize is only supported on X11 (as of 2019)
	int top, bottom, left, right;
	if (!g->fullscreen && !SDL_GetWindowBordersSize(g->win, &top, &bottom, &left, &right)) {
		g->scr_w -= left + right;
		g->scr_h -= top + bottom;
		SDL_SetWindowSize(g->win, g->scr_w, g->scr_h);
		SDL_SetWindowPosition(g->win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	// No real reason for hardware acceleration and especially on older and/or mobile gpu's you can
	// run into images larger than the max texture size which will then fail to load/display
	//
	// on the other hand, hardware rendering does decrease scaling artifacts...
	//g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_ACCELERATED);
	g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_SOFTWARE);
	if (!g->ren) {
		snprintf(error_str, STRBUF_SZ, "Software rendering failed: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		puts(error_str);
		cleanup(1, 1);
	}

	SDL_GetWindowMaximumSize(g->win, &max_w, &max_h);
	SDL_Log("Window Max dimensions: %d %d\n", max_w, max_h);

	// could adjust for dpi, then adjust for font size if necessary
	g->x_scale = 2; //hdpi/72;
	g->y_scale = 2; //vdpi/72;

	if (!(g->ctx = nk_sdl_init(g->win, g->ren, g->x_scale, g->y_scale))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "nk_sdl_init() failed!\n");
		cleanup(1, 1);
	}

	// Make GUI partially transparent
	g->ctx->style.window.fixed_background.data.color.a *= 0.75;
	//g->ctx->style.window.background.a *= 0.75;

	// Trying to figure out/fix why menu_item_labels are wider than selectables
	//g->ctx->style.selectable.padding = nk_vec2(4.0f,4.0f);
	//g->ctx->style.selectable.touch_padding = nk_vec2(4.0f,4.0f);

	// TODO
	// next and prev events?
	g->userevent = SDL_RegisterEvents(1);
	if (g->userevent == (u32)-1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	// can't create textures till after we have a renderer (otherwise we'd pass SDL_TRUE)
	// to load_image above
	if (!create_textures(&g->img[0]))
		cleanup(1, 1);

	SET_MODE1_SCR_RECT();
	SDL_RenderClear(g->ren);
	SDL_RenderCopy(g->ren, g->img[0].tex[g->img[0].frame_i], NULL, &g->img[0].disp_rect);
	SDL_RenderPresent(g->ren);

	if (!(g->cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* loading_thrd;
	if (!(loading_thrd = SDL_CreateThread(load_new_images, "loading_thrd", NULL))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "couldn't create image loader thread: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	SDL_DetachThread(loading_thrd);

	SDL_Log("Starting with %s\n", img_name);

	g->gui_timer = SDL_GetTicks();
	g->show_gui = 1;
}

// probably now worth having a 2 line function used 3 places?
void set_fullscreen()
{
	g->status = REDRAW;
	SDL_SetWindowFullscreen(g->win, g->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
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

void rotate_img90(img_state* img, int left)
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
	img->edited = ROTATED;

	// since rotate90 actually rotates the pixels (doesn't just update textures)
	// the rotated image becomes the new base image so we can't/shouldn't update degs
	// img->rotdegs += ((left) ? 90 : -90);
}

void rotate_img(img_state* img)
{
	int w, h, frames;
	frames = img->frames;

	if (g->orig_pix) {
		w = g->orig_w;
		h = g->orig_h;
	} else {
		w = img->w;
		h = img->h;
	}

	// So normally we'd have to negate rads because we have a left handed
	// coordinate system where y is down, so a positive rotation is rotating
	// clockwise, however to avoid missing pixels due to aliasing, we are rotating
	// the rotated image backward to find the closest pixel in the original image
	// (ie one pixel could end up multiple places in the rotated image).  This
	// flips the positive rotation direction back to normal.
	float rads = -img->rotdegs * (3.14159265f/180.0f);

	// As long as I need SDL2_gfx for nuklear I might as well use it for other stuff
	// I can always go back to my own code later
	int wrot, hrot;
	rotozoomSurfaceSize(w, h, -img->rotdegs, 1, &wrot, &hrot);

	int hrot2 = hrot / 2;
	int wrot2 = wrot / 2;
	int sz = w*h*4;
	int sz_rot = wrot*hrot*4;

	size_t alloc_size = frames * (sz_rot + ((frames>1)?2:0));

	u8* rot_pixels = NULL;
	if (g->orig_pix) {
		rot_pixels = realloc(img->pixels, alloc_size);
		memset(rot_pixels, 0, alloc_size);
	} else {
		g->orig_pix = img->pixels;
		g->orig_w = w;
		g->orig_h = h;
		rot_pixels = calloc(1, alloc_size);
	}
	if (!rot_pixels) {
		cleanup(0, 1);
	}

	u8* pix = g->orig_pix;
	
	u32* outu32;
	u32* inu32;

	int w2old = w / 2;
	int h2old = h / 2;
	int x, y, xout, yout;

	// TODO anti-aliasing, GL_LINEAR style interpolation?
	for (int k=0; k<frames; ++k) {
		inu32 = (u32*)&pix[(sz+2)*k];
		outu32 = (u32*)&rot_pixels[(sz_rot+2)*k];
		for (int i=0; i<hrot; ++i) {
			y = i - hrot2;
			for (int j=0; j<wrot; ++j) {
				x = j - wrot2;
				xout = x * cos(rads) - y * sin(rads) + w2old;
				yout = x * sin(rads) + y * cos(rads) + h2old;

				if (xout >= 0 && xout < w && yout >= 0 && yout < h) {
					//memcpy(&rotimg[(i*dim + j)*4], &pix[(yout*w + xout)*4], 4);
					outu32[i*wrot + j] = inu32[yout*w + xout];
				}
			}
		}
	}

	img->w = wrot;
	img->h = hrot;
	img->pixels = rot_pixels;
	img->edited = ROTATED;

	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}
	if (!create_textures(img)) {
		cleanup(0, 1);
	}

	// TODO refactor this, rotate_img90 and do_rotate
	if (g->n_imgs == 1)
		SET_MODE1_SCR_RECT();
	else if (g->n_imgs == 2)
		SET_MODE2_SCR_RECTS();
	else if (g->n_imgs == 4)
		SET_MODE4_SCR_RECTS();
	else
		SET_MODE8_SCR_RECTS();
	g->status = REDRAW;

	// Ok was pressed when a change hadn't been done
	// so we couldn't clear in draw_gui because we still
	// needed it
	if (!g->show_rotate) {
		free(g->orig_pix);
		g->orig_pix = NULL;
		g->orig_w = 0;
		g->orig_h = 0;
	}
}

int try_move(int direction)
{
	// TODO prevent moves and some other
	// actions while g->show_rotate.  Since we already
	// hide the GUI while the popups up, we really just have
	// to worry about keyboard actions.
	if (!g->loading) {
		SDL_LockMutex(g->mtx);
		g->loading = direction;
		SDL_CondSignal(g->cnd);
		SDL_UnlockMutex(g->mtx);
		return 1;
	}
	return 0;
}

void do_shuffle()
{
	if (g->n_imgs != 1 || g->generating_thumbs) {
		return;
	}
	char* save = g->files.a[g->img[0].index].path;
	file tmpf;

	thumb_state tmp_thumb;
	int j;
	// Fisher-Yates, aka Knuth Shuffle
	for (int i=g->files.size-1; i>0; --i) {
		j = rand() % (i+1);

		tmpf = g->files.a[i];
		g->files.a[i] = g->files.a[j];
		g->files.a[j] = tmpf;

		if (g->thumbs.a) {
			tmp_thumb = g->thumbs.a[i];
			g->thumbs.a[i] = g->thumbs.a[j];
			g->thumbs.a[j] = tmp_thumb;
		}
	}

	for (int i=0; i<g->files.size; ++i) {
		if (!strcmp(save, g->files.a[i].path)) {
			g->img[0].index = i;
			g->thumb_sel = i;
			g->selection = i;
			break;
		}
	}

	g->sorted_state = NONE;
}

void do_sort(compare_func cmp)
{
	if (g->n_imgs != 1 || g->generating_thumbs) {
		return;
	}

	char* save = g->files.a[g->img[0].index].path;

	// TODO is it worth preserving the list selection?  Or just reset it to current image?
	// especially since it would likely jump out of view unless we reset the scroll position
	// like we do on list startup
	//char* list_sel = (IS_LIST_MODE()) ? g->files.a[g->selection].path : NULL;

	// g->thumbs.a is either NULL or valid
	//sort(g->files.a, g->thumbs.a, g->files.size, cmp);
	if (g->thumbs.a)
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), cmp, 1, g->thumbs.a, sizeof(thumb_state));
	else
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), cmp, 0);

	for (int i=0; i<g->files.size; ++i) {
		if (!strcmp(save, g->files.a[i].path)) {
			g->img[0].index = i;

			// for now just keep current image (what they'll go back to if they hit
			// ESC instead of double clicking or hitting Enter on another one
			g->selection = i;
			break;
		}
	}

	// should work even while viewing results
	if (g->state & LIST_RESULTS) {
		search_filenames();
	}
}

void do_zoom(int dir, int use_mouse)
{
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			if (g->img[i].frames > 1) {
				dir /= GIF_ZOOM_DIV;
				break;
			}
		}
		for (int i=0; i<g->n_imgs; ++i)
			set_rect_zoom(&g->img[i], dir, use_mouse);
	} else {
		if (g->img_focus->frames > 1)
			dir /= GIF_ZOOM_DIV;
		set_rect_zoom(g->img_focus, dir, use_mouse);
	}
}

void do_rotate(int left, int is_90)
{
	img_state* img;
	if (!g->loading) {
		img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
		if (img) {
			if (is_90) {
				rotate_img90(img, left);
				create_textures(img);
			} else {
				g->show_rotate = nk_true;
				g->show_gui = 1;
				g->gui_timer = SDL_GetTicks();
				return;
			}

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
}

void do_flip(int is_vertical)
{
	int w, h;
	int sz;
	int frames;

	img_state* img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
	if (!g->loading && img) {
		w = img->w;
		h = img->h;
		frames = img->frames;

		sz = w * h;
		u8* pix = img->pixels;
		u8* flip_pix = NULL;

		if (frames > 1) {
			if (!(flip_pix = malloc(frames * (sz*4+2)))) {
				perror("Couldn't allocate flipped");
				cleanup(0, 1);
			}
			*(i16*)(&flip_pix[sz*4]) = img->delay;
		} else {
			if (!(flip_pix = malloc(sz*4))) {
				perror("Couldn't allocate flipped");
				cleanup(0, 1);
			}
		}

		i32* p;
		i32* flip;
		if (is_vertical) {
			for (int i=0; i<frames; ++i) {
				p = (i32*)&pix[i*(sz*4+2)];
				flip = (i32*)&flip_pix[i*(sz*4+2)];
				for (int j=0; j<h; ++j) {

					// TODO replace with memcpy
					for (int k=0; k<w; ++k) {
						flip[(h-1-j)*w + k] = p[j*w + k];
					}
				}
			}

		} else {
			for (int i=0; i<frames; ++i) {
				p = (i32*)&pix[i*(sz*4+2)];
				flip = (i32*)&flip_pix[i*(sz*4+2)];
				for (int j=0; j<h; ++j) {
					for (int k=0; k<w; ++k) {
						flip[j*w + w-1-k] = p[j*w+k];
					}
				}
			}

		}
		// don't call clear_img here because could do multiple
		// edits (flips/rotations etc.) and don't want to prompt to save every time
		for (int i=0; i<img->frames; ++i) {
			SDL_DestroyTexture(img->tex[i]);
		}
		free(pix);

		img->pixels = flip_pix;
		img->edited = FLIPPED;

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

void do_mode_change(intptr_t mode)
{
	if (IS_VIEW_RESULTS()) {
		SDL_Log("Multi-mode not (yet) supported when viewing results\n");
		return;
	}

	// mode is an enum that also == the number of images
	if (g->n_imgs != mode && g->files.size >= mode) {
		g->status = REDRAW;
		g->slide_timer =  SDL_GetTicks();

		if (g->n_imgs < mode) {
			SDL_LockMutex(g->mtx);
			g->loading = mode;
			SDL_CondSignal(g->cnd);
			SDL_UnlockMutex(g->mtx);
			//g->n_imgs gets updated in handle_events() once loading finishes
		} else {
			if (mode != MODE1 || !g->img_focus || g->img_focus == &g->img[0]) {
				for (int i=g->n_imgs-1; i>mode-1; --i)
					clear_img(&g->img[i]);
			} else {
				// if mode1 and focus and focus != img[0] have to
				// clear the others and move focused img to img[0]
				for (int i=0; i<g->n_imgs; ++i) {
					if (g->img_focus != &g->img[i]) {
						clear_img(&g->img[i]);
					}
				}
				replace_img(&g->img[0], g->img_focus);
			}

			if (mode == MODE1)
				SET_MODE1_SCR_RECT();
			if (mode == MODE2)
				SET_MODE2_SCR_RECTS();
			else if (mode == MODE4)
				SET_MODE4_SCR_RECTS();
			else if (mode == MODE8)
				SET_MODE8_SCR_RECTS();

			g->n_imgs = mode;
			g->img_focus = NULL;
		}
	}
}

void do_delete(SDL_Event* next)
{
	if (g->generating_thumbs) {
		puts("Can't delete images while generating thumbnails!");
		return;
	}

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
	int buttonid;

	char msgbox_prompt[STRBUF_SZ];

	// only delete in single image mode to avoid confusion and complication
	// and not while loading of course
	if (g->loading || g->n_imgs != 1) {
		// TODO messagebox here saying only support deletion in single mode?
		SDL_Log("Only support deletion in single image mode");
		return;
	}

	char* full_img_path;
	if (!IS_VIEW_RESULTS()) {
		full_img_path = g->files.a[g->img[0].index].path;
	} else {
		full_img_path = g->files.a[g->search_results.a[g->img[0].index]].path;
	}

	snprintf(msgbox_prompt, STRBUF_SZ, "Are you sure you want to delete '%s'?", full_img_path);
	messageboxdata.message = msgbox_prompt;
	SDL_ShowMessageBox(&messageboxdata, &buttonid);
	if (buttonid == 1) {
		if (remove(full_img_path)) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to delete image: %s", strerror(errno));
		} else {
			SDL_Log("Deleted %s\n", full_img_path);

			int files_index = g->img[0].index;

			if (IS_VIEW_RESULTS()) {
				// Have to remove from results and decrement all higher results (this works
				// because results are always found from front to back so later results always have higher
				// g->files indices)
				cvec_erase_i(&g->search_results, g->img[0].index, g->img[0].index);
				for (int i=g->img[0].index; i<g->search_results.size; ++i) {
					g->search_results.a[i]--;
				}

				// get actual index to delete correct location from files and thumbs
				files_index = g->search_results.a[g->img[0].index];
			}

			cvec_erase_file(&g->files, files_index, files_index);

			if (g->thumbs.a) {
				cvec_erase_thumb_state(&g->thumbs, files_index, files_index);
			}

			g->img[0].index--; // since everything shifted left, we need to pre-decrement to not skip an image
			SDL_PushEvent(next);
		}
	}
}

void do_remove(SDL_Event* next)
{
	// TODO allow in multi modes?
	if (g->loading || g->n_imgs > 1)
		return;

	int files_index = g->img[0].index;

	if (IS_VIEW_RESULTS()) {
		// Have to remove from results and decrement all higher results (this works
		// because results are always found from front to back so later results always have higher
		// g->files indices)
		cvec_erase_i(&g->search_results, g->img[0].index, g->img[0].index);
		for (int i=g->img[0].index; i<g->search_results.size; ++i) {
			g->search_results.a[i]--;
		}

		// get actual index to delete correct location from files and thumbs
		files_index = g->search_results.a[g->img[0].index];
	}

	// TODO should remove in VIEW_RESULTS remove from results only or also files and thumbs?
	cvec_erase_file(&g->files, files_index, files_index);

	if (g->thumbs.a) {
		cvec_erase_thumb_state(&g->thumbs, files_index, files_index);
	}

	g->img[0].index--; // since everything shifted left, we need to pre-decrement to not skip an image
	SDL_PushEvent(next);
}

void do_actual_size()
{
	g->status = REDRAW;
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			adjust_rect(&g->img[i], g->img[i].w, g->img[i].h, SDL_FALSE);
		}
	} else {
		adjust_rect(g->img_focus, g->img_focus->w, g->img_focus->h, SDL_FALSE);
	}
}

// simple way to handle both cases.  Will remove paths when/if I switch to
// some other format for favorites, sqlite maybe?
void read_list(cvector_file* files, cvector_str* paths, FILE* list_file)
{
	char* s;
	char line[STRBUF_SZ] = { 0 };
	int len;
	file f = { 0 }; // 0 out time and size since we don't stat lists
	struct tm* tmp_tm;
	char* sep;

	struct stat file_stat;

	while ((s = fgets(line, STRBUF_SZ, list_file))) {
		// ignore comments in gqview/gthumb collection format useful
		// when combined with findimagedupes collection output
		if (s[0] == '#')
			continue;

		len = strlen(s);
		if (s[len-1] == '\n')
			s[len-1] = 0;
		// handle quoted paths
		if ((s[len-2] == '"' || s[len-2] == '\'') && s[len-2] == s[0]) {
			s[len-2] = 0;
			memmove(s, &s[1], len-2);
		}
		normalize_path(s);

		if (files) {
			if (stat(s, &file_stat)) {
				// assume it's a valid url, it will just skip over if it isn't
				f.path = mystrdup(s);
				f.size = 0;
				f.modified = 0;

				// leave whole url as name so user knows why size and modified are unknown
				f.name = f.path;
				strncpy(f.size_str, "unknown", SIZE_STR_BUF);
				strncpy(f.mod_str, "unknown", MOD_STR_BUF);
				cvec_push_file(&g->files, &f);
			} else if (S_ISDIR(file_stat.st_mode)) {
				// Should I allow directories in a list?  Or make the user
				// do the expansion so the list only has files/urls?
				//
				//// TODO warning not info?
				SDL_Log("Skipping directory found in list, only files and urls allowed.\n%s\n", s);
			} else if(S_ISREG(file_stat.st_mode)) {
				f.path = mystrdup(s);
				f.size = file_stat.st_size;
				f.modified = file_stat.st_mtime;
				
				bytes2str(f.size, f.size_str, SIZE_STR_BUF);
				tmp_tm = localtime(&f.modified);
				strftime(f.mod_str, MOD_STR_BUF, "%F %T", tmp_tm);
				sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
				f.name = (sep) ? sep+1 : f.path;

				cvec_push_file(&g->files, &f);
			}
		}

		if (paths) {
			cvec_push_str(paths, s);
		}
	}
}

#ifndef _WIN32

int cvec_contains_str(cvector_str* list, char* s)
{
	for (int i=0; i<list->size; ++i) {
		if (!strcmp(list->a[i], s)) {
			return 1;
		}
	}
	return 0;
}

// TODO simple cross platform realpath
void do_save()
{
	if (g->loading)
		return;

	char buf[STRBUF_SZ];
	char* prefpath = SDL_GetPrefPath("", "sdl_img");
	snprintf(buf, STRBUF_SZ, "%s/favorites.txt", prefpath);
	SDL_free(prefpath);

	FILE* f = NULL;
	if (!g->favs.size) {
		f = fopen(buf, "r");
		read_list(NULL, &g->favs, f);
		fclose(f);
	}

	SDL_Log("saving to %s\n", buf);
	f = fopen(buf, "a");
	if (g->img_focus) {
		if (cvec_contains_str(&g->favs, g->img_focus->fullpath)) {
			SDL_Log("%s already in favorites\n", g->img_focus->fullpath);
		} else {
			fprintf(f, "%s\n", g->img_focus->fullpath);
			cvec_push_str(&g->favs, g->img_focus->fullpath);
		}
	} else {
		for (int i=0; i<g->n_imgs; ++i) {
			if (cvec_contains_str(&g->favs, g->img[i].fullpath)) {
				SDL_Log("%s already in favorites\n", g->img[i].fullpath);
			} else {
				fprintf(f, "%s\n", g->img[i].fullpath);
				cvec_push_str(&g->favs, g->img[i].fullpath);
			}
		}
	}

	fclose(f);
}
#endif

// There is no easy way to do cross platform visual copy paste.
// SDL lets you do text but to get visual, I'd have to be using something
// like Qt, or start pulling in x11, winapi, etc. and write it myself
// which defeats the purpose of using/preferring single header libraries
// and trying to minimize external dependencies.
int do_copy()
{
	static int show_warning = 1;

	if (g->loading)
		return 0;

	img_state* img = (g->n_imgs == 1) ? &g->img[0] : g->img_focus;
	if (!img)
		return 0;

#ifndef _WIN32
	SDL_SetClipboardText(img->fullpath);
#else
	SDL_SetClipboardText(g->files.a[img->index].path);
#endif

	SDL_MessageBoxButtonData buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "no" }
	};

	SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_WARNING, /* .flags */
		g->win, /* .window */
		"Warning: No Visual Copy Supported", /* .title */
		NULL, /* .message to be set later */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		NULL /* .colorScheme, NULL = system default */
	};
	int buttonid;

	char msgbox_prompt[] =
	"No visual copy supported. The path of the image has been copied to the clipboard.\n"
	"Use ALT + Print Screen, or copy it from your file browser to get a visual copy.\n\n"
	"Show this warning next time?";
	messageboxdata.message = msgbox_prompt;

	if (show_warning) {
		SDL_ShowMessageBox(&messageboxdata, &buttonid);
		show_warning = !!buttonid;
		return !buttonid;  // just means escape could have been hit, not that it did
	}

	return 0;
}

void do_listmode()
{
	if (g->n_imgs != 1) {
		SDL_Log("Do not yet support listmode from multiimage mode");
		return;
	}
	// TODO hmm handle switching directly from thumb to list and vice versa
	g->state = LIST_DFLT;
	g->selection = g->img[0].index;
	g->list_setscroll = SDL_TRUE;
	text[0] = 0;
	text_len = 0;
	g->search_results.size = 0;
	g->status = REDRAW;
	SDL_ShowCursor(SDL_ENABLE);
}

void do_thumbmode()
{
	generate_thumbs(SDL_TRUE);
	g->state = THUMB_DFLT;
	g->thumb_sel = g->img[0].index;
	g->thumb_start_row = g->thumb_sel / g->thumb_cols;
	g->search_results.size = 0;
	g->status = REDRAW;
	// TODO what a mess, need to think about the best way
	// to handle GUI vs mouse in thumb vs normal mode
	// and I definitely want the infobar or a variant of it
	// in visual mode, like with vim show row and image number
	// total rows etc.
	SDL_ShowCursor(SDL_ENABLE);
	g->gui_timer = SDL_GetTicks();
	g->show_gui = SDL_TRUE;
}

void fix_thumb_sel(int dir)
{
	if (g->thumb_sel < 0)
		g->thumb_sel = 0;
	if (g->thumb_sel >= g->files.size)
		g->thumb_sel = g->files.size-1;
	dir = (dir < 0) ? -1 : 1; // don't want to skip
	// This can happen while thumbs are still being generated
	// TODO think about this logic
	while (!g->thumbs.a[g->thumb_sel].tex && g->thumb_sel && g->thumb_sel != g->files.size-1) {
		g->thumb_sel += dir;
	}
}

void do_thumb_rem_del(int do_delete, int invert)
{
	// TODO code for invert, free selection, after I update cvector with cvec_remove* functions
	// equivalent to cvec_erase* except not calling destructors (if any)

	// so code below works for both ON and VISUAL mode
	if (g->state == THUMB_DFLT) {
		g->thumb_sel_end = g->thumb_sel;
	}

	int start = g->thumb_sel, end = g->thumb_sel_end;
	if (g->thumb_sel > g->thumb_sel_end) {
		end = g->thumb_sel;
		start = g->thumb_sel_end;
	}
	if (do_delete) {
		for (int i=start; i<=end; ++i) {
			if (remove(g->files.a[i].path))
				perror("Failed to delete image");
			else
				SDL_Log("Deleted %s\n", g->files.a[i].path);
		}
	}
	cvec_erase_file(&g->files, start, end);
	cvec_erase_thumb_state(&g->thumbs, start, end);

	if (!g->files.size) {
		SDL_Log("You removed all your currently viewed images, exiting\n");
		cleanup(0, 1);
	}

	// TODO maybe set some state variable to trigger a NEXT event
	// if they return to normal mode by ESC rather than hitting
	// enter otherwise the removed/deleted image will still be their
	// currently viewed image until they move (and if they move left
	// they'll actually skip the image to the left since we artificially
	// subtract one so going right will work normally)
	for (int i=0; i<g->n_imgs; ++i) {
		if (g->img[i].index >= start && g->img[i].index <= end) {
			g->img[i].index = start-1;
		}
	}
	g->thumb_sel = start;  // in case it was > _sel_end
	fix_thumb_sel(1);

	// exit Visual mode after x like vim
	g->state = THUMB_DFLT;
}

#include "events.c"

void print_help(char* prog_name, int verbose)
{
	puts("Usage:");
	printf("  %s image_name\n", prog_name);
	printf("  %s -l text_list_of_image_paths/urls\n", prog_name);
	puts("\nOr any combination of those uses, ie:");
	printf("  %s image.jpg -l list1 example.com/image.jpg -l list3 image4.gif\n", prog_name);

	if (verbose) {
		puts("\nApplication Options:");
		puts("  -f, --fullscreen                   Start in fullscreen mode");
		puts("  -s, --slide-show [delay=3]         Start in slideshow mode");
		puts("  -c, --cache ./your_cache_loc       Use specified directory as cache");
		puts("  -v, --version                      Show the version");
		puts("  -h, --help                         Show this help");
	}
}

int main(int argc, char** argv)
{
	char dirpath[STRBUF_SZ] = { 0 };
	char img_name[STRBUF_SZ] = { 0 };
	char fullpath[STRBUF_SZ] = { 0 };
	char cachedir[STRBUF_SZ] = { 0 };
	char thumbdir[STRBUF_SZ] = { 0 };
	char datebuf[200] = { 0 };
	int ticks;
	struct stat file_stat;

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

	if (argc < 2) {
		print_help(argv[0], SDL_FALSE);
		exit(0);
	}

	if (curl_global_init(CURL_GLOBAL_ALL)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize libcurl\n");
		cleanup(1, 0);
	}
	cvec_file(&g->files, 0, 100, free_file, NULL);
	cvec_str(&g->favs, 0, 50);
	// g->thumbs initialized if needed in generate_thumbs()

	// Not currently used
	// char* exepath = SDL_GetBasePath();

	// TODO think of a company/org name
	char* prefpath = SDL_GetPrefPath("", "sdl_img");
	//SDL_Log("%s\n%s\n\n", exepath, prefpath);
	// SDL_free(exepath);

	time_t t;
	struct tm *tmp;
	char* sep;
	t = time(NULL);

	srand(t);

	tmp = localtime(&t);
	strftime(datebuf, sizeof(datebuf), "%F", tmp);
	//strftime(datebuf, sizeof(datebuf), "%Y%m%d", tmp);

	int len = snprintf(cachedir, STRBUF_SZ, "%scache/%s", prefpath, datebuf);
	if (len >= STRBUF_SZ) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "cache path too long\n");
		cleanup(1, 0);
	}
	if (mkdir_p(cachedir, S_IRWXU) && errno != EEXIST) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory: %s\n", strerror(errno));
		cleanup(1, 0);
	}
	g->cachedir = cachedir;

	len = snprintf(thumbdir, STRBUF_SZ, "%sthumbnails", prefpath);
	if (len >= STRBUF_SZ) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "thumbnail path too long\n");
		cleanup(1, 0);
	}
	if (mkdir_p(thumbdir, S_IRWXU) && errno != EEXIST) {
		perror("Failed to make cache directory");
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory: %s\n", strerror(errno));
		cleanup(1, 0);
	}
	g->thumbdir = thumbdir;

	SDL_free(prefpath);


	file f;
	int img_args = 0;
	int given_list = 0;
	int given_dir = 0;
	int recurse = 0;
	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "-l")) {
			if (i+1 == argc) {
				puts("Error missing list file following -l");
				break;
			}
			FILE* file = fopen(argv[++i], "r");
			if (!file) {
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to open %s: %s\n", argv[i], strerror(errno));
				cleanup(1, 0);
			}
			given_list = 1;
			read_list(&g->files, NULL, file);
			fclose(file);
		} else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--slide-show")) {
			int delay;
			if (i+1 == argc) {
				SDL_Log("No delay following -s, defaulting to 3 second delay.");
				delay = 3;
			} else {
				char* end;
				delay = strtol(argv[++i], &end, 10);
				if (delay <= 0 || delay > 10) {
					if (delay == 0 && end == argv[i]) {
						SDL_Log("No time given for -s, defaulting to 3 seconds\n");
						i--;
					} else {
						SDL_Log("Invalid slideshow time given %d (should be 1-10), defaulting to 3 seconds\n", delay);
					}
					delay = 3;
				}
			}
			// have to do this rather than just setting g->slideshow/slide_timer because
			// timer should start *after* first image is displayed
			SDL_Event start_slideshow;
			start_slideshow.type = SDL_KEYUP;
			start_slideshow.key.keysym.scancode = SDL_SCANCODE_CAPSLOCK + delay; // get proper F1-10 code
			SDL_PushEvent(&start_slideshow);
		} else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cache")) {
			if (i+1 == argc) {
				SDL_Log("no cache directory provieded, using default cache location.\n");
			} else {
				if (mkdir_p(argv[++i], S_IRWXU) && errno != EEXIST) {
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory %s: %s\n", argv[i], strerror(errno));
					cleanup(1, 0);
				}
				g->cachedir = argv[i];
			}
		} else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fullscreen")) {
			g->fullscreen = 1;
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			puts(VERSION_STR);
			cleanup(1, 0);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0], SDL_TRUE);
			cleanup(1, 0);
		} else if (!strcmp(argv[i], "-R")) {
			recurse = 1;
		} else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--recursive")) {
			; // TODO
			if (i+1 == argc) {
				SDL_Log("Error missing directory following -r\n");
				break;
			}
			i++;
			if (stat(argv[i], &file_stat) || !S_ISDIR(file_stat.st_mode)) {
				SDL_Log("Bad argument, expected directory following -r: \"%s\", skipping\n", argv[i]);
				continue;
			}
			given_dir = 1;
			len = strlen(argv[i]);
			if (argv[i][len-1] == '/')
				argv[i][len-1] = 0;

			myscandir(argv[i], exts, num_exts, SDL_TRUE);

		} else {
			normalize_path(argv[i]);
			if (stat(argv[i], &file_stat)) {
				// assume it's a valid url, it will just skip over if it isn't
				f.path = mystrdup(argv[i]);
				f.size = 0;
				f.modified = 0;

				// leave name as url so user knows why the other 2 are unknown
				f.name = f.path;
				strncpy(f.size_str, "unknown", SIZE_STR_BUF);
				strncpy(f.mod_str, "unknown", MOD_STR_BUF);

				cvec_push_file(&g->files, &f);
			} else if (S_ISDIR(file_stat.st_mode)) {
				given_dir = 1;
				len = strlen(argv[i]);
				if (argv[i][len-1] == '/')
					argv[i][len-1] = 0;
				myscandir(argv[i], exts, num_exts, recurse);
			} else if(S_ISREG(file_stat.st_mode)) {
				img_args++;
				f.path = mystrdup(argv[i]);
				f.size = file_stat.st_size;
				f.modified = file_stat.st_mtime;
				// TODO list cache members
				
				bytes2str(f.size, f.size_str, SIZE_STR_BUF);
				tmp = localtime(&f.modified);
				strftime(f.mod_str, MOD_STR_BUF, "%F %T", tmp);
				sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
				f.name = (sep) ? sep+1 : f.path;

				cvec_push_file(&g->files, &f);
			}
		}
	}
	if (!g->files.size) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "No images provided, exiting (empty list perhaps?)\n");
		cleanup(1, 0);
	}

	int start_index = 0;

	// if given a single local image, scan all the files in the same directory
	// don't do this if a list and/or directory was given even if they were empty
	if (g->files.size == 1 && img_args == 1 && !given_list && !given_dir) {
		mydirname(g->files.a[0].path, dirpath);
		mybasename(g->files.a[0].path, img_name);

		cvec_pop_file(&g->files, NULL);

		myscandir(dirpath, exts, num_exts, recurse); // allow recurse for base case?

		snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, img_name);

		sort(g->files.a, NULL, g->files.size, filename_cmp_lt);

		SDL_Log("finding current image to update index\n");
		// this is fine because it's only used when given a single image, which then scans
		// only that directory, hence no duplicate filenames are possible
		//
		// in all other cases (list, multiple files/urls, directory(ies) or some
		// combination of those) there is no "starting image", we just sort and
		// start at the beginning of the g->files in those cases
		file* res;
		f.name = img_name;
		res = bsearch(&f, g->files.a, g->files.size, sizeof(file), filename_cmp_lt);
		if (!res) {
			cleanup(0, 1);
		}
		start_index = res - g->files.a;
	} else {
		sort(g->files.a, NULL, g->files.size, filename_cmp_lt);
	}

	SDL_Log("Loaded %lu filenames\n", (unsigned long)g->files.size);

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "start_index = %d\n", start_index);
	setup(start_index);


	int is_a_gif;
	while (1) {
		if (handle_events())
			break;

		is_a_gif = 0;
		ticks = SDL_GetTicks();

		if ((!IS_LIST_MODE() || IS_VIEW_RESULTS()) && g->show_gui && ticks - g->gui_timer > g->gui_delay*1000) {
			SDL_ShowCursor(SDL_DISABLE);
			g->show_gui = 0;
			g->status = REDRAW;
		}

		// TODO testing, naming/organization of showing/hiding GUI vs mouse
		if ((IS_LIST_MODE() && !IS_VIEW_RESULTS()) || g->show_gui || (g->fullscreen && g->fullscreen_gui == ALWAYS)) {
			draw_gui(g->ctx);
			g->status = REDRAW;
		}

		if (IS_THUMB_MODE() && !IS_VIEW_RESULTS()) {
			SDL_SetRenderDrawColor(g->ren, g->bg.r, g->bg.g, g->bg.b, g->bg.a);
			SDL_RenderSetClipRect(g->ren, NULL);
			SDL_RenderClear(g->ren);

			int start = g->thumb_start_row * g->thumb_cols;
			int end = start + g->thumb_cols*g->thumb_rows;
			int w = g->scr_w/(float)g->thumb_cols;
			int h = g->scr_h/(float)g->thumb_rows;
			SDL_Rect r = { 0, 0, w, h };
			for (int i = start; i < end && i<g->files.size; ++i) {
				// We create tex's in sequence and exit if any fail and
				// erase them when it's source image is deleted so
				// we can break rather than continue here
				//
				// EDIT: with bad paths in list we could fail to create
				// a thumb but we also have never removed bad paths/non-images
				// so we have to continue
				if (!g->thumbs.a[i].tex) {
					//break;
					continue;
				}

				// to fill screen use these rather than following 4 lines
				//r.x = ((i-start) % g->thumb_cols) * w;
				//r.y = ((i-start) / g->thumb_cols) * h;

				// scales and centers thumbs appropriately
				r.w = g->thumbs.a[i].w/(float)THUMBSIZE * w;
				r.h = g->thumbs.a[i].h/(float)THUMBSIZE * h;
				r.x = (((i-start) % g->thumb_cols) * w) + (w-r.w)/2;
				r.y = (((i-start) / g->thumb_cols) * h) + (h-r.h)/2;

				SDL_RenderCopy(g->ren, g->thumbs.a[i].tex, NULL, &r);
				if (g->state & THUMB_DFLT) {
					if (i == g->thumb_sel) {
						SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 255);
						// have selection box take up whole screen space, easier to see
						r.x = ((i-start) % g->thumb_cols) * w;
						r.y = ((i-start) / g->thumb_cols) * h;
						r.w = w;
						r.h = h;
						SDL_RenderDrawRect(g->ren, &r);
					}
				} else if (g->state & THUMB_VISUAL) {
					if ((i >= g->thumb_sel && i <= g->thumb_sel_end) ||
					    (i <= g->thumb_sel && i >= g->thumb_sel_end)) {
						// TODO why doesn't setting this in setup work?  Where else is it changed?
						SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND);

						SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 100);
						// have selection box take up whole screen space, easier to see
						r.x = ((i-start) % g->thumb_cols) * w;
						r.y = ((i-start) / g->thumb_cols) * h;
						r.w = w;
						r.h = h;
						SDL_RenderFillRect(g->ren, &r);
					}
				} else if (g->state & THUMB_RESULTS) {
					
					// TODO optimize since results are in order
					for (int k = 0; k<g->search_results.size; ++k) {
						if (g->search_results.a[k] == i) {
							SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND);

							SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 100);
							// have selection box take up whole screen space, easier to see
							r.x = ((i-start) % g->thumb_cols) * w;
							r.y = ((i-start) / g->thumb_cols) * h;
							r.w = w;
							r.h = h;
							SDL_RenderFillRect(g->ren, &r);
							break;
						}
					}
					if (g->thumb_sel == i) {
						SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 255);
						SDL_RenderDrawRect(g->ren, &r);
					}

				}
			}

		} else if (!IS_LIST_MODE() || IS_VIEW_RESULTS()) {
			// make above plain else to do transparently show image beneath list, could work as a preview...
			// normal mode
			for (int i=0; i<g->n_imgs; ++i) {
				if (g->img[i].frames > 1 && !g->img[i].paused) {
					if (ticks - g->img[i].frame_timer >= g->img[i].delay) {
						g->img[i].frame_i = (g->img[i].frame_i + 1) % g->img[i].frames;
						if (g->img[i].frame_i == 0)
							g->img[i].looped = 1;
						g->img[i].frame_timer = ticks; // should be set after present ...
						g->status = REDRAW;
					}
					is_a_gif = 1;
				}
			}

			if (g->show_gui || g->status == REDRAW) {
				// gui drawing changes draw color so have to reset to black every time
				SDL_SetRenderDrawColor(g->ren, g->bg.r, g->bg.g, g->bg.b, g->bg.a);
				SDL_RenderSetClipRect(g->ren, NULL);
				SDL_RenderClear(g->ren);
				for (int i=0; i<g->n_imgs; ++i) {
					SDL_RenderSetClipRect(g->ren, &g->img[i].scr_rect);
					SDL_RenderCopy(g->ren, g->img[i].tex[g->img[i].frame_i], NULL, &g->img[i].disp_rect);
					print_img_state(&g->img[i]);
				}
				SDL_RenderSetClipRect(g->ren, NULL); // reset for gui drawing
			}
		}
		// TODO ?
		if ((IS_LIST_MODE() && !IS_VIEW_RESULTS()) || g->show_gui || (g->fullscreen && g->fullscreen_gui == ALWAYS)) {
			SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
			nk_sdl_render(NULL, nk_false);
			SDL_RenderSetScale(g->ren, 1, 1);
		}
		SDL_RenderPresent(g->ren);


		//"sleep" save CPU cycles/battery especially when not viewing animated gifs
		if (!is_a_gif) { // && !g->loading)
			SDL_Delay(SLEEP_TIME);
		} else {
			SDL_Delay(MIN_GIF_DELAY/2);
		}
	}

	cleanup(0, 1);
	//never get here
	return 0;
}


