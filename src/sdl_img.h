
// Add REDRAW2 to handle double buffering when doing accelerated rendering
enum { NOCHANGE, REDRAW, REDRAW2, NUM_STATUSES };

// file list state
enum { CLEAN, UNKNOWN, HAS_BAD };

enum { NOTHING = 0, MODE1 = 1, MODE2 = 2, MODE4 = 4, MODE8 = 8, LEFT, RIGHT, SELECTION, EXIT };
enum { NOT_EDITED, ROTATED, TO_ROTATE, FLIPPED};
enum { DELAY, ALWAYS, NEVER };
enum { NONE, NAME_UP, NAME_DOWN, PATH_UP, PATH_DOWN, SIZE_UP, SIZE_DOWN, MODIFIED_UP, MODIFIED_DOWN };
enum { NEXT, PREV, ZOOM_PLUS, ZOOM_MINUS, ROT_LEFT, ROT_RIGHT, FLIP_H, FLIP_V, MODE_CHANGE,
       THUMB_MODE, LIST_MODE, SAVE_IMG, UNSAVE_IMG, REMOVE_IMG, DELETE_IMG, ACTUAL_SIZE, ROT360, REMOVE_BAD,
       SHUFFLE, SORT_NAME, SORT_PATH, SORT_SIZE, SORT_MODIFIED, OPEN_FILE_NEW,
       OPEN_FILE_MORE, OPEN_PLAYLIST_MANAGER, SELECT_FILE, SELECT_DIR, FONT_CHANGE, NUM_USEREVENTS };

// return values for handle_selection(), says what the arg was
enum { URL, DIRECTORY, IMAGE };

// Better names/macros

enum {
	NORMAL           = NK_FLAG(0),
	THUMB_DFLT       = NK_FLAG(1),
	THUMB_VISUAL     = NK_FLAG(2),
	THUMB_SEARCH     = NK_FLAG(3),
	LIST_DFLT        = NK_FLAG(4),
	SEARCH_RESULTS   = NK_FLAG(5),

	// rename?  FILE_BROWSER?
	FILE_SELECTION   = NK_FLAG(6),
	SCANNING         = NK_FLAG(7),

	// popups
	ABOUT            = NK_FLAG(8),
	PREFS            = NK_FLAG(9),
	ROTATE           = NK_FLAG(10),
	PLAYLIST_MANAGER = NK_FLAG(11),

};

#define THUMB_MASK (THUMB_DFLT | THUMB_VISUAL | THUMB_SEARCH)
#define LIST_MASK (LIST_DFLT)
#define RESULT_MASK (SEARCH_RESULTS)
#define POPUP_MASK (ABOUT | PREFS | ROTATE | PLAYLIST_MANAGER)
//#define VIEW_MASK (NORMAL)

#define IS_THUMB_MODE() (g->state & THUMB_MASK)
#define IS_LIST_MODE() (g->state & LIST_MASK)
#define IS_RESULTS() (g->state & RESULT_MASK)
#define IS_VIEW_RESULTS() (g->state & NORMAL && g->state & SEARCH_RESULTS)
#define IS_FS_MODE() (g->state & FILE_SELECTION)
#define IS_SCANNING_MODE() (g->state == SCANNING)
#define IS_POPUP_ACTIVE() (g->state & POPUP_MASK)

#ifdef _WIN32
#define mkdir(A, B) mkdir(A)
#endif

// baked in settings
#include "compile_constants.h"

// Used for run-time settable preferences loaded/stored in config.lua
#include "config_constants.h"

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SET_MODE1_SCR_RECT()                                                   \
	do {                                                                       \
	g->img[0].scr_rect.x = g->scr_rect.x;                                      \
	g->img[0].scr_rect.y = g->scr_rect.y;                                      \
	g->img[0].scr_rect.w = g->scr_rect.w;                                      \
	g->img[0].scr_rect.h = g->scr_rect.h;                                      \
	set_rect_bestfit(&g->img[0], g->fullscreen | g->slideshow | g->fill_mode); \
	} while (0)

#define SET_MODE2_SCR_RECTS()                                                  \
	do {                                                                       \
	g->img[0].scr_rect.x = g->scr_rect.x;                                      \
	g->img[0].scr_rect.y = g->scr_rect.y;                                      \
	g->img[0].scr_rect.w = g->scr_rect.w/2;                                    \
	g->img[0].scr_rect.h = g->scr_rect.h;                                      \
	g->img[1].scr_rect.x = g->scr_rect.w/2;                                    \
	g->img[1].scr_rect.y = g->scr_rect.y;                                      \
	g->img[1].scr_rect.w = g->scr_rect.w/2;                                    \
	g->img[1].scr_rect.h = g->scr_rect.h;                                      \
	set_rect_bestfit(&g->img[0], g->fullscreen | g->slideshow | g->fill_mode); \
	set_rect_bestfit(&g->img[1], g->fullscreen | g->slideshow | g->fill_mode); \
	} while (0)

#define SET_MODE4_SCR_RECTS()                                                       \
	do {                                                                            \
	int w = g->scr_rect.w/2, h = g->scr_rect.h/2;                                   \
	int x = g->scr_rect.x, y = g->scr_rect.y;                                       \
	for (int i=0; i<4; ++i) {                                                       \
		g->img[i].scr_rect.x = (i%2)*w + x;                                         \
		g->img[i].scr_rect.y = (i/2)*h + y;                                         \
		g->img[i].scr_rect.w = w;                                                   \
		g->img[i].scr_rect.h = h;                                                   \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);  \
	}                                                                               \
	} while (0)

#define SET_MODE8_SCR_RECTS()                                                       \
	do {                                                                            \
	int w = g->scr_rect.w/4, h = g->scr_rect.h/2;                                   \
	int x = g->scr_rect.x, y = g->scr_rect.y;                                       \
	for (int i=0; i<8; ++i) {                                                       \
		g->img[i].scr_rect.x = (i%4)*w + x;                                         \
		g->img[i].scr_rect.y = (i/4)*h + y;                                         \
		g->img[i].scr_rect.w = w;                                                   \
		g->img[i].scr_rect.h = h;                                                   \
		set_rect_bestfit(&g->img[i], g->fullscreen | g->slideshow | g->fill_mode);  \
	}                                                                               \
	} while (0)

#define SDL_LogDebugApp(...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define SDL_LogCriticalApp(...) SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

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

	SDL_Rect scr_rect;
	int needs_scr_rect_update; // true when need to update image_scr_rects

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
	int cur_result; // keep track of which thumb result they're on
	//int using_search_indices // whether img[].index refers to search indices

	int status;

	// booleans
	
	// needed because default cachedir changes every day so we don't by default
	// save that to config file, only if the user manually adds it
	int cfg_cachedir;

	// Needed so we know to free them.  Having them as a config value rather than
	// only hardcoded is so users can specify a subset of extensions supported
	int cfg_img_exts;

	char* cachedir;
	char* thumbdir;
	char* logdir;
	char* playlistdir;
	char* prefpath;

	char cachedir_buf[STRBUF_SZ];
	char thumbdir_buf[STRBUF_SZ];
	char logdir_buf[STRBUF_SZ];
	char playlistdir_buf[STRBUF_SZ];
	char font_path_buf[STRBUF_SZ];

	// TODO naming
	char cur_playlist_path[STRBUF_SZ];
	char* cur_playlist; // points into above

	char* default_playlist;  // allocated

	cvector_file files;
	cvector_str favs;
	cvector_str playlists;
	int save_status_uptodate;  // bool

	cvector_str sources;  // files/directories to scan etc.
	int done_scanning;

	// for file selection
	file_browser filebrowser;

	// (should I move these to file_browser?)
	int open_single;    // boolean
	int open_playlist;  // boolean
	int open_recursive; // boolean
	int is_open_new;    // boolean

	int old_state; // TODO remind myself why I need this?
	char* fs_output;  // place to assign fb output
	cvector_str bookmarks;

	const char** img_exts;
	int n_exts;

	int bad_path_state;

	int state; // better name?
	int is_exiting;

	// flag to do load right returning from thumb mode
	// TODO different name? cur_img_was_removed?
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
	int confirm_delete;
	int confirm_rotation;
	int warn_text_copy;

	struct nk_font_atlas* atlas;
	struct nk_font_config config;
	struct nk_font* font;
	float font_size;

	// booleans
	int pixel_snap;
	int oversample;

	// gui dimensions that have to be adjusted based on font size
	int gui_bar_ht;
	int gui_menu_win_w;
	int gui_menu_w;  // menu button width
	int gui_prev_next_w;
	int gui_zoom_rot_w;
	int gui_sidebar_w;


	// TODO once stable bake into executable and remove
	char* controls_text;
	int ct_len;

	int list_search_active; // text field has focus in gui
	int list_setscroll;

	int thumbs_done;
	int thumbs_loaded;
	int thumb_start_row;
	int is_thumb_visual_line;
	int thumb_rows;
	int thumb_cols;

	int thumb_sel;  // current image
	int thumb_sel_end; // start or end of visual selection (_sel is other side)
	int selection;  // actual selection made (switching to normal mode)

	int menu_state;
	int sorted_state;

	// sdl_img colors
	struct nk_color bg;
	// for now this is used for regular box and visual mode
	// with alpha hardcoded as 255 for regular mode
	struct nk_color thumb_highlight;

	// alpha for above color in search/selection/visual
	int thumb_opacity;

	// Nuklear colors
	struct nk_color color_table[NK_COLOR_COUNT];

	int slide_delay;
	int slideshow;
	int slide_timer;

	char* lua_error;

	// threading
	int generating_thumbs;
	int loading_thumbs;

	// only one for both since you never have the generating
	// and the loading thread at the same time...for now
	SDL_cond* thumb_cnd;
	SDL_mutex* thumb_mtx;


	int loading;
	int done_loading;
	SDL_cond* img_loading_cnd;
	SDL_mutex* img_loading_mtx;

	SDL_cond* scanning_cnd;
	SDL_mutex* scanning_mtx;

	// debugging
	FILE* logfile;

} global_state;

