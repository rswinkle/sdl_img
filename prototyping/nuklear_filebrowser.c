
#include "myinttypes.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

//POSIX (mostly) works with MinGW64
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// for getpwuid and getuid
#include <pwd.h>
#include <unistd.h>

// TODO sin, cos, sqrt etc.
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"


#define CVECTOR_IMPLEMENTATION
#define CVEC_ONLY_INT
#define CVEC_ONLY_STR
#define CVEC_SIZE_T i64
#define PRIcv_sz PRIiMAX
#include "cvector.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define STRBUF_SZ 512

#define GUI_BAR_HEIGHT 50
#define GUI_MENU_WIN_W 550
#define GUI_MENU_WIN_H 600

#define FONT_SIZE 24
#define PATH_SEPARATOR '/'
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

#define NUM_DFLT_EXTS 11

#define TRUE 1
#define FALSE 0

enum { MENU_NONE, MENU_MISC, MENU_SORT, MENU_EDIT, MENU_VIEW };
enum { DELAY, ALWAYS, NEVER };
enum { SORT_NAME, SORT_PATH, SORT_SIZE, SORT_MODIFIED, NUM_USEREVENTS };

#define RESIZE(x) ((x+1)*2)

#include "string_compare.c"
#include "file.c"

#define FILE_LIST_SZ 20
#define MAX_PATH_LEN STRBUF_SZ

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

	// does not own memory
	const char** exts;
	int num_exts;
	int ignore_exts; // if true, show all files, not just matching exts

	// list of files in cur directory
	cvector_file files;

	
	cvector_i search_results;
	//int selection;

	int begin;
	int end;

	int sorted_state;
	cmp_func c_func;

} file_browser;

// Better names/macros
enum {
	NORMAL           = 0x1,
	THUMB_DFLT       = 0x2,
	THUMB_VISUAL     = 0x4,
	THUMB_SEARCH     = 0x8,
	LIST_DFLT        = 0x10,
	SEARCH_RESULTS   = 0x20,
	VIEW_RESULTS     = 0x40,
	VIEWED_RESULTS   = 0x80
};



typedef struct global_state
{
	SDL_Window* win;
	SDL_Renderer* ren;
	struct nk_context *ctx;

	int scr_w;
	int scr_h;

	// do these belong here or in file_browser?
	int selection;
	int list_setscroll;
	u32 userevent;

	struct nk_color bg;

	float x_scale;
	float y_scale;

} global_state;

global_state g_state;
global_state* g = &g_state;

int do_filebrowser(file_browser* fb, struct nk_context* ctx, int scr_w, int scr_h);

int init_file_browser(file_browser* browser, const char** exts, int num_exts, const char* start_dir, recents_func r_func, void* userdata);
void free_file_browser(file_browser* fb);
void switch_dir(file_browser* fb, const char* dir);
void handle_recents(file_browser* fb);

void search_filenames(file_browser* fb);
int gnome_recents(cvector_str* recents, void* userdata);
const char* get_homedir();
int fb_scandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts);
char* mydirname(const char* path, char* dirpath);
char* mybasename(const char* path, char* base);
void normalize_path(char* path);
int bytes2str(int bytes, char* buf, int len);

//int handle_events(struct nk_context* ctx);
int handle_events(file_browser* fb, struct nk_context* ctx);
void draw_gui(struct nk_context* ctx);
void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h);
void draw_simple_gui(struct nk_context* ctx);

int main(int argc, char** argv)
{
	/* float bg[4]; */

	printf("sizeof(file) == %d\n", (int)sizeof(file));
	printf("sizeof(time_t) == %d\n", (int)sizeof(time_t));
	printf("sizeof(long) == %d\n", (int)sizeof(long));

	printf("homedir = '%s'\n", get_homedir());

	/* SDL setup */
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf( "Can't init SDL:  %s\n", SDL_GetError( ) );
		return 1;
	}

	const char* default_exts[NUM_DFLT_EXTS] =
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


	int running = 1;

	int win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;

	g->win = SDL_CreateWindow("File Selector",
	                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                       WINDOW_WIDTH, WINDOW_HEIGHT,
	                       win_flags);

	if (!g->win) {
		printf("Can't create window: %s\n", SDL_GetError());
		return 1;
	}
	/* try VSYNC and ACCELERATED */
	g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_SOFTWARE);
	if (!g->ren) {
		printf("Can't create ren: %s\n", SDL_GetError());
		return 1;
	}

	float hdpi, vdpi, ddpi;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	printf("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);

	SDL_Rect r;
	SDL_GetDisplayBounds(0, &r);
	printf("display bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);

	g->x_scale = 1; //hdpi/72;  // adjust for dpi, then go from 8pt font to 12pt
	g->y_scale = 1; //vdpi/72;

	g->userevent = SDL_RegisterEvents(1);
	if (g->userevent == (u32)-1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		return 0;
	}

	/*
	if (SDL_RenderSetLogicalSize(g->ren, WINDOW_WIDTH*g->x_scale, WINDOW_HEIGHT*g->y_scale)) {
		printf("logical size failure: %s\n", SDL_GetError());
		return 1;
	}
	*/

	if (!(g->ctx = nk_sdl_init(g->win, g->ren))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}

	// TODO Font stuff, refactor/reorganize
	int render_w, render_h;
	int window_w, window_h;
	SDL_GetRendererOutputSize(g->ren, &render_w, &render_h);
	SDL_GetWindowSize(g->win, &window_w, &window_h);
	g->x_scale = (float)(render_w) / (float)(window_w);
	g->y_scale = (float)(render_h) / (float)(window_h);
	// could adjust for dpi, then adjust for font size if necessary
	//g->x_scale = 2; //hdpi/72;
	//g->y_scale = 2; //vdpi/72;
	//SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
	float font_scale = g->y_scale;

	printf("scale %f %f\n", g->x_scale, g->y_scale);
	SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
	nk_sdl_scale(g->x_scale, g->y_scale);

	struct nk_font_atlas* atlas;
	struct nk_font_config config = nk_font_config(0);
	struct nk_font* font;

	nk_sdl_font_stash_begin(&atlas);
	font = nk_font_atlas_add_default(atlas, 24*font_scale, &config);
	//font = nk_font_atlas_add_from_file(atlas, "../fonts/kenvector_future_thin.ttf", 13 * font_scale, &config);
	nk_sdl_font_stash_end();

	font->handle.height /= font_scale;
	nk_style_set_font(g->ctx, &font->handle);

	struct nk_style_toggle* tog = &g->ctx->style.option;
	printf("padding = %f %f border = %f\n", tog->padding.x, tog->padding.y, tog->border);
	//tog->padding.x = 2;
	//tog->padding.y = 2;

    static struct nk_color color_table[NK_COLOR_COUNT];
    memcpy(color_table, nk_default_color_style, sizeof(color_table));

	if (SDL_GetDisplayUsableBounds(0, &r)) {
		SDL_Log("Error getting usable bounds: %s\n", SDL_GetError());
		r.w = WINDOW_WIDTH;
		r.h = WINDOW_HEIGHT;
	} else {
		SDL_Log("Usable Bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);
	}
    g->scr_w = WINDOW_WIDTH;
    g->scr_h = WINDOW_HEIGHT;

	file_browser browser = { 0 };

	char* start_dir = NULL;
	if (argc == 2) {
		start_dir = argv[1];
	}
	init_file_browser(&browser, default_exts, NUM_DFLT_EXTS, start_dir, gnome_recents, NULL);

	// default no no selection
	g->selection = -1;

	//struct nk_colorf bg2 = nk_rgb(28,48,62);
	g->bg = nk_rgb(0,0,0);
	while (running) {
		//SDL_RenderSetScale(ren, x_scale, y_scale);

		if (handle_events(&browser, g->ctx))
			break;

		/* GUI */
		if (!do_filebrowser(&browser, g->ctx, g->scr_w, g->scr_h)) {
			running = FALSE;
		}


		SDL_SetRenderDrawColor(g->ren, g->bg.r, g->bg.g, g->bg.b, g->bg.a);
		SDL_RenderSetClipRect(g->ren, NULL);
		SDL_RenderClear(g->ren);

		SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
		nk_sdl_render(NK_ANTI_ALIASING_ON);
		SDL_RenderSetScale(g->ren, 1, 1);

		SDL_RenderPresent(g->ren);
		SDL_Delay(15);
	}

	if (browser.file[0]) {
		printf("Selected: %s\n", browser.file);
	} else {
		printf("No file selected\n");
	}

	free_file_browser(&browser);

	nk_sdl_shutdown();
	SDL_DestroyRenderer(g->ren);
	SDL_DestroyWindow(g->win);
	SDL_Quit();
	return 0;
}


int handle_events(file_browser* fb, struct nk_context* ctx)
{
	SDL_Event e;
	int sym;
	int code, sort_timer;
	int ret = 0;
	int did_sort = 0;
	//SDL_Keymod mod_state = SDL_GetModState();
	
	cvector_file* f = &fb->files;

	nk_input_begin(ctx);
	while (SDL_PollEvent(&e)) {
		// TODO edit menu/GUI as appropriate for list mode, see which
		// actions make sense or are worth supporting (re-evaluate if I
		// have some sort of preview)
		if (e.type == g->userevent) {
			code = e.user.code;
			switch (code) {
			case SORT_NAME:
				SDL_Log("Starting sort by name\n");
				sort_timer = SDL_GetTicks();
				if (fb->sorted_state != NAME_UP) {
					qsort(f->a, f->size, sizeof(file), filename_cmp_lt);
					fb->sorted_state = NAME_UP;
					fb->c_func = filename_cmp_lt;
				} else {
					qsort(f->a, f->size, sizeof(file), filename_cmp_gt);
					fb->sorted_state = NAME_DOWN;
					fb->c_func = filename_cmp_gt;
				}
				did_sort = TRUE;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_SIZE:
				SDL_Log("Starting sort by size\n");
				sort_timer = SDL_GetTicks();
				if (fb->sorted_state != SIZE_UP) {
					qsort(f->a, f->size, sizeof(file), filesize_cmp_lt);
					fb->sorted_state = SIZE_UP;
					fb->c_func = filesize_cmp_lt;
				} else {
					qsort(f->a, f->size, sizeof(file), filesize_cmp_gt);
					fb->sorted_state = SIZE_DOWN;
					fb->c_func = filesize_cmp_gt;
				}
				did_sort = TRUE;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			case SORT_MODIFIED:
				SDL_Log("Starting sort by modified\n");
				sort_timer = SDL_GetTicks();
				if (fb->sorted_state != MODIFIED_UP) {
					qsort(f->a, f->size, sizeof(file), filemodified_cmp_lt);
					fb->sorted_state = MODIFIED_UP;
					fb->c_func = filemodified_cmp_lt;
				} else {
					qsort(f->a, f->size, sizeof(file), filemodified_cmp_gt);
					fb->sorted_state = MODIFIED_DOWN;
					fb->c_func = filemodified_cmp_gt;
				}
				did_sort = TRUE;
				SDL_Log("Sort took %d\n", SDL_GetTicks()-sort_timer);
				break;
			default:
				SDL_Log("Unknown user event!");
			}
			if (did_sort && fb->is_search_results) {
				search_filenames(fb);
			}
			continue;
		}
		switch (e.type) {
		case SDL_QUIT:
			// don't think I really need these since we'll be exiting anyway
			nk_input_end(ctx);
			return 1;
		case SDL_KEYUP:
			sym = e.key.keysym.sym;
			switch (sym) {
			case SDLK_ESCAPE:
				nk_input_end(ctx);
				return 1;
				break;

			// switch to normal mode on that image
			case SDLK_RETURN:
				if (g->selection >= 0) {
					int sel = (fb->is_search_results) ? fb->search_results.a[g->selection] : g->selection;
					if (f->a[sel].size == -1) {
						switch_dir(fb, f->a[sel].path);
					} else {
						strncpy(fb->file, f->a[sel].path, MAX_PATH_LEN);
						ret = 1;
					}
				}
				break;
			}
			break;

		case SDL_KEYDOWN:
			sym = e.key.keysym.sym;
			switch (sym) {
			// TODO navigate through the list mode like thumb mode ie vim?
			case SDLK_UP:
			case SDLK_DOWN:
			case SDLK_k:
			case SDLK_j:
				//puts("arrow up/down");
				g->selection += (sym == SDLK_DOWN || sym == SDLK_j) ? 1 : -1;
				if (g->selection < 0)
					g->selection += f->size;
				else
					g->selection %= f->size;
				// TODO don't set unless necessary
				g->list_setscroll = SDL_TRUE;
				break;
			}

		case SDL_WINDOWEVENT: {
			//g->status = REDRAW;
			int x, y;
			SDL_GetWindowSize(g->win, &x, &y);
			//SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "windowed %d %d %d %d\n", g->scr_w, g->scr_h, x, y);
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				g->scr_w = e.window.data1;
				g->scr_h = e.window.data2;

				break;
			}
		} break;

		//case SDL_MOUSEBUTTONUP:
			//printf("mouse click: %d, %d\n", e.button.x, e.button.y);

		break;

		// all other event types
		default:
			break;
		}

		nk_sdl_handle_event(&e);
	}
	nk_input_end(g->ctx);

	return ret;
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

	char* tmp;
	char* sep;
	char* ext = NULL;
	file f;

	while ((entry = readdir(dir))) {

		// faster than 2 strcmp calls? ignore "." and ".."
		if (entry->d_name[0] == '.' && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2]))) {
			continue;
		}

		ret = snprintf(fullpath, STRBUF_SZ, "%s/%s", dirpath, entry->d_name);
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
		tmp = realpath(fullpath, NULL);
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

	printf("Found %"PRIcv_sz" files in %s\n", files->size, dirpath);

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

	printf("switching to '%s'\n", fb->dir);
	fb_scandir(&fb->files, fb->dir, fb->exts, (fb->ignore_exts) ? 0 : fb->num_exts);
	qsort(fb->files.a, fb->files.size, sizeof(file), fb->c_func);
	g->list_setscroll = SDL_TRUE;
	g->selection = 0;
}

#define SIDEBAR_W 200

//scr_w and scr_h are logical dimensions not raw pixels
int do_filebrowser(file_browser* fb, struct nk_context* ctx, int scr_w, int scr_h)
{
	int is_selected = SDL_FALSE;
	int symbol;
	int list_height;
	int active;
	float search_height = 0;
	int ret = 1;

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	// TODO move to browser?
	static struct nk_list_view lview, rview;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	static int splitter_down = 0;


	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;

	SDL_Event event = { .type = g->userevent };

	cvector_file* f = &fb->files;

	if (fb->file[0]) {
		// You've already selected a file why are you calling do_filebrowser?
		return 0;
	}

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	if (nk_begin(ctx, "File Selector", nk_rect(0, 0, scr_w, scr_h), NK_WINDOW_NO_SCROLLBAR)) {

		struct nk_rect win_content_rect = nk_window_get_content_region(ctx);
		struct nk_vec2 win_spacing = ctx->style.window.spacing;


		//printf("scr_w,scr_h = %d, %d\n%f %f\n", scr_w, scr_h, win_content_rect.w, win_content_rect.h);
		//search_height = nk_widget_bounds(ctx).h;

		nk_layout_row_template_begin(ctx, 0);
		nk_layout_row_template_push_static(ctx, 100);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_push_static(ctx, 100);
		nk_layout_row_template_end(ctx);
		if (nk_button_label(ctx, "Cancel")) {
			// TODO maybe just have a done flag in file browser?
			ret = 0;
		}

		// Search field
		// TODO
		//nk_button_label(ctx, "Search");
		active = nk_edit_string(ctx, search_flags, fb->text_buf, &fb->text_len, STRBUF_SZ, nk_filter_default);
		if (active & NK_EDIT_COMMITED && fb->text_len) {

			search_filenames(fb);
			memset(&rview, 0, sizeof(rview));
			fb->is_search_results = TRUE;

			// use no selection to ignore the "Enter" in events so we don't exit
			// list mode.  Could add state to handle keeping the selection but meh
			g->selection = -1;  // no selection among search
			//nk_edit_unfocus(ctx);
		}

		// only enable "Open" button if you have a selection
		if (g->selection < 0) {
			nk_widget_disable_begin(ctx);
		}
		if (nk_button_label(ctx, "Open")) {
			if (f->a[g->selection].size == -1) {
				switch_dir(fb, f->a[g->selection].path);
			} else {
				strncpy(fb->file, f->a[g->selection].path, MAX_PATH_LEN);
				ret = 0;
			}
		}
		nk_widget_disable_end(ctx);

		// don't show path if recents or in root directory "/"
		if (!fb->is_recents && fb->dir[1]) {
			// method 1
			// breadcrumb buttons
			if (!fb->is_text_path) {
				ctx->style.window.spacing.x = 0;
				char *d = fb->dir;
				char *begin = d + 1;
				char tmp;
				nk_layout_row_dynamic(ctx, 0, 6);
				while (*d++) {
					tmp = *d;
					if (tmp == '/' || !tmp) {
						*d = '\0';
						if (nk_button_label(ctx, begin)) {
							switch_dir(fb, NULL);
							break;
						}
						if (tmp) *d = '/';
						begin = d + 1;
					}
				}
				ctx->style.window.spacing.x = win_spacing.x;
			} else {

				// 2 Methods of text path mode

				// method 2
				// TODO how to make this look like method 3, submit issue/documentation
				const float path_szs[] = { win_content_rect.w-win_spacing.x-100, 100 };
				nk_layout_row(ctx, NK_STATIC, 0, 2, path_szs);
				
				// method 3
				/*
				nk_layout_row_template_begin(ctx, 0);
				nk_layout_row_template_push_dynamic(ctx);
				nk_layout_row_template_push_static(ctx, 100);
				nk_layout_row_template_end(ctx);
				*/
				
				int dir_len = strlen(fb->dir);
				nk_edit_string(ctx, NK_EDIT_SELECTABLE|NK_EDIT_CLIPBOARD, fb->dir, &dir_len, MAX_PATH_LEN, nk_filter_default);


				if (nk_button_label(ctx, "Up")) {
					char* s = strrchr(fb->dir, '/');
					if (s != fb->dir) {
						*s = 0;
						switch_dir(fb, NULL);
					} else {
						switch_dir(fb, "/");
					}
				}
			}
		}

		const float group_szs[] = { SIDEBAR_W, scr_w-SIDEBAR_W };

		nk_layout_row(ctx, NK_STATIC, scr_h, 2, group_szs);

		if (nk_group_begin(ctx, "Sidebar", 0)) {

			// Make dynamic array to add saved bookmarked locations
			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label(ctx, "Settings", NK_TEXT_CENTERED);

			//nk_checkbox_label(ctx, "All Files", &fb->ignore_exts);
			static const char* ext_opts[] = { "Images", "All Files" };
			struct nk_rect bounds = nk_widget_bounds(ctx);
			int old = fb->ignore_exts;
			fb->ignore_exts = nk_combo(ctx, ext_opts, NK_LEN(ext_opts), old, FONT_SIZE, nk_vec2(bounds.w, 300));
			if (fb->ignore_exts != old) {
				if (!fb->is_recents) {
					switch_dir(fb, NULL);
				} else {
					handle_recents(fb);
					//fb->get_recents(fb, fb->userdata);
				}
			}
			static const char* path_opts[] = { "Breadcrumbs", "Text" };
			fb->is_text_path = nk_combo(ctx, path_opts, NK_LEN(path_opts), fb->is_text_path, FONT_SIZE, nk_vec2(bounds.w, 300));

			nk_label(ctx, "Bookmarks", NK_TEXT_CENTERED);
			if (fb->get_recents) {
				if (nk_button_label(ctx, "Recents")) {
					handle_recents(fb);
				}
			}
			if (nk_button_label(ctx, "Home")) {
				switch_dir(fb, fb->home);
			}
			if (nk_button_label(ctx, "Desktop")) {
				switch_dir(fb, fb->desktop);
			}
			if (nk_button_label(ctx, "Computer")) {
				switch_dir(fb, "/");
			}


			nk_group_end(ctx);
		}

		if (nk_group_begin(ctx, "List", 0)) {

			// main list column headers and splitters
			nk_layout_row(ctx, NK_DYNAMIC, 0, 5, header_ratios);

			symbol = NK_SYMBOL_NONE; // 0
			if (fb->sorted_state == NAME_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (fb->sorted_state == NAME_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			// TODO name or path?
			if (nk_button_symbol_label(ctx, symbol, "Name", NK_TEXT_LEFT)) {
				event.user.code = SORT_NAME;
				SDL_PushEvent(&event);
			}

			bounds = nk_widget_bounds(ctx);
			nk_spacing(ctx, 1);
			if ((splitter_down == 1 || (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
				nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
				float change = in->mouse.delta.x/(ctx->current->layout->bounds.w-8);
				header_ratios[0] += change;
				header_ratios[2] -= change;
				if (header_ratios[2] < 0.05f) {
					header_ratios[2] = 0.05f;
					header_ratios[0] = 0.93f - header_ratios[4];
				} else if (header_ratios[0] < 0.05f) {
					header_ratios[0] = 0.05f;
					header_ratios[2] = 0.93f - header_ratios[4];
				}
				splitter_down = 1;
			}

			// I hate redundant logic but the alternative is repeated gui code
			// TODO think of a better way
			symbol = NK_SYMBOL_NONE; // 0
			if (fb->sorted_state == SIZE_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (fb->sorted_state == SIZE_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			if (nk_button_symbol_label(ctx, symbol, "Size", NK_TEXT_LEFT)) {
				event.user.code = SORT_SIZE;
				SDL_PushEvent(&event);
			}

			bounds = nk_widget_bounds(ctx);
			nk_spacing(ctx, 1);
			if ((splitter_down == 2 || (nk_input_is_mouse_hovering_rect(in, bounds) && !splitter_down)) &&
				nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
				float change = in->mouse.delta.x/(ctx->current->layout->bounds.w-8);
				header_ratios[2] += change;
				header_ratios[4] -= change;
				if (header_ratios[2] < 0.05f) {
					header_ratios[2] = 0.05f;
					header_ratios[4] = 0.93f - header_ratios[0];
				} else if (header_ratios[4] < 0.05f) {
					header_ratios[4] = 0.05f;
					header_ratios[2] = 0.93f - header_ratios[0];
				}
				splitter_down = 2;
			}

			symbol = NK_SYMBOL_NONE; // 0
			if (fb->sorted_state == MODIFIED_UP)
				symbol = NK_SYMBOL_TRIANGLE_UP;
			else if (fb->sorted_state == MODIFIED_DOWN)
				symbol = NK_SYMBOL_TRIANGLE_DOWN;

			if (nk_button_symbol_label(ctx, symbol, "Modified", NK_TEXT_LEFT)) {
				event.user.code = SORT_MODIFIED;
				SDL_PushEvent(&event);
			}

			float ratios[] = { header_ratios[0]+0.01f, header_ratios[2], header_ratios[4]+0.01f };
			
			nk_layout_row_dynamic(ctx, scr_h-GUI_BAR_HEIGHT-2*search_height, 1);


			if (fb->is_search_results) {
				if (!fb->search_results.size) {
					if (nk_button_label(ctx, "No matching results")) {
						fb->is_search_results = FALSE;
						fb->text_buf[0] = 0;
						fb->text_len = 0;
						g->selection = -1;
						g->list_setscroll = SDL_TRUE;
					}
				} else {
					if (nk_list_view_begin(ctx, &rview, "Result List", NK_WINDOW_BORDER, FONT_SIZE+16, fb->search_results.size)) {
						nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
						int i;
						for (int j=rview.begin; j<rview.end; ++j) {
							i = fb->search_results.a[j];
							// TODO Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
							// also thumb_sel serves the same/similar purpose
							is_selected = g->selection == j;
							if (nk_selectable_label(ctx, f->a[i].name, NK_TEXT_LEFT, &is_selected)) {
								if (is_selected) {
									g->selection = j;
								} else {
									// could support unselecting, esp. with CTRL somehow if I ever allow
									// multiple selection
									// g->selection = -1;

									// for now, treat clicking a selection as a "double click" ie same as return
									if (f->a[i].size == -1) {
										switch_dir(fb, f->a[i].path);
										break;
									} else {
										strncpy(fb->file, f->a[i].path, MAX_PATH_LEN);
										ret = 0;
									}
									//break; //?
								}
							}
							nk_label(ctx, f->a[i].size_str, NK_TEXT_RIGHT);
							nk_label(ctx, f->a[i].mod_str, NK_TEXT_RIGHT);
						}
						list_height = ctx->current->layout->clip.h; // ->bounds.h?
						nk_list_view_end(&rview);
					}
				}
				if (g->list_setscroll && (rview.end-rview.begin < f->size)) {
					nk_uint x = 0, y;
					int scroll_limit = rview.total_height - list_height; // little off
					y = (g->selection/(float)(f->size-1) * scroll_limit) + 0.999f;
					//nk_group_get_scroll(ctx, "Image List", &x, &y);
					nk_group_set_scroll(ctx, "Result List", x, y);
					g->list_setscroll = SDL_FALSE;
				}
			} else {
				if (nk_list_view_begin(ctx, &lview, "File List", NK_WINDOW_BORDER, FONT_SIZE+16, f->size)) {
					// TODO ratio layout 0.5 0.2 0.3 ? give or take
					//nk_layout_row_dynamic(ctx, 0, 3);
					nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
					for (int i=lview.begin; i<lview.end; ++i) {
						assert(i < f->size);
						// Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
						// also thumb_sel serves the same/similar purpose
						is_selected = g->selection == i;
						if (nk_selectable_label(ctx, f->a[i].name, NK_TEXT_LEFT, &is_selected)) {
							if (is_selected) {
								g->selection = i;
							} else {
								if (f->a[i].size == -1) {
									switch_dir(fb, f->a[i].path);
									break;
								} else {
									strncpy(fb->file, f->a[i].path, MAX_PATH_LEN);
									ret = 0;
								}
							}
						}
						nk_label(ctx, f->a[i].size_str, NK_TEXT_RIGHT);
						nk_label(ctx, f->a[i].mod_str, NK_TEXT_RIGHT);
					}
					list_height = ctx->current->layout->clip.h; // ->bounds.h?
					nk_list_view_end(&lview);
				}

				if (g->list_setscroll && (lview.end-lview.begin < f->size)) {
					nk_uint x = 0, y;
					int scroll_limit = lview.total_height - list_height; // little off
					y = (g->selection/(float)(f->size-1) * scroll_limit) + 0.999f;
					//nk_group_get_scroll(ctx, "Image List", &x, &y);
					nk_group_set_scroll(ctx, "File List", x, y);
					g->list_setscroll = SDL_FALSE;
				}
			}
			nk_group_end(ctx);
		}
	}
	nk_end(ctx);

	return ret;
}


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
	browser->home[MAX_PATH_LEN - 1] = 0;

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
	g->selection = 0;

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
	g->list_setscroll = SDL_TRUE;

	printf("Found %"PRIcv_sz" recent files\n", fb->files.size);

	cvec_free_str(&recents);
}

char* uri_decode(const char* str)
{
	int len = strlen(str);
	char* dst = malloc(len+1);
	int x, a, b;
	char hex[3] = {0};
	int j = 0;
	
	for (int i=0; str[i]; ++i) {
		if (str[i] == '%') {
			a = str[i+1];
			b = str[i+2];
			if (!a) {
				// copy %?
				return dst;
			}
			if (isxdigit(a) && isxdigit(b)) {
				hex[0] = a;
				hex[1] = b;
				x = strtol(hex, NULL, 16);
				dst[j++] = x;
				i += 2;
			}
		} else if (str[i] == '&') {
			if (!strncmp(str+i+1, "apos;", 5)) {
				dst[j++] = '\'';
				i += 5;
			} else if (!strncmp(str+i+1, "amp;", 4)) {
				dst[j++] = '&';
				i += 4;
			} else {
				dst[j++] = str[i];
			}
		} else {
			dst[j++] = str[i];
		}
	}
	dst = realloc(dst, j+1);
	dst[j] = 0;

	return dst;
}

/** Read file into into allocated string, return in *out
 * Data is NULL terminated.  file is closed before returning (since you
 * just read the entire file ...). */
int file_read(FILE* file, char** out)
{
	assert(file);
	assert(out);

	char* data = NULL;
	long size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (size <= 0) {
		if (size == -1)
			perror("ftell failure");
		fclose(file);
		return 0;
	}

	data = (char*)malloc(size+1);
	if (!data) {
		fclose(file);
		return 0;
	}

	rewind(file);
	if (!fread(data, size, 1, file)) {
		perror("fread failure");
		fclose(file);
		free(data);
		return 0;
	}

	data[size] = 0; /* null terminate in all cases even if reading binary data */

	*out = data;

	fclose(file);
	return size;
}

int gnome_recents(cvector_str* recents, void* userdata)
{
	assert(recents);

	char path[MAX_PATH_LEN];
	snprintf(path, MAX_PATH_LEN, "%s/.local/share/recently-used.xbel", get_homedir());
	FILE* recents_file = fopen(path, "r");
	if (!recents_file) {
		perror("Could not open recents file");
		return 0;
	}

	// TODO cvector_str* or char***, obviously leaning toward former
	cvec_reserve_str(recents, 200);

	char* text = NULL;
	int file_len = 0;

	if (!(file_len = file_read(recents_file, &text))) {
		// empty file
		return 0;
	}

	const char needle[] = "bookmark href=\"file://";
	int needle_len = strlen(needle);

	char* start_search = text;

	char* result = NULL;
	char* end;
	char* dst;
	int i = 0;
	while ((result = strstr(start_search, needle))) {
		result += needle_len;
		end = strchr(result, '\"');

		*end = 0;

		dst = uri_decode(result);
		//printf("%d %s\n%d %s\n", i, result, i, dst);
		cvec_pushm_str(recents, dst);

		// continue search just past the end of last result
		start_search = end+1;
		i++;
	}

	free(text);

	// unnecessary with size included in cvector_str...
	return i;
}


void search_filenames(file_browser* fb)
{
	// fast enough to do here?  I do it in events?
	char* text = fb->text_buf;
	text[fb->text_len] = 0;
	
	SDL_Log("Final text = \"%s\"\n", text);

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
			SDL_Log("Adding %s\n", files->a[i].path);
			cvec_push_i(&fb->search_results, i);
		}
	}
	SDL_Log("found %d matches\n", (int)fb->search_results.size);
}



