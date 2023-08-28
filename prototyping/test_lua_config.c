
#include "myinttypes.h"

#include "lua_helper.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <stdio.h>
#include <string.h>

#define MAX_COLOR 255
#define STRBUF_SZ 1024

enum { DELAY, ALWAYS, NEVER };

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

ColorEntry colortable[] =
{
	{ "WHITE", { MAX_COLOR, MAX_COLOR, MAX_COLOR } },
	{ "RED",   { MAX_COLOR, 0, 0 } },
	{ "GREEN", { 0, MAX_COLOR, 0 } },
	{ "BLUE",  { 0, 0, MAX_COLOR } },
	{ "BLACK", { 0, 0, 0 } },
	{ NULL,    { 0, 0, 0} }     // sentinal
};

float gui_scale;
Color background;
int slide_delay;
int hide_gui_delay;
int thumb_rows;
int thumb_cols;

// enum, string in config
int fullscreen_gui;


// bools
int show_info_bar;
int x_deletes_thumb;
int relative_offsets;

// string
char* cache_dir;
char cachedir_buf[STRBUF_SZ];

int main(int argc, char** argv)
{
	char buff[256];

	if (argc != 2) {
		printf("usage: %s lua_config_file\n", argv[0]);
		return 0;
	}

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);


	if (luaL_dofile(L, argv[1])) {
		error(L, "cannot run config. file: %s", lua_tostring(L, -1));
	}

	gui_scale = get_global_number(L, "gui_scale");

	slide_delay = get_global_int(L, "slide_delay");
	hide_gui_delay = get_global_int(L, "hide_gui_delay");
	thumb_rows = get_global_int(L, "thumb_rows");
	thumb_cols = get_global_int(L, "thumb_cols");

	// enum
	fullscreen_gui = load_fullscreen_gui(L);

	load_color(L, "background", &background);

	show_info_bar = get_global_bool(L, "show_info_bar");
	x_deletes_thumb  = get_global_bool(L, "x_deletes_thumb");
	relative_offsets = get_global_bool(L, "relative_offsets");

	cache_dir = NULL;
	if (!get_global_strbuf(L, "cache_dir", cachedir_buf, STRBUF_SZ)) {
		fprintf(stderr, "cache_dir string is too long!\n");
		return 0;
	}
	cache_dir = cachedir_buf;

	printf("%f\n", gui_scale);
	printf("%d,%d,%d\n", background.r, background.g, background.b);
	printf("%d\n", slide_delay);
	printf("%d\n", hide_gui_delay);
	printf("%d %s\n", fullscreen_gui, fullscreen_gui_str(fullscreen_gui));
	printf("%d\n", thumb_rows);
	printf("%d\n", thumb_cols);
	printf("%d\n", show_info_bar);
	printf("%d\n", x_deletes_thumb);
	printf("%d\n", relative_offsets);
	printf("'%s'\n", cache_dir);


	lua_close(L);
	return 0;
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
		error(L, "invalid value for enum %s: '%s'", name, value);
	} else {
		error(L, "'%s' is not a string", name);
	}
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
			error(L, "invalid color name (%s)", colorname);
		} else {
			*c = colortable[i].c;
		}
	} else if (lua_istable(L, -1)) {
		c->r = get_color_field(L, "red");
		c->g = get_color_field(L, "green");
		c->b = get_color_field(L, "blue");
	} else {
		error(L, "'%s' is not a table", name);
	}
}

// assume table background is on top of the stack
int get_color_field(lua_State* L, const char* key)
{
	int result, isnum;

	// get background[key]
	if (lua_getfield(L, -1, key) != LUA_TNUMBER) {
		error(L, "invalid component in background color");
	}
	if (lua_isinteger(L, -1)) {
		result = lua_tointeger(L, -1);
		printf("Is integer: %d\n", result);
	} else {
		float tmp = (float)lua_tonumber(L, -1);
		printf("is float: %f\n", tmp);
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


