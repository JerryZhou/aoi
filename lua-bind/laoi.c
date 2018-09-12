/*
 * lua-binding for libaoi
 *
 * Copyright (C) 2016, zhupeng<rocaltair@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "aoi.h"


 
/** 
 * #define ENABLE_LAOI_DEBUG
 */


#ifdef ENABLE_LAOI_DEBUG
# define DLOG(fmt, ...) fprintf(stderr, "<laoi>" fmt, ##__VA_ARGS__)
#else
# define DLOG(...)
#endif

#if (LUA_VERSION_NUM < 502)
# ifndef luaL_newlib
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
# endif
# ifndef lua_getuservalue
#  define lua_getuservalue(L, n) lua_getfenv(L, n)
# endif
# ifndef lua_setuservalue
#  define lua_setuservalue(L, n) lua_setfenv(L, n)
# endif
#endif

#define AOI_MAP "cls{aoi_map}"
#define AOI_UNIT "cls{aoi_unit}"

#define AOI_UNITS_MAP_NAME "units"

#define CHECK_AOI_UNIT(L, idx)\
	(*(iunit **) luaL_checkudata(L, idx, AOI_UNIT))

#define CHECK_AOI_MAP(L, idx)\
	(*(imap **) luaL_checkudata(L, idx, AOI_MAP))

#define LUA_SETMACRO(L, index, name, var) 	\
	(lua_pushstring(L, name), 		\
	lua_pushinteger(L, var), 		\
	lua_settable(L, index >= 0 ? index : index - 2))

#define LUA_SET_ENUM(L, index, name) LUA_SETMACRO(L, index, #name, name)

#define LUA_BIND_META(L, type_t, ptr, mname) do {                   \
	type_t **my__p = lua_newuserdata(L, sizeof(void *));        \
	*my__p = ptr;                                               \
	luaL_getmetatable(L, mname);                                \
	lua_setmetatable(L, -2);                                    \
} while(0)

#ifndef __iallmeta
# define __iallmeta(XX)
#endif

#if LUA_VERSION_NUM >= 503
/* one can simply enable LUA_COMPAT_5_2 to be backward compatible.
However, this does not work when we are trying to use system-installed lua,
hence these redefines
*/
#define luaL_optlong(L,n,d)     ((long)luaL_optinteger(L, (n), (d)))
#define luaL_optint(L,n,d)  ((int)luaL_optinteger(L, (n), (d)))
#define luaL_checklong(L,n)     ((long)luaL_checkinteger(L, (n)))
#define luaL_checkint(L,n)      ((int)luaL_checkinteger(L, (n)))
#endif


/* {{ map */

static int lua__map_new(lua_State *L)
{
	imap *map = NULL;
	int divide = 1;
	ipos pos = {0.0, 0.0};
	isize size;
	int top = lua_gettop(L);

	luaL_checktype(L, 1, LUA_TTABLE);
	divide = luaL_optint(L, 2, 1);

	if (lua_type(L, 3) == LUA_TTABLE) {
		lua_rawgeti(L, 3, 1);
		pos.x = luaL_optnumber(L, -1, 0);
		lua_rawgeti(L, 3, 2);
		pos.y = luaL_optnumber(L, -1, 0);
	}
	lua_settop(L, top);

	lua_rawgeti(L, 1, 1);
	size.w = luaL_checknumber(L, -1);
	lua_rawgeti(L, 1, 2);
	size.h = luaL_checknumber(L, -1);

	map = imapmake(&pos, &size, divide);
	if (map == NULL) {
		return 0;
	}
	LUA_BIND_META(L, imap, map, AOI_MAP);

	/* bind a fenv table onto map, to save units */
	lua_newtable(L);
	lua_setuservalue(L, -2);

	DLOG("new map,map=%p\n", map);
	return 1;
}

static int lua__map_gc(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	if (map != NULL) {
		DLOG("map gc,map=%p\n", map);
		imapfree(map);
	}
	return 0;
}

static int lua__map_dump(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	int require = luaL_optint(L, 2, EnumNodePrintStateAll);
	_aoi_print(map, require);
	return 0;
}

static int lua__map_dumpstate(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	const char * intag = luaL_optstring(L, 2, "tag");
	const char * inhead = luaL_optstring(L, 3, "head");
	int require = luaL_optint(L, 4, EnumMapStateAll);
	imapstatedesc(map, require, intag, inhead);
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

	lua_pushnumber(L, (int)map->state.nodecount);
	lua_setfield(L, -2, "nodecount");

	lua_pushnumber(L, (int)map->state.leafcount);
	lua_setfield(L, -2, "leafcount");

	lua_pushnumber(L, (int)map->state.unitcount);
	lua_setfield(L, -2, "unitcount");

	return 1;
}

static int lua__map_unit_add(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);

	lua_getuservalue(L, 1);
	lua_pushinteger(L, (lua_Integer)unit->id);
	lua_rawget(L, -2);

	if (lua_isnil(L, -1)) {
		imapaddunit(map, unit);

		lua_pop(L, 1);
		lua_pushinteger(L, (lua_Integer)unit->id);
		lua_pushvalue(L, 2);
		lua_rawset(L, -3);
	}

	return 0;
}

static int lua__map_unit_del_byid(lua_State *L)
{
	iunit * unit = NULL;
	imap * map = CHECK_AOI_MAP(L, 1);
	iid id = (iid)luaL_checkinteger(L, 2);
	lua_settop(L, 2);

	lua_getuservalue(L, 1);
	lua_pushinteger(L, (lua_Integer)id);
	lua_rawget(L, -2);
	if (lua_isnoneornil(L, -1)) {
		DLOG("%s,id=%" PRId64 " not found\n", __FUNCTION__, id);
		return 0;
	}

	unit = CHECK_AOI_UNIT(L, -1);
	imapremoveunit(map, unit);

	lua_pushinteger(L, (lua_Integer)id);
	lua_pushnil(L);
	lua_rawset(L, 3);
	DLOG("%s,id=%" PRId64 " removed\n", __FUNCTION__, id);
	/* fprintf(stderr, "%s,id=%" PRId64 " removed\n", __FUNCTION__, id); */

	return 0;
}


static int lua__map_unit_del(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	imapremoveunit(map, unit);

	lua_getuservalue(L, 1);
	lua_pushinteger(L, (lua_Integer)unit->id);
	lua_pushnil(L);
	lua_rawset(L, -3);
	return 0;
}

static int lua__map_unit_update(lua_State *L)
{
	ipos pos;
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	(void) map;
	if (lua_type(L, 3) == LUA_TTABLE) {
		lua_rawgeti(L, 3, 1);
		pos.x = luaL_checknumber(L, -1);
		lua_rawgeti(L, 3, 2);
		pos.y = luaL_checknumber(L, -1);

                DLOG("unit move,id=%" PRId64 ",from=(%.2f,%.2f),to=(%.2f,%.2f)\n",
		     unit->id,
		     unit->pos.x, unit->pos.y,
		     pos.x, pos.y);
		unit->pos.x = pos.x;
		unit->pos.y = pos.y;
	}

	imapupdateunit(map, unit);
	return 0;
}

static int lua__map_get_units(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	(void)map;
	lua_getuservalue(L, 1);
	return 1;
}


static int lua__map_unit_search(lua_State *L)
{
	ireal range;
	isearchresult *result = NULL;
	irefjoint* joint = NULL;

	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	range = luaL_checknumber(L, 3);

	lua_getuservalue(L, 1);

	/* to restore result */
	lua_newtable(L);

	result = isearchresultmake();
	imapsearchfromunit(map, unit, result, range);

	joint = ireflistfirst(result->units);

	/* 追溯最大的公共父节点 */
	while(joint) {
		iunit *unit = icast(iunit, joint->value);
		iid id = unit->id;

		lua_pushinteger(L, (lua_Integer)id);
		lua_pushinteger(L, (lua_Integer)id);
		lua_rawget(L, -4);
		lua_rawset(L, -3);

		joint = joint->next;
	}

	isearchresultfree(result);

	return 1;
}


static int lua__map_searchcircle(lua_State *L)
{
	ipos pos;
	ireal range;
	isearchresult *result = NULL;

	irefjoint* joint = NULL;

	imap * map = CHECK_AOI_MAP(L, 1);
	range = (ireal)luaL_checknumber(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	lua_rawgeti(L, 3, 1);
	pos.x = luaL_checknumber(L, -1);
	lua_rawgeti(L, 3, 2);
	pos.y = luaL_checknumber(L, -1);

	lua_getuservalue(L, 1);

	lua_newtable(L);

	result = isearchresultmake();
	imapsearchfrompos(map, &pos, result, range);

	joint = ireflistfirst(result->units);

	/* 追溯最大的公共父节点 */
	while(joint) {
		iunit *unit = icast(iunit, joint->value);
		iid id = unit->id;

		lua_pushinteger(L, (lua_Integer)id);
		lua_pushinteger(L, (lua_Integer)id);
		lua_rawget(L, -4);
		lua_rawset(L, -3);

		joint = joint->next;
	}

	isearchresultfree(result);

	return 1;
}


/* }} map */


/* {{ iunit */
static int lua__unit_new(lua_State *L)
{
	iunit *u = NULL;
	lua_Number x, y;
	iid id = (iid)luaL_checkinteger(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_rawgeti(L, 2, 1);
	x = luaL_checknumber(L, -1);

	lua_rawgeti(L, 2, 2);
	y = luaL_checknumber(L, -1);

	u = imakeunit(id, (ireal)x, (ireal)y);
	if (u == NULL) {
		return 0;
	}
	LUA_BIND_META(L, iunit, u, AOI_UNIT);
	DLOG("new unit,id=%" PRId64 "\n", id);
	return 1;
}


static int lua__unit_new_with_radius(lua_State *L)
{
	iunit *u = NULL;
	lua_Number x, y, radius;
	iid id = (iid)luaL_checknumber(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_rawgeti(L, 2, 1);
	x = luaL_checknumber(L, -1);

	lua_rawgeti(L, 2, 2);
	y = luaL_checknumber(L, -1);

	lua_rawgeti(L, 2, 3);
	radius = luaL_checknumber(L, -1);

	u = imakeunitwithradius(id, (ireal)x, (ireal)y, (ireal)radius);
	if (u == NULL) {
		return 0;
	}
	LUA_BIND_META(L, iunit, u, AOI_UNIT);
	DLOG("new unit,id=%" PRId64 "\n", id);
	return 1;
}

static int lua__unit_gc(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	if (unit != NULL) {
		DLOG("unit gc,id=%" PRId64 "\n", unit->id);
		ifreeunit(unit);
	}
	return 0;
}

static int lua__unit_get_id(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	lua_pushinteger(L, (lua_Integer)unit->id);
	return 1;
}

static int lua__unit_get_pos(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	lua_pushnumber(L, (lua_Integer)unit->pos.x);
	lua_pushnumber(L, (lua_Integer)unit->pos.y);
	return 2;
}

static int lua__unit_get_dis_pow2(lua_State *L)
{
	iunit * unit1 = CHECK_AOI_UNIT(L, 1);
	iunit * unit2 = CHECK_AOI_UNIT(L, 2);
	ireal dis = idistancepow2(&unit1->pos, &unit2->pos);
	lua_pushnumber(L, dis);
	return 1;
}

static int lua__unit_get_tick(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	lua_pushnumber(L, (lua_Number)unit->tick);
	return 1;
}

/* }} iunit */

static int luac__meta_cache_clear(const char *typename)
{
	if (strcmp(typename, "all") == 0) {
#define CLEAR_IMETA_CACHE(imeta_type, imeta_cache_cap) imetacacheclear(imeta_type);
		__iallmeta(CLEAR_IMETA_CACHE)
#undef CLEAR_IMETA_CACHE
		return 0;
#define CLEAR_IMETA_CACHE_ELSEIF(imeta_type, imeta_cache_cap) \
	} else if (strcmp(typename, #imeta_type) == 0) { \
		imetacacheclear(imeta_type); return 0;
		__iallmeta(CLEAR_IMETA_CACHE_ELSEIF)
#undef CLEAR_IMETA_CACHE_ELSEIF
	}
	/* failed */
	return -1;
}

static void luac__clear_all_meta_cache()
{
	luac__meta_cache_clear("all");
}

static int lua__meta_cache_clear(lua_State *L)
{
	int ret;
	const char * typename = luaL_optstring(L, 1, "all");
	DLOG("meta_cache_clear %s\n", typename);
	ret = luac__meta_cache_clear(typename);
	if (ret != 0)
		return luaL_error(L, "error type in meta_cache_clear");
	return 0;
}


static int lua__getcurmicro(lua_State *L)
{
	int64_t ret = igetcurmicro();
	lua_pushnumber(L, (lua_Number)ret);
	return 1;
}

static int lua__getcurtick(lua_State *L)
{
	int64_t ret = igetcurtick();
	lua_pushnumber(L, (lua_Number)ret);
	return 1;
}

static int lua__getnextmicro(lua_State *L)
{
	int64_t ret = igetnextmicro();
	lua_pushnumber(L, (lua_Number)ret);
	return 1;
}

static int opencls__map(lua_State *L)
{
	luaL_Reg lmethods[] = {
		{"get_size", lua__map_get_size},
		{"get_state", lua__map_get_state},
		{"get_units", lua__map_get_units},

		{"search_circle", lua__map_searchcircle},
		{"unit_search", lua__map_unit_search},
		{"unit_add", lua__map_unit_add},
		{"unit_del", lua__map_unit_del},
		{"unit_del_by_id", lua__map_unit_del_byid},
		{"unit_update", lua__map_unit_update},
		{NULL, NULL},
	};
	luaL_newmetatable(L, AOI_MAP);
	luaL_newlib(L, lmethods);
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
		{"get_dispow2", lua__unit_get_dis_pow2},
		{NULL, NULL},
	};
	luaL_newmetatable(L, AOI_UNIT);
	luaL_newlib(L, lmethods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction (L, lua__unit_gc);
	lua_setfield (L, -2, "__gc");
	return 1;
}

int luac__new_node_enum(lua_State *L)
{
	lua_newtable(L);
	LUA_SET_ENUM(L, -1, EnumNodePrintStateTick);
	LUA_SET_ENUM(L, -1, EnumNodePrintStateUnits);
	LUA_SET_ENUM(L, -1, EnumNodePrintStateMap);
	LUA_SET_ENUM(L, -1, EnumNodePrintStateNode);
	LUA_SET_ENUM(L, -1, EnumNodePrintStateAll);
	return 1;
}

int luac__new_imeta_map(lua_State *L)
{
	lua_newtable(L);
#define NEW_META_CACHE_CAPMAP(imeta_type, imeta_cache_cap) \
	LUA_SETMACRO(L, -1, #imeta_type, imeta_cache_cap);

	__iallmeta(NEW_META_CACHE_CAPMAP);
#undef NEW_META_CACHE_CAPMAP
	return 1;
}

int luac__new_map_enum(lua_State *L)
{
	lua_newtable(L);
	LUA_SET_ENUM(L, -1, EnumMapStateAll);
	LUA_SET_ENUM(L, -1, EnumMapStateHead);
	LUA_SET_ENUM(L, -1, EnumMapStateTail);
	LUA_SET_ENUM(L, -1, EnumMapStateBasic);
	LUA_SET_ENUM(L, -1, EnumMapStatePrecisions);
	LUA_SET_ENUM(L, -1, EnumMapStateNode);
	LUA_SET_ENUM(L, -1, EnumMapStateUnit);
	LUA_SET_ENUM(L, -1, EnumMapStateAll);
	LUA_SET_ENUM(L, -1, EnumMapStateAllNoHeadTail);
	LUA_SET_ENUM(L, -1, EnumMapStateNone);
	return 1;
}

int luaopen_laoi(lua_State* L)
{
	luaL_Reg lfuncs[] = {
		{"new_map", lua__map_new},
		{"new_unit", lua__unit_new},
		{"new_unit_with_radius", lua__unit_new_with_radius},
		{"imeta_cache_clear", lua__meta_cache_clear},
		{"getcurmicro", lua__getcurmicro},
		{"getcurtick", lua__getcurtick},
		{"getnextmicro", lua__getnextmicro},
		{"dbg_dump_map", lua__map_dump},
		{"dbg_dump_mapstate", lua__map_dumpstate},
		{NULL, NULL},
	};
	opencls__map(L);
	opencls__unit(L);
	luaL_newlib(L, lfuncs);

	luac__new_node_enum(L);
	lua_setfield(L, -2, "NodeEnum");

	luac__new_map_enum(L);
	lua_setfield(L, -2, "MapEnum");

	luac__new_imeta_map(L);
	lua_setfield(L, -2, "IMetaCacheCap");

	atexit(luac__clear_all_meta_cache);
	return 1;
}
