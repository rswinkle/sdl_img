// The MIT License (MIT)
// 
// Copyright (c) 2017-2025 Robert Winkler
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

#include "controls_str.c"

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

// Putting this after SDL so it doesn't complain about SDL_LOG
#define FILE_TYPE_STR "Images"
#define FB_LOG(A, ...) SDL_Log(A, __VA_ARGS__)
#define FILE_BROWSER_IMPLEMENTATION
#include "file_browser.h"

#include "curl_stuff.h"
#include "thumbs.h"

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

#include "sdl_img.h"

// Use a pointer in case I ever move this to another TU, though it's unlikely
// Also I know initializing a global to 0 is redundant but meh
static global_state state = { 0 };
global_state* g = &state;

char text_buf[STRBUF_SZ];
int text_len;
char* composition;
Sint32 cursor;
Sint32 selection_len;

int cvec_contains_str(cvector_str* list, char* s);
void my_switch_dir(const char* dir);
void reset_behavior_prefs(void);
void setup_dirs(void);
void setup_font(char* font_file, float height);
void cleanup(int ret, int called_setup);
int handle_common_evts(void* userdata, SDL_Event* e);
void remove_bad_paths(void);

#include "playlists.c"
#include "thumbs.c"
#include "curl_stuff.c"

// has to come after all the enums/macros/struct defs and bytes2str
#include "gui.c"

#include "sorting.c"

// depends on color_labels in gui.c
#include "lua_config.c"

// depends on file_browser etc.
#include "recent_files.c"

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

	// Why is the extra \n needed when every individual message already has a \n?
	fprintf(logfile, "%s: %s\n", priority_prefixes[priority], message);
	fflush(logfile);
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
		int buttonid = 1;

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

		char* name;
		char* full_img_path;
		if (!IS_VIEW_RESULTS()) {
			name = g->files.a[img->index].name;
			full_img_path = g->files.a[img->index].path;
		} else {
			name = g->files.a[g->search_results.a[img->index]].name;
			full_img_path = g->files.a[g->search_results.a[img->index]].path;
		}

		snprintf(msgbox_prompt, STRBUF_SZ, "Do you want to save changes to '%s'?", name);
		messageboxdata.message = msgbox_prompt;
		if (g->confirm_rotation) {
			SDL_ShowMessageBox(&messageboxdata, &buttonid);
		}

		if (buttonid == 1) {
			char* ext = strrchr(full_img_path, '.');
			if (!ext) {
				ext = ".jpg";
				strcat(full_img_path, ext);
				// TODO should I overwrite the original? If not, I need
				// to update g->files.a[img->index].path with the new name
			}

			if (!strcasecmp(ext, ".png")) {
				stbi_write_png(full_img_path, img->w, img->h, 4, img->pixels, img->w*4);
			} else if (!strcasecmp(ext, ".bmp")) {
				stbi_write_bmp(full_img_path, img->w, img->h, 4, img->pixels);
			} else if (!strcasecmp(ext, ".tga")) {
				stbi_write_tga(full_img_path, img->w, img->h, 4, img->pixels);
			} else {
				stbi_write_jpg(full_img_path, img->w, img->h, 4, img->pixels, 100);
			}

			// TODO update thumb if it exists and reload if g->thumbs_loaded
			char thumbpath[STRBUF_SZ] = { 0 };
			get_thumbpath(full_img_path, thumbpath, sizeof(thumbpath));

			struct stat thumb_stat;
			if (!stat(thumbpath, &thumb_stat)) {
				remove(thumbpath);
			}
			if (!g->thumbs_done) {
				;
				// just delete it, the updated version will be made
				// when/if they generate thumbs
			} else if (!g->thumbs_loaded) {
				g->thumbs_done = SDL_FALSE;
				// regeneration is usually quick if they're all already done
				// TODO
			} else {
				// they're generated and loaded so just make it here
				// and update the texture
				int i = (IS_VIEW_RESULTS()) ? g->search_results.a[img->index] : img->index;
				make_thumb(i, img->w, img->h, img->pixels, thumbpath, SDL_TRUE);
			}
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

// cleanup was here

void remove_bad_paths(void)
{
	if (g->bad_path_state == CLEAN) {
		SDL_Log("No bad paths!\n");
		return;
	}

	if (g->bad_path_state == UNKNOWN) {
		if (!g->thumbs_done) {
			SDL_Log("No bad paths to remove, have you generated thumbnails to check all images?\n");
		}
		// TODO optionally loop through all of them using stbi_info()/curl to find
		// bad paths and then remove any found
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
	g->bad_path_state = CLEAN;
	SDL_Log("Done removing bad paths\n");
}

void my_switch_dir(const char* dir)
{
	// We automatically turn open_list on when going into
	// the playlist directory, so we turn it off leaving

	// NOTE this doesn't occur if dir is NULL so don't modify fb->dir in place
	// and call with dir == NULL; dir == NULL should only be used for refreshing
	// the current dir
	
	// NOTE we also don't do this if this is a regular file/dir selection
	// popup not an "Open New/More"
	if (dir && !g->fs_output) {
		if (!strcmp(dir, g->playlistdir)) {
			g->open_list = SDL_TRUE;
			g->open_single = SDL_FALSE;
			g->open_recursive = SDL_FALSE;
			g->filebrowser.ignore_exts = SDL_TRUE;
			g->filebrowser.select_dir = SDL_FALSE;
		} else if (!strcmp(g->filebrowser.dir, g->playlistdir)) {
			// Only turn it off when leaving playlistdir explicitly
			// not when entering any non-playlistdir
			g->open_list = SDL_FALSE;
		}
	}
	switch_dir(&g->filebrowser, dir);
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

	int start_size = g->files.size;


	dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1, 1);
	}

	char* sep;
	char* tmp;
	char* ext = NULL;
	file f;

	//SDL_Log("Scanning %s for images...\n", dirpath);
	while ((entry = readdir(dir))) {

		if (g->is_exiting) {
			break;
		}

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, entry->d_name);
		if (ret >= STRBUF_SZ) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "path too long\n");
			cleanup(0, 1);
		}

		// TODO speedup with fstatat, compare timing for large recursive scan
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
			// ignore unrecognized extensions
			// (unless the image was specifically specified but that's handled at
			// the bottom of scan_sources())
			if (i == num_exts)
				continue;
		}

		// have to use fullpath not d_name in case we're in a recursive call
		// resize to exact length to save memory, reduce internal
		// fragmentation.  This dropped memory use by 80% in certain
		// extreme cases.
		tmp = myrealpath(fullpath, NULL);
		f.path = realloc(tmp, strlen(tmp)+1);
#ifdef _WIN32
		normalize_path(f.path);
#endif

#ifdef CHECK_IF_NO_EXTENSION
		if (!ext && !stbi_info(f.path, NULL, NULL, NULL)) {
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
			if ((path = curl_image(i))) {
				ret = load_image(path, img, SDL_FALSE);
			}
		}
		if (!ret) {
			free(g->files.a[i].path);
			g->files.a[i].path = NULL;
			g->files.a[i].name = NULL;
			g->bad_path_state = HAS_BAD;
		} else {
			// TODO this is still not actually used anywhere
			img->file_size = g->files.a[i].size;
		}
	}
	return ret;
}

int load_new_images(void* data)
{
	int tmp;
	int load_what;
	int last;
	cvec_sz cnt;

	while (1) {
		SDL_LogDebugApp("top of load_new_images\n");
		SDL_LockMutex(g->img_loading_mtx);
		while (g->loading < 2) {
			SDL_CondWait(g->img_loading_cnd, g->img_loading_mtx);
			SDL_LogDebugApp("loading thread woke with load: %d\n", g->loading);
		}
		load_what = g->loading;
		SDL_UnlockMutex(g->img_loading_mtx);

		if (load_what == EXIT)
			break;

		SDL_LogDebugApp("loading thread received a load: %d\n", load_what);
		if (load_what >= LEFT) {
			img_state* img;
			if (g->img == g->img1)
				img = g->img2;
			else
				img = g->img1;

			// TODO this is where we have to worry about the main
			// thread writing to g->img[].scr_rect at the same time
			for (int i=0; i<g->n_imgs; ++i)
				img[i].scr_rect = g->img[i].scr_rect;

			// TODO possible (very unlikely) infinite loop if there
			// are allocation failures for every valid image in the list
			// TODO wrap all loading loops to "goto error/exit" if no valid images
			// and triger a switch to FILE_SELECTION
			if (!g->img_focus) {
				if (load_what >= RIGHT) {
					last = (load_what == RIGHT) ? g->img[g->n_imgs-1].index : g->selection;
					for (int i=0; i<g->n_imgs; ++i) {
						if (g->ind_mm && load_what != SELECTION) {
							last = g->img[i].index;
						}
						for (cnt = 0; cnt < g->files.size; ++cnt) {
							last = wrap(last + 1);
							if (attempt_image_load(last, &img[i])) {
								break;
							}
						}
						if (cnt == g->files.size) {
							goto failed_load;
						}
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
						for (cnt = 0; cnt < g->files.size; ++cnt) {
							last = wrap(last - 1);
							if (attempt_image_load(last, &img[i])) {
								break;
							}
						}
						if (cnt == g->files.size) {
							goto failed_load;
						}
						set_rect_bestfit(&img[i], g->fullscreen | g->slideshow | g->fill_mode);
						SDL_Log("Loaded %d\n", last);
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
				for (cnt = 0; cnt < g->files.size; ++cnt) {
					last = wrap(last + tmp);
					if (attempt_image_load(last, &img[0])) {
						break;
					}
				}
				if (cnt == g->files.size) {
					goto failed_load;
				}
				img[0].index = last;
				img[0].scr_rect = g->img_focus->scr_rect;
				set_rect_bestfit(&img[0], g->fullscreen | g->slideshow | g->fill_mode);

				int index = (IS_VIEW_RESULTS()) ? g->search_results.a[img[0].index] : img[0].index;
				SDL_SetWindowTitle(g->win, g->files.a[index].name);
			}
		} else {
			last = g->img[g->n_imgs-1].index;
			for (int i=g->n_imgs; i<load_what; ++i) {
				for (cnt = 0; cnt < g->files.size; ++cnt) {
					last = wrap(last + 1);
					if (attempt_image_load(last, &g->img[i])) {
						break;
					}
				}
				if (cnt == g->files.size) {
					goto failed_load;
				}
				g->img[i].index = last;
			}
		}

		goto success_load;

failed_load:
		SDL_Log("No valid file, clearing g->files\n");
		cvec_clear_file(&g->files);
		SDL_Log("g->files.size = %"PRIcv_sz"\n", g->files.size);

success_load:
		SDL_LockMutex(g->img_loading_mtx);
		g->done_loading = load_what;
		g->loading = 0;
		SDL_UnlockMutex(g->img_loading_mtx);
	}

	SDL_LockMutex(g->img_loading_mtx);
	SDL_Log("Exiting loading thread\n");
	g->done_loading = EXIT;
	g->loading = 0;
	SDL_CondSignal(g->img_loading_cnd);
	SDL_UnlockMutex(g->img_loading_mtx);

	return 0;
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
		char* tmppath = myrealpath(path, NULL);
		f.path = realloc(tmppath, strlen(tmppath)+1);
#ifdef _WIN32
		normalize_path(f.path);
#endif
		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;
		// TODO list cache members <-- what?

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

int remove_duplicates(void)
{
	char path_buf[STRBUF_SZ] = {0};
	int did_removals = 0;
	cvector_file* f = &g->files;
	int save_cur = !g->is_open_new;

	// TODO I should always have a valid image/path when calling this function
	// save current image path (could be freed as a duplicate)
	if (save_cur) {
		snprintf(path_buf, STRBUF_SZ, "%s", g->files.a[g->img[0].index].path);
	}

	// NOTE this only works after g->files is sorted obvously though I suppose I could
	// do it sorted by name with more complicated code
	mirrored_qsort(g->files.a, g->files.size, sizeof(file), filepath_cmp_lt, 0);

	int j;
	for (int i=f->size-1; i>0; --i) {
		j = i-1;
		//SDL_LogDebugApp("comparing\n%s\n%s\n\n", f->a[i].path, f->a[j].path);
		while (j >= 0 && !strcmp(f->a[i].path, f->a[j].path)) {
			SDL_LogDebugApp("found a duplicate\n");
			j--;
		}
		j++;

		if (j != i) {
			// found matching [j, i] so remove [j+1, i] to keep j to minimize movement
			SDL_LogDebugApp("Removing duplicates %d %d\n", j+1, i);
			cvec_erase_file(f, j+1, i);
			// continue to the left of j (after --i)
			i = j;
			did_removals = 1;
		}
	}

	if (save_cur) {
		for (int i=0; i<f->size; ++i) {
			if (!strcmp(path_buf, f->a[i].path)) {
				g->img[0].index = i;
				g->thumb_sel = i;
				g->selection = i;
				break;
			}
		}
	}

	return did_removals;
}

// assumes g->files is sorted using compare_func, but not necessarily by path which is the only
// unique identifier
file* find_file(file* f, compare_func cmp)
{
	/*
	file* res;
	res = bsearch(&f, g->files.a, g->files.size, sizeof(file), cmp);
	if (!res) {
		return res;
	}

	if (cmp == filepath_cmp_lt || cmp == filepath_cmp_gt) {
		return res;
	} else if (!strcmp(f->path, res->path)) {
		return res;
	}

	if (cmp == filename_cmp_lt || cmp == filename_cmp_gt) {
	}

	start_index =(int)(res - g->files.a);

	*/
	return NULL;
}

int find_file_simple(const char* path)
{
	for (int i=0; i<g->files.size; ++i) {
		if (!strcmp(path, g->files.a[i].path)) {
			return i;
		}
	}
	return -1;
}

int scan_sources(void* data)
{
	char dirpath[STRBUF_SZ] = { 0 };
	int start_index;

	int given_list = 0;
	int given_dir = 0;
	int given_url = 0;
	int recurse = 0;
	int img_args = 0;
	file f;
	file* res;

	cvector_str* srcs = &g->sources;
	while (1) {
		SDL_LockMutex(g->scanning_mtx);
		while (!srcs->size && !g->is_exiting) {
			SDL_CondWait(g->scanning_cnd, g->scanning_mtx);
		}
		// anything here?
		SDL_UnlockMutex(g->scanning_mtx);
		if (g->is_exiting) {
			break;
		}

		given_list = 0;
		given_dir = 0;
		given_url = 0;
		recurse = 0;
		img_args = 0;

		SDL_LogDebugApp("before scanning, files.size = %"PRIcv_sz"\n", g->files.size);

		char** a = srcs->a;
		for (int i=0; i<srcs->size; ++i) {
			if (g->is_exiting) {
				goto exit_scan_sources;
			}
			SDL_LogDebugApp("Scanning source: %s\n", a[i]);
			if (!strcmp(a[i], "-l")) {

				// sanity check extension
				// TODO GUI indication and don't exit just skip?
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
			} else if (!strcmp(a[i], "-p")) {
				if (load_sql_playlist_name(&g->files, a[++i])) {
					given_list = 1;
				}
				// no logging for error as it's handled in function
			} else if (!strcmp(a[i], "-R")) {
				recurse = 1;
			} else if (!strcmp(a[i], "-r") || !strcmp(a[i], "--recursive")) {
				// TODO decide if I'll support -r for non-directories (ie same as calling on
				// containing dir -r /some/dir/myimage.jpg == -r /some/dir
				// For now I don't, commented that code out in gui.c but could move here and
				// only run if a[++i] is not already a dir
				myscandir(a[++i], g->img_exts, g->n_exts, SDL_TRUE);
				given_dir |= 1;
			} else {
				int r = handle_selection(a[i], recurse);
				given_dir |= r == DIRECTORY;
				given_url |= r == URL;
				img_args += r == IMAGE;
			}
		}

		// need this in case there was only 1 source so we finished/backed out
		// of that but never hit the check at the top of the loop again
		if (g->is_exiting) {
			break; //goto exit_scan_sources; //same
		}

		cvec_clear_str(srcs);


		// if given a single local image, scan all the files in the same directory
		// don't do this if a list and/or directory was given even if they were empty
		if (img_args == 1 && !given_list && !given_dir && !given_url) {

			if (g->open_single) {
				f = g->files.a[g->files.size-1];
				SDL_Log("Added 1 image for %"PRIcv_sz" images total\n", g->files.size);

				if ((start_index = find_file_simple(f.path)) != g->files.size-1) {
					SDL_Log("You already had that image open, not adding duplicate..\n");
					cvec_pop_file(&g->files, &f);
				} else {
					mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);
					start_index = find_file_simple(f.path);
					// NOTE it will always be there in open single because there was no
					// removal + myscandir() like below so not subject to extension filtering
				}
				g->selection = (start_index) ? start_index-1 : g->files.size-1;
				g->open_single = SDL_FALSE;
			} else {
				// This is right even if g->files.size != 1 (ie we did an "Open More")
				// because it will still be the last file in the list
				//
				// popm to not free the string and keep the file in case
				// the start image is not added in the scan
				cvec_popm_file(&g->files, &f);
				mydirname(f.path, dirpath);

				myscandir(dirpath, g->img_exts, g->n_exts, recurse); // allow recurse for base case?

				SDL_Log("Found %"PRIcv_sz" images total. Sorting by file name now...\n", g->files.size);

				// remove duplicates first
				remove_duplicates();

				mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);

				SDL_Log("finding current image to update index\n");
				// in all other cases (list, multiple files/urls, directory(ies) or some
				// combination of those) there is no "starting image", we just sort and
				// start at the beginning of the g->files in those cases
				int idx = find_file_simple(f.path);
				res = (idx >= 0) ? &g->files.a[idx] : NULL;
				//res = find_file(&f, filename_cmp_lt);
				if (!res) {
					SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not find starting image '%s' when scanning containing directory\n", f.name);
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
				g->selection = (start_index) ? start_index-1 : g->files.size-1;
			}
		} else if (g->files.size) {
			SDL_Log("Found %"PRIcv_sz" images total. Sorting by file name now...\n", g->files.size);

			remove_duplicates();
			char* path = g->files.a[g->img[0].index].path;

			mirrored_qsort(g->files.a, g->files.size, sizeof(file), filename_cmp_lt, 0);

			if (g->is_open_new) {
				g->selection = g->files.size-1;
			} else {
				// TODO is there a way to avoid the unecessary load with minimal code changes?
				//
				// should be able to just do this line since we're already on the image
				// but we can't.  It would get us out of scanning mode but then
				// it would trigger the "done_loading" code which will crash since
				// no load actually happened
				//g->done_loading = 1;

				int idx = find_file_simple(path);
				g->selection = idx-1;
			}
		} else {
			SDL_Log("Found 0 images, switching to File Browser...\n");
		}

		g->bad_path_state |= UNKNOWN;

		// TODO protect this from generating..
		// Doing this before try_move to avoid bad urls/paths being freed out from
		// under this loop
		// set save status in current playlist
		g->save_status_uptodate = SDL_FALSE;
		if (!g->generating_thumbs && !g->loading_thumbs) {
			UPDATE_PLAYLIST_SAVE_STATUS();
			g->save_status_uptodate = SDL_TRUE;
		}

		try_move(SELECTION);

		g->is_open_new = SDL_FALSE;
		SDL_LockMutex(g->scanning_mtx);
		SDL_LogDebugApp("done_scanning = 1\n");
		g->done_scanning = 1;
		SDL_UnlockMutex(g->scanning_mtx);
	}

exit_scan_sources:
	SDL_LockMutex(g->scanning_mtx);
	SDL_LogDebugApp("Exiting scanning thread\n");
	g->done_scanning = EXIT;
	SDL_CondSignal(g->scanning_cnd);
	SDL_UnlockMutex(g->scanning_mtx);
	return 0;
}

//recents

// Need to dig deeper into font API before I finalize this function
// Still not sure it doesn't leak memory
void setup_font(char* font_file, float height)
{
	NK_UNUSED(font_file);
	NK_UNUSED(height);

	if (g->atlas) {
    	nk_font_atlas_clear(g->atlas);
		g->atlas = NULL;
	}

	g->config = nk_font_config(0);
	g->config.pixel_snap = g->pixel_snap;
	if (!g->oversample) {
		g->config.oversample_h = 1;
	}
	g->font = NULL;

	float font_scale = g->y_scale;

	nk_sdl_font_stash_begin(&g->atlas);

	if (g->font_path_buf[0]) {
		g->font = nk_font_atlas_add_from_file(g->atlas, g->font_path_buf, g->font_size * font_scale, &g->config);

		if (!g->font) {
			g->font_path_buf[0] = 0;
			g->font = nk_font_atlas_add_default(g->atlas, g->font_size*font_scale, &g->config);
		}
	} else {
		g->font = nk_font_atlas_add_default(g->atlas, g->font_size*font_scale, &g->config);
	}

	//font = nk_font_atlas_add_from_file(g->atlas, "../fonts/kenvector_future_thin.ttf", 13 * font_scale, &config);

	nk_sdl_font_stash_end();

	g->font->handle.height /= font_scale;
	nk_style_set_font(g->ctx, &g->font->handle);
	// adjust heights
	g->gui_bar_ht = g->font_size + 28;
	g->needs_scr_rect_update = SDL_TRUE;

	// adjust widths
	// until I write/find those text width functions in nuklear, best guess
	float ratio = g->font_size / DFLT_FONT_SIZE;
	
	g->gui_menu_win_w = ratio * GUI_MENU_WIN_W;
	g->gui_menu_w = ratio * GUI_MENU_W;
	g->gui_prev_next_w = ratio * GUI_PREV_NEXT_W;
	g->gui_zoom_rot_w = ratio * GUI_ZOOM_ROTATE_W;
	g->gui_sidebar_w = ratio * FB_SIDEBAR_W;
}

// TODO rename
extern inline void set_show_gui(int show)
{
	if (!show) {
		SDL_ShowCursor(SDL_DISABLE);
		g->progress_hovered = nk_false;
	} else {
		SDL_ShowCursor(SDL_ENABLE);
		g->gui_timer = SDL_GetTicks();
	}
	g->show_gui = show;

	// if we do GUI on/off again (and want the images to not be under it)
	// we would put back the g->scr_rect update here
	//g->needs_scr_rect_update = SDL_TRUE;
	//g->adj_img_rects = SDL_TRUE
	g->status = REDRAW;
}

// setup was here

// probably now worth having a 2 line function used 3 places?
inline void set_fullscreen()
{
	g->status = REDRAW;

	if (g->fullscreen) {
		SDL_SetWindowFullscreen(g->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
		if (g->fullscreen_gui != NEVER) {
			g->gui_timer = SDL_GetTicks();
			printf("Setting gui timer in fs %d\n", g->gui_timer);
			g->show_gui = SDL_TRUE;
		} else {
			g->show_gui = SDL_FALSE;
		}
	} else {
		SDL_SetWindowFullscreen(g->win, 0);
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
	if (!img) {
		return;
	}

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

	g->adj_img_rects = SDL_TRUE;
	g->status = REDRAW;

	// Ok was pressed when a change hadn't been done
	// so we couldn't clear in draw_gui because we still
	// needed it
	if (!(g->state & ROTATE)) {
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
		g->done_scanning = 0;
		SDL_LogDebugApp("start scanning\n");

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
	// actions while g->state & ROTATE.  Since we already
	// hide the GUI while the popup's up, we really just have
	// to worry about keyboard actions.
	if (!g->loading && !g->done_loading) {
		SDL_LogDebugApp("signaling a load\n");
		SDL_LockMutex(g->img_loading_mtx);
		g->loading = direction;
		SDL_CondSignal(g->img_loading_cnd);
		SDL_UnlockMutex(g->img_loading_mtx);
		return 1;
	}
	return 0;
}

// depends on recents and more
#include "setup_shutdown.c"

#include "actions.c"

#include "rendering.c"
#include "events.c"

int main(int argc, char** argv)
{
	int ticks, old_ticks, len;
	int is_a_gif;
	struct stat file_stat;

#ifndef USE_SOFTWARE_RENDERER
	NK_UNUSED(is_a_gif);
#endif

	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--playlist")) {
			if (i+1 == argc) {
				SDL_Log("Error missing playlist name following %s\n", argv[i]);
				break;
			}
			cvec_push_str(&g->sources, "-p");
			cvec_push_str(&g->sources, argv[++i]);
		} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list")) {
			if (i+1 == argc) {
				SDL_Log("Error missing list file following %s\n", argv[i]);
				break;
			}
			cvec_push_str(&g->sources, "-l");
			cvec_push_str(&g->sources, argv[++i]);
		} else if (!strcmp(argv[i], "-R")) {
			cvec_push_str(&g->sources, "-R");
		} else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--recursive")) {
			// TODO maybe --recursive should be --recurse and --recursive should be long form
			// of -R?
			if (i+1 == argc) {
				SDL_Log("Error missing directory following -r\n");
				break;
			}
			i++;
			if (stat(argv[i], &file_stat) || !S_ISDIR(file_stat.st_mode)) {
				SDL_Log("Bad argument, expected directory following -r: \"%s\", skipping\n", argv[i]);
				continue;
			}
			len = strlen(argv[i]);
			if (argv[i][len-1] == '/')
				argv[i][len-1] = 0;

			cvec_push_str(&g->sources, argv[i-1]);
			cvec_push_str(&g->sources, argv[i]);
		} else if (argv[i][0] != '-') {
			// a file named '-' or similar can be done by giving the path ie sdl_img ./-
			normalize_path(argv[i]);
			cvec_push_str(&g->sources, argv[i]);
		}
	}

	setup(argc, argv);

	old_ticks = SDL_GetTicks();
	int frame_count;
	while (1) {
		if (handle_events())
			break;

		//printf("g->state = %d\n", g->state);
		is_a_gif = 0;
		ticks = SDL_GetTicks();
#ifndef NDEBUG
		if (ticks - old_ticks >= 5000) {
			SDL_Log("FPS = %.2f\n", frame_count/((ticks-old_ticks)/1000.0f));
			old_ticks = ticks;
			frame_count = 0;
		}
		frame_count++;
#endif

		if (g->show_gui && ticks - g->gui_timer > g->gui_delay*1000) {
			printf("ticks = %d, gui_timer = %d\n", ticks, g->gui_timer);
			set_show_gui(SDL_FALSE);
		}

		draw_gui(g->ctx);
		g->status = REDRAW; // maybe integrate this into draw_gui()?

		// IS_NORMAL covers popups rendered over normal as well
		// as VIEW_RESULSTS()
		if (IS_NORMAL()) {
			is_a_gif = render_normal(ticks);
		} else if (IS_THUMB_MODE()) {
			render_thumbs();
		}

		SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
		// TODO try it off?
#ifndef USE_SOFTWARE_RENDERER
		nk_sdl_render(NK_ANTI_ALIASING_ON);
#else
		nk_sdl_render(NK_ANTI_ALIASING_OFF);
#endif
		SDL_RenderSetScale(g->ren, 1, 1);

		SDL_RenderPresent(g->ren);


#ifdef USE_SOFTWARE_RENDERER
		//"sleep" save CPU cycles/battery especially when not viewing animated gifs
		if (!is_a_gif) { // && !g->loading)
			SDL_Delay(SLEEP_TIME);
		} else {
			SDL_Delay(MIN_GIF_DELAY/2);
		}
#endif
	}

	cleanup(0, 1);
	//never get here
	return 0;
}


