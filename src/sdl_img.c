// The MIT License (MIT)
// 
// Copyright (c) 2017-2019 Robert Winkler
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
#define CVEC_ONLY_STR
#define CVEC_ONLY_INT
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
enum { OFF, ON, VISUAL, SEARCH, RESULTS }; // better names?
enum { NOT_EDITED, ROTATED, TO_ROTATE, FLIPPED};
enum { DELAY, ALWAYS, NEVER };
enum { NEXT, PREV, ZOOM_PLUS, ZOOM_MINUS, ROT_LEFT, ROT_RIGHT, FLIP_H, FLIP_V,
       MODE_CHANGE, THUMB_MODE, DELETE_IMG, ACTUAL_SIZE, ROT360, SHUFFLE,
       SORT_NAME, SORT_SIZE, SORT_MODIFIED, NUM_USEREVENTS };

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
	int modified; // time_t is long int but 2038 problem is because it's really 32-bit counter
	int size;     // in bytes (hard to believe it'd be bigger than ~2.1 GB)
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
	int is_dup;

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

	int fullscreen;
	int fill_mode;
	int gui_delay;
	int gui_timer;
	int show_gui;
	int fullscreen_gui;
	int show_infobar;

	int thumb_mode;
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

// has to come after all the enums/macros/struct defs etc. 
#include "gui.c"

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
			printf("Error creating texture: %s\n", SDL_GetError());
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
		printf("path too long\n");
		cleanup(0, 1);
	}
}

void make_thumb_tex(int i, int w, int h, u8* pix)
{
	if (!pix)
		return;

	g->thumbs.a[i].tex = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
	if (!g->thumbs.a[i].tex) {
		printf("Error creating texture: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	if (SDL_UpdateTexture(g->thumbs.a[i].tex, NULL, pix, w*4)) {
		printf("Error updating texture: %s\n", SDL_GetError());
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
			printf("Couldn't load %s for thumbnail generation\n", g->files.a[i].path);
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
		printf("generated thumb %d for %s\n", i, g->files.a[i].path);
	}

	g->generating_thumbs = SDL_FALSE;
	g->thumbs_done = SDL_TRUE;
	puts("Done generating thumbs, exiting thread.");
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
	puts("Starting thread to generate thumbs and create thumb textures...");
	SDL_Thread* thumb_thrd;
	if (!(thumb_thrd = SDL_CreateThread(thumb_thread, "thumb_thrd", (void*)do_load))) {
		puts("couldn't create thumb thread");
	}
	SDL_DetachThread(thumb_thrd);
}

// debug
#if 0
void print_img_state(img_state* img)
{
	printf("{\nimg = %p\n", img);
	printf("pixels = %p\n", img->pixels);
	printf("WxH = %dx%d\n", img->w, img->h);
	printf("index = %d\n", img->index);
	printf("rotdegs = %d\n", img->rotdegs);
	printf("frame_i = %d\ndelay = %d\nframes = %d\nframe_cap = %d\n", img->frame_i, img->delay, img->frames, img->frame_capacity);
	printf("frame_timer = %d\nlooped = %d\n", img->frame_timer, img->looped);

	printf("tex = %p\n", img->tex);
	for (int i=0; i<img->frames; ++i) {
		printf("tex[%d] = %p\n", i, img->tex[i]);
	}

	printf("scr_rect = %d %d %d %d\n", img->scr_rect.x, img->scr_rect.y, img->scr_rect.w, img->scr_rect.h);
	printf("disp_rect = %d %d %d %d\n}\n", img->disp_rect.x, img->disp_rect.y, img->disp_rect.w, img->disp_rect.h);
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

	free(g->files.a[img_idx].path);
	g->files.a[img_idx].path = mystrdup(filename);

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
	printf("loading %s\n", fullpath);
#endif

	img->pixels = stbi_xload(fullpath, &img->w, &img->h, &n, STBI_rgb_alpha, &frames);
	if (!img->pixels) {
		printf("failed to load %s: %s\n", fullpath, stbi_failure_reason());
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

// renamed to not conflict with <dirent.h>'s scandir
// which I could probably use to accomplish the most of this...
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
	file f;

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
		cvec_push_file(&g->files, &f);
	}

	closedir(dir);
	g->loading = 0;
	return 1;
}

int wrap(int z)
{
   int n = g->files.size;
   while (z < 0) z += n;
   while (z >= n) z -= n;
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

		if (load_what == EXIT)
			break;

		//printf("loading %p = %d\n", &g->loading, g->loading);
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
							ret = load_image(g->files.a[last].path, &img[i], SDL_FALSE);
							if (!ret)
								if (curl_image(last))
									ret = load_image(g->files.a[last].path, &img[i], SDL_FALSE);
						} while (!ret);
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						img[i].index = last;
					}
				} else if (load_what == LEFT) {
					last = g->img[0].index;
					for (int i=g->n_imgs-1; i>=0; --i) {
						do {
							last = wrap(last - 1);
							ret = load_image(g->files.a[last].path, &img[i], SDL_FALSE);
							if (!ret)
								if (curl_image(last))
									ret = load_image(g->files.a[last].path, &img[i], SDL_FALSE);
						} while (!ret);
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						img[i].index = last;
					}
				}

				// just set title to upper left image when !img_focus
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[img[0].index].path, title_buf));
			} else {
				tmp = (load_what == RIGHT) ? 1 : -1;
				last = g->img_focus->index;
				do {
					last = wrap(last + tmp);
					ret = load_image(g->files.a[last].path, &img[0], SDL_FALSE);
					if (!ret)
						if (curl_image(last))
							ret = load_image(g->files.a[last].path, &img[0], SDL_FALSE);
				} while (!ret);
				img[0].index = last;
				img[0].scr_rect = g->img_focus->scr_rect;
				set_rect_bestfit(&img[0], g->fullscreen | g->slideshow | g->fill_mode);
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[img[0].index].path, title_buf));
			}
		} else {
			last = g->img[g->n_imgs-1].index;
			for (int i=g->n_imgs; i<load_what; ++i) {
				do {
					last = wrap(last + 1);
					ret = load_image(g->files.a[last].path, &g->img[i], SDL_FALSE);
					if (!ret)
						if (curl_image(last))
							ret = load_image(g->files.a[last].path, &g->img[i], SDL_FALSE);
				} while (!ret);
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
		puts(error_str);
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

	if (!(g->img[0].tex = malloc(100*sizeof(SDL_Texture*)))) {
		perror("Couldn't allocate tex array");
		cleanup(0, 1);
	}
	g->img[0].frame_capacity = 100;

	g->img[0].index = start_idx;
	char* img_name = g->files.a[start_idx].path;
	// TODO best way to structure this and use in main()?
	int ret = load_image(img_name, &g->img[0], SDL_FALSE);
	if (!ret) {
		if (curl_image(0)) {
			ret = load_image(g->files.a[0].path, &g->img[0], SDL_FALSE);
			img_name = g->files.a[0].path;
		}
	}

	if (!ret) {
		cleanup(0, 1);
	}

	SDL_Rect r;
	if (SDL_GetDisplayUsableBounds(0, &r)) {
		printf("Error getting usable bounds: %s\n", SDL_GetError());
		r.w = START_WIDTH;
		r.h = START_HEIGHT;
	}
	g->scr_w = MAX(g->img[0].w, START_WIDTH);
	g->scr_h = MAX(g->img[0].h, START_HEIGHT);
	g->scr_w = MIN(g->scr_w, r.w);
	g->scr_h = MIN(g->scr_h, r.h-40); // UsableBounds doesn't account for bottom panel in Mate :-/

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

	if (!g->ren) {
		puts(error_str);
	}

	float hdpi, vdpi, ddpi;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	printf("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);

	// could adjust for dpi, then adjust for font size if necessary
	g->x_scale = 2; //hdpi/72;
	g->y_scale = 2; //vdpi/72;

	if (!(g->ctx = nk_sdl_init(g->win, g->ren, g->x_scale, g->y_scale))) {
		puts("nk_sdl_init() failed!");
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
		printf("Error: %s", SDL_GetError());
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
		printf("Error: %s", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->mtx = SDL_CreateMutex())) {
		printf("Error: %s", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* loading_thrd;
	if (!(loading_thrd = SDL_CreateThread(load_new_images, "loading_thrd", NULL))) {
		puts("couldn't create image loader thread");
		cleanup(0, 1);
	}
	SDL_DetachThread(loading_thrd);

	printf("Done with setup\nStarting with %s\n", img_name);

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
			break;
		}
	}
}

void do_sort(compare_func cmp)
{
	if (g->n_imgs != 1 || g->generating_thumbs) {
		return;
	}

	char* save = g->files.a[g->img[0].index].path;

	// g->thumbs.a is either NULL or valid
	sort(g->files.a, g->thumbs.a, g->files.size, cmp);

	for (int i=0; i<g->files.size; ++i) {
		if (!strcmp(save, g->files.a[i].path)) {
			g->img[0].index = i;
			g->thumb_sel = i;
			break;
		}
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
		return;
	}

	char* full_img_path = g->files.a[g->img[0].index].path;

	snprintf(msgbox_prompt, STRBUF_SZ, "Are you sure you want to delete '%s'?", full_img_path);
	messageboxdata.message = msgbox_prompt;
	SDL_ShowMessageBox(&messageboxdata, &buttonid);
	if (buttonid == 1) {
		if (remove(full_img_path)) {
			perror("Failed to delete image");
		} else {
			printf("Deleted %s\n", full_img_path);
			cvec_erase_file(&g->files, g->img[0].index, g->img[0].index);

			if (g->thumbs.a) {
				cvec_erase_thumb_state(&g->thumbs, g->img[0].index, g->img[0].index);
			}
			g->img[0].index--; // since everything shifted left, we need to pre-decrement to not skip an image
			SDL_PushEvent(next);
		}
	}
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

#ifndef _WIN32
// TODO simple cross platform realpath
void do_save()
{
	if (g->loading)
		return;

	char buf[STRBUF_SZ];
	char* prefpath = SDL_GetPrefPath("", "sdl_img");
	snprintf(buf, STRBUF_SZ, "%s/favorites.txt", prefpath);
	SDL_free(prefpath);

	printf("saving to %s\n", buf);
	FILE* f = fopen(buf, "a");
	if (g->img_focus) {
		fprintf(f, "%s\n", g->img_focus->fullpath);
	} else {
		for (int i=0; i<g->n_imgs; ++i) {
			fprintf(f, "%s\n", g->img[i].fullpath);
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

void do_thumbmode()
{
	generate_thumbs(SDL_TRUE);
	g->thumb_mode = ON;
	g->thumb_sel = g->img[0].index;
	g->thumb_start_row = g->thumb_sel / g->thumb_cols;
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

int handle_thumb_events()
{
	SDL_Event e;
	int sym;
	SDL_Keymod mod_state = SDL_GetModState();
	int mouse_x, mouse_y;
	u32 mouse_button_mask = SDL_GetMouseState(&mouse_x, &mouse_y);
	char title_buf[STRBUF_SZ];

	// TODO add page or half page jumps (CTRL+U/D) and maybe / vim search
	// mode?

	g->status = NOCHANGE;
	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			//nk_input_end(g->ctx); // TODO need these?
			return 1;
		case SDL_KEYUP:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_ESCAPE:
				// TODO merge with T, remove T?
				// Also need to regenerate DISP_RECT(s) for normal mode
				// if window has changed size since switching to THUMB ...
				// or keep it updated in SIZE_CHANGED below
				if (g->thumb_mode >= VISUAL) {
					g->thumb_mode = ON;
				} else {
					g->thumb_mode = OFF;
					g->thumb_start_row = 0; // TODO?
					g->show_gui = SDL_TRUE;
				}
				g->status = REDRAW;
				break;
			case SDLK_c:
				// turn of VISUAL (or any other mode I add later)
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->thumb_mode = ON;
				}
				break;
			case SDLK_SLASH:
				g->thumb_mode = SEARCH;
				text[0] = 0;
				text_len = 0;
				g->search_results.size = 0;
				SDL_StartTextInput();
				break;
			case SDLK_v:
				if (g->thumb_mode == ON) {
					g->thumb_mode = VISUAL;
					g->thumb_sel_end = g->thumb_sel;
				} else if (g->thumb_mode == VISUAL) {
					g->thumb_mode = ON;
				}
				g->status = REDRAW;
				break;
			case SDLK_g:
				if (g->thumb_mode != SEARCH) {
					if (mod_state & (KMOD_LSHIFT | KMOD_RSHIFT)) {
						g->thumb_start_row = g->files.size-1; // will get fixed at the bottom
						g->thumb_sel = g->files.size-1;
					} else {
						g->thumb_start_row = 0;;
						g->thumb_sel = 0;
					}
				}
				break;
			case SDLK_x:
				if (g->thumb_mode == ON) {
					// TODO warning? message prompt?  Maybe one time with a preference
					// to not show again?
					//
					// TODO refactor to combine normal and visual mode x
					if (remove(g->files.a[g->thumb_sel].path)) {
						perror("Failed to delete image");
					} else {
						printf("Deleted %s\n", g->files.a[g->thumb_sel].path);
						cvec_erase_file(&g->files, g->thumb_sel, g->thumb_sel);
						cvec_erase_thumb_state(&g->thumbs, g->thumb_sel, g->thumb_sel);
						if (!g->files.size) {
							puts("You deleted all your currently viewed images, exiting");
							cleanup(0, 1);
						}
						for (int i=0; i<g->n_imgs; ++i) {
							if (g->img[i].index == g->thumb_sel) {
								g->img[i].index--;
								break;
							}
						}
						fix_thumb_sel(1);
					}
				} else if (g->thumb_mode == VISUAL) {
					int start = g->thumb_sel, end = g->thumb_sel_end;
					if (g->thumb_sel > g->thumb_sel_end) {
						end = g->thumb_sel;
						start = g->thumb_sel_end;
					}
					for (int i=start; i<=end; ++i) {
						if (remove(g->files.a[i].path))
							perror("Failed to delete image");
						else
							printf("Deleted %s\n", g->files.a[i].path);
					}
					cvec_erase_file(&g->files, start, end);
					cvec_erase_thumb_state(&g->thumbs, start, end);

					if (!g->files.size) {
						puts("You deleted all your currently viewed images, exiting");
						cleanup(0, 1);
					}

					for (int i=0; i<g->n_imgs; ++i) {
						if (g->img[i].index >= start && g->img[i].index <= end) {
							g->img[i].index = start-1;
						}
					}
					g->thumb_sel = start;  // in case it was > _sel_end
					fix_thumb_sel(1);

					// exit Visual mode after x like vim
					g->thumb_mode = ON;
				}
				break;
			case SDLK_RETURN:
				if (g->thumb_mode == ON || g->thumb_mode == RESULTS) {
					// subtract 1 since we reuse RIGHT loading code
					g->selection = (g->thumb_sel) ? g->thumb_sel - 1 : g->files.size-1;
					g->thumb_mode = OFF;
					g->thumb_start_row = 0;
					g->show_gui = SDL_TRUE;
					g->status = REDRAW;
					try_move(SELECTION);
				} else if (g->thumb_mode == SEARCH) {
					SDL_StopTextInput();
					printf("Final text = \"%s\"\n", text);
					//text[0] = 0;
					for (int i=0; i<g->files.size; ++i) {
						// GNU function...
						if (strcasestr(g->files.a[i].path, text)) {
							printf("Adding %s\n", g->files.a[i].path);
							cvec_push_i(&g->search_results, i);
						}
					}
					printf("found %d matches\n", (int)g->search_results.size);
					if (g->search_results.size) {
						g->thumb_sel = g->search_results.a[0];
						g->thumb_mode = RESULTS;
					} else {
						g->thumb_mode = ON;
					}
				}
				break;
			}
			break;
		case SDL_KEYDOWN:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_UP:
			case SDLK_DOWN:
			case SDLK_k:
			case SDLK_j:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					// should down increase or decrease?  I say increase to match right = increase
					g->thumb_rows += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
					if (g->thumb_rows < 2)
						g->thumb_rows = 2;
					if (g->thumb_rows > 8)
						g->thumb_rows = 8;
				} else {
					if (g->thumb_mode != SEARCH) {
						g->thumb_sel += (sym == SDLK_DOWN || sym == SDLK_j) ? g->thumb_cols : -g->thumb_cols;
						fix_thumb_sel((sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1);
						SDL_ShowCursor(SDL_ENABLE);
						g->gui_timer = SDL_GetTicks();
						g->show_gui = 1;
					}
				}
				break;
			case SDLK_LEFT:
			case SDLK_RIGHT:
			case SDLK_h:
			case SDLK_l:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					g->thumb_cols += (sym == SDLK_LEFT || sym == SDLK_h) ? -1 : 1;
					if (g->thumb_cols < 4)
						g->thumb_cols = 4;
					if (g->thumb_cols > 15)
						g->thumb_cols = 15;
				} else {
					if (g->thumb_mode != SEARCH) {
						g->thumb_sel += (sym == SDLK_h || sym == SDLK_LEFT) ? -1 : 1;
						fix_thumb_sel((sym == SDLK_h || sym == SDLK_LEFT) ? -1 : 1);
						SDL_ShowCursor(SDL_ENABLE);
						g->gui_timer = SDL_GetTicks();
						g->show_gui = 1;
					}
				}
				break;
			case SDLK_n:
				if (g->thumb_mode == RESULTS) {
					if (mod_state & (KMOD_LSHIFT | KMOD_RSHIFT)) {
						if (g->thumb_sel == g->search_results.a[g->cur_result]) {
							g->cur_result--;
							if (g->cur_result < 0)
								g->cur_result += g->search_results.size;
						} else {
							// TODO if move to closest result in negative direction
							int i;
							for (i = 0; i<g->search_results.size; ++i) {
								if (g->search_results.a[i] > g->thumb_sel)
									break;
							}
							if (!i)
								g->cur_result = g->search_results.size-1;
							else
								g->cur_result = i-1;
						}
					} else {
						if (g->thumb_sel == g->search_results.a[g->cur_result]) {
							g->cur_result = (g->cur_result + 1) % g->search_results.size;
						} else {
							// TODO if move to closest result in positive direction
							int i;
							for (i = 0; i<g->search_results.size; ++i) {
								if (g->search_results.a[i] > g->thumb_sel)
									break;
							}
							if (i == g->search_results.size)
								g->cur_result = 0;
							else
								g->cur_result = i;
						}
					}
					g->thumb_sel = g->search_results.a[g->cur_result];
					SDL_ShowCursor(SDL_ENABLE);
					g->gui_timer = SDL_GetTicks();
					g->show_gui = 1;
				}
				break;
			case SDLK_BACKSPACE:
				if (text_len)
					text[--text_len] = 0;
				printf("text is \"%s\"\n", text);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			SDL_ShowCursor(SDL_ENABLE);
			g->gui_timer = SDL_GetTicks();
			g->show_gui = 1;
			break;
		case SDL_MOUSEBUTTONUP:
			// TODO have this behavior in VISUAL MODE too?  Single click changes
			// g->thumb_sel?
			if (e.button.button == SDL_BUTTON_LEFT) {
				g->selection = g->thumb_start_row * g->thumb_cols +
				               (mouse_y / (g->scr_h/g->thumb_rows)) * g->thumb_cols +
				               (mouse_x / (g->scr_w/g->thumb_cols));

				// TODO better way to avoid duplication?
				if (g->selection >= g->files.size)
					break;
				if (e.button.clicks == 2) {
					// since we reuse the RIGHT loading code, have to subtract 1 so we
					// "move right" to the selection
					g->selection = (g->selection) ? g->selection - 1 : g->files.size-1;
					g->thumb_mode = OFF;
					g->show_gui = SDL_TRUE;
					g->thumb_start_row = 0;
					g->status = REDRAW;
					try_move(SELECTION);
				} else {
					// TODO is there anything besides clicks == 1 or 2?
					g->thumb_sel = g->selection;
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			g->status = REDRAW;
			if (g->thumb_mode == ON) {
				if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
					g->thumb_sel -= e.wheel.y * g->thumb_cols;
					fix_thumb_sel(-e.wheel.y);
				} else {
					g->thumb_sel += e.wheel.y * g->thumb_cols;
					fix_thumb_sel(e.wheel.y);
				}
				SDL_ShowCursor(SDL_ENABLE);
				g->gui_timer = SDL_GetTicks();
				g->show_gui = 1;
			}
			break;

		case SDL_TEXTINPUT:
			// could probably just do text[text_len++] = e.text.text[0]
			// since I only handle ascii
			if (g->thumb_mode == SEARCH && text_len < STRBUF_SZ-1) {
				strcat(text, e.text.text);
				text_len += strlen(e.text.text);
				printf("text is \"%s\" \"%s\" %d %d\n", text, composition, cursor, selection_len);
			}
			break;

		case SDL_TEXTEDITING:
			if (g->thumb_mode == SEARCH) {
				printf("recieved edit \"%s\"\n", e.edit.text);
				composition = e.edit.text;
				cursor = e.edit.start;
				selection_len = e.edit.length;
			}
			break;

		case SDL_WINDOWEVENT: {
			g->status = REDRAW;
			int x, y;
			SDL_GetWindowSize(g->win, &x, &y);
			//printf("windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;
				break;
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
			}
		} break;

		default: // all other event types
			break;
		}

		// TODO leave it here where it calls for every event
		// or put it back in mouse and key events?
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	if (g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols >= g->files.size+g->thumb_cols)
		g->thumb_start_row = (g->files.size / g->thumb_cols - g->thumb_rows+1);
	if (g->thumb_start_row < 0)
		g->thumb_start_row = 0;

	// can happen while thumbs are being generated/loaded
	if (!g->thumbs.a[g->thumb_sel].tex) {
		if (!g->thumb_sel) {
			for (; !g->thumbs.a[g->thumb_sel].tex && g->thumb_sel<g->files.size-1; ++g->thumb_sel);
		} else {
			for (; !g->thumbs.a[g->thumb_sel].tex && g->thumb_sel>0; --g->thumb_sel);
		}
		// No valid thumbs found, turn off visual
		// TODO also prevent thumbmode in the first place if there are no valid thumbs?
		if (!g->thumbs.a[g->thumb_sel].tex) {
			g->thumb_mode = ON;
		}
	}

	if (g->thumb_sel < g->thumb_start_row*g->thumb_cols) {
		g->thumb_start_row = g->thumb_sel / g->thumb_cols;
	} else if (g->thumb_sel >= g->thumb_start_row*g->thumb_cols + g->thumb_rows*g->thumb_cols) {
		g->thumb_start_row = g->thumb_sel / g->thumb_cols - g->thumb_rows + 1;
	}
		
	if (g->thumb_mode) {
		SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->thumb_sel].path, title_buf));
	} else if (g->img_focus) {
		SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
	} else {
		SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index].path, title_buf));
	}

	return 0;

}

int handle_events_normally()
{
	SDL_Event e;
	int sc;
	int zoomed;
	char title_buf[STRBUF_SZ];
	img_state* img;

	// eat all escapes this frame after copy dialog ended with "no"
	int copy_escape = 0;

	g->status = NOCHANGE;

	SDL_Keymod mod_state = SDL_GetModState();

	// use space to move to next image(s) even if zoomed in, ie during slideshow
	SDL_Event space;
	space.type = SDL_KEYDOWN;
	space.key.keysym.scancode = SDL_SCANCODE_SPACE;

	int ticks = SDL_GetTicks();

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

	if (g->slideshow) {
		// pause slideshow if popup is up
		if (g->show_about || g->show_prefs) {
			g->slide_timer = ticks;
		} else if (!g->loading && ticks - g->slide_timer > g->slideshow) {
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
	}

	int mouse_x, mouse_y;
	u32 mouse_button_mask = SDL_GetMouseState(&mouse_x, &mouse_y);
	
	int done_rotate = 0;
	int code;
	nk_input_begin(g->ctx);
	while (SDL_PollEvent(&e)) {
		if (e.type == g->userevent) {
			// reset this everytime they interact with GUI
			// so it doesn't disappear even if they're holding
			// the mouse down but still (on zoom controls for example)
			g->gui_timer = SDL_GetTicks();

			code = e.user.code;
			switch (code) {
			case NEXT:
			case PREV:
				try_move(code == NEXT ? RIGHT : LEFT);
				break;
			case ZOOM_PLUS:
			case ZOOM_MINUS:
				do_zoom(code == ZOOM_PLUS ? GUI_ZOOM : -GUI_ZOOM, SDL_FALSE);
				break;
			case ROT_LEFT:
			case ROT_RIGHT:
				do_rotate(code == ROT_LEFT, SDL_TRUE);
				break;
			case FLIP_H:
			case FLIP_V:
				do_flip(code == FLIP_V);
				break;
			case ROT360:
				// TODO
				rotate_img((g->n_imgs == 1) ? &g->img[0] : g->img_focus);
				break;
			case THUMB_MODE:
				do_thumbmode();
				break;
			case MODE_CHANGE:
				g->status = REDRAW;
				g->slide_timer =  SDL_GetTicks();
				do_mode_change((intptr_t)e.user.data1);
				break;
			case ACTUAL_SIZE:
				do_actual_size();
				break;
			case SHUFFLE:
				do_shuffle();
				break;
			case SORT_NAME:
				do_sort(filepath_cmp);
				break;
			case SORT_SIZE:
				do_sort(filesize_cmp);
				break;
			case SORT_MODIFIED:
				do_sort(filemodified_cmp);
				break;
			case DELETE_IMG:
				do_delete(&space);
				break;
			default:
				puts("Unknown user event!");
			}
			continue;
		}

		switch (e.type) {
		case SDL_QUIT:
			//nk_input_end(g->ctx); // TODO need these?
			return 1;
		case SDL_KEYUP:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				if (!copy_escape && !g->fullscreen && !g->slideshow && !g->show_about && !g->show_prefs && !g->show_rotate) {
					//nk_input_end(g->ctx);
					return 1;
				} else {
					if (g->show_rotate) {
						// TODO handle case where user hits ESC with image as
						// TO_ROTATE (could still be pristine or they rotated with preview, changed the angle
						// again and then hit ESC, only want to prompt to save in the latter case)
						g->show_rotate = nk_false;
					} else if (g->show_about) {
						g->show_about = nk_false;
					} else if (g->show_prefs) {
						g->show_prefs = nk_false;
					}else if (g->slideshow) {
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
				do_delete(&space);
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
				g->fullscreen = !g->fullscreen;
				set_fullscreen();
				break;

			case SDL_SCANCODE_0:
				g->img_focus = NULL;
				SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img[0].index].path, title_buf));
				break;
			case SDL_SCANCODE_1:
				if (!g->loading && mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					do_mode_change(MODE1);
				} else if (g->n_imgs >= 2) {
					g->img_focus = &g->img[0];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_2:
				if (!g->loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					do_mode_change(MODE2);
				} else if (g->n_imgs >= 2) {
					g->img_focus = &g->img[1];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_3:
				g->status = REDRAW;
				if (g->n_imgs >= 3) {
					g->img_focus = &g->img[2];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_4:
				if (!g->loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					do_mode_change(MODE4);
				} else if (g->n_imgs >= 4) {
					g->img_focus = &g->img[3];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_5:
				g->status = REDRAW;
				if (g->n_imgs >= 5) {
					g->img_focus = &g->img[4];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_6:
				g->status = REDRAW;
				if (g->n_imgs >= 6) {
					g->img_focus = &g->img[5];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_7:
				g->status = REDRAW;
				if (g->n_imgs >= 7) {
					g->img_focus = &g->img[6];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;
			case SDL_SCANCODE_8:
				if (!g->loading && (mod_state & (KMOD_LCTRL | KMOD_RCTRL))) {
					do_mode_change(MODE8);
				} else if (g->n_imgs >= 8) {
					g->img_focus = &g->img[7];
					SDL_SetWindowTitle(g->win, mybasename(g->files.a[g->img_focus->index].path, title_buf));
				}
				break;

			case SDL_SCANCODE_A:
				do_actual_size();
				break;

			case SDL_SCANCODE_M:
				do_shuffle();
				break;

			case SDL_SCANCODE_N:
				do_sort(filepath_cmp);
				break;
			case SDL_SCANCODE_Z:
				do_sort(filesize_cmp);
				break;
			case SDL_SCANCODE_T:
				do_sort(filemodified_cmp);
				break;

			case SDL_SCANCODE_P:
				// doesn't matter if we "pause" static images
				if (!g->img_focus) {
					for (int i=0; i<g->n_imgs; ++i) {
						g->img[i].paused = !g->img[i].paused;
					}
				} else {
					g->img_focus->paused = !g->img_focus->paused;
				}
				break;

			case SDL_SCANCODE_U:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					do_thumbmode();
				} else {
					// TODO GUI for this?
					generate_thumbs(SDL_FALSE);
				}
				break;

			case SDL_SCANCODE_C:
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					// TODO maybe just flush events here and return 0 so
					// no input for the current frame after CTRL+V? can I do
					// that without breaking the GUI?
					copy_escape = do_copy();
				}
			break;

#ifndef _WIN32
			case SDL_SCANCODE_S:
				do_save();
			break;
#endif

			case SDL_SCANCODE_H:
			case SDL_SCANCODE_V:
				do_flip(sc == SDL_SCANCODE_V);
			break;

			case SDL_SCANCODE_L:
			case SDL_SCANCODE_R:
				if (!done_rotate) {
					if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
						do_rotate(sc == SDL_SCANCODE_L, SDL_FALSE);
					} else {
						do_rotate(sc == SDL_SCANCODE_L, SDL_TRUE);
					}
					done_rotate = 1;
				}
				break;


			case SDL_SCANCODE_F: {
				g->status = REDRAW;
				if (mod_state & (KMOD_LALT | KMOD_RALT)) {
					g->fullscreen = !g->fullscreen;
					set_fullscreen();
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
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					try_move(LEFT);
				} else {
					try_move(RIGHT);
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
				if (!zoomed) {
					try_move(RIGHT);
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
				if (!zoomed) {
					try_move(RIGHT);
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
				if (!zoomed) {
					try_move(LEFT);
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
				if (!zoomed) {
					try_move(LEFT);
				}
				break;

			case SDL_SCANCODE_MINUS:
				g->status = REDRAW;
				if (!(mod_state & (KMOD_LALT | KMOD_RALT))) {
					do_zoom(-KEY_ZOOM, SDL_FALSE);
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
					do_zoom(KEY_ZOOM, SDL_FALSE);
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
			if (mouse_button_mask & SDL_BUTTON(SDL_BUTTON_LEFT)) {
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
			}
			g->status = REDRAW;

			SDL_ShowCursor(SDL_ENABLE);
			g->gui_timer = SDL_GetTicks();
			g->show_gui = 1;
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			g->status = REDRAW;
			SDL_ShowCursor(SDL_ENABLE);
			g->gui_timer = SDL_GetTicks();
			g->show_gui = 1;
			break;

		case SDL_MOUSEWHEEL:
			g->status = REDRAW;
			if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
				do_zoom(e.wheel.y*SCROLL_ZOOM, SDL_TRUE);
			} else {
				do_zoom(-e.wheel.y*SCROLL_ZOOM, SDL_TRUE);
			}
			break;

		case SDL_WINDOWEVENT: {
			g->status = REDRAW;
			int x, y;
			SDL_GetWindowSize(g->win, &x, &y);
			//printf("windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;
				break;
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
			case SDL_WINDOWEVENT_EXPOSED:
				//puts("exposed");
				//printf("windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
				//puts("exposed event");
				break;
			}
		} break; // end WINDOWEVENTS

		default: // all other event types
			break;
		}

		// TODO leave it here where it calls for every event
		// or put it back in mouse and key events?
		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return 0;
}

int handle_events()
{
	if (!g->thumb_mode)
		return handle_events_normally();
	else
		return handle_thumb_events();
}

//stupid windows
void normalize_path(char* path)
{
	for (int i=0; path[i]; ++i) {
		if (path[i] == '\\')
			path[i] = '/';
	}
}

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

void read_list(cvector_file* images, FILE* list_file)
{
	char* s;
	char line[STRBUF_SZ] = { 0 };
	int len;
	file f = { 0 }; // 0 out time and size since we don't stat lists

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
		f.path = mystrdup(s);
		cvec_push_file(images, &f);
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
		puts("Failed to initialize libcurl");
		cleanup(1, 0);
	}
	cvec_file(&g->files, 0, 100, free_file, NULL);
	// g->thumbs initialized if needed in generate_thumbs()

	// Not currently used
	// char* exepath = SDL_GetBasePath();

	// TODO think of a company/org name
	char* prefpath = SDL_GetPrefPath("", "sdl_img");
	//printf("%s\n%s\n\n", exepath, prefpath);
	// SDL_free(exepath);

	time_t t;
	struct tm *tmp;
	t = time(NULL);

	srand(t);

	tmp = localtime(&t);
	strftime(datebuf, sizeof(datebuf), "%F", tmp);
	//strftime(datebuf, sizeof(datebuf), "%Y%m%d", tmp);

	int len = snprintf(cachedir, STRBUF_SZ, "%scache/%s", prefpath, datebuf);
	if (len >= STRBUF_SZ) {
		puts("cache path too long");
		cleanup(1, 0);
	}
	if (mkdir_p(cachedir, S_IRWXU) && errno != EEXIST) {
		perror("Failed to make cache directory");
		cleanup(1, 0);
	}
	g->cachedir = cachedir;

	len = snprintf(thumbdir, STRBUF_SZ, "%sthumbnails", prefpath);
	if (len >= STRBUF_SZ) {
		puts("thumbnail path too long");
		cleanup(1, 0);
	}
	if (mkdir_p(thumbdir, S_IRWXU) && errno != EEXIST) {
		perror("Failed to make cache directory");
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
				// TODO print filename
				perror("fopen");
				cleanup(1, 0);
			}
			given_list = 1;
			read_list(&g->files, file);
			fclose(file);
		} else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--slide-show")) {
			int delay;
			if (i+1 == argc) {
				puts("No delay following -s, defaulting to 3 second delay.");
				delay = 3;
			} else {
				char* end;
				delay = strtol(argv[++i], &end, 10);
				if (delay <= 0 || delay > 10) {
					if (delay == 0 && end == argv[i]) {
						puts("No time given for -s, defaulting to 3 seconds");
						i--;
					} else {
						printf("Invalid slideshow time given %d (should be 1-10), defaulting to 3 seconds\n", delay);
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
				puts("no cache directory provieded, using default cache location.");
			} else {
				if (mkdir_p(argv[++i], S_IRWXU) && errno != EEXIST) {
					perror("Failed to make cache directory");
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
				puts("Error missing directory following -r");
				break;
			}
			i++;
			if (stat(argv[i], &file_stat) || !S_ISDIR(file_stat.st_mode)) {
				printf("Bad argument, expected directory following -r: \"%s\", skipping\n", argv[i]);
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
				printf("Bad argument: \"%s\", skipping\n", argv[i]);
				continue;
			}
			if (S_ISDIR(file_stat.st_mode)) {
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
				cvec_push_file(&g->files, &f);
			} else {
				f.path = mystrdup(argv[i]);
				f.size = 0;
				f.modified = 0;
				cvec_push_file(&g->files, &f);
			}
		}
	}
	if (!g->files.size) {
		puts("No images provided, exiting (empty list perhaps?)");
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

		sort(g->files.a, NULL, g->files.size, filepath_cmp);

		printf("finding current image to update index\n");
		file* res;
		f.path = fullpath;
		res = bsearch(&f, g->files.a, g->files.size, sizeof(file), filepath_cmp);
		if (!res) {
			cleanup(0, 1);
		}
		start_index = res - g->files.a;
	} else {
		sort(g->files.a, NULL, g->files.size, filepath_cmp);
	}

	printf("Loaded %lu filenames\n", (unsigned long)g->files.size);

	printf("start_index = %d\n", start_index);
	setup(start_index);


	int is_a_gif;
	while (1) {
		if (handle_events())
			break;

		is_a_gif = 0;
		ticks = SDL_GetTicks();

		if (g->show_gui && ticks - g->gui_timer > g->gui_delay*1000) {
			SDL_ShowCursor(SDL_DISABLE);
			g->show_gui = 0;
			g->status = REDRAW;
		}

		// TODO testing, naming/organization of showing/hiding GUI vs mouse
		if (g->show_gui || (g->fullscreen && g->fullscreen_gui == ALWAYS)) {
			draw_gui(g->ctx);
			g->status = REDRAW;
		}

		if (!g->thumb_mode) {
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
		} else {
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
				if (g->thumb_mode == ON) {
					if (i == g->thumb_sel) {
						SDL_SetRenderDrawColor(g->ren, 0, 255, 0, 255);
						// have selection box take up whole screen space, easier to see
						r.x = ((i-start) % g->thumb_cols) * w;
						r.y = ((i-start) / g->thumb_cols) * h;
						r.w = w;
						r.h = h;
						SDL_RenderDrawRect(g->ren, &r);
					}
				} else if (g->thumb_mode == VISUAL) {
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
				} else if (g->thumb_mode == RESULTS) {
					
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
		}
		if (g->show_gui || (g->fullscreen && g->fullscreen_gui == ALWAYS)) {
			SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
			nk_sdl_render(NULL, nk_false);
			SDL_RenderSetScale(g->ren, 1, 1);
		}
		SDL_RenderPresent(g->ren);


		//"sleep" save CPU cycles/battery especially when not viewing animated gifs
		if (!is_a_gif) // && !g->loading)
			SDL_Delay(SLEEP_TIME);
		else
			SDL_Delay(MIN_GIF_DELAY/2);
	}

	cleanup(0, 1);
	//never get here
	return 0;
}


