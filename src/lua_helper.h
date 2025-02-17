#ifndef LUA_HELPER_H
#define LUA_HELPER_H

#include "lua.h"
#include "lauxlib.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(lua_State* L, const char *fmt, ...);

int get_int(lua_State* L, const char* var);
int get_int_clamp(lua_State* L, const char* var, int min, int max);

double get_number(lua_State* L, const char* var);
double get_number_clamp(lua_State* L, const char* var, double min, double max);

int get_bool(lua_State* L, const char* var);
char* get_str(lua_State* L, const char* var);
int get_strbuf(lua_State* L, const char* var, char* buf, int buf_sz);
int get_str_array(lua_State* L, const char* var, char*** out_array);

int try_int(lua_State* L, const char* var, int* success);
int try_int_clamp_dflt(lua_State* L, const char* var, int min, int max, int dflt);

double try_number(lua_State* L, const char* var, int* success);
double try_number_clamp_dflt(lua_State* L, const char* var, double min, double max, double dflt);

int try_bool_dflt(lua_State* L, const char* var, int dflt);
int try_bool(lua_State* L, const char* var, int* success);

char* try_str(lua_State* L, const char* var, int* success);
int try_strbuf(lua_State* L, const char* var, char* buf, int buf_sz);
int try_str_array(lua_State* L, const char* var, char*** out_array);


void call_va(lua_State* L, const char* func, const char* sig, ...);
void stackDump(lua_State* L);


void error(lua_State* L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);

	lua_close(L);
	exit(EXIT_FAILURE);
}

void lh_log(lua_State* L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
}

int try_int_clamp_dflt(lua_State* L, const char* var, int min, int max, int dflt)
{
	int success = 0;
	int ret = try_int(L, var, &success);
	if (success) {
		if (ret < min) {
			ret = min;
		}
		if (ret > max) {
			ret = max;
		}
	} else {
		ret = dflt;
	}
	return ret;
}

int try_int(lua_State* L, const char* var, int* success)
{
	int isnum, result;
	if (success) {
		*success = 0;
	}

	if (!lua_getglobal(L, var)) {
		lua_pop(L, 1);  // remove result from stack
		return 0;
	}
	result = (int)lua_tointegerx(L, -1, &isnum);
	lua_pop(L, 1);  // remove result from stack
	if (!isnum) {
		lh_log(L, "'%s', should be an integer\n", var);
		return 0;
	}
	
	if (success) {
		*success = 1;
	}
	return result;
}

int get_int_clamp(lua_State* L, const char* var, int min, int max)
{
	int ret = get_int(L, var);
	if (ret < min) {
		ret = min;
	}
	if (ret > max) {
		ret = max;
	}
	return ret;
}

int get_int(lua_State* L, const char* var)
{
	int isnum, result;
	lua_getglobal(L, var);
	result = (int)lua_tointegerx(L, -1, &isnum);
	if (!isnum) {
		error(L, "'%s', should be an integer\n", var);
	}
	lua_pop(L, 1);  // remove result from stack
	return result;
}

double try_number(lua_State* L, const char* var, int* success)
{
	int isnum;
	double result;
	if (success) {
		*success = 0;
	}

	if (!lua_getglobal(L, var)) {
		lua_pop(L, 1);  // remove result from stack
		return 0;
	}
	result = (double)lua_tonumberx(L, -1, &isnum);
	lua_pop(L, 1);  // remove result from stack
	if (!isnum) {
		lh_log(L, "'%s', should be an number\n", var);
		return 0;
	}
	
	if (success) {
		*success = 1;
	}
	return result;
}

double try_number_clamp_dflt(lua_State* L, const char* var, double min, double max, double dflt)
{
	int success = 0;
	double ret = try_number(L, var, &success);
	if (success) {
		if (ret < min) {
			ret = min;
		}
		if (ret > max) {
			ret = max;
		}
	} else {
		ret = dflt;
	}
	return ret;
}

double get_number(lua_State* L, const char* var)
{
	int isnum;

	lua_getglobal(L, var);
	double result = (double)lua_tonumberx(L, -1, &isnum);
	if (!isnum) {
		error(L, "'%s', should be a number\n", var);
	}
	lua_pop(L, 1);  // remove result from stack
	return result;
}

double get_number_clamp(lua_State* L, const char* var, double min, double max)
{
	double ret = get_number(L, var);
	if (ret < min) {
		ret = min;
	}
	if (ret > max) {
		ret = max;
	}
	return ret;
}

int try_bool_dflt(lua_State* L, const char* var, int dflt)
{
	int success = 0;
	int ret = try_bool(L, var, &success);
	if (success) {
		return ret;
	}
	return dflt;
}

int try_bool(lua_State* L, const char* var, int* success)
{
	if (success) {
		*success = 0;
	}
	int type;
	if (!(type = lua_getglobal(L, var))) {
		lua_pop(L, 1);  // remove result from stack
		return 0;
	}
	if (type != LUA_TBOOLEAN) {
		lh_log(L, "'%s', should be a boolean\n", var);
		lua_pop(L, 1);  // remove result from stack
		return 0;
	}
	int result = lua_toboolean(L, -1);
	lua_pop(L, 1);  // remove result from stack
	
	if (success) {
		*success = 1;
	}
	return result;
}

int get_bool(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (!lua_isboolean(L, -1)) {
		error(L, "'%s', should be a boolean\n", var);
	}
	int result = lua_toboolean(L, -1);
	lua_pop(L, 1);  // remove result from stack
	return result;
}

char* try_str(lua_State* L, const char* var, int* success)
{
	if (success) *success = 0;
	int type;
	if (!(type = lua_getglobal(L, var))) {
		lua_pop(L, 1);  // remove result from stack
		return NULL;
	}
	if (type != LUA_TSTRING) {
		lh_log(L, "'%s', should be a string\n", var);
		lua_pop(L, 1);  // remove result from stack
		return NULL;
	}
	char* result = strdup(lua_tostring(L, -1));
	lua_pop(L, 1);  // remove result from stack
	return result;
}

char* get_str(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (lua_type(L, -1) != LUA_TSTRING) {
		error(L, "'%s', should be a string\n", var);
	}
	char* result = strdup(lua_tostring(L, -1));
	lua_pop(L, 1);  // remove result from stack
	return result;
}

int try_strbuf(lua_State* L, const char* var, char* buf, int buf_sz)
{
	int type;
	if (!(type = lua_getglobal(L, var))) {
		lua_pop(L, 1);  // remove result from stack
		return 0;
	}

	if (type != LUA_TSTRING) {
		lh_log(L, "'%s', should be a string\n", var);
		lua_pop(L, 1);  // remove result from stack
		return 0;
	}

	size_t len;
	const char* result = lua_tolstring(L, -1, &len);
	lua_pop(L, 1);  // remove result from stack
	if (len >= buf_sz) {
		lh_log(L, "'%s' is too long for provided string buffer\n", var);
		return 0;
	}
	memcpy(buf, result, len+1);

	return 1;
}

int get_strbuf(lua_State* L, const char* var, char* buf, int buf_sz)
{
	if (!lua_getglobal(L, var)) {
		error(L, "global var '%s' does not exist\n", var);
	}

	if (lua_type(L, -1) != LUA_TSTRING) {
		error(L, "'%s', should be a string\n", var);
	}
	size_t len;
	const char* result = lua_tolstring(L, -1, &len);
	if (len >= buf_sz) {
		error(L, "'%s' is too long for provided string buffer\n", var);
	}
	memcpy(buf, result, len+1);

	lua_pop(L, 1);  // remove result from stack
	return 1;
}

int try_str_array(lua_State* L, const char* var, char*** out_array)
{
	int type;

	if (!out_array) {
		return 0;
	}

	if (!(type = lua_getglobal(L, var))) {
		lua_pop(L, 1); // remove nil from stack
		return 0;
	}

	if (type != LUA_TTABLE) {
		lua_pop(L, 1);
		lh_log(L, "'%s', should be a table (array of strings)\n", var);
		return 0;
	}

	int n_strs = lua_rawlen(L, -1);
	char** arr = malloc(n_strs * sizeof(char*));

	const char* tmp;
	size_t len;

	for (int i=1; i<=n_strs; i++) {
		if (lua_geti(L, -1, i) != LUA_TSTRING) {
			lh_log(L, "expected a string\n");
			// TODO fail entirely here or let it attempt to convert below?
		}

		// inline strdup that also handles
		tmp = lua_tolstring(L, -1, &len);
		arr[i-1] = malloc(len+1);
		memcpy(arr[i-1], tmp, len+1);

		lua_pop(L, 1); // remove string
	}

	// Pop table
	lua_pop(L, 1);

	*out_array = arr;

	return n_strs;

}

int get_str_array(lua_State* L, const char* var, char*** out_array)
{
	// assert(out_array); //?
	if (!out_array) {
		return 0;
	}

	if (!lua_getglobal(L, var)) {
		error(L, "global var '%s' does not exist\n", var);
	}

	if (!lua_istable(L, -1)) {
		error(L, "'%s', should be a table (array of strings)\n", var);
	}

	int n_strs = lua_rawlen(L, -1);
	char** arr = malloc(n_strs * sizeof(char*));

	const char* tmp;
	size_t len;

	for (int i=1; i<=n_strs; i++) {
		if (lua_geti(L, -1, i) != LUA_TSTRING) {
			error(L, "expected a string\n");
		}

		// inline strdup that also handles
		tmp = lua_tolstring(L, -1, &len);
		arr[i-1] = malloc(len+1);
		memcpy(arr[i-1], tmp, len+1);

		lua_pop(L, 1);
	}

	lua_pop(L, 1); // remove table
	*out_array = arr;

	return n_strs;
}


void call_va(lua_State* L, const char* func, const char* sig, ...)
{
	va_list v1;
	int narg, nres;

	va_start(v1, sig);
	lua_getglobal(L, func);  // push func

	// for each arg
	for (narg = 0; *sig; narg++) {
		// check stack space
		// if (!lua_checkstack(L, 1)) {
		// 	break;
		// }
		//
		// TODO raises an error if it cannot grow
		luaL_checkstack(L, 1, "too many arguments");

		switch (*sig++) {
		case 'f':
			lua_pushnumber(L, va_arg(v1, double));
			break;
		case 'd':
		case 'i':
			lua_pushinteger(L, va_arg(v1, int));
			break;
		case 's':
			lua_pushstring(L, va_arg(v1, char*));
			break;
		case 'b':
			lua_pushboolean(L, va_arg(v1, int));
		case '>':
			goto endargs;
		default:
			error(L, "invalid option (%c)\n", sig[-1]);
		}
	}
endargs:
	
	nres = strlen(sig); // number of expected results

	if (lua_pcall(L, narg, nres, 0)) {
		error(L, "error calling '%s': %s\n", func, lua_tostring(L, -1));
	}

	nres = -nres;  // stack index of first result
	int isnum;
	double dres;
	int ires, bool_res;
	const char* sres;

	// for each result
	while (*sig) {
		switch (*sig++) {
		case 'f':
			dres = lua_tonumberx(L, nres, &isnum);
			if (!isnum) {
				error(L, "wrong result type\n");
			}
			*va_arg(v1, double*) = dres;
			break;
		case 'd':
		case 'i':
			ires = lua_tointegerx(L, nres, &isnum);
			if (!isnum) {
				error(L, "wrong result type\n");
			}
			*va_arg(v1, int*) = ires;
			break;
		case 's':
			if (!(sres = lua_tostring(L, nres))) {
				error(L, "wrong result type\n");
			}
			*va_arg(v1, char**) = (char*)sres;
			break;
		case 'b':
			if (!(bool_res = lua_toboolean(L, nres))) {
				error(L, "wrong result type\n");
			}
			*va_arg(v1, int*) = bool_res;
			break;
		default:
			error(L, "invalid option (%c)\n", sig[-1]);
		}
		nres++;
	}
	
	va_end(v1);
}

void stackDump(lua_State* L)
{
	int i;
	int top = lua_gettop(L);
	for (i=1; i<=top; i++) {
		int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			printf("'%s'", lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			printf(lua_toboolean(L, i) ? "true" : "false");
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, i)) {
				printf("%lld", lua_tointeger(L, i));
			} else {
				printf("%g", lua_tonumber(L, i));
			}
			break;
			/*
		case LUA_TNIL:
			printf("nil");
			break;
			*/
		default:
			printf("%s", lua_typename(L, t));
			break;
		}
		printf("  ");
	}
	putchar('\n');
}

// TODO better name
typedef void (*table_member_callback)(lua_State* L, void* userdata);

// table is at stack idx
void map_table(lua_State* L, int idx, table_member_callback callback, void* userdata)
{
	lua_pushnil(L); // first "key"
	idx--;
	while (lua_next(L, idx)) {
		callback(L, userdata);
		lua_pop(L, 1);  // pop value, keeps key
	}
}

void loop_globals(lua_State* L, table_member_callback callback, void* userdata)
{
	lua_pushglobaltable(L);
	map_table(L, -1, callback, userdata);
}


#endif

