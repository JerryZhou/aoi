#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "aoi.h"

#if LUA_VERSION_NUM < 502
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif

#define AOI_MAP "cls{aoi_map}"
#define AOI_UNIT "cls{aoi_unit}"

#define CHECK_AOI_UNIT(L, idx)\
	(*(iunit **) luaL_checkudata(L, idx, AOI_UNIT))

#define CHECK_AOI_MAP(L, idx)\
	(*(imap **) luaL_checkudata(L, idx, AOI_MAP))


#define LUA_BIND_META(L, type_t, ptr, mname) do {                   \
	type_t **my__p = lua_newuserdata(L, sizeof(void *));        \
	*my__p = ptr;                                               \
	luaL_getmetatable(L, mname);                                \
	lua_setmetatable(L, -2);                                    \
} while(0)


/* {{ map */
static int lua__map_new(lua_State *L)
{
	imap *map = NULL;
	int divide = 1;
	ipos pos;
	isize size;

	size.w = luaL_checknumber(L, 1);
	size.h = luaL_checknumber(L, 2);

	divide = luaL_optint(L, 3, 1);

	pos.x = luaL_optnumber(L, 4, 0);
	pos.y = luaL_optnumber(L, 5, 0);

	map = imapmake(&pos, &size, divide);
	if (map == NULL) {
		return 0;
	}
	LUA_BIND_META(L, imap, map, AOI_MAP);
	return 1;
}

static int lua__map_gc(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	if (map != NULL) {
		imapfree(map);
	}
	return 0;
}

/* method of map */

static int lua__map_get_size(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	lua_pushnumber(L, map->size.w);
	lua_pushnumber(L, map->size.h);
	return 2;
}

static int lua__map_get_state(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	lua_newtable(L);

	lua_pushnumber(L, map->state.nodecount);
	lua_setfield(L, -2, "nodecount");

	lua_pushnumber(L, map->state.leafcount);
	lua_setfield(L, -2, "leafcount");

	lua_pushnumber(L, map->state.unitcount);
	lua_setfield(L, -2, "unitcount");

	return 1;
}

static int lua__map_unit_add(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	imapaddunit(map, unit);
	return 0;
}

static int lua__map_unit_del(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	imapremoveunit(map, unit);
	return 0;
}

static int lua__map_unit_move(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	imapremoveunit(map, unit);
	return 0;
}

static int lua__map_unit_update(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	imapupdateunit(map, unit);
	return 0;
}

static int lua__map_unit_search(lua_State *L)
{
	ireal range;
	isearchresult *result = NULL;
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	range = luaL_checknumber(L, 3);
	
	result = isearchresultmake();
	imapsearchfromunit(map, unit, result, range);
	isearchresultfree(result);

	return 0;
}


static int lua__map_searchcircle(lua_State *L)
{
	ipos pos;
	ireal range;
	isearchresult *result = NULL;

	imap * map = CHECK_AOI_MAP(L, 1);
	pos.x = luaL_checknumber(L, 2);
	pos.y = luaL_checknumber(L, 3);
	range = (ireal)luaL_checknumber(L, 4);

	result = isearchresultmake();
	imapsearchfrompos(map, &pos, result, range);
	isearchresultfree(result);

	return 0;
}


/* }} map */


/* {{ iunit */
static int lua__unit_new(lua_State *L)
{
	iunit *u = NULL;
	iid id = (iid)luaL_checknumber(L, 1);
	lua_Number x = luaL_checknumber(L, 2);
	lua_Number y = luaL_checknumber(L, 3);
	u = imakeunit(id, (ireal)x, (ireal)y);
	if (u == NULL) {
		return 0;
	}
	LUA_BIND_META(L, iunit, u, AOI_UNIT);
	return 1;
}

static int lua__unit_gc(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	if (unit != NULL) {
		ifreeunit(unit);
	}
	return 0;
}

static int lua__unit_get_id(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	lua_pushnumber(L, unit->id);
	return 1;
}

static int lua__unit_get_pos(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	lua_pushnumber(L, unit->pos.x);
	lua_pushnumber(L, unit->pos.y);
	return 2;
}

static int lua__unit_get_tick(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	lua_pushnumber(L, (lua_Number)unit->tick);
	return 1;
}

/* }} iunit */

static int opencls__map(lua_State *L)
{
	luaL_Reg lmethods[] = {
		{"get_size", lua__map_get_size},
		{"get_state", lua__map_get_state},
		{"search_circle", lua__map_searchcircle},

		{"unit_search", lua__map_unit_search},
		{"unit_add", lua__map_unit_add},
		{"unit_del", lua__map_unit_del},
		{"unit_move", lua__map_unit_move},
		{"unit_update", lua__map_unit_update},
		{NULL, NULL},
	};
	luaL_newmetatable(L, AOI_MAP);
	lua_newtable(L);
	luaL_register(L, NULL, lmethods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction (L, lua__map_gc);
	lua_setfield (L, -2, "__gc");
	return 1;
}

static int opencls__unit(lua_State *L)
{
	luaL_Reg lmethods[] = {
		{"get_pos", lua__unit_get_pos},
		{"get_id", lua__unit_get_id},
		{"get_tick", lua__unit_get_tick},
		{NULL, NULL},
	};
	luaL_newmetatable(L, AOI_UNIT);
	lua_newtable(L);
	luaL_register(L, NULL, lmethods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction (L, lua__unit_gc);
	lua_setfield (L, -2, "__gc");
	return 1;
}

int luaopen_laoi(lua_State* L)
{
	luaL_Reg lfuncs[] = {
		{"new_map", lua__map_new},
		{"new_unit", lua__unit_new},
		{NULL, NULL},
	};
	opencls__map(L);
	opencls__unit(L);
	luaL_newlib(L, lfuncs);
	return 1;
}

