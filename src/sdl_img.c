// The MIT License (MIT)
// 
// Copyright (c) 2017-2024 Robert Winkler
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

#include "myinttypes.h"

#define CVECTOR_IMPLEMENTATION
#define CVEC_ONLY_INT
#define CVEC_ONLY_STR
#define CVEC_SIZE_T i64
#define PRIcv_sz PRIiMAX
#include "cvector.h"

#include "WjCryptLib_Md5.c"

// for TCC
//#define STBIR_NO_SIMD

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

// for TCC
//#define STBI_NO_SIMD
//
//#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
//#define NK_IMPLEMENTATION
#include "nuklear.h"

#define SDL_MAIN_HANDLED
//#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear_sdl_renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

//POSIX (mostly) works with MinGW64
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <curl/curl.h>

enum { QUIT, REDRAW, NOCHANGE };
enum { NOTHING = 0, MODE1 = 1, MODE2 = 2, MODE4 = 4, MODE8 = 8, LEFT, RIGHT, SELECTION, EXIT };
enum { NOT_EDITED, ROTATED, TO_ROTATE, FLIPPED};
enum { DELAY, ALWAYS, NEVER };
enum { NONE, NAME_UP, NAME_DOWN, PATH_UP, PATH_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };
enum { NEXT, PREV, ZOOM_PLUS, ZOOM_MINUS, ROT_LEFT, ROT_RIGHT, FLIP_H, FLIP_V,
       MODE_CHANGE, THUMB_MODE, LIST_MODE, DELETE_IMG, ACTUAL_SIZE, ROT360, REMOVE_BAD,
       SHUFFLE, SORT_NAME, SORT_PATH, SORT_SIZE, SORT_MODIFIED, PROCESS_SELECTION,
       NUM_USEREVENTS };

// return values for handle_selection(), says what the arg was
enum { URL, DIRECTORY, IMAGE };

// Better names/macros
enum {
	NORMAL           = 0x1,
	THUMB_DFLT       = 0x2,
	THUMB_VISUAL     = 0x4,
	THUMB_SEARCH     = 0x8,
	LIST_DFLT        = 0x10,
	SEARCH_RESULTS   = 0x20,
	FILE_SELECTION   = 0x40,
	SCANNING         = 0x80,
};

#define THUMB_MASK (THUMB_DFLT | THUMB_VISUAL | THUMB_SEARCH)
#define LIST_MASK (LIST_DFLT)
#define RESULT_MASK (SEARCH_RESULTS)
//#define VIEW_MASK (NORMAL)

#define IS_THUMB_MODE() (g->state & THUMB_MASK)
#define IS_LIST_MODE() (g->state & LIST_MASK)
#define IS_RESULTS() (g->state & RESULT_MASK)
#define IS_VIEW_RESULTS() (g->state & NORMAL && g->state != NORMAL)
#define IS_FS_MODE() (g->state == FILE_SELECTION)
#define IS_SCANNING_MODE() (g->state == SCANNING)

#ifdef _WIN32
#define mkdir(A, B) mkdir(A)
#endif

#define VERSION 1.0
#define VERSION_STR "sdl_img 1.0-RC2"

#define PATH_SEPARATOR '/'
#define PAN_RATE 0.05
#define MAX_GIF_FPS 100
#define MIN_GIF_DELAY (1000/MAX_GIF_FPS)
#define DFLT_GIF_FPS 20
#define DFLT_GIF_DELAY (1000/DFLT_GIF_FPS)
#define DFLT_GUI_DELAY 2
#define DFLT_BUTTON_REPEAT_DELAY 1.0f
#define SLEEP_TIME 50
#define STRBUF_SZ 1024
#define START_WIDTH 1200
#define START_HEIGHT 800
#define THUMBSIZE 128
#define MIN_THUMB_ROWS 2
#define MIN_THUMB_COLS 4
#define MAX_THUMB_ROWS 8
#define MAX_THUMB_COLS 15
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24
#define FONT_SIZE 24
#define NUM_DFLT_EXTS 11

// If this is defined, sdl_img will add files without extensions
// to the list in directory scans if stbi_info() returns true.
// This can make the startup a bit slower if you are scanning a
// large directory (possibly recursively) with many files without
// extensions since stbi_info actually has to open those files
// to determine if they are a valid supported image type
#define CHECK_IF_NO_EXTENSION

// If defined, all log output goes to log.txt in the
// same directory as config.lua
//#define USE_LOGFILE

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
#define PINCH_ZOOM 3
#define GIF_ZOOM_DIV 3

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

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

typedef struct thumb_state
{
	int w;
	int h;
	SDL_Texture* tex;
} thumb_state;

#define RESIZE(x) ((x+1)*2)

CVEC_NEW_DECLS2(thumb_state)

CVEC_NEW_DEFS2(thumb_state, RESIZE)

// maybe I should just include the former in the latter...
#include "string_compare.c"
#include "file.c"

#define FILE_TYPE_STR "Images"
#define FB_LOG(A, ...) SDL_Log(A, __VA_ARGS__)
#include "filebrowser.c"

typedef struct img_state
{
	u8* pixels;
	int w;
	int h;
	int file_size;
	char* fullpath;  // just points to file.path, doesn't own

	int index;

	nk_size frame_i;
	u16* delays;
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

	int cfg_cachedir;
	char* cachedir;
	char* thumbdir;
	char* prefpath;

	cvector_file files;
	cvector_str favs;

	cvector_str sources;  // files/directories to scan etc.
	int done_scanning;

	// for file selection
	file_browser filebrowser;

	const char** img_exts;
	int n_exts;

	int has_bad_paths;

	int state; // better name?

	// flag to do load returning from thumb mode
	int do_next;

	// whether you're hovering over GIF progress bar
	int progress_hovered;

	int fullscreen;
	int fill_mode;
	int gui_delay;
	float button_rpt_delay;
	int gui_timer;
	int show_gui;
	int thumb_x_deletes;
	int ind_mm;         // independent multimode, better name?
	int fullscreen_gui;
	int show_infobar;

	int list_setscroll;

	int thumbs_done;
	int thumbs_loaded;
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
	SDL_cond* img_loading_cnd;
	SDL_mutex* img_loading_mtx;

	SDL_cond* scanning_cnd;
	SDL_mutex* scanning_mtx;

	// debugging
	FILE* logfile;

} global_state;

// Use a pointer in case I ever move this to another TU, though it's unlikely
// Also I know initializing a global to 0 is redundant but meh
static global_state state = { 0 };
global_state* g = &state;

char text_buf[STRBUF_SZ];
int text_len;
char* composition;
Sint32 cursor;
Sint32 selection_len;

// has to come after all the enums/macros/struct defs and bytes2str 
#include "gui.c"

#include "sorting.c"

#include "lua_config.c"

void log_output_func(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
	static const char *priority_prefixes[] = {
    	NULL,
    	"VERBOSE",
    	"DEBUG",
    	"INFO",
    	"WARN",
    	"ERROR",
    	"CRITICAL"
	};

	FILE* logfile = userdata;
    fprintf(logfile, "%s: %s\n", priority_prefixes[priority], message);
    fflush(logfile);
}

size_t write_data(void* buf, size_t size, size_t num, void* userp)
{
	return fwrite(buf, 1, size*num, (FILE*)userp);
}

//need to think about best way to organize following 4 functions' functionality
void adjust_rect(img_state* img, int w, int h, int use_mouse)
{
	assert(w > 0 && h > 0);
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

	float w_aspect = img->scr_rect.w/aspect;
	int tmp = MIN(img->scr_rect.h, w_aspect);
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
		SDL_SetTextureBlendMode(img->tex[i], SDL_BLENDMODE_BLEND);
		if (!img->tex[i]) {
			SDL_Log("Error creating texture: %s\n", SDL_GetError());
			return 0;
		}
		if (SDL_UpdateTexture(img->tex[i], NULL, img->pixels+size*i, img->w*4)) {
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

		char* full_img_path;
		if (!IS_VIEW_RESULTS()) {
			full_img_path = g->files.a[img->index].path;
		} else {
			full_img_path = g->files.a[g->search_results.a[img->index]].path;
		}

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
	free(img->delays); // NULL or valid

	img->fullpath = NULL; // now just points to file.path so don't free
	img->pixels = NULL;
	img->delays = NULL;
	img->frames = 0;
	img->rotdegs = 0;
	img->edited = NOT_EDITED;
	img->file_size = 0;
}

void cleanup(int ret, int called_setup)
{
	char buf[STRBUF_SZ] = { 0 };

	if (g->logfile) {
		SDL_Log("In cleanup()");
		fclose(g->logfile);
	}
	if (called_setup) {
		// appends prefpath inside
		write_config_file("config.lua");
		write_config(stdout);

		// not really necessary to exit thread but
		// valgrind reports it as possibly lost if not
		SDL_LockMutex(g->img_loading_mtx);
		g->loading = EXIT;
		SDL_CondSignal(g->img_loading_cnd);
		SDL_UnlockMutex(g->img_loading_mtx);

		for (int i=0; i<g->n_imgs; ++i) {
			clear_img(&g->img[i]);
			free(g->img[i].tex);
		}

		SDL_DestroyRenderer(g->ren);
		SDL_DestroyWindow(g->win);
		SDL_Quit();

		if (g->favs.size) {
			snprintf(buf, STRBUF_SZ, "%sfavorites.txt", g->prefpath);
			FILE* f = fopen(buf, "w");
			if (!f) {
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create %s: %s\nAborting save\n", buf, strerror(errno));
			} else {
				for (int i=0; i<g->favs.size; i++) {
					fprintf(f, "%s\n", g->favs.a[i]);
				}
				fclose(f);
			}
		}
	}

	free(g->prefpath);
	cvec_free_thumb_state(&g->thumbs);
	cvec_free_file(&g->files);
	cvec_free_str(&g->favs);
	curl_global_cleanup();
	exit(ret);
}

void remove_bad_paths()
{
	if (!g->has_bad_paths) {
		if (!g->thumbs_done) {
			SDL_Log("No bad paths to remove, have you generated thumbnails to check all images?\n");
		} else {
			SDL_Log("No bad paths!\n");
		}
		return;
	}

	if (g->generating_thumbs) {
		SDL_Log("You're already generating thumbs, wait for it to finish\n");
		return;
	}
	// NOTE For now no need for flag/mutex here that prevents
	// thumb thread starting because it only starts from the
	// main thread, ie the one busy here

	char* cur_paths[8];
	for (int i=0; i<g->n_imgs; i++) {
		cur_paths[i] = g->files.a[g->img[i].index].path;
	}

	int j;
	// bad paths were already set to NULL, and thumb_state array was
	// calloc'd so tex also NULL, so can remove rather than erase
	for (int i=0; i<g->files.size; i++) {
		if (!g->files.a[i].path) {
			for (j=i+1; j < g->files.size && !g->files.a[j].path; j++);

			SDL_Log("Removing [%d %d]\n", i, j-1);
			cvec_remove_file(&g->files, i, j-1);
			if (g->thumbs.a)
				cvec_remove_thumb_state(&g->thumbs, i, j-1);
		}
	}

	// inefficient brute force, meh
	// TODO use bsearch if list is currently sorted
	int n = g->n_imgs;
	for (int i=0; i<n; i++) {
		for (int j=0; j<g->files.size; j++) {
			if (!strcmp(cur_paths[i], g->files.a[j].path)) {
				g->img[i].index = j;
				break;
			}
		}
	}
	g->has_bad_paths = SDL_FALSE;
	SDL_Log("Done removing bad paths\n");
}

char* curl_image(int img_idx)
{
	CURL* curl = curl_easy_init();
	CURLcode res;
	char filename[STRBUF_SZ];
	char curlerror[CURL_ERROR_SIZE];
	char* s = g->files.a[img_idx].path;
	FILE* imgfile;

	// Do I even need to set WRITEFUNCTION?  It says it'll use fwrite by default
	// which is all I do...
	//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
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
		perror("fopen");

		goto exit_cleanup;
	}

	curl_easy_setopt(curl, CURLOPT_URL, s);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, imgfile);
	// follow redirect
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	res = curl_easy_perform(curl);
	long http_code = 0;
	char* content_type = NULL;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
	if (res != CURLE_OK || http_code != 200 ||
		!content_type || strncmp(content_type, "image", 5)) {
		SDL_Log("curlcode: %d '%s'\nhttp_code: %ld", res, curlerror, http_code);
		if (!content_type) {
			SDL_Log("No content-type returned, ignoring\n");
		} else {
			SDL_Log("Not an image: %s\n", content_type);
		}
		fclose(imgfile);
		remove(filename);
		goto exit_cleanup;
	}
	fclose(imgfile);


	struct stat file_stat;
	stat(filename, &file_stat);
	// TODO don't think I need this any more, could also use
	// CURLINFO_CONTENT_LENGTH_DOWNLOAD_T
	/*
	if (!file_stat.st_size) {
		SDL_Log("file size is 0\n");
		remove(filename);
		goto exit_cleanup;
	} else {
		SDL_Log("file size is %ld\n", file_stat.st_size);
	}
	*/

	file* f = &g->files.a[img_idx];
	free(f->path);

	// Have to call realpath in case the user passed
	// a relative path cachedir as a command line argument
#ifndef _WIN32
	f->path = realpath(filename, NULL);
#else
	f->path = CVEC_STRDUP(filename);
#endif
	f->size = file_stat.st_size;
	f->modified = file_stat.st_mtime;

	bytes2str(f->size, f->size_str, SIZE_STR_BUF);
	struct tm* tmp_tm = localtime(&f->modified);
	strftime(f->mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
	char* sep = strrchr(f->path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
	f->name = (sep) ? sep+1 : f->path;


	curl_easy_cleanup(curl);
	return f->path;

exit_cleanup:
	curl_easy_cleanup(curl);
	return NULL;
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

int gen_thumbs(void* data)
{
	int w, h, channels;
	int out_w, out_h;
	char thumbpath[STRBUF_SZ] = { 0 };

	struct stat thumb_stat, orig_stat;

	intptr_t do_load = (intptr_t)data;

	int start = SDL_GetTicks();
	u8* pix;
	u8* outpix;
	for (int i=0; i<g->files.size; ++i) {
		if (!g->files.a[i].path) {
			continue;
		}

		if (stat(g->files.a[i].path, &orig_stat)) {
			// TODO threading issue if user is trying
			// to load i at the same time, both will try
			// to download it
			SDL_Log("Couldn't stat %d %s\n", i, g->files.a[i].path);
			if (!curl_image(i)) {
				SDL_Log("Couldn't curl %d\n", i);
				free(g->files.a[i].path);
				g->files.a[i].path = NULL;
				g->files.a[i].name = NULL;
				g->has_bad_paths = SDL_TRUE;
				continue;
			}
		}

		// path was already set to realpath in myscandir if using linux
		// Windows will just generate duplicate thumbnails when accessing the
		// same image from a different relative path
		get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));

		if (!stat(thumbpath, &thumb_stat)) {
			// make sure original hasn't been modified since thumb was made
			// don't think it's necessary to check nanoseconds
			if (orig_stat.st_mtime < thumb_stat.st_mtime) {
				if (do_load) {
					outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
					make_thumb_tex(i, w, h, outpix);
					free(outpix);
				}
				continue;
			}
		}

		pix = stbi_load(g->files.a[i].path, &w, &h, &channels, 4);
		if (!pix) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s for thumbnail generation\nError %s", g->files.a[i].path, stbi_failure_reason());
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

		if (!stbir_resize_uint8_linear(pix, w, h, 0, outpix, out_w, out_h, 0, STBIR_RGBA)) {
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
	g->thumbs_loaded = do_load;

	if (do_load) {
		// same as in load_thumbs, generating and loading can take even longer
		g->thumb_sel = g->img[0].index;
		g->thumb_sel_end = g->img[0].index;
		g->thumb_start_row = g->thumb_sel / g->thumb_cols;
	}


	SDL_Log("Done generating thumbs in %.2f seconds, exiting thread.\n", (SDL_GetTicks()-start)/1000.0f);
	return 0;
}

void free_thumb(void* t)
{
	SDL_DestroyTexture(t);
}

void generate_thumbs(intptr_t do_load)
{
	if (g->thumbs.a)
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
	SDL_Log("Starting thread to generate thumbs\n");
	if (do_load) {
		SDL_Log("Will load them as well\n");
	}
	SDL_Thread* thumb_thrd;
	if (!(thumb_thrd = SDL_CreateThread(gen_thumbs, "gen_thumbs_thrd", (void*)do_load))) {
		// TODO warning?
		SDL_Log("couldn't create thumb thread\n");
	}
	// passing NULL is a no-op like free
	SDL_DetachThread(thumb_thrd);
}

int load_thumbs(void* data)
{
	int w, h, channels;
	u8* outpix;
	char thumbpath[STRBUF_SZ] = { 0 };
	int start = SDL_GetTicks();

	for (int i=0; i<g->thumbs.size; i++) {
		if (!g->files.a[i].path) {
			continue;
		}
		get_thumbpath(g->files.a[i].path, thumbpath, sizeof(thumbpath));

		outpix = stbi_load(thumbpath, &w, &h, &channels, 4);
		make_thumb_tex(i, w, h, outpix);
		free(outpix);
	}

	// make sure we are on current image after we're done loading
	// since loading can take a while if there are 1000's of images
	g->thumb_sel = g->img[0].index;
	g->thumb_sel_end = g->img[0].index;
	g->thumb_start_row = g->thumb_sel / g->thumb_cols;

	g->thumbs_loaded = SDL_TRUE;
	SDL_Log("Done loading thumbs in %.2f seconds, exiting thread.\n", (SDL_GetTicks()-start)/1000.0f);
	return 0;
}

// debug
#if 1
void print_img_state(img_state* img)
{
	puts("here");
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "{\nimg = %p\n", img);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "pixels = %p\n", img->pixels);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "WxH = %dx%d\n", img->w, img->h);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "index = %d\n", img->index);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "rotdegs = %d\n", img->rotdegs);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "frame_i = %ld\ndelay = %d\nframes = %d\nframe_cap = %d\n", img->frame_i, (img->delays) ? img->delays[0] : 0, img->frames, img->frame_capacity);
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "frame_timer = %d\nlooped = %d\n", img->frame_timer, img->looped);

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "tex = %p\n", img->tex);
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

int load_image(const char* fullpath, img_state* img, int make_textures)
{
	int frames, n;
	u16* delays = NULL;

	// img->frames should always be 0 and there should be no allocated textures
	// in tex because clear_img(img) should always have been called before

	SDL_Log("loading %s\n", fullpath);
	img->pixels = stbi_xload(fullpath, &img->w, &img->h, &n, STBI_rgb_alpha, &frames, &delays);
	if (!img->pixels) {
		SDL_Log("failed to load %s: %s\n", fullpath, stbi_failure_reason());
		return 0;
	}

	struct stat file_stat = { 0 };
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

	//gif delays is in 100ths, ticks are 1000ths, but newer stb_image converts for us
	//and if delay is 0, default to DFLT_GIF_FPS fps
	img->looped = 1;
	img->paused = 0;
	if (frames > 1) {
		img->looped = 0;
		for (int i=0; i<frames; i++) {
			if (!delays[i]) {
				delays[i] = DFLT_GIF_DELAY;
			}
			delays[i] = MAX(MIN_GIF_DELAY, delays[i]);
		}
		SDL_Log("%d frames, delays[0] = %d\n", frames, delays[0]);
	}

	img->delays = delays; // should be NULL or valid
	img->frames = frames;
	img->frame_i = 0;

	if (make_textures) {
		if (!create_textures(img))
			return 0;
	}

	// just a reference not a deep copy
	img->fullpath = (char*)fullpath;

	return 1;
}

#define GET_EXT(s) strrchr(s, '.')


// renamed to not conflict with <dirent.h>'s scandir
// which I could probably use to accomplish  most of this...
int myscandir(const char* dirpath, const char** exts, int num_exts, int recurse)
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

	int start_size = g->files.size;


	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1, 1);
	}

	char* tmp;
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

		ext = GET_EXT(entry->d_name);

#ifndef CHECK_IF_NO_EXTENSION
		// only add supported extensions
		if (!ext)
			continue;
#else
		if (ext)
#endif
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

#ifdef CHECK_IF_NO_EXTENSION
		if (!ext && !stbi_info(f.path, &x, &y, &n)) {
			free(f.path);
			continue;
		}
#endif

		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;

		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(&g->files, &f);
	}

	SDL_Log("Found %"PRIcv_sz" images in %s\n", g->files.size-start_size, dirpath);

	closedir(dir);
	g->loading = 0;
	return 1;
}

int wrap(int z)
{
	assert(g->files.size);

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
	int i = last;
	if (IS_VIEW_RESULTS()) {
		i = g->search_results.a[last];
	}

	path = g->files.a[i].path;
	int ret = 0;
	if (path) {
		if (!(ret = load_image(path, img, SDL_FALSE))) {
			if ((path = curl_image(last))) {
				ret = load_image(path, img, SDL_FALSE);
			}
		}
		if (!ret) {
			free(g->files.a[i].path);
			g->files.a[i].path = NULL;
			g->files.a[i].name = NULL;
			g->has_bad_paths = SDL_TRUE;
		}
	}
	return ret;
}

int load_new_images(void* data)
{
	int tmp;
	int load_what;
	int last;

	while (1) {
		puts("top of load_new_images");
		SDL_LockMutex(g->img_loading_mtx);
		puts("locked img_loading_mtx");
		while (g->loading < 2) {
			SDL_CondWait(g->img_loading_cnd, g->img_loading_mtx);
			printf("loading thread woke with load: %d\n", g->loading);
		}
		load_what = g->loading;
		SDL_UnlockMutex(g->img_loading_mtx);

		if (load_what == EXIT)
			break;

		printf("loading thread received a load: %d\n", load_what);
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
						if (g->ind_mm && load_what != SELECTION) {
							last = g->img[i].index;
						}
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
						if (g->ind_mm) {
							last = g->img[i].index;
						}
						do {
							last = wrap(last - 1);
						} while (!attempt_image_load(last, &img[i]));
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						img[i].index = last;
					}
				}

				// just set title to upper left image when !img_focus
				// TODO use file.name for all of these
				int index = (IS_VIEW_RESULTS()) ? g->search_results.a[img[0].index] : img[0].index;
				SDL_SetWindowTitle(g->win, g->files.a[index].name);
			} else {
				tmp = (load_what >= RIGHT) ? 1 : -1;
				last = (load_what != SELECTION) ? g->img_focus->index : g->selection;
				do {
					last = wrap(last + tmp);
				} while (!attempt_image_load(last, &img[0]));
				img[0].index = last;
				img[0].scr_rect = g->img_focus->scr_rect;
				set_rect_bestfit(&img[0], g->fullscreen | g->slideshow | g->fill_mode);

				int index = (IS_VIEW_RESULTS()) ? g->search_results.a[img[0].index] : img[0].index;
				SDL_SetWindowTitle(g->win, g->files.a[index].name);
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

		SDL_LockMutex(g->img_loading_mtx);
		g->done_loading = load_what;
		g->loading = 0;
		SDL_UnlockMutex(g->img_loading_mtx);
	}

	return 0;
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
		if (len < 2)
			continue;

		if (s[len-1] == '\n') {
			len--;
			s[len] = 0;
		}

		if (len < 2)
			continue;

		// TODO why did I do len-2 instead of len-1?
		// handle quoted paths
		if ((s[len-2] == '"' || s[len-2] == '\'') && s[len-2] == s[0]) {
			s[len-2] = 0;
			memmove(s, &s[1], len-2);
			if (len < 4) continue;
		}
		normalize_path(s);

		if (files) {
			if (stat(s, &file_stat)) {
				// assume it's a valid url, it will just skip over if it isn't
				f.path = CVEC_STRDUP(s);
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
				f.path = CVEC_STRDUP(s);
				f.size = file_stat.st_size;
				f.modified = file_stat.st_mtime;

				bytes2str(f.size, f.size_str, SIZE_STR_BUF);
				tmp_tm = localtime(&f.modified);
				strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
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

int handle_selection(char* path, int recurse)
{
	file f;
	struct stat file_stat;
	int len;
	int ret = 0;

	if (stat(path, &file_stat)) {
		// assume it's a valid url, it will just skip over if it isn't
		f.path = CVEC_STRDUP(path);
		f.size = 0;
		f.modified = 0;

		// leave name as url so user knows why the other 2 are unknown
		f.name = f.path;
		strncpy(f.size_str, "unknown", SIZE_STR_BUF);
		strncpy(f.mod_str, "unknown", MOD_STR_BUF);

		cvec_push_file(&g->files, &f);

		ret = URL;
	} else if (S_ISDIR(file_stat.st_mode)) {
		len = strlen(path);
		if (path[len-1] == '/')
			path[len-1] = 0;
		myscandir(path, g->img_exts, g->n_exts, recurse);

		ret = DIRECTORY;
	} else if(S_ISREG(file_stat.st_mode)) {
		f.path = CVEC_STRDUP(path);
		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;
		// TODO list cache members

		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		struct tm* tmp = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp); // %F %T
		char* sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;

		cvec_push_file(&g->files, &f);
		ret = IMAGE;
	}

	return ret;
}

/*
void setup_initial_image(int start_idx)
{
	// TODO Handle clearing out images if opening a fresh set
	for (int i=0; i<g->n_imgs; ++i) {
		clear_img(&g->img[i]);
		//free(g->img[i].tex);
	}
	g->n_imgs = 1;
	g->img_focus = NULL;
	g->state = NORMAL;

	// handle when first image (say in a list that's out of date) is gone/invalid
	// loop through till valid
	i64 last = start_idx;
	int ret;
	img_state* img = &g->img[0];
	img->index = last;
	do {
		ret = attempt_image_load(last, img);
		last = wrap(last+1);
	} while (!ret && last != start_idx);

	if (!ret) {
		g->state = FILE_SELECTION;
		cleanup(0, 1);
	}

	img->index = wrap(last-1);
	char* img_name = g->files.a[img->index].name;

	//g->scr_w = MAX(g->img[0].w, START_WIDTH);
	//g->scr_h = MAX(g->img[0].h, START_HEIGHT);

	// can't create textures till after we have a renderer (otherwise we'd pass SDL_TRUE)
	// to load_image above
	if (!create_textures(&g->img[0])) {
		cleanup(1, 1);
	}

	SET_MODE1_SCR_RECT();
	SDL_RenderClear(g->ren);
	SDL_RenderCopy(g->ren, g->img[0].tex[g->img[0].frame_i], NULL, &g->img[0].disp_rect);
	SDL_RenderPresent(g->ren);

	SDL_SetWindowTitle(g->win, img_name);

	SDL_Log("Starting with %s\n", img_name);
}
*/

int scan_sources(void* data)
{
	char dirpath[STRBUF_SZ] = { 0 };
	char img_name[STRBUF_SZ] = { 0 };
	char fullpath[STRBUF_SZ] = { 0 };

	int given_list = 0;
	int given_dir = 0;
	int recurse = 0;
	int img_args = 0;
	int start_index = 0;
	file f;


	cvector_str* srcs = &g->sources;
	while (1) {
		SDL_LockMutex(g->scanning_mtx);
		while (!srcs->size) {
			SDL_CondWait(g->scanning_cnd, g->scanning_mtx);
		}
		// anything here?
		SDL_UnlockMutex(g->scanning_mtx);

		given_list = 0;
		given_dir = 0;
		recurse = 0;
		img_args = 0;

		printf("scanning files.size = %ld", g->files.size);

		char** a = srcs->a;
		for (int i=0; i<srcs->size; ++i) {
			printf("Scanning source: %s\n", a[i]);
			if (!strcmp(a[i], "-l")) {

				// sanity check extension
				char* ext = GET_EXT(a[i+1]);
				if (ext) {
					for (int j=0; j<g->n_exts; ++j) {
						if (!strcasecmp(ext, g->img_exts[j])) {
							SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Trying to load a list with a recognized image extension (%s): %s\n", ext, a[++i]);
							// TODO
							cleanup(1, 0);
						}
					}
				}

				FILE* file = fopen(a[++i], "r");
				if (!file) {
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to open %s: %s\n", a[i], strerror(errno));
					cleanup(1, 0);
				}
				given_list = 1;
				read_list(&g->files, NULL, file);
				fclose(file);
			} else if (!strcmp(a[i], "-R")) {
				recurse = 1;
			} else if (!strcmp(a[i], "-r") || !strcmp(a[i], "--recursive")) {
				myscandir(a[++i], g->img_exts, g->n_exts, SDL_TRUE);
			} else {
				int r = handle_selection(a[i], recurse);
				given_dir = r == DIRECTORY;
				img_args += r == IMAGE;
			}
		}

		cvec_clear_str(srcs);


		// if given a single local image, scan all the files in the same directory
		// don't do this if a list and/or directory was given even if they were empty
		if (g->files.size == 1 && img_args == 1 && !given_list && !given_dir) {
			mydirname(g->files.a[0].path, dirpath);
			mybasename(g->files.a[0].path, img_name);

			// popm to not free the string and keep the file in case
			// the start image is not added in the scan
			cvec_popm_file(&g->files, &f);

			myscandir(dirpath, g->img_exts, g->n_exts, recurse); // allow recurse for base case?

			SDL_Log("Found %"PRIcv_sz" images total\nSorting by file name now...\n", g->files.size);

			snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, img_name);

			mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);

			SDL_Log("finding current image to update index\n");
			// this is fine because it's only used when given a single image, which then scans
			// only that directory, hence no duplicate filenames are possible
			//
			// in all other cases (list, multiple files/urls, directory(ies) or some
			// combination of those) there is no "starting image", we just sort and
			// start at the beginning of the g->files in those cases
			file* res;
			res = bsearch(&f, g->files.a, g->files.size, sizeof(file), filename_cmp_lt);
			if (!res) {
				SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not find starting image '%s' when scanning containing directory\n", img_name);
				SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "This means it did not have a searched-for extension; adding to list and attempting load anyway\n");
				int i;
				for (i=0; i<g->files.size; i++) {
					if (filename_cmp_lt(&f, &g->files.a[i]) <= 0) {
						cvec_insert_file(&g->files, i, &f);
						res = &g->files.a[i];
						break;
					}
				}
				if (i == g->files.size) {
					cvec_push_file(&g->files, &f);
					res = &g->files.a[i];
				}
			} else {
				// no longer need this, it was found in the scan
				free(f.path);
			}
			// I could change all indexes to i64 but but no one will
			// ever open over 2^31-1 images so just explicitly convert
			// from ptrdiff_t (i64) to int here and use ints everywhere
			int start_index =(int)(res - g->files.a);
			//setup_initial_image(start_index);
			g->selection = (start_index) ? start_index-1 : g->files.size-1;
			try_move(SELECTION);
		} else if (g->files.size) {
			SDL_Log("Found %"PRIcv_sz" images total\nSorting by file name now...\n", g->files.size);
			printf("%s\n", g->files.a[0].path);
			mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);
			g->selection = g->files.size-1;
			try_move(SELECTION);
		} else {
			SDL_Log("Found 0 images, switching to File Browser...");
		}


		SDL_LockMutex(g->scanning_mtx);
		puts("done scanning = 1");
		g->done_scanning = 1;
		SDL_UnlockMutex(g->scanning_mtx);
	}

	return 0;
}

void setup_dirs()
{
	char datebuf[200] = { 0 };
	time_t t;
	struct tm *tmp;
	int len;

	t = time(NULL);
	srand(t);

	char* prefpath = g->prefpath;

	// cachedir wasn't passed as an argument or loaded from config.lua
	if (!g->cachedir[0]) {
		tmp = localtime(&t);
		strftime(datebuf, sizeof(datebuf), "%Y-%m-%d", tmp);  // %F

		len = snprintf(g->cachedir, STRBUF_SZ, "%scache/%s", prefpath, datebuf);
		if (len >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "cache path too long\n");
			cleanup(1, 1);
		}
	}
	if (mkdir_p(g->cachedir, S_IRWXU) && errno != EEXIST) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory: %s\n", strerror(errno));
		cleanup(1, 1);
	}

	// if thumbdir was not set from config file
	if (!g->thumbdir[0]) {
		len = snprintf(g->thumbdir, STRBUF_SZ, "%sthumbnails", prefpath);
		if (len >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "thumbnail path too long\n");
			cleanup(1, 1);
		}
	}
	if (mkdir_p(g->thumbdir, S_IRWXU) && errno != EEXIST) {
		perror("Failed to make cache directory");
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to make cache directory: %s\n", strerror(errno));
		cleanup(1, 1);
	}

	printf("cache: %s\nthumbnails: %s\n", g->cachedir, g->thumbdir);
}

int load_config()
{
	char config_path[STRBUF_SZ] = { 0 };
	snprintf(config_path, STRBUF_SZ, "%sconfig.lua", g->prefpath);
	SDL_Log("config file: %s\n", config_path);

	// If no config file, set default preferences
	// NOTE cachedir already set to default in main
	if (!read_config_file(config_path)) {
		g->slide_delay = 3;
		g->gui_delay = DFLT_GUI_DELAY;
		g->button_rpt_delay = DFLT_BUTTON_REPEAT_DELAY;
		g->show_infobar = nk_true;
		g->fullscreen_gui = DELAY;
		g->thumb_x_deletes = nk_false;
		g->ind_mm = nk_false;
		g->bg = nk_rgb(0,0,0);
		g->thumb_rows = MAX_THUMB_ROWS;
		g->thumb_cols = MAX_THUMB_COLS;

		return nk_false;
	}
	SDL_Log("Successfully loaded config file\n");

	return nk_true;
}

void setup()
{
	static char cachedir[STRBUF_SZ] = { 0 };
	static char thumbdir[STRBUF_SZ] = { 0 };

	char error_str[STRBUF_SZ] = { 0 };
	char title_buf[STRBUF_SZ] = { 0 };
	char buf[STRBUF_SZ] = { 0 };
	char* img_name = NULL;

	static const char* default_exts[NUM_DFLT_EXTS] =
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

	g->img_exts = default_exts;
	g->n_exts = NUM_DFLT_EXTS;

#ifndef NDEBUG
	puts("does this work");
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
#endif

	// already NULL from static initialization
	g->win = NULL;
	g->ren = NULL;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, STRBUF_SZ, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, g->win);
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s\n", error_str);
		exit(1);
	}

	// Not currently used
	// char* exepath = SDL_GetBasePath();

	// TODO think of a company/org name
	g->prefpath = SDL_GetPrefPath("", "sdl_img");
	//SDL_Log("%s\n%s\n\n", exepath, g->prefpath);
	// SDL_free(exepath);

	// have to set these before load_config
	// just point these at a buffer that will live forever
	g->cachedir = cachedir;
	g->thumbdir = thumbdir;

	int got_config = load_config();

	if (curl_global_init(CURL_GLOBAL_ALL)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize libcurl\n");
		cleanup(1, 0);
	}
	cvec_file(&g->files, 0, 100, free_file, NULL);
	cvec_str(&g->favs, 0, 50);
	// g->thumbs initialized if needed in generate_thumbs()



#ifdef USE_LOGFILE
	char log_path[STRBUF_SZ];
	snprintf(log_path, sizeof(log_path), "%slog.txt", g->prefpath);
	g->logfile = fopen(log_path, "w");
	SDL_LogSetOutputFunction(log_output_func, g->logfile);
#endif


	// TODO some of these could be stored preferences?
	g->n_imgs = 1;
	g->img = g->img1;
	g->do_next = nk_false;
	g->progress_hovered = nk_false;
	g->fill_mode = 0;
	g->sorted_state = NAME_UP;  // ie by name ascending
	//g->state = FILE_SELECTION;
	g->state = SCANNING;
	g->has_bad_paths = SDL_FALSE;

	setup_dirs();

	// read favorites
	snprintf(buf, STRBUF_SZ, "%sfavorites.txt", g->prefpath);
	FILE* f = NULL;
	if (!(f = fopen(buf, "r"))) {
		SDL_Log("%s does not exist, will try creating it on exit\n", buf);
	} else {
		read_list(NULL, &g->favs, f);
		SDL_Log("Read %ld favorites from %s\n", g->favs.size, buf);
		fclose(f);
	}

	SDL_Rect r;
	if (SDL_GetDisplayUsableBounds(0, &r)) {
		SDL_Log("Error getting usable bounds: %s\n", SDL_GetError());
		r.w = START_WIDTH;
		r.h = START_HEIGHT;
	} else {
		SDL_Log("Usable Bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);
	}

	if (!(g->img[0].tex = malloc(100*sizeof(SDL_Texture*)))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Couldn't allocate tex array: %s\n", strerror(errno));
		cleanup(0, 1);
	}
	g->img[0].frame_capacity = 100;

	g->scr_w = START_WIDTH;
	g->scr_h = START_HEIGHT;

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

	snprintf(title_buf, STRBUF_SZ, "Select File/Folder");

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

	// first load will switch and copy img[0].scr_rect
	g->img = g->img2;
	// Should be a way to use existing code for this but there isn't without changes/additions
	//g->img[0].scr_rect.x = 0;
	//g->img[0].scr_rect.y = 0;
	g->img[0].scr_rect.w = g->scr_w;
	g->img[0].scr_rect.h = g->scr_h;

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

	// init file browser
	init_file_browser(&g->filebrowser, g->img_exts, g->n_exts, NULL, NULL, NULL);
	g->filebrowser.selection = -1; // default to no selection
	// TODO handle different recents functions for linux/windows
	//init_file_browser(&g->filebrowser, default_exts, NUM_DFLT_EXTS, NULL, gnome_recents, NULL);

	if (!(g->ctx = nk_sdl_init(g->win, g->ren))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "nk_sdl_init() failed!\n");
		cleanup(1, 1);
	}


	// TODO Font stuff, refactor/reorganize
	if (!got_config) {
		int render_w, render_h;
		int window_w, window_h;
		SDL_GetRendererOutputSize(g->ren, &render_w, &render_h);
		SDL_GetWindowSize(g->win, &window_w, &window_h);
		g->x_scale = (float)(render_w) / (float)(window_w);
		g->y_scale = (float)(render_h) / (float)(window_h);
	}
	// could adjust for dpi, then adjust for font size if necessary
	//g->x_scale = 2; //hdpi/72;
	//g->y_scale = 2; //vdpi/72;
	float font_scale = g->y_scale;

	printf("scale %f %f\n", g->x_scale, g->y_scale);
	nk_sdl_scale(g->x_scale, g->y_scale);

	struct nk_font_atlas* atlas;
	struct nk_font_config config = nk_font_config(0);
	struct nk_font* font;

	nk_sdl_font_stash_begin(&atlas);
	font = nk_font_atlas_add_default(atlas, FONT_SIZE*font_scale, &config);
	//font = nk_font_atlas_add_from_file(atlas, "../fonts/kenvector_future_thin.ttf", 13 * font_scale, &config);
	nk_sdl_font_stash_end();

	font->handle.height /= font_scale;
	nk_style_set_font(g->ctx, &font->handle);


	// Make GUI partially transparent
	g->ctx->style.window.fixed_background.data.color.a *= 0.75;
	//g->ctx->style.window.background.a *= 0.75;

	// Trying to figure out/fix why menu_item_labels are wider than selectables
	//g->ctx->style.selectable.padding = nk_vec2(4.0f,4.0f);
	//g->ctx->style.selectable.touch_padding = nk_vec2(4.0f,4.0f);

	// type of event for all GUI initiated events
	g->userevent = SDL_RegisterEvents(1);
	if (g->userevent == (u32)-1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->img_loading_cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->img_loading_mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* loading_thrd;
	if (!(loading_thrd = SDL_CreateThread(load_new_images, "loading_thrd", NULL))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "couldn't create image loader thread: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	SDL_DetachThread(loading_thrd);

	if (!(g->scanning_cnd = SDL_CreateCond())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,"Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	if (!(g->scanning_mtx = SDL_CreateMutex())) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		cleanup(0, 1);
	}

	SDL_Thread* scanning_thrd;
	if (!(scanning_thrd = SDL_CreateThread(scan_sources, "scanning_thrd", NULL))) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "couldn't create scanner thread: %s\n", SDL_GetError());
		cleanup(0, 1);
	}
	SDL_DetachThread(scanning_thrd);


	// Setting both of these last to maximize time/accuracy
	g->gui_timer = SDL_GetTicks();
	g->show_gui = nk_true;
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

	if (!(rotated = malloc(frames*(sz*4)))) {
		perror("Couldn't allocate rotated");
		cleanup(0, 1);
	}

	u8* pix = img->pixels;
	i32 *p, *rot;
	for (int k=0; k<frames; ++k) {
		rot = (i32*)&rotated[k*(sz*4)];
		p = (i32*)&pix[k*(sz*4)];
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

void calc_rotated_size(int w, int h, float degrees, int* out_w, int* out_h)
{
	// heavily based on SDL2_gfx's rotozoomSurfaceSize that I used to
	// use
	double rads = degrees * (3.14159265/180.0);

	double sin_t = sin(rads);
	double cos_t = cos(rads);

	double w2 = w/2;
	double h2 = h/2;

	double c_w2 = cos_t*w2;
	double c_h2 = cos_t*h2;

	double s_w2 = sin_t*w2;
	double s_h2 = sin_t*h2;

	int w_h = MAX((int)
		ceil(MAX(MAX(MAX(fabs(c_w2 + s_h2), fabs(c_w2 - s_h2)), fabs(-c_w2 + s_h2)), fabs(-c_w2 - s_h2))), 1);
	int h_h = MAX((int)
		ceil(MAX(MAX(MAX(fabs(s_w2 + c_h2), fabs(s_w2 - c_h2)), fabs(-s_w2 + c_h2)), fabs(-s_w2 - c_h2))), 1);
	*out_w = 2 * w_h;
	*out_h = 2 * h_h;
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

	int wrot, hrot;
	calc_rotated_size(w, h, -img->rotdegs, &wrot, &hrot);

	int hrot2 = hrot / 2;
	int wrot2 = wrot / 2;
	int sz = w*h*4;
	int sz_rot = wrot*hrot*4;

	size_t alloc_size = frames * sz_rot;

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
		inu32 = (u32*)&pix[sz*k];
		outu32 = (u32*)&rot_pixels[sz_rot*k];
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

int start_scanning(void)
{
	if (g->sources.size) {
		SDL_LockMutex(g->scanning_mtx);
		// anything to do here?
		g->done_scanning = 0;
		puts("start scanning");

		g->state = SCANNING;
		SDL_CondSignal(g->scanning_cnd);
		SDL_UnlockMutex(g->scanning_mtx);
		return 1;
	}
	return 0;
}

int try_move(int direction)
{
	// TODO prevent moves and some other
	// actions while g->show_rotate.  Since we already
	// hide the GUI while the popup's up, we really just have
	// to worry about keyboard actions.
	if (!g->loading && !g->done_loading) {
		puts("signaling a load");
		SDL_LockMutex(g->img_loading_mtx);
		g->loading = direction;
		SDL_CondSignal(g->img_loading_cnd);
		SDL_UnlockMutex(g->img_loading_mtx);
		return 1;
	}
	return 0;
}

void do_shuffle()
{
	if (g->n_imgs != 1 || g->generating_thumbs || IS_VIEW_RESULTS()) {
		return;
	}

	if (g->has_bad_paths) {
		SDL_Log("Removing bad paths before shuffling...");
		remove_bad_paths();
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
		SDL_Log("Can't sort in multi-image modes or while generating thumbs");
		return;
	}

	if (g->has_bad_paths) {
		SDL_Log("Removing bad paths before sorting...");
		remove_bad_paths();
	}

	char* save;
	if (g->state & RESULT_MASK)
		save = g->files.a[g->search_results.a[g->img[0].index]].path;
	else
		save = g->files.a[g->img[0].index].path;

	// g->thumbs.a is either NULL or valid
	if (g->thumbs.a) {
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), cmp, 1, g->thumbs.a, sizeof(thumb_state));
	} else {
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), cmp, 0);
	}

	// find new index of img[0]
	// TODO use bsearch?
	int i;
	for (i=0; i<g->files.size; ++i) {
		if (!strcmp(save, g->files.a[i].path)) {
			break;
		}
	}

	// should work even while in result modes
	if (g->state & RESULT_MASK) {
		search_filenames(SDL_FALSE);

		for (int j=0; j<g->search_results.size; ++j) {
			if (g->search_results.a[j] == i) {

				// selection is used in listmode results, = index in results
				g->selection = g->img[0].index = j;

				// thumb_sel is the actual index in g->files, since results are
				// not separated out, just highlighted like vim
				g->thumb_sel = i;
				g->thumb_start_row = g->thumb_sel / g->thumb_cols;
				break;
			}
		}
	} else {
		// In non-result modes, index and selection are the files index
		g->img[0].index = i;

		// NOTE(rswinkle): Decided it's not worth preserving g->selection/g->thumb_sel
		// partly because I can't even add consistent keyboard shortcuts in thumb_mode since thumb_mode
		// uses keycodes (since the idea is to use typing muscle memory) and normal mode uses scancodes.
		// And n is already used when going through search results so there'd have to be an additional state check.
		//
		// Plus, it's very fast to just hit enter/double click on an image (in list or thumb mode), do the sort you want
		// and switch back to list/thumb mode.
		//
		// So we just keep current image (what they'll go back to if they hit
		// ESC instead of double clicking or hitting Enter on another one)
		g->selection = i;
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

void do_pan(int dx, int dy)
{
	img_state* img = NULL;
	if (!g->img_focus) {
		for (int i=0; i<g->n_imgs; ++i) {
			img = &g->img[i];
			if (dx != 0 && img->disp_rect.w > img->scr_rect.w) {
				img->disp_rect.x += dx;
			}
			if (dy != 0 && img->disp_rect.h > img->scr_rect.h) {
				img->disp_rect.y += dy;
			}
			fix_rect(img);
		}
	} else {
		img = g->img_focus;
		if (dx != 0 && img->disp_rect.w > img->scr_rect.w) {
			img->disp_rect.x += dx;
		}
		if (dy != 0 && img->disp_rect.h > img->scr_rect.h) {
			img->disp_rect.y += dy;
		}
		fix_rect(img);
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
				g->show_gui = nk_true;
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

		if (!(flip_pix = malloc(frames * (sz*4)))) {
			perror("Couldn't allocate flipped");
			cleanup(0, 1);
		}

		i32* p;
		i32* flip;
		if (is_vertical) {
			for (int i=0; i<frames; ++i) {
				p = (i32*)&pix[i*(sz*4)];
				flip = (i32*)&flip_pix[i*(sz*4)];
				for (int j=0; j<h; ++j) {
					memcpy(&flip[(h-1-j)*w], &p[j*w], w*sizeof(i32));
					//for (int k=0; k<w; ++k) {
					//	flip[(h-1-j)*w + k] = p[j*w + k];
					//}
				}
			}

		} else {
			for (int i=0; i<frames; ++i) {
				p = (i32*)&pix[i*(sz*4)];
				flip = (i32*)&flip_pix[i*(sz*4)];
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
			SDL_LockMutex(g->img_loading_mtx);
			g->loading = mode;
			SDL_CondSignal(g->img_loading_cnd);
			SDL_UnlockMutex(g->img_loading_mtx);
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

	// TODO log removal

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

#ifndef _WIN32

int cvec_contains_str(cvector_str* list, char* s)
{
	for (int i=0; i<list->size; ++i) {
		if (!strcmp(list->a[i], s)) {
			return i;
		}
	}
	return -1;
}

// TODO simple cross platform realpath
void do_save(int removing)
{
	if (g->loading)
		return;

	i64 loc;
	if (removing) {
		if (g->img_focus) {
			if ((loc = cvec_contains_str(&g->favs, g->img_focus->fullpath)) < 0) {
				SDL_Log("%s not in favorites\n", g->img_focus->fullpath);
			} else {
				SDL_Log("removing %s\n", g->img_focus->fullpath);
				cvec_erase_str(&g->favs, loc, loc);
				SDL_Log("%ld left after removal\n", g->favs.size);
			}
		} else {
			for (int i=0; i<g->n_imgs; ++i) {
				if ((loc = cvec_contains_str(&g->favs, g->img[i].fullpath)) < 0) {
					SDL_Log("%s not in favorites\n", g->img[i].fullpath);
				} else {
					SDL_Log("removing %s\n", g->img[i].fullpath);
					cvec_erase_str(&g->favs, loc, loc);
					SDL_Log("%ld after removal\n", g->favs.size);
				}
			}
		}
	} else {
		if (g->img_focus) {
			if (cvec_contains_str(&g->favs, g->img_focus->fullpath) >= 0) {
				SDL_Log("%s already in favorites\n", g->img_focus->fullpath);
			} else {
				SDL_Log("saving %s\n", g->img_focus->fullpath);
				cvec_push_str(&g->favs, g->img_focus->fullpath);
			}
		} else {
			for (int i=0; i<g->n_imgs; ++i) {
				if (cvec_contains_str(&g->favs, g->img[i].fullpath) >= 0) {
					SDL_Log("%s already in favorites\n", g->img[i].fullpath);
				} else {
					SDL_Log("saving %s\n", g->img[i].fullpath);
					cvec_push_str(&g->favs, g->img[i].fullpath);
				}
			}
		}
	}
}
#endif

// There is no easy way to do cross platform visual copy paste.
// SDL lets you do text but to get visual, I'd have to be using something
// like Qt, or start pulling in x11, winapi, etc. and write it myself
// which defeats the purpose of using/preferring single header libraries
// and trying to minimize external dependencies.
int do_copy()
{
	static int show_warning = SDL_TRUE;

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
	// TODO hmm handle switching directly from thumb to list and vice versa
	g->state = LIST_DFLT;
	g->selection = g->img[0].index;
	g->list_setscroll = SDL_TRUE;
	text_buf[0] = 0;
	text_len = 0;
	g->search_results.size = 0;
	g->status = REDRAW;
	SDL_ShowCursor(SDL_ENABLE);
}

void do_thumbmode()
{
	if (!g->thumbs_done) {
		generate_thumbs(SDL_TRUE);
	} else if (!g->thumbs_loaded) {
		SDL_Thread* thumb_thrd;
		if (!(thumb_thrd = SDL_CreateThread(load_thumbs, "load_thumbs_thrd", NULL))) {
			// TODO warning?
			SDL_Log("couldn't create load thumb thread\n");
		}
		// passing NULL is a no-op like free
		SDL_DetachThread(thumb_thrd);
	}

	g->state = THUMB_DFLT;
	g->thumb_sel = g->img[0].index;
	g->thumb_sel_end = g->img[0].index;
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
	g->show_gui = nk_true;
}

void fix_thumb_sel(int dir)
{
	if (g->thumb_sel < 0)
		g->thumb_sel = 0;
	if (g->thumb_sel >= g->files.size)
		g->thumb_sel = g->files.size-1;

	// TODO redundant with bottom of handle_thumb_events()?
	dir = (dir < 0) ? -1 : 1; // don't want to skip
	// This can happen while thumbs are still being generated
	// TODO think about this logic
	while (!g->thumbs.a[g->thumb_sel].tex && g->thumb_sel && g->thumb_sel != g->files.size-1) {
		g->thumb_sel += dir;
	}
}

void do_thumb_rem_del_search(int do_delete, int invert)
{
	int i;
	if (!invert) {
		// this can only happen normally, with invert, obviously whatever you selected
		// is still there, should be no duplicates in search_results so we can
		// check up front without actually doing the removal
		if (g->search_results.size == g->files.size) {
			SDL_Log("You removed all your currently viewed images, exiting\n");
			cleanup(0, 1);
		}

		// search_results is sorted so we can go backward
		// to not mess up earlier result indices
		do {
			i = cvec_pop_i(&g->search_results);
			// TODO try to detect runs to erase more at once?

			if (do_delete) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
			cvec_erase_file(&g->files, i, i);
			cvec_erase_thumb_state(&g->thumbs, i, i);
		} while (g->search_results.size);
	} else {
		int end = g->files.size-1;
		do {
			i = cvec_pop_i(&g->search_results);

			if (do_delete) {
				for (int j=i+1; j<=end; ++j) {
					if (remove(g->files.a[j].path))
						perror("Failed to delete image");
					else
						SDL_Log("Deleted %s\n", g->files.a[j].path);
				}
			}
			// This works even if i == end.  erase becomes a no-op
			cvec_erase_file(&g->files, i+1, end);
			cvec_erase_thumb_state(&g->thumbs, i+1, end);

			end = i-1;
		} while (g->search_results.size);
	}

	// Not worth trying to handle arbitrary selection imo so just reset to 0
	g->do_next = nk_true;
	int idx = g->files.size-1;
	for (int i=0; i<g->n_imgs; ++i) {
		g->img[i].index = idx++;
	}
	g->thumb_sel = 0;
}

void do_thumb_rem_del_dflt_visual(int do_delete, int invert)
{
	// so code below works for both THUMB_DFLT and VISUAL mode
	if (g->state == THUMB_DFLT) {
		g->thumb_sel_end = g->thumb_sel;
	}

	int start = g->thumb_sel, end = g->thumb_sel_end;
	if (g->thumb_sel > g->thumb_sel_end) {
		end = g->thumb_sel;
		start = g->thumb_sel_end;
	}
	if (!invert) {
		if (end - start + 1 == g->files.size) {
			SDL_Log("You removed all your currently viewed images, exiting\n");
			cleanup(0, 1);
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
	} else {
		// invert selection means erase the pictures after and before
		// (in that order so less shifting is needed ... could also
		// just make a new vector, remove the selection and copy into
		// that and then free the original
		int start1 = end+1, end1 = g->files.size-1;
		if (do_delete) {
			for (int i=start1; i<=end1; ++i) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
		}
		// NOTE(rswinkle) cvector does not check for invalid parameters
		// but erase will become a no-op if start1 is end1+1
		// as long as cvec_sz is a signed integer type
		cvec_erase_file(&g->files, start1, end1);
		cvec_erase_thumb_state(&g->thumbs, start1, end1);

		// now the images to the left
		start1 = 0, end1 = start-1;
		if (do_delete) {
			for (int i=start1; i<=end1; ++i) {
				if (remove(g->files.a[i].path))
					perror("Failed to delete image");
				else
					SDL_Log("Deleted %s\n", g->files.a[i].path);
			}
		}
		// Not an error but would waste time copying the entire vectors
		// onto themselves, becomes memmove(&v[0], &v[0], v.size*sizeof(int))
		if (end1 >= 0) {
			cvec_erase_file(&g->files, start1, end1);
			cvec_erase_thumb_state(&g->thumbs, start1, end1);
		}
	}
	// If the current images are among the removed, update to 1
	// to the left and turn on the do_next flag (do when exiting thumb
	// mode).
	//
	// If the current images remain but images to the left were
	// removed, their index needs to be updated for the shift in
	// position
	for (int i=0; i<g->n_imgs; ++i) {
		if (!invert) {
			if (g->img[i].index >= start && g->img[i].index <= end) {
				g->img[i].index = (start) ? start-1 : g->files.size-1;
				g->do_next = nk_true;
			} else if (g->img[i].index > end) {
				g->img[i].index -= end - start + 1;
			}
		} else if (g->img[i].index < start || g->img[i].index > end) {
			g->img[i].index = (i) ? (g->img[i-1].index + 1) % g->files.size : g->files.size - 1;
			g->do_next = nk_true;
		} else {
			g->img[i].index -= start;
		}
	}
	g->thumb_sel = (!invert) ? start : 0;  // in case it was > _sel_end
}

void do_thumb_rem_del(int do_delete, int invert)
{
	if (g->state & THUMB_SEARCH) {
		do_thumb_rem_del_search(do_delete, invert);
	} else {
		do_thumb_rem_del_dflt_visual(do_delete, invert);
	}

	fix_thumb_sel(1);

	// exit Visual mode after r/x (and backspace in this case) like vim
	g->state = THUMB_DFLT;
}

#include "rendering.c"
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
		puts("  -r, --recursive dir                Scan dir recursively for images to add to the list");
		puts("  -R                                 Scan all directories that come after recursively (-r after -R is redundant)");
		puts("  -c, --cache ./your_cache_loc       Use specified directory as cache");
		puts("  -v, --version                      Show the version");
		puts("  -h, --help                         Show this help");
	}
}

int main(int argc, char** argv)
{
	int ticks, len;
	struct stat file_stat;

	if (argc < 2) {
		print_help(argv[0], SDL_FALSE);
		exit(0);
	}

	setup();

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
			cvec_push_str(&g->sources, "-l");
			cvec_push_str(&g->sources, argv[++i]);


			/*
			// sanity check extension
			char* ext = GET_EXT(argv[i+1]);
			if (ext) {
				for (int j=0; j<g->n_exts; ++j) {
					if (!strcasecmp(ext, g->img_exts[j])) {
						SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Trying to load a list with a recognized image extension (%s): %s\n", ext, argv[++i]);
						cleanup(1, 0);
					}
				}
			}

			FILE* file = fopen(argv[++i], "r");
			if (!file) {
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to open %s: %s\n", argv[i], strerror(errno));
				cleanup(1, 0);
			}
			given_list = 1;
			read_list(&g->files, NULL, file);
			fclose(file);
			*/
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

			cvec_push_str(&g->sources, argv[i-1]);
			cvec_push_str(&g->sources, argv[i]);

			//myscandir(argv[i], g->img_exts, g->n_exts, SDL_TRUE);

		} else {
			normalize_path(argv[i]);
			cvec_push_str(&g->sources, argv[i]);

			/*
			int r = handle_selection(argv[i], recurse);
			given_dir = r == DIRECTORY;
			img_args += r == IMAGE;
			*/
		}
	}



	/*
	if (!g->sources.size && !g->files.size) {
		puts("Shouldn't be here");
		g->state = FILE_SELECTION;
		start_index = -1;
		init_file_browser(&g->filebrowser, default_exts, NUM_DFLT_EXTS, NULL, NULL, NULL);
		g->filebrowser.selection = -1; // default to no selection
		// TODO handle different recents functions for linux/windows
		//init_file_browser(&g->filebrowser, default_exts, NUM_DFLT_EXTS, NULL, gnome_recents, NULL);

		//SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "No images provided, exiting (empty list perhaps?)\n");
		//cleanup(1, 0);
	}
	*/

/*
	// if given a single local image, scan all the files in the same directory
	// don't do this if a list and/or directory was given even if they were empty
	if (g->files.size == 1 && img_args == 1 && !given_list && !given_dir) {
		mydirname(g->files.a[0].path, dirpath);
		mybasename(g->files.a[0].path, img_name);

		// popm to not free the string and keep the file in case
		// the start image is not added in the scan
		cvec_popm_file(&g->files, &f);

		myscandir(dirpath, g->img_exts, g->n_exts, recurse); // allow recurse for base case?

		SDL_Log("Found %"PRIcv_sz" images total\nSorting by file name now...\n", g->files.size);

		snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, img_name);

		mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);

		SDL_Log("finding current image to update index\n");
		// this is fine because it's only used when given a single image, which then scans
		// only that directory, hence no duplicate filenames are possible
		//
		// in all other cases (list, multiple files/urls, directory(ies) or some
		// combination of those) there is no "starting image", we just sort and
		// start at the beginning of the g->files in those cases
		file* res;
		res = bsearch(&f, g->files.a, g->files.size, sizeof(file), filename_cmp_lt);
		if (!res) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not find starting image '%s' when scanning containing directory\n", img_name);
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "This means it did not have a searched-for extension; adding to list and attempting load anyway\n");
			int i;
			for (i=0; i<g->files.size; i++) {
				if (filename_cmp_lt(&f, &g->files.a[i]) <= 0) {
					cvec_insert_file(&g->files, i, &f);
					res = &g->files.a[i];
					break;
				}
			}
			if (i == g->files.size) {
				cvec_push_file(&g->files, &f);
				res = &g->files.a[i];
			}
		} else {
			// no longer need this, it was found in the scan
			free(f.path);
		}
		// I could change all indexes to i64 but but no one will
		// ever open over 2^31-1 images so just explicitly convert
		// from ptrdiff_t (i64) to int here and use ints everywhere
		start_index =(int)(res - g->files.a);
	} else {
		SDL_Log("Found %"PRIcv_sz" images total\nSorting by file name now...\n", g->files.size);
		mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);
	}
	*/


	int is_a_gif;
	while (1) {
		if (handle_events())
			break;

		printf("g->state = %d\n", g->state);
		is_a_gif = 0;
		ticks = SDL_GetTicks();

		// TODO this whole GUI logic system needs to be simplified a lot
		if (!IS_FS_MODE() && !IS_SCANNING_MODE() && (!IS_LIST_MODE() || IS_VIEW_RESULTS()) && g->show_gui && ticks - g->gui_timer > g->gui_delay*1000) {
			SDL_ShowCursor(SDL_DISABLE);
			g->show_gui = nk_false;
			g->progress_hovered = nk_false;
			g->status = REDRAW;
		}

		// TODO testing, naming/organization of showing/hiding GUI vs mouse
		if (IS_FS_MODE() || IS_SCANNING_MODE() || (IS_LIST_MODE() && !IS_VIEW_RESULTS()) || g->show_gui || (g->fullscreen && g->fullscreen_gui == ALWAYS)) {
			draw_gui(g->ctx);
			g->status = REDRAW;
		}

		if (!IS_FS_MODE() && !IS_SCANNING_MODE()) {
			if (IS_THUMB_MODE() && !IS_VIEW_RESULTS()) {
				render_thumbs();
			} else if (!IS_LIST_MODE() || IS_VIEW_RESULTS()) {
				// make above plain else to do transparently show image beneath list, could work as a preview...
				is_a_gif = render_normal(ticks);
			}
		}

		// TODO ?
		if (IS_FS_MODE() || IS_SCANNING_MODE() || (IS_LIST_MODE() && !IS_VIEW_RESULTS()) || g->show_gui || (g->fullscreen && g->fullscreen_gui == ALWAYS)) {
			SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
			nk_sdl_render(NK_ANTI_ALIASING_ON);
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


