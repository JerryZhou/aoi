#include "aoi.h"

#ifndef iunused 
#define iunused(v) (void)(v)
#endif

#define open_profc_main_entry (1)

typedef struct minmaxrange {
    ireal min;
    ireal max;
    ireal total;
    int trys;
} minmaxrange;

static void minmaxrange_add(minmaxrange *r , ireal i) {
    r->total += i;
    if (r->min > i) {
        r->min = i;
    } else if (r->max < i) {
        r->max = i;
    }
    ++r->trys;
}

typedef struct __argmap_profile {
    ireal add;
    ireal update;

    ireal t_searchpos;
    ireal t_searchunit;

    minmaxrange nr_searchpos;
    minmaxrange xr_searchpos;

    minmaxrange nr_searchunit;
    minmaxrange xr_searchunit;
}__argmap_profile;

typedef struct __argmap {
    iunit** units;
    int unitcnt;

    imap* map;
    isearchresult *result;

    int minrange;
    int maxrange;
    int bench;

    /* 性能相关数据 */
    __argmap_profile profile;
} __argmap;

static __argmap* _aoi_argmap_new(int argc, char* argv[]) {
    ipos pos = {0, 0};
	isize size = {512, 512};
    __argmap *map = (__argmap *)icalloc(1, sizeof(__argmap));

	int divide = 8;/*10; */
	int randcount = 2000;
    int i = 0;

    map->minrange = 5;
    map->maxrange = 25;
    map->bench = 10 * 10000;

	iunused(argc);
	iunused(argv);

	if (argc >= 2) {
		divide = atoi(argv[1]);
	}
	if (argc >= 3) {
		randcount = atoi(argv[2]);
	}
    if (argc >= 4) {
        map->minrange = atoi(argv[3]);
    }
    if (argc >= 5) {
        map->maxrange = atoi(argv[4]);
    }
    if (argc >= 6) {
        map->bench = atoi(argv[5]);
    }

	srand((unsigned)time(NULL));

	map->map = imapmake(&pos, &size, divide);
    map->units = (iunit**)icalloc(randcount, sizeof(iunit*));
	map->result = isearchresultmake();
    map->unitcnt = randcount;

	for (i=0; i<randcount; ++i) {
        map->units[i] = imakeunit((iid)i, (ireal)(rand()%512), (ireal)(rand()%512));
		imapaddunit(map->map, map->units[i]);
	}

	/*imapstatedesc(map->map, EnumMapStateAll, NULL, "[Check]");*/
    return map;
}

static void _aoi_argmap_free(__argmap* map) {
    int i = 0;
	isearchresultfree(map->result);
    map->result = NULL;
	for (i=0; i<map->unitcnt; ++i) {
		ifreeunit(map->units[i]);
        map->units[i] = NULL;
    }
    ifree(map->units);

	imapfree(map->map);
    ifree(map);
}

/*
 * why do prof works in c ??
 * 1. only prof the inner key path functions in native c
 * 2. prof in native c, skip all of outter controlless situations(such as lua gc or stack roll)
 * */

/* copy from aoi.c */
/* 一堆时间宏，常用来做性能日志 */
static int open_log_profile	= 1;

#define __ProfileThreashold 10

#define __ShouldLog(t) (t > __ProfileThreashold)

#define __Millis igetcurtick()

#define __Micros  igetcurmicro()

#define __Since(t) (__Micros - t)

#define iplogwhen(t, when, ...) do { if(open_log_profile && t > when) {printf("[PROFILE] Take %lld micros ", t); printf(__VA_ARGS__); } } while (0)

#define iplog(t, ...) iplogwhen(t, __ProfileThreashold, __VA_ARGS__)

#define __Begin int64_t t = __Micros

#define __Stop  t = __Since(t)

#define __End(...) iplogwhen(t, __ProfileThreashold, __VA_ARGS__)

#define __EndBe(threshold, ...) iplogwhen(t, threshold, __VA_ARGS__)

#ifndef ilog 
#define ilog(...) if (open_log_profile) { printf(__VA_ARGS__); }
#endif

/*
 * 性能测试imapaddunit;imapremoveunit  
 * */
static void aoi_prof_unitadd(__argmap *map) {
    __Begin;
    int base = map->bench;
    int i=0;
    iunit *u = NULL;

    ilog("开始测试%d 单元的地图加入，退出操作的耗时\n", base);
    u = imakeunit(i + base, (ireal)(rand()%512), (ireal)(rand()%512));
    for(;i<base; ++i) {
        imapaddunit(map->map, u);
        imapremoveunit(map->map, u);
    }
    ifreeunit(u);
    __Stop;

    map->profile.add = 1.0 * t / base; 

    __End("aoi_prof_unitadd: 平均一个单元耗时 %f micros\n\n", map->profile.add);
}

/*
 * 性能测试imapupdateunit
 * */
static void aoi_prof_unitupdate(__argmap *map) {
    __Begin;
    int base = map->bench;
    int i=0;
    iunit *u = NULL;

    ilog("开始测试%d 单元的地图更新操作的耗时\n", base);
    u = imakeunit(i + base, (ireal)(rand()%512), (ireal)(rand()%512));
    imapaddunit(map->map, u);
    for(;i<base; ++i) {
        u->pos.x = (ireal)(rand()%512);
        u->pos.y = (ireal)(rand()%512);
        imapupdateunit(map->map, u);
    }
    imapremoveunit(map->map, u);
    ifreeunit(u);
    __Stop;

    map->profile.update = 1.0 * t / base; 
    __End("aoi_prof_unitupdate: 平均一个单元耗时 %f micros\n\n", map->profile.update);
}

/*
 * 性能测试imapsearchfromunit
 * */
static void aoi_prof_unitsearchfromunit(__argmap *map) {
    __Begin;
    int base = map->bench / 100;
    int i=0;
    ireal range = 0;
    minmaxrange xr = {512, 0, 0, 0};
    minmaxrange nr = {1000000, 0, 0, 0};
    iunit *u = NULL;

    ilog("开始测试%d 单元的地图搜索操作的耗时\n", base);
    u = imakeunit(i + base, (ireal)(rand()%512), (ireal)(rand()%512));
    imapaddunit(map->map, u);
    for(;i<base; ++i) {
        u->pos.x = (ireal)(rand()%512);
        u->pos.y = (ireal)(rand()%512);
        imapupdateunit(map->map, u);

        range = (ireal)(rand()%map->maxrange + map->minrange);
        imapsearchfromunit(map->map, u, map->result, range);
        minmaxrange_add(&xr, range);
        minmaxrange_add(&nr, (ireal)(ireflistlen(map->result->units)));
    }
    imapremoveunit(map->map, u);
    ifreeunit(u);
    __Stop;

    map->profile.t_searchunit = 1.0 * t / base - map->profile.update;
    __End("aoi_prof_unitsearchfromunit: 平均一次搜索耗时 %f micros \n"
            "搜索范围均值 %.2f, 平均每次的搜索到 %.2f 单元\n" 
            "搜索范围最大值 %.2f, 搜索范围最小值 %.2f \n" 
            "搜索结果的最多单元数 %.2f, 搜索结果的最少单元数 %.2f \n\n", 
            map->profile.t_searchunit, /*减掉测到的单元平均更新耗时*/ 
            xr.total/base, nr.total/base,
            xr.max, xr.min,
            nr.max, nr.min);
    map->profile.xr_searchunit = xr;
    map->profile.nr_searchunit = nr;
}

/*
 * 性能测试imapsearchfrompos
 * */
static void aoi_prof_unitsearchfrompos(__argmap *map) {
    __Begin;
    int base = map->bench / 100;
    int i=0;
    ipos pos = {0, 0};
    ireal range = 0;
    minmaxrange xr = {512, 0, 0, 0};
    minmaxrange nr = {1000000, 0, 0, 0};

    ilog("开始测试%d 单元的地图搜索操作的耗时\n", base);
    for(;i<base; ++i) {
        pos.x = (ireal)(rand()%512);
        pos.y = (ireal)(rand()%512);
        range = (ireal)(rand()%map->maxrange + map->minrange);
        imapsearchfrompos(map->map, &pos, map->result, range);
        minmaxrange_add(&xr, range);
        minmaxrange_add(&nr, (ireal)(ireflistlen(map->result->units)));
    }
    __Stop;

    map->profile.t_searchpos=1.0 * t / base;
    __End("aoi_prof_unitsearchfrompos: 平均一次搜索耗时 %f micros \n"
            "搜索范围均值 %.2f, 平均每次的搜索到 %.2f 单元\n" 
            "搜索范围最大值 %.2f, 搜索范围最小值 %.2f \n" 
            "搜索结果的最多单元数 %.2f, 搜索结果的最少单元数 %.2f \n\n", 
            map->profile.t_searchpos, 
            xr.total/base, nr.total/base,
            xr.max, xr.min,
            nr.max, nr.min);
    map->profile.xr_searchpos = xr;
    map->profile.nr_searchpos = nr;
}

static int aoi_prof(int argc, char *argv[]) {
    __argmap* map = _aoi_argmap_new(argc, argv);

    aoi_prof_unitadd(map);
    aoi_prof_unitupdate(map);
    aoi_prof_unitsearchfromunit(map);
    aoi_prof_unitsearchfrompos(map);
    if (open_log_profile == 0) {
        printf("d:%2d (%6.2f %6.2f) n:%6lld l:%6lld u:%6lld "
                "=> add:%3.2f up:%3.2f spos:%7.2f sunit:%7.2f srange:%3.2f snum:%4.2f\n",
                map->map->divide,
                map->map->nodesizes[map->map->divide].w,
                map->map->nodesizes[map->map->divide].h,
                map->map->state.nodecount,
                map->map->state.leafcount,
                map->map->state.unitcount,
                map->profile.add, 
                map->profile.update,
                map->profile.t_searchpos,
                map->profile.t_searchunit,
                map->profile.xr_searchpos.total/map->profile.xr_searchpos.trys,
                map->profile.nr_searchpos.total/map->profile.nr_searchpos.trys
                );
    }

    _aoi_argmap_free(map);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>

static int aoi_prof_array(int argc, char *argv[]) {
    int divide = 8;/*10; */
	int randcount = 2000;
    int i = 1;
    open_log_profile = 0;

	iunused(argc);
	iunused(argv);

	if (argc >= 3) {
		divide = atoi(argv[2]);
	}
	if (argc >= 4) {
		randcount = atoi(argv[3]);
	}

    for (; i<= divide; i++) {
        char sdivide[256] = {0};
        char scount[256] = {0}; 
        int offset = 0;

        snprintf(sdivide, 256, "%d", i);
        snprintf(scount, 256, "%d", randcount);
        
        char* args[6] = {
            NULL,
            sdivide,
            scount,
            argc >= 5 ? (offset++, argv[4]) : NULL,
            argc >= 6 ? (offset++, argv[5]) : NULL,
            argc >= 7 ? (offset++, argv[6]) : NULL,
        };
        aoi_prof(3 + offset, args);
    }
    return 0;
}

int iprof_main(int argc, char *argv[]) {
    if (argc >= 2 && argv[1][0] == 'c') {
        return aoi_prof_array(argc, argv);
    } else {
        return aoi_prof(argc, argv);
    }
}

#if open_profc_main_entry

int main(int argc, char *argv[])
{
    return iprof_main(argc, argv);
}

#endif
