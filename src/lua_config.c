#include <stdio.h>
#include <string.h>

#include "lua_helper.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define MAX_COLOR 255

// TODO figure out how to handle cachedir needs changes in main()
enum {
	GUI_SCALE,
	BACKGROUND,
	SLIDE_DELAY,
	HIDE_GUI_DELAY,
	FULLSCREEN_GUI,
	THUMB_ROWS,
	THUMB_COLS,
	SHOW_INFO_BAR,
	X_DELETES_THUMB,
	RELATIVE_OFFSETS,
	NUM_KEYS,
	CACHE_DIR,
};

char* keys[] =
{
	"gui_scale",
	"background",
	"slide_delay",
	"hide_gui_delay",
	"fullscreen_gui",
	"thumb_rows",
	"thumb_cols",
	"show_info_bar",
	"x_deletes_thumb",
	"relative_offsets",
	"cache_dir"
};

// TODO better name
typedef struct Color
{
	u8 r,g,b;
} Color;

typedef struct ColorEntry {
	char* name;
	Color c;
} ColorEntry;

void load_color(lua_State* L, const char* name, Color* c);

// assume table (a color) is on top of the stack
int get_color_field(lua_State* L, const char* key);
void set_color_field(lua_State* L, const char* index, int value);
void set_color(lua_State* L, ColorEntry* ct);

int load_fullscreen_gui(lua_State* L);

char* fullscreen_gui_str(int fsg_enum);
void write_config(FILE* cfg_file);

ColorEntry colortable[] =
{
	{ "WHITE", { MAX_COLOR, MAX_COLOR, MAX_COLOR } },
	{ "RED",   { MAX_COLOR, 0, 0 } },
	{ "GREEN", { 0, MAX_COLOR, 0 } },
	{ "BLUE",  { 0, 0, MAX_COLOR } },
	{ "BLACK", { 0, 0, 0 } },
	{ NULL,    { 0, 0, 0} }     // sentinal
};

int read_config_file(char* filename)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_dofile(L, filename)) {
		//error(L, "cannot run config. file: %s\n", lua_tostring(L, -1));
		perror("Cannot run lua config file");
		fprintf(stderr, "lua error: %s\n", lua_tostring(L, -1));
		lua_close(L);
		return 0;
	}

	g->x_scale = g->y_scale = get_global_number(L, "gui_scale");

	g->slide_delay = get_global_int(L, "slide_delay");
	g->gui_delay = get_global_int(L, "hide_gui_delay");
	g->thumb_rows = get_global_int(L, "thumb_rows");
	g->thumb_cols = get_global_int(L, "thumb_cols");

	// enum
	g->fullscreen_gui = load_fullscreen_gui(L);

	Color background = {0};
	load_color(L, "background", &background);
	g->bg = nk_rgb(background.r,background.g,background.b); // clamps for us

	g->show_infobar = get_global_bool(L, "show_info_bar");
	g->thumb_x_deletes  = get_global_bool(L, "x_deletes_thumb");
	g->ind_mm = get_global_bool(L, "relative_offsets");

	/*
	cache_dir = NULL;
	if (!get_global_strbuf(L, "cache_dir", cachedir_buf, STRBUF_SZ)) {
		fprintf(stderr, "cache_dir string is too long!\n");
		return 0;
	}
	cache_dir = cachedir_buf;
	*/

	// For debug purposes
	write_config(stdout);

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

		fprintf(cfg_file, "%s = ", keys[i]);

		switch (i) {
		case GUI_SCALE:
			// TODO either save x and y scale separately or combine
			// into a single member g->scale if they're always the same
			fprintf(cfg_file, "%.1f\n", g->x_scale);
			break;
		case BACKGROUND:
			fprintf(cfg_file, "{ red = %u, green = %u, blue = %u }\n", g->bg.r, g->bg.g, g->bg.b);
			break;
		case SLIDE_DELAY:
			fprintf(cfg_file, "%d\n", g->slide_delay);
			break;
		case HIDE_GUI_DELAY:
			fprintf(cfg_file, "%d\n", g->gui_delay);
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
		case X_DELETES_THUMB:
			fprintf(cfg_file, "%s\n", bool_str[g->thumb_x_deletes]);
			break;
		case RELATIVE_OFFSETS:
			fprintf(cfg_file, "%s\n", bool_str[g->ind_mm]);
			break;

		case CACHE_DIR:
			//fprintf(cfg_file, "%s\n", g->cachedir);
			break;
		}
	}
}

int load_str2enum(lua_State* L, const char* name, const char** enum_names, int* enum_values, int len)
{
	lua_getglobal(L, name);
	if (lua_isstring(L, -1)) {
		const char* value = lua_tostring(L, -1);
		for (int i=0; i<len; i++) {
			if (!strcasecmp(value, enum_names[i])) {
				return enum_values[i];
			}
		}
		error(L, "invalid value for enum %s: '%s'\n", name, value);
	} else {
		stackDump(L);
		error(L, "'%s' is not a string\n", name);
	}
	return -1;
}

int load_fullscreen_gui(lua_State* L)
{
	const char* enum_names[] = { "delay", "always", "never" };
	int enum_values[] = { 0, 1, 2 };
	return load_str2enum(L, "fullscreen_gui", enum_names, enum_values, 3);
}

void load_color(lua_State* L, const char* name, Color* c)
{
	lua_getglobal(L, name);
	if (lua_isstring(L, -1)) {
		const char* colorname = lua_tostring(L, -1);
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
	} else if (lua_istable(L, -1)) {
		c->r = get_color_field(L, "red");
		c->g = get_color_field(L, "green");
		c->b = get_color_field(L, "blue");
	} else {
		error(L, "'%s' is not a table\n", name);
	}
}

// assume table background is on top of the stack
int get_color_field(lua_State* L, const char* key)
{
	int result;

	// get background[key]
	if (lua_getfield(L, -1, key) != LUA_TNUMBER) {
		error(L, "invalid component in background color\n");
	}
	if (lua_isinteger(L, -1)) {
		result = lua_tointeger(L, -1);
		//printf("Is integer: %d\n", result);
	} else {
		float tmp = (float)lua_tonumber(L, -1);
		//printf("is float: %f\n", tmp);
		result = (int)(tmp*255);
	}

	// clamp here?
	if (result < 0) result = 0;
	if (result > 255) result = 255;

	lua_pop(L, 1);

	return result;
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


