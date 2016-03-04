#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "aoi.h"

/*
 * #define ENABLE_LAOI_DEBUG
 */
#ifdef ENABLE_LAOI_DEBUG
# define DLOG(fmt, ...) fprintf(stderr, "<laoi>" fmt, ##__VA_ARGS__)
#else
# define DLOG(...)
#endif

#if LUA_VERSION_NUM < 502
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
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

#define META_MAP(XX)                 \
	XX(iobj, 0)                  \
	XX(iref, 0)                  \
	XX(ireflist, 1000)           \
	XX(irefjoint, 200000)        \
	XX(inode, 4000)              \
	XX(iunit, 2000)              \
	XX(imap, 0)                  \
	XX(irefcache, 0)             \
	XX(ifilter, 2000)            \
	XX(isearchresult, 0)         \
	XX(irefautoreleasepool, 0)

#if LUA_VERSION_NUM < 502
static void luac__map_getfield(lua_State *L, int mapidx, const char * fieldname)
{
	lua_getfenv(L, mapidx);
	lua_getfield(L, -1, fieldname);
	lua_replace(L, -2);
}

static void luac__map_setfield(lua_State *L, int mapidx, const char* fieldname) 
{
	/* bind a fenv table onto map, to save units */
	lua_newtable(L);

	lua_newtable(L);
	lua_setfield(L, -2, AOI_UNITS_MAP_NAME);

	lua_setfenv(L, -2);
}
#else
static void luac__map_getfield(lua_State *L, int mapidx, const char * fieldname)
{
    lua_getuservalue(L, mapidx);
}

static void luac__map_setfield(lua_State *L, int mapidx, const char* fieldname) 
{
	lua_newtable(L);
    lua_setuservalue(L, -2);
}
#endif


/* {{ map */
static int lua__map_new(lua_State *L)
{
	imap *map = NULL;
	int divide = 1;
	ipos pos = {0.0, 0.0};
	isize size;

	luaL_checktype(L, 1, LUA_TTABLE);

	lua_rawgeti(L, 1, 1);
	size.w = luaL_checknumber(L, -1);
	lua_rawgeti(L, 1, 2);
	size.h = luaL_checknumber(L, -1);

	divide = luaL_optint(L, 2, 1);

	if (lua_type(L, 3) == LUA_TTABLE) {
		lua_rawgeti(L, 3, 1);
		pos.x = luaL_optnumber(L, -1, 0);
		lua_rawgeti(L, 3, 2);
		pos.y = luaL_optnumber(L, -1, 0);
	}

	map = imapmake(&pos, &size, divide);
	if (map == NULL) {
		return 0;
	}
	LUA_BIND_META(L, imap, map, AOI_MAP);

    /*
     * */
	/* bind a fenv table onto map, to save units */
luac__map_setfield(L, -1, AOI_UNITS_MAP_NAME); 

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

	luac__map_getfield(L, 1, AOI_UNITS_MAP_NAME);
	lua_pushnumber(L, unit->id);
	lua_rawget(L, -2);

	if (lua_isnil(L, -1)) {
		imapaddunit(map, unit);

		lua_pop(L, 1);
		lua_pushnumber(L, unit->id);
		lua_pushvalue(L, 2);
		lua_rawset(L, -3);
	}

	return 0;
}

static int lua__map_unit_del_byid(lua_State *L)
{
	iunit * unit = NULL;
	imap * map = CHECK_AOI_MAP(L, 1);
	iid id = (iid)luaL_checknumber(L, 2);

	luac__map_getfield(L, 1, AOI_UNITS_MAP_NAME);
	lua_pushnumber(L, id);
	lua_rawget(L, -2);
	if (lua_isnoneornil(L, -1)) {
		/* fprintf(stderr, "%s,id=%lld not found\n", __FUNCTION__, id); */
		return 0;
	}

	unit = CHECK_AOI_UNIT(L, -1);
	imapremoveunit(map, unit);

	lua_pushnumber(L, id);
	lua_pushnil(L);
	lua_rawset(L, 3);
	/* fprintf(stderr, "%s,id=%lld removed\n", __FUNCTION__, id); */

	return 0;
}


static int lua__map_unit_del(lua_State *L)
{
	imap * map = CHECK_AOI_MAP(L, 1);
	iunit * unit = CHECK_AOI_UNIT(L, 2);
	imapremoveunit(map, unit);

	luac__map_getfield(L, 1, AOI_UNITS_MAP_NAME);
	lua_pushnumber(L, unit->id);
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

                DLOG("unit move,id=%lld,from=(%.2f,%.2f),to=(%.2f,%.2f)\n",
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
	luac__map_getfield(L, 1, AOI_UNITS_MAP_NAME);
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

	luac__map_getfield(L, 1, AOI_UNITS_MAP_NAME);

	/* to restore result */
	lua_newtable(L);

	result = isearchresultmake();
	imapsearchfromunit(map, unit, result, range);

	joint = ireflistfirst(result->units);

	/* 追溯最大的公共父节点 */
	while(joint) {
		iunit *unit = icast(iunit, joint->value);
		iid id = unit->id;

		lua_pushnumber(L, id);
		lua_pushnumber(L, id);
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

	luac__map_getfield(L, 1, AOI_UNITS_MAP_NAME);

	lua_newtable(L);

	result = isearchresultmake();
	imapsearchfrompos(map, &pos, result, range);

	joint = ireflistfirst(result->units);

	/* 追溯最大的公共父节点 */
	while(joint) {
		iunit *unit = icast(iunit, joint->value);
		iid id = unit->id;

		lua_pushnumber(L, id);
		lua_pushnumber(L, id);
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
	iid id = (iid)luaL_checknumber(L, 1);
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
	DLOG("new unit,id=%lld\n", id);
	return 1;
}

static int lua__unit_gc(lua_State *L)
{
	iunit * unit = CHECK_AOI_UNIT(L, 1);
	if (unit != NULL) {
		DLOG("unit gc,id=%lld\n", unit->id);
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

static int lua__meta_cache_clear(lua_State *L)
{
	const char * typename = luaL_optstring(L, 1, "all");
	if (strcmp(typename, "all") == 0) {
#define XX(imeta_type, imeta_cache_cap) imetacacheclear(imeta_type);
	META_MAP(XX)
#undef XX

#define XX(imeta_type, imeta_cache_cap) \
	} else if (strcmp(typename, #imeta_type) == 0) { \
		imetacacheclear(imeta_type);
	META_MAP(XX)
#undef XX
	} else {
		return luaL_error(L, "unkown imeta_type");
	}
	return 0;
}


static int lua__getcurmicro(lua_State *L)
{
	int64_t ret = igetcurmicro();
	lua_pushnumber(L, ret);
	return 1;
}

static int lua__getcurtick(lua_State *L)
{
	int64_t ret = igetcurtick();
	lua_pushnumber(L, ret);
	return 1;
}

static int lua__getnextmicro(lua_State *L)
{
	int64_t ret = igetnextmicro();
	lua_pushnumber(L, ret);
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
#define XX(imeta_type, imeta_cache_cap) \
	LUA_SETMACRO(L, -1, #imeta_type, imeta_cache_cap);

	META_MAP(XX);
#undef XX
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

	return 1;
}

