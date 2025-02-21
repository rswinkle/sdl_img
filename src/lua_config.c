#include <stdio.h>
#include <string.h>

//#define LUA_ERROR_NO_EXIT
#include "lua_helper.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define MAX_COLOR 255

enum {
	GUI_X_SCALE,
	GUI_Y_SCALE,
	FONT_SIZE,
	BACKGROUND,
	WINDOW_OPACITY,
	THUMB_HIGHLIGHT,
	THUMB_OPACITY,
	SLIDE_DELAY,
	HIDE_GUI_DELAY,
	BUTTON_REPEAT_DELAY,
	FULLSCREEN_GUI,
	THUMB_ROWS,
	THUMB_COLS,
	SHOW_INFO_BAR,
	FILL_MODE,
	X_DELETES_THUMB,
	CONFIRM_DELETE,
	CONFIRM_ROTATION,
	RELATIVE_OFFSETS,
	IMG_EXTS,
	BOOKMARKS,
	DEFAULT_PLAYLIST,
	PLAYLIST_DIR,
	LOG_DIR,
	CACHE_DIR,
	THUMB_DIR,
	GUI_COLORS,
	NUM_KEYS
};

char* keys[] =
{
	"gui_x_scale",
	"gui_y_scale",
	"font_size",
	"background",
	"window_opacity",
	"thumb_highlight",
	"thumb_opacity",
	"slide_delay",
	"hide_gui_delay",
	"button_repeat_delay",
	"fullscreen_gui",
	"thumb_rows",
	"thumb_cols",
	"show_info_bar",
	"fill_mode",
	"x_deletes_thumb",
	"confirm_delete",
	"confirm_rotation",
	"relative_offsets",
	"img_exts",
	"bookmarks",
	"default_playlist",
	"playlist_dir",
	"log_dir",
	"cache_dir",
	"thumb_dir",
	"gui_colors"
};

// TODO better name support alpha?
typedef struct Color
{
	u8 r,g,b;
} Color;

typedef struct ColorEntry {
	char* name;
	Color c;
} ColorEntry;

int load_color(lua_State* L, const char* name, Color* c);
void convert_color(lua_State* L, Color* c, int index);
int convert_color_field(lua_State* L, int index);

// assume table (a color) is on top of the stack
int get_color_field(lua_State* L, const char* key);
void set_color_field(lua_State* L, const char* index, int value);
void set_color(lua_State* L, ColorEntry* ct);

void do_gui_colors(lua_State* L);
int load_fullscreen_gui(lua_State* L, int dflt);
char* fullscreen_gui_str(int fsg_enum);

void write_config(FILE* cfg_file);

ColorEntry colortable[] =
{
	{ "WHITE",   { MAX_COLOR, MAX_COLOR, MAX_COLOR } },
	{ "RED",     { MAX_COLOR, 0, 0 } },
	{ "GREEN",   { 0, MAX_COLOR, 0 } },
	{ "BLUE",    { 0, 0, MAX_COLOR } },
	{ "CYAN",    { 0, MAX_COLOR, MAX_COLOR} },
	{ "YELLOW",  { MAX_COLOR, MAX_COLOR, 0 } },
	{ "MAGENTA",  { MAX_COLOR, 0, MAX_COLOR } },
	{ "BLACK",   { 0, 0, 0 } },
	
	// TODO more web colors?
	{ "BROWN",   {165, 42, 42 } },
	{ "ORANGE",  { 255, 165, 0 } },

	{ NULL,      { 0, 0, 0} }     // sentinal
};


void handle_gui_colors(lua_State* L, void* userdata)
{
	// key at -2, value at -1
	if (lua_type(L, -2) != LUA_TSTRING) {
		error(L, "gui_colors should have string keys\n");
	}
	Color c = {0};
	const char* key = lua_tostring(L, -2);

	//printf("Handling %s\n", key);
	
	struct nk_color* ct = g->color_table;
	for (int i=0; i<NK_COLOR_COUNT; ++i) {
		if (!strcasecmp(color_labels[i], key)) {
			convert_color(L, &c, -1);
			ct[i].r = c.r;
			ct[i].g = c.g;
			ct[i].b = c.b;
			ct[i].a = 255;
			break;
		}
	}
}

int read_config_file(char* filename)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_dofile(L, filename)) {
		//error(L, "cannot run config. file: %s\n", lua_tostring(L, -1));
		fprintf(stderr, "lua error: %s\n", lua_tostring(L, -1));
		lua_close(L);
		return 0;
	}

	// NOTE unlike GUI control, not trying to restrict to 0.25 or 0.5 increments
	g->x_scale = try_number_clamp_dflt(L, "gui_x_scale", MIN_GUI_SCALE, MAX_GUI_SCALE, DFLT_GUI_SCALE);
	g->y_scale = try_number_clamp_dflt(L, "gui_y_scale", MIN_GUI_SCALE, MAX_GUI_SCALE, DFLT_GUI_SCALE);

	g->slide_delay = try_int_clamp_dflt(L, "slide_delay", MIN_SLIDE_DELAY, MAX_SLIDE_DELAY, DFLT_SLIDE_DELAY);
	g->gui_delay = try_int_clamp_dflt(L, "hide_gui_delay", MIN_GUI_DELAY, MAX_GUI_DELAY, DFLT_GUI_DELAY);
	g->button_rpt_delay = try_number_clamp_dflt(L, "button_repeat_delay", MIN_BUTTON_RPT_DELAY, MAX_BUTTON_RPT_DELAY, DFLT_BUTTON_RPT_DELAY);
	g->thumb_rows = try_int_clamp_dflt(L, "thumb_rows", MIN_THUMB_ROWS, MAX_THUMB_ROWS, DFLT_THUMB_ROWS);
	g->thumb_cols = try_int_clamp_dflt(L, "thumb_cols", MIN_THUMB_COLS, MAX_THUMB_COLS, DFLT_THUMB_COLS);

	// enum
	g->fullscreen_gui = load_fullscreen_gui(L, DFLT_FULLSCREEN_GUI);

	Color background = {0}, thumb_hl = {0};
	if (load_color(L, "background", &background)) {
		g->bg = nk_rgb(background.r,background.g,background.b); // clamps for us
	}

	if (load_color(L, "thumb_highlight", &thumb_hl)) {
		g->thumb_highlight = nk_rgb(thumb_hl.r, thumb_hl.g, thumb_hl.b);
	}

	g->thumb_opacity = try_int_clamp_dflt(L, "thumb_opacity", MIN_THUMB_OPACITY, MAX_THUMB_OPACITY, DFLT_THUMB_OPACITY);

	g->show_infobar = try_bool_dflt(L, "show_info_bar", DFLT_SHOW_INFOBAR);
	g->fill_mode = try_bool_dflt(L, "show_info_bar", DFLT_FILL_MODE);
	g->thumb_x_deletes  = try_bool_dflt(L, "x_deletes_thumb", DFLT_THUMB_X_DELETES);
	g->confirm_delete  = try_bool_dflt(L, "confirm_delete", DFLT_CONFIRM_DELETE);
	g->confirm_rotation  = try_bool_dflt(L, "confirm_rotation", DFLT_CONFIRM_ROTATION);
	g->ind_mm = try_bool_dflt(L, "relative_offsets", DFLT_IND_MM);


	char** exts = NULL;
	int n = try_str_array(L, "img_exts", &exts);
	if (n) {
		g->img_exts = (const char**)exts;
		g->n_exts = n;
		g->cfg_img_exts = SDL_TRUE;
	}

	g->bookmarks.size = try_str_array(L, "bookmarks", &g->bookmarks.a);
	g->bookmarks.capacity = g->bookmarks.size;

	g->default_playlist = try_str(L, "default_playlist", NULL);

	try_strbuf(L, "playlist_dir", g->playlistdir_buf, STRBUF_SZ);

	try_strbuf(L, "log_dir", g->logdir_buf, STRBUF_SZ);

	// TODO think about where I really want cachedir storage/ownership...maybe
	// just make cachedir and thumbdir actual arrays in g and be done with it?
	if (try_strbuf(L, "cache_dir", g->cachedir_buf, STRBUF_SZ)) {
		g->cfg_cachedir = SDL_TRUE;
	}

	try_strbuf(L, "thumb_dir", g->thumbdir, STRBUF_SZ);
	// TODO playlistdir, logdir

	do_gui_colors(L);

	// For debug purposes
#ifndef NDEBUG
	write_config(stdout);
#endif

	lua_close(L);
	return 1;
}

int write_config_file(char* filename)
{
	char filepath[STRBUF_SZ];
	snprintf(filepath, STRBUF_SZ, "%s%s", g->prefpath, filename);
	FILE* cfg_file = fopen(filepath, "w");
	if (!cfg_file) {
		perror("Failed to open config file for writing");
		return 0;
	}

	write_config(cfg_file);

	fclose(cfg_file);

	return 1;
}

// TODO replace with something that's more automatic, maybe put everything in a table
// and serialize it
void write_config(FILE* cfg_file)
{
	const char* bool_str[] = { "false", "true" };
	const char* fullscreen_gui_str[] = { "delay", "always", "never" };

	for (int i=0; i<NUM_KEYS; i++) {

		// TODO handle optional configs with a variable default value better
		if (i != CACHE_DIR || g->cfg_cachedir) {
			fprintf(cfg_file, "%s = ", keys[i]);
		}

		switch (i) {
		case GUI_X_SCALE:
			fprintf(cfg_file, "%.1f\n", g->x_scale);
			break;
		case GUI_Y_SCALE:
			fprintf(cfg_file, "%.1f\n", g->y_scale);
			break;
		case FONT_SIZE:
			fprintf(cfg_file, "%d\n", DFLT_FONT_SIZE);
			break;
		case BACKGROUND:
			fprintf(cfg_file, "{ red = %u, green = %u, blue = %u }\n", g->bg.r, g->bg.g, g->bg.b);
			break;
		case THUMB_HIGHLIGHT:
			fprintf(cfg_file, "{ red = %u, green = %u, blue = %u }\n", g->thumb_highlight.r, g->thumb_highlight.g, g->thumb_highlight.b);
			break;
		case THUMB_OPACITY:
			fprintf(cfg_file, "%d\n", g->thumb_opacity);
			break;
		case SLIDE_DELAY:
			fprintf(cfg_file, "%d\n", g->slide_delay);
			break;
		case HIDE_GUI_DELAY:
			fprintf(cfg_file, "%d\n", g->gui_delay);
			break;
		case BUTTON_REPEAT_DELAY:
			fprintf(cfg_file, "%f\n", g->button_rpt_delay);
			break;
		case FULLSCREEN_GUI:
			fprintf(cfg_file, "\"%s\"\n", fullscreen_gui_str[g->fullscreen_gui]);
			break;
		case THUMB_ROWS:
			fprintf(cfg_file, "%d\n", g->thumb_rows);
			break;
		case THUMB_COLS:
			fprintf(cfg_file, "%d\n", g->thumb_cols);
			break;


		// anything not "true" is false
		case SHOW_INFO_BAR:
			fprintf(cfg_file, "%s\n", bool_str[g->show_infobar]);
			break;
		case FILL_MODE:
			fprintf(cfg_file, "%s\n", bool_str[g->fill_mode]);
			break;
		case X_DELETES_THUMB:
			fprintf(cfg_file, "%s\n", bool_str[g->thumb_x_deletes]);
			break;
		case CONFIRM_DELETE:
			fprintf(cfg_file, "%s\n", bool_str[g->confirm_delete]);
			break;
		case CONFIRM_ROTATION:
			fprintf(cfg_file, "%s\n", bool_str[g->confirm_rotation]);
			break;
		case RELATIVE_OFFSETS:
			fprintf(cfg_file, "%s\n", bool_str[g->ind_mm]);
			break;

		case DEFAULT_PLAYLIST:
			fprintf(cfg_file, "'%s'\n", g->default_playlist);
			break;

		case PLAYLIST_DIR:
			// Will always be true unless/until I add a -p playlistdir arg
			if (g->playlistdir == g->playlistdir_buf) {
				fprintf(cfg_file, "'%s'\n", g->playlistdir);
			}
			break;

		case LOG_DIR:
			// Will always be true unless/until I add a --logdir logdir arg
			if (g->logdir == g->logdir_buf) {
				fprintf(cfg_file, "'%s'\n", g->logdir);
			}
			break;

		case CACHE_DIR:
			if (g->cfg_cachedir) {
				fprintf(cfg_file, "'%s'\n", g->cachedir_buf);
			}
			break;
		case THUMB_DIR:
			fprintf(cfg_file, "'%s'\n", g->thumbdir);
			break;
		case IMG_EXTS:
			fprintf(cfg_file, "{\n");
			for (int j=0; j<g->n_exts; j++) {
				fprintf(cfg_file, "\t'%s',\n", g->img_exts[j]);
			}
			fprintf(cfg_file, "}\n");
			break;

		case BOOKMARKS:
			fprintf(cfg_file, "{\n");
			for (int j=0; j<g->bookmarks.size; j++) {
				fprintf(cfg_file, "\t'%s',\n", g->bookmarks.a[j]);
			}
			fprintf(cfg_file, "}\n");
			break;

		case GUI_COLORS:
			fprintf(cfg_file, "{\n");
			// not writing alpha, alpha always 255 for now
			for (int j=0; j<NK_COLOR_COUNT; ++j) {
				fprintf(cfg_file, "\t%s = { %u, %u, %u },\n", color_labels[j], g->color_table[j].r, g->color_table[j].g, g->color_table[j].b);

			}
			fprintf(cfg_file, "}\n");
			break;

		case WINDOW_OPACITY:
			fprintf(cfg_file, "%d\n", g->color_table[NK_COLOR_WINDOW].a);
			break;
		}
	}
}

int convert_str2enum(lua_State* L, int idx, char* name, const char** enum_names, int* enum_values, int len)
{
	int type;
	if ((type = lua_type(L, idx)) == LUA_TSTRING) {
		const char* value = lua_tostring(L, -1);
		// TODO can I call lua_pop here without invalidating value?
		for (int i=0; i<len; i++) {
			if (!strcasecmp(value, enum_names[i])) {
				lua_pop(L, 1);
				return enum_values[i];
			}
		}
		// TODO list allowed values in error?
		error(L, "invalid value for enum %s: '%s'\n", name, value);
	} else {
		error(L, "%s is a %s not a string, expected a string value for enum\n", name, lua_typename(L, type));
	}

	// should never get here, just getting rid of compiler warning
	return -1;
}

int load_fullscreen_gui(lua_State* L, int dflt)
{
	const char* enum_names[] = { "delay", "always", "never" };
	int enum_values[] = { 0, 1, 2 };

	if (!lua_getglobal(L, "fullscreen_gui")) {
		return dflt;
	}
	return convert_str2enum(L, -1, "fullscreen_gui", enum_names, enum_values, 3);
}

// TODO better function names
// color value is at index
void convert_color(lua_State* L, Color* c, int index)
{
	if (lua_isstring(L, index)) {
		const char* colorname = lua_tostring(L, index);
		int i;
		for (i=0; colortable[i].name; i++) {
			if (!strcasecmp(colorname, colortable[i].name)) {
				break;
			}
		}
		if (!colortable[i].name) {
			error(L, "invalid color name (%s)\n", colorname);
		} else {
			*c = colortable[i].c;
		}
	} else if (lua_istable(L, index)) {
		if (lua_rawlen(L, index) >= 3) {
			lua_geti(L, index, 1);
			c->r = convert_color_field(L, -1);
			lua_geti(L, index-1, 2);
			c->g = convert_color_field(L, -1);
			lua_geti(L, index-2, 3);
			c->b = convert_color_field(L, -1);

			lua_pop(L, 3);
		} else {
			if (lua_getfield(L, -1, "red") != LUA_TNIL) {
				lua_pop(L, 1);
				c->r = get_color_field(L, "red");
				c->g = get_color_field(L, "green");
				c->b = get_color_field(L, "blue");
			} else if (lua_getfield(L, -1, "r") != LUA_TNIL) {
				lua_pop(L, 1);
				c->r = get_color_field(L, "r");
				c->g = get_color_field(L, "g");
				c->b = get_color_field(L, "b");
			}
		}
	} else {
		error(L, "Expected color is not a table or valid color string\n");
	}
}

int load_color(lua_State* L, const char* name, Color* c)
{
	if (!lua_getglobal(L, name)) {
		lua_pop(L, 1);
		return 0;
	}
	convert_color(L, c, -1);
	return 1;
}

// field is at index
int convert_color_field(lua_State* L, int index)
{
	int result = 0;

	if (lua_type(L, index) != LUA_TNUMBER) {
		error(L, "invalid component in color, must be a number\n");
	}

	if (lua_isinteger(L, index)) {
		result = lua_tointeger(L, index);
		//printf("Is integer: %d\n", result);
	} else {
		float tmp = (float)lua_tonumber(L, index);
		//printf("is float: %f\n", tmp);
		result = (int)(tmp*255);
	}

	// clamp here?
	if (result < 0) result = 0;
	if (result > 255) result = 255;

	return result;
}

// assume table is on top of the stack
int get_color_field(lua_State* L, const char* key)
{
	lua_getfield(L, -1, key);
	int ret = convert_color_field(L, -1);
	lua_pop(L, 1);
	return ret;
}

// assume table is top of stack
void set_color_field(lua_State* L, const char* index, int value)
{
	lua_pushstring(L, index); // key ie "red"
	lua_pushnumber(L, (double)value / MAX_COLOR);
	lua_settable(L, -3);
}

void set_color(lua_State* L, ColorEntry* ct)
{
	lua_newtable(L);
	set_color_field(L, "red", ct->c.r);
	set_color_field(L, "green", ct->c.g);
	set_color_field(L, "blue", ct->c.b);
	lua_setglobal(L, ct->name);
}

char* fullscreen_gui_str(int fsg_enum)
{
	switch (fsg_enum) {
	case DELAY: return "delay";
	case ALWAYS: return "always";
	case NEVER: return "never";
	default: return "INVALID";
	}
}

void do_gui_colors(lua_State* L)
{
	if (lua_getglobal(L, "gui_colors") == LUA_TTABLE) {
		map_table(L, -1, handle_gui_colors, NULL);
		// do this after gui_colors since that does 255 for all alphas
		g->color_table[NK_COLOR_WINDOW].a = try_int_clamp_dflt(L, "window_opacity", 0, 255, DFLT_WINDOW_OPACITY);
	}
	lua_pop(L, 1);
}



