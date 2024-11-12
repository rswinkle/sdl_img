
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

#include "style_configurator.c"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define STRBUF_SZ 1024

#define GUI_BAR_HEIGHT 50
#define GUI_MENU_WIN_W 550
#define GUI_MENU_WIN_H 600

#define FONT_SIZE 24
#define PATH_SEPARATOR '/'
#define SIZE_STR_BUF 16
#define MOD_STR_BUF 24

enum { MENU_NONE, MENU_MISC, MENU_SORT, MENU_EDIT, MENU_VIEW };
enum { DELAY, ALWAYS, NEVER };
enum { SORT_NAME, SORT_PATH, SORT_SIZE, SORT_MODIFIED, NUM_USEREVENTS };

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

#define MAX_PATH_LEN 512
// TODO name? file_explorer? selector?
typedef struct file_browser
{
	char home[MAX_PATH_LEN];
	char dir[MAX_PATH_LEN];
	char file[MAX_PATH_LEN];
	//char desktop[MAX_PATH_LEN];
	cvector_file files;
	int up_to_date;

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

enum { NONE, NAME_UP, NAME_DOWN, PATH_UP, PATH_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };

SDL_Window* win;
SDL_Renderer* ren;
int running;


u32 userevent;

struct nk_colorf bg;
struct nk_color bg2;

float x_scale;
float y_scale;

cvector_str list1;
//cvector_i selected;
cvector_file files;

void search_filenames(cvector_i* search_results, cvector_file* files, char* text);
void do_filebrowser(struct nk_context* ctx, int scr_w, int scr_h);

const char* get_homedir();
int myscandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts, int recurse);
char* mydirname(const char* path, char* dirpath);
char* mybasename(const char* path, char* base);
void normalize_path(char* path);
int bytes2str(int bytes, char* buf, int len);

int handle_events(struct nk_context* ctx);
void draw_gui(struct nk_context* ctx);
void draw_prefs(struct nk_context* ctx, int scr_w, int scr_h);
void draw_simple_gui(struct nk_context* ctx);

int main(void)
{
	struct nk_context *ctx;
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

	const char* exts[] = { ".jpg" };
	//myscandir(&files, "/home/robert/", NULL, 0, 0);
	//myscandir(&files, "/home/robert/Pictures/Monica/", NULL, 0, 0);
	myscandir(&files, "/home/robert/Pictures/Monica", exts, 1, 0);

	printf("n_results = %ld\n", files.size);
	for (int i=0; i<files.size; i++) {
		printf("%-20s%20s%30s\n", files.a[i].name, files.a[i].size_str, files.a[i].mod_str);
	}

	return 0;


	running = 1;

	int win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;

	win = SDL_CreateWindow("sdl_img gui",
	                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                       WINDOW_WIDTH, WINDOW_HEIGHT,
	                       win_flags);

	if (!win) {
		printf("Can't create window: %s\n", SDL_GetError());
		return 1;
	}
	/* try VSYNC and ACCELERATED */
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	if (!ren) {
		printf("Can't create ren: %s\n", SDL_GetError());
		return 1;
	}

	float hdpi, vdpi, ddpi;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	printf("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);

	SDL_Rect r;
	SDL_GetDisplayBounds(0, &r);
	printf("display bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);

	x_scale = 1; //hdpi/72;  // adjust for dpi, then go from 8pt font to 12pt
	y_scale = 1; //vdpi/72;

	userevent = SDL_RegisterEvents(1);
	if (userevent == (u32)-1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Error: %s\n", SDL_GetError());
		return 0;
	}

	/*
	if (SDL_RenderSetLogicalSize(ren, WINDOW_WIDTH*x_scale, WINDOW_HEIGHT*y_scale)) {
		printf("logical size failure: %s\n", SDL_GetError());
		return 1;
	}
	*/

	char buffer[256];

	cvec_str(&list1, 0, 100);
	//cvec_i(&selected, 50, 100);
	//cvec_set_val_sz_i(&selected, 0);
	for (int i=0; i<50; i++) {
		sprintf(buffer, "hello %d", i);
		cvec_push_str(&list1, buffer);
	}

	if (!(ctx = nk_sdl_init(win, ren))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}

	// TODO Font stuff, refactor/reorganize
	int render_w, render_h;
	int window_w, window_h;
	SDL_GetRendererOutputSize(ren, &render_w, &render_h);
	SDL_GetWindowSize(win, &window_w, &window_h);
	x_scale = (float)(render_w) / (float)(window_w);
	y_scale = (float)(render_h) / (float)(window_h);
	// could adjust for dpi, then adjust for font size if necessary
	//g->x_scale = 2; //hdpi/72;
	//g->y_scale = 2; //vdpi/72;
	//SDL_RenderSetScale(g->ren, g->x_scale, g->y_scale);
	float font_scale = y_scale;

	printf("scale %f %f\n", x_scale, y_scale);
	SDL_RenderSetScale(ren, x_scale, y_scale);

	struct nk_font_atlas* atlas;
	struct nk_font_config config = nk_font_config(0);
	struct nk_font* font;

	nk_sdl_font_stash_begin(&atlas);
	font = nk_font_atlas_add_default(atlas, 24*font_scale, &config);
	//font = nk_font_atlas_add_from_file(atlas, "../fonts/kenvector_future_thin.ttf", 13 * font_scale, &config);
	nk_sdl_font_stash_end();

	font->handle.height /= font_scale;
	nk_style_set_font(ctx, &font->handle);

	struct nk_style_toggle* tog = &ctx->style.option;
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
    int scr_w, scr_h;
    scr_w = WINDOW_WIDTH;
    scr_h = WINDOW_HEIGHT;
    /*
	scr_w = MIN(g->scr_w, r.w - 20);  // to account for window borders/titlebar on non-X11 platforms
	scr_h = MIN(g->scr_h, r.h - 40);
	*/

	int show_filebrowser = 1;
	file_browser browser = { 0 };

	bg2 = nk_rgb(28,48,62);
	bg = nk_color_cf(bg2);
	int start_time = SDL_GetTicks();
	int time;
	int mx, my;
	Uint32 mstate;
	while (running)
	{
		/*
		time = SDL_GetTicks();
		mstate = SDL_GetMouseState(&mx, &my);
		if (!(SDL_BUTTON_LMASK & mstate) && time - start_time > 50) {
			gif_prog++;
			gif_prog %= 112;
			printf("gif_prog = %lu\n", gif_prog);
			start_time = SDL_GetTicks();
		}
		*/
		//SDL_RenderSetScale(ren, x_scale, y_scale);

		if (handle_events(ctx))
			break;

		/* GUI */
		if (show_filebrowser) {
			show_filebrowser = do_filebrowser(ctx, scr_w, scr_h);
		}
		//draw_gui(ctx);
		//draw_simple_gui(ctx);

		//style_configurator(ctx, color_table);

		SDL_SetRenderDrawColor(ren, bg2.r, bg2.g, bg2.b, bg2.a);
		SDL_RenderSetClipRect(ren, NULL);
		SDL_RenderClear(ren);
		nk_sdl_render(NK_ANTI_ALIASING_ON);
		SDL_RenderPresent(ren);
		SDL_Delay(15);

	}

cleanup:
	nk_sdl_shutdown();
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}



void draw_simple_gui(struct nk_context* ctx)
{
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	int win_w, win_h;
	int scr_w, scr_h;
	int out_w, out_h;
	SDL_GetWindowSize(win, &win_w, &win_h);
	SDL_RenderGetLogicalSize(ren, &scr_w, &scr_h);
	SDL_GetRendererOutputSize(ren, &out_w, &out_h);

	float cur_x_scale, cur_y_scale;
	SDL_RenderGetScale(ren, &cur_x_scale, &cur_y_scale);
	//printf("scale = %.2f x %.2f\n", cur_x_scale, cur_y_scale);

	int fill = 0, slideshow = 0;

	if (nk_begin(ctx, "demo", nk_rect(100/cur_x_scale, 100/cur_y_scale, 200, 400), gui_flags)) {
		nk_layout_row_static(ctx, 0, 80, 2);
		if (nk_button_label(ctx, "hello")) {
			puts("down");
			x_scale -= 0.25;
			y_scale -= 0.25;
		}
		if (nk_button_label(ctx, "goodbye")) {
			puts("up");
			x_scale += 0.25;
			y_scale += 0.25;
		}
		nk_checkbox_label(ctx, "Best Fit", &fill);
		nk_checkbox_label(ctx, "Slideshow", &slideshow);
	}
	nk_end(ctx);

	/*
	if (nk_begin(ctx, "demo2", nk_rect(400/cur_x_scale, 100/cur_y_scale, 200, 400), gui_flags)) {
		nk_layout_row_static(ctx, 0, 80, 2);
		if (nk_button_label(ctx, "hello")) {
			;
		}
		if (nk_button_label(ctx, "goodbye")) {
			;
		}
		nk_checkbox_label(ctx, "Best Fit", &fill);
		nk_checkbox_label(ctx, "Slideshow", &slideshow);
	}
	nk_end(ctx);
	*/

}





int handle_events(struct nk_context* ctx)
{
	SDL_Event evt;
	int sc;
	nk_input_begin(ctx);


	while (SDL_PollEvent(&evt)) {
		sc = evt.key.keysym.scancode;
		if (evt.type == SDL_QUIT)
			return 1;

		if (evt.type == SDL_KEYUP) {
			if (sc == SDL_SCANCODE_ESCAPE) {
					return 1;
			}
		} else if (evt.type == SDL_WINDOWEVENT) {
			if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				//SDL_RenderSetLogicalSize(ren, evt.window.data1*x_scale, evt.window.data2*y_scale);
			}
		}
		nk_sdl_handle_event(&evt);
	}
	nk_input_end(ctx);

	return 0;
}


// renamed to not conflict with <dirent.h>'s scandir
// which I could probably use to accomplish  most of this...
int myscandir(cvector_file* files, const char* dirpath, const char** exts, int num_exts, int recurse)
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

	// empty files if not recursing, otherwise assume user knows what they're doing and
	// emptied it before top level call if they wanted to
	if (!recurse) {
		cvec_clear_file(files);
	}
	int start_size = files->size;

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
			perror("stat");
			continue;
		}


		// S_ISLNK() doesn't seem to work but d_type works, though the man page
		// says it's not supported on all filesystems... or windows TODO?
		// aggh I hate windows
#ifndef _WIN32
		if (S_ISDIR(file_stat.st_mode) && entry->d_type != DT_LNK)
#else
		if (S_ISDIR(file_stat.st_mode))
#endif
		{
			if (recurse) {
				myscandir(files, fullpath, exts, num_exts, recurse);
				continue;
			}

			memset(&f, 0, sizeof(file));
			tmp = realpath(fullpath, NULL);
			f.path = realloc(tmp, strlen(tmp)+1);
			sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
			f.name = (sep) ? sep+1 : f.path;
			f.size = -1;
			cvec_push_file(files, &f);
			continue;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			continue;
		}

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

		f.size = file_stat.st_size;
		f.modified = file_stat.st_mtime;

		bytes2str(f.size, f.size_str, SIZE_STR_BUF);
		tmp_tm = localtime(&f.modified);
		strftime(f.mod_str, MOD_STR_BUF, "%Y-%m-%d %H:%M:%S", tmp_tm); // %F %T
		sep = strrchr(f.path, PATH_SEPARATOR); // TODO test on windows but I think I normalize
		f.name = (sep) ? sep+1 : f.path;
		cvec_push_file(files, &f);
	}

	printf("Found %"PRIcv_sz" files in %s\n", files->size-start_size, dirpath);

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

//scr_w and scr_h are logical dimensions not raw pixels
void do_filebrowser(file_browser* browser, struct nk_context* ctx, int scr_w, int scr_h)
{
	int is_selected = SDL_FALSE;
	int symbol;
	float search_ratio[] = { 0.25f, 0.75f };
	int list_height;
	int active;
	float search_height;

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	// TODO move to browser?
	static struct nk_list_view lview, rview;
	static float header_ratios[] = {0.49f, 0.01f, 0.15f, 0.01f, 0.34f };
	static int splitter_down = 0;

	static int state = LIST_DFLT;
	static int selection;
	static char text_buf[STRBUF_SZ];
	static int text_len;
	static sorted_state = NAME_UP;
	static int list_setscroll;

	SDL_Event event = { .type = userevent };

	static cvector_i search_results;
	int cur_result;

	int search_flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE;

	if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
		splitter_down = 0;

	if (nk_begin(ctx, "List", nk_rect(0, GUI_BAR_HEIGHT, scr_w, scr_h-GUI_BAR_HEIGHT), NK_WINDOW_NO_SCROLLBAR)) {

		// TODO With Enter to search, should I even have the Search button?  It's more of a label now... maybe put it on
		// the left?  and make it smaller?
		// TODO How to automatically focus on the search box if they start typing?
		nk_layout_row(ctx, NK_DYNAMIC, 0, 2, search_ratio);
		search_height = nk_widget_bounds(ctx).h;
		nk_label(ctx, "Search Filenames:", NK_TEXT_LEFT);
		active = nk_edit_string(ctx, search_flags, text_buf, &text_len, STRBUF_SZ, nk_filter_default);
		if (active & NK_EDIT_COMMITED) {
			text_buf[text_len] = 0;
			search_filenames(&search_results, &files, text_buf);
			memset(&rview, 0, sizeof(rview));
			state |= SEARCH_RESULTS;

			// use no selection to ignore the "Enter" in events so we don't exit
			// list mode.  Could add state to handle keeping the selection but meh
			selection = -1;  // no selection among search
		}

		// main list column headers and splitters
		nk_layout_row(ctx, NK_DYNAMIC, 0, 5, header_ratios);

		symbol = NK_SYMBOL_NONE; // 0
		if (sorted_state == NAME_UP)
			symbol = NK_SYMBOL_TRIANGLE_UP;
		else if (sorted_state == NAME_DOWN)
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
		if (sorted_state == SIZE_UP)
			symbol = NK_SYMBOL_TRIANGLE_UP;
		else if (sorted_state == SIZE_DOWN)
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
		if (sorted_state == MODIFIED_UP)
			symbol = NK_SYMBOL_TRIANGLE_UP;
		else if (sorted_state == MODIFIED_DOWN)
			symbol = NK_SYMBOL_TRIANGLE_DOWN;

		if (nk_button_symbol_label(ctx, symbol, "Modified", NK_TEXT_LEFT)) {
			event.user.code = SORT_MODIFIED;
			SDL_PushEvent(&event);
		}

		float ratios[] = { header_ratios[0]+0.01f, header_ratios[2], header_ratios[4]+0.01f };
		
		nk_layout_row_dynamic(ctx, scr_h-GUI_BAR_HEIGHT-2*search_height, 1);

		if (state & SEARCH_RESULTS) {
			if (!search_results.size) {
				if (nk_button_label(ctx, "No matching results")) {
					state = LIST_DFLT;
					text_buf[0] = 0;
					text_len = 0;
					selection = 0;
					list_setscroll = SDL_TRUE;
					// redundant since we clear before doing the search atm
					search_results.size = 0;
				}
			} else {
				if (nk_list_view_begin(ctx, &rview, "Result List", NK_WINDOW_BORDER, FONT_SIZE+16, search_results.size)) {
					nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
					int i;
					for (int j=rview.begin; j<rview.end; ++j) {
						i = search_results.a[j];
						// TODO Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
						// also thumb_sel serves the same/similar purpose
						is_selected = selection == j;
						if (nk_selectable_label(ctx, files.a[i].name, NK_TEXT_LEFT, &is_selected)) {
							if (is_selected) {
								selection = j;
							} else {
								// could support unselecting, esp. with CTRL somehow if I ever allow
								// multiple selection
								// g->selection = -1;

								// for now, treat clicking a selection as a "double click" ie same as return
								//int tmp = g->search_results.a[g->selection];
								//g->selection = (tmp) ? tmp - 1 : g->files.size-1;
								selection = (selection) ? selection - 1 : search_results.size-1;
							}
						}
						nk_label(ctx, files.a[i].size_str, NK_TEXT_RIGHT);
						nk_label(ctx, files.a[i].mod_str, NK_TEXT_RIGHT);
					}
					nk_list_view_end(&rview);
				}
			}
		} else {
			if (nk_list_view_begin(ctx, &lview, "File List", NK_WINDOW_BORDER, FONT_SIZE+16, files.size)) {
				// TODO ratio layout 0.5 0.2 0.3 ? give or take
				//nk_layout_row_dynamic(ctx, 0, 3);
				nk_layout_row(ctx, NK_DYNAMIC, 0, 3, ratios);
				for (int i=lview.begin; i<lview.end; ++i) {
					// Do I really need g->selection?  Can I use g->img[0].index (till I get multiple selection)
					// also thumb_sel serves the same/similar purpose
					is_selected = selection == i;
					if (nk_selectable_label(ctx, files.a[i].name, NK_TEXT_LEFT, &is_selected)) {
						if (is_selected) {
							selection = i;
						} else {
							// could support unselecting, esp. with CTRL somehow if I ever allow
							// multiple selection
							// selection = -1;

							// for now, treat clicking a selection as a "double click" ie same as return
							selection = (selection) ? selection - 1 : files.size-1;

							/*
							g->state = NORMAL;
							SDL_ShowCursor(SDL_ENABLE);
							g->gui_timer = SDL_GetTicks();
							g->show_gui = SDL_TRUE;
							g->status = REDRAW;
							try_move(SELECTION);
							*/
						}
					}
					nk_label(ctx, files.a[i].size_str, NK_TEXT_RIGHT);
					nk_label(ctx, files.a[i].mod_str, NK_TEXT_RIGHT);
				}
				list_height = ctx->current->layout->clip.h; // ->bounds.h?
				nk_list_view_end(&lview);
			}

			if (list_setscroll) {
				nk_uint x = 0, y;
				int scroll_limit = lview.total_height - list_height; // little off
				y = (selection/(float)(files.size-1) * scroll_limit) + 0.999f;
				//nk_group_get_scroll(ctx, "Image List", &x, &y);
				nk_group_set_scroll(ctx, "File List", x, y);
				list_setscroll = SDL_FALSE;
			}
			//printf("scroll %u %u\n", x, y);
		} // end main list
	}
	nk_end(ctx);
}

void search_filenames(cvector_i* search_results, cvector_file* files, char* text)
{
	SDL_Log("Final text = \"%s\"\n", text);

	// strcasestr is causing problems on windows
	// so just convert to lower before using strstr
	char lowertext[STRBUF_SZ] = { 0 };
	char lowername[STRBUF_SZ] = { 0 };

	// start at 1 to cut off '/'
	for (int i=0; text[i]; ++i) {
		lowertext[i] = tolower(text[i]);
	}

	// it'd be kind of cool to add results of multiple searches together if we leave this out
	// of course there might be duplicates.  Or we could make it search within the existing
	// search results, so consecutive searches are && together...
	search_results->size = 0;
	
	int j;
	for (int i=0; i<files->size; ++i) {

		for (j=0; files->a[i].name[j]; ++j) {
			lowername[j] = tolower(files->a[i].name[j]);
		}
		lowername[j] = 0;

		// searching name since I'm showing names not paths in the list
		if (strstr(lowername, lowertext)) {
			SDL_Log("Adding %s\n", files->a[i].path);
			cvec_push_i(search_results, i);
		}
	}
	SDL_Log("found %d matches\n", (int)search_results->size);
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
int init_file_browser(file_browser* fb)
{
	//if (fb->up_to_date) return
	
	char* home = get_homedir();

	size_t l;
	strncpy(browser->home, home, MAX_PATH_LEN);
	browser->home[MAX_PATH_LEN - 1] = 0;
	l = strlen(browser->home);
	strcpy(browser->home + l, "/");
	strcpy(browser->directory, browser->home);

	strcpy(browser->desktop, browser->home);
	l = strlen(browser->desktop);
	strcpy(browser->desktop + l, "desktop/");

	myscandir(&fb->files, fb->home, NULL, 0, 0);
}

int update_file_browser





