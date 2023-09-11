#ifndef LUA_HELPER_H
#define LUA_HELPER_H

#include "lua.h"
#include "lauxlib.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(lua_State* L, const char *fmt, ...);

int getglobint(lua_State* L, const char* var);
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


int get_global_int(lua_State* L, const char* var)
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

double get_global_number(lua_State* L, const char* var)
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

int get_global_bool(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (!lua_isboolean(L, -1)) {
		error(L, "'%s', should be a boolean\n", var);
	}
	int result = lua_toboolean(L, -1);
	lua_pop(L, 1);  // remove result from stack
	return result;
}

char* get_global_str(lua_State* L, const char* var)
{
	lua_getglobal(L, var);
	if (lua_type(L, -1) != LUA_TSTRING) {
		error(L, "'%s', should be a string\n", var);
	}
	char* result = strdup(lua_tostring(L, -1));
	lua_pop(L, 1);  // remove result from stack
	return result;
}

int get_global_strbuf(lua_State* L, const char* var, char* buf, int buf_sz)
{
	lua_getglobal(L, var);
	if (lua_type(L, -1) != LUA_TSTRING) {
		error(L, "'%s', should be a string\n", var);
	}
	size_t len;
	const char* result = lua_tolstring(L, -1, &len);
	if (len >= buf_sz) {
		// error or log?
		return 0;
	}
	strcpy(buf, result);

	lua_pop(L, 1);  // remove result from stack
	return 1;
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


#endif

