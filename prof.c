#include "aoi.h"

#ifndef iunused 
#define iunused(v) (void)(v)
#endif

#ifndef ilog 
#define ilog(...) printf(__VA_ARGS__)
#endif

typedef struct __argmap {
    iunit** units;
    int unitcnt;

    imap* map;
    isearchresult *result;
} __argmap;

static __argmap* _aoi_argmap_new(int argc, char* argv[]) {
    ipos pos = {0, 0};
	isize size = {512, 512};
    __argmap *map = (__argmap *)icalloc(1, sizeof(__argmap));

	int divide = 8;/*10; */
	int randcount = 2000;
    int i = 0;

	iunused(argc);
	iunused(argv);

	if (argc >= 2) {
		divide = atoi(argv[1]);
	}
	if (argc >= 3) {
		randcount = atoi(argv[2]);
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
#define open_log_profile	(1)

#define __ProfileThreashold 10

#define __ShouldLog(t) (t > __ProfileThreashold)

#define __Millis igetcurtick()

#define __Micros  igetcurmicro()

#define __Since(t) (__Micros - t)

#define iplogwhen(t, when, ...) do { if(open_log_profile && t > when) {printf("[PROFILE] Take %lld micros ", t); printf(__VA_ARGS__); } } while (0)

#define iplog(t, ...) iplogwhen(t, __ProfileThreashold, __VA_ARGS__)

#define __Begin int64_t t = __Micros

#define __End(...) t = __Since(t); iplogwhen(t, __ProfileThreashold, __VA_ARGS__)

#define __EndBe(threshold, ...) t = __Since(t); iplogwhen(t, threshold, __VA_ARGS__)


/*
 * 性能测试imapaddunit;imapremoveunit  
 * */
static void aoi_prof_unitadd(__argmap *map) {
    __Begin;
    int base = 100 * 10000;
    int i=0;
    iunit *u = NULL;

    printf("开始测试%d 单元的地图加入，退出操作的耗时\n", base);
    u = imakeunit(i + base, (ireal)(rand()%512), (ireal)(rand()%512));
    for(;i<base; ++i) {
        imapaddunit(map->map, u);
        imapremoveunit(map->map, u);
    }
    ifreeunit(u);
    __End("aoi_prof_unitadd: 平均一个单元耗时 %f micros\n\n", 1.0 * t / base );
}

/*
 * 性能测试imapupdateunit
 * */
static ireal _g_t_unitupdate = 0; 
static void aoi_prof_unitupdate(__argmap *map) {
    __Begin;
    int base = 100 * 10000;
    int i=0;
    iunit *u = NULL;

    printf("开始测试%d 单元的地图更新操作的耗时\n", base);
    u = imakeunit(i + base, (ireal)(rand()%512), (ireal)(rand()%512));
    imapaddunit(map->map, u);
    for(;i<base; ++i) {
        u->pos.x = (ireal)(rand()%512);
        u->pos.y = (ireal)(rand()%512);
        imapupdateunit(map->map, u);
    }
    imapremoveunit(map->map, u);
    ifreeunit(u);

    __End("aoi_prof_unitupdate: 平均一个单元耗时 %f micros\n\n", (_g_t_unitupdate = 1.0 * t / base, _g_t_unitupdate) );
}

typedef struct minmaxrange {
    ireal min;
    ireal max;
    ireal total;
} minmaxrange;

static void minmaxrange_add(minmaxrange *r , ireal i) {
    r->total += i;
    if (r->min > i) {
        r->min = i;
    } else if (r->max < i) {
        r->max = i;
    }
}

/*
 * 性能测试imapsearchfromunit
 * */
static void aoi_prof_unitsearchfromunit(__argmap *map) {
    __Begin;
    int base = 10 * 10000;
    int i=0;
    ireal range = 0;
    minmaxrange xr = {512, 0, 0};
    minmaxrange nr = {1000000, 0, 0};
    iunit *u = NULL;

    printf("开始测试%d 单元的地图搜索操作的耗时\n", base);
    u = imakeunit(i + base, (ireal)(rand()%512), (ireal)(rand()%512));
    imapaddunit(map->map, u);
    for(;i<base; ++i) {
        u->pos.x = (ireal)(rand()%512);
        u->pos.y = (ireal)(rand()%512);
        imapupdateunit(map->map, u);

        range = (ireal)(rand()%32 + 10);
        imapsearchfromunit(map->map, u, map->result, range);
        minmaxrange_add(&xr, range);
        minmaxrange_add(&nr, (ireal)(ireflistlen(map->result->units)));
    }
    imapremoveunit(map->map, u);
    ifreeunit(u);
    __End("aoi_prof_unitsearchfromunit: 平均一次搜索耗时 %f micros \n"
            "搜索范围均值 %.2f, 平均每次的搜索到 %.2f 单元\n" 
            "搜索范围最大值 %.2f, 搜索范围最小值 %.2f \n" 
            "搜索结果的最多单元数 %.2f, 搜索结果的最少单元数 %.2f \n\n", 
            1.0 * t / base - _g_t_unitupdate, /*减掉测到的单元平均更新耗时*/ 
            xr.total/base, nr.total/base,
            xr.max, xr.min,
            nr.max, nr.min);
}

/*
 * 性能测试imapsearchfrompos
 * */
static void aoi_prof_unitsearchfrompos(__argmap *map) {
    __Begin;
    int base = 10 * 10000;
    int i=0;
    ipos pos = {0, 0};
    ireal range = 0;
    minmaxrange xr = {512, 0, 0};
    minmaxrange nr = {1000000, 0, 0};

    printf("开始测试%d 单元的地图搜索操作的耗时\n", base);
    for(;i<base; ++i) {
        pos.x = (ireal)(rand()%512);
        pos.y = (ireal)(rand()%512);
        range = (ireal)(rand()%32 + 10);
        imapsearchfrompos(map->map, &pos, map->result, range);
        minmaxrange_add(&xr, range);
        minmaxrange_add(&nr, (ireal)(ireflistlen(map->result->units)));
    }
    __End("aoi_prof_unitsearchfrompos: 平均一次搜索耗时 %f micros \n"
            "搜索范围均值 %.2f, 平均每次的搜索到 %.2f 单元\n" 
            "搜索范围最大值 %.2f, 搜索范围最小值 %.2f \n" 
            "搜索结果的最多单元数 %.2f, 搜索结果的最少单元数 %.2f \n\n", 
            1.0 * t / base , 
            xr.total/base, nr.total/base,
            xr.max, xr.min,
            nr.max, nr.min);
}

static int aoi_prof(int argc, char *argv[]) {
    __argmap* map = _aoi_argmap_new(argc, argv);

    aoi_prof_unitadd(map);
    aoi_prof_unitupdate(map);
    aoi_prof_unitsearchfromunit(map);
    aoi_prof_unitsearchfrompos(map);

    _aoi_argmap_free(map);
    return 0;
}

int main(int argc, char *argv[])
{
	return aoi_prof(argc, argv);
}
