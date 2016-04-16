//
//  aoitest.h
//  ml
//
//  Created by JerryZhou on 15/5/21.
//  Copyright (c) 2015年 JerryZhou. All rights reserved.
//

#ifndef ml_aoitest_h
#define ml_aoitest_h

#include "aoi.h"
#include "navi.h"
#include "simpletest.h"

// 清理掉所有缓存并打印内存日志
void clearalliaoicacheandmemrorystate() {
    iaoicacheclear(imetaof(iwref));
    iaoicacheclear(imetaof(ifilter));
    iaoicacheclear(imetaof(iunit));
    iaoicacheclear(imetaof(inode));
    iaoicacheclear(imetaof(ireflist));
    iaoicacheclear(imetaof(irefjoint));
    iaoicacheclear(imetaof(inavicell));
    iaoicacheclear(imetaof(inavicellconnection));
    iaoicacheclear(imetaof(inaviwaypoint));
    iaoicacheclear(imetaof(inavinode));
    //iaoicacheclear(imetaof(sp_test_cache_clear));
    
    iaoimemorystate();
}

// **********************************************************************************
// imeta
SP_SUIT(imeta);

SP_CASE(imeta, imetaget) {
    
    imeta *iobjmeta = imetaget(imetaindex(iobj));
    
    SP_TRUE(iobjmeta != NULL);
    
    SP_EQUAL(iobjmeta->size, sizeof(iobj));
    SP_TRUE(strcmp(iobjmeta->name, "iobj") == 0);
}

typedef struct sp_eg_meta_obj {
    irefdeclare;
    int i, j;
}sp_eg_meta_obj;

// 注册类型
iimplementregister(sp_eg_meta_obj, 0);

SP_CASE(imeta, imetaregister) {
    imeta * meta = imetaof(sp_eg_meta_obj);
    SP_EQUAL(meta->size, sizeof(sp_eg_meta_obj));
    SP_TRUE(strcmp(meta->name, "sp_eg_meta_obj") == 0);
    
    SP_EQUAL(meta->current, 0);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, 0);
    
    sp_eg_meta_obj *obj = iobjmalloc(sp_eg_meta_obj);
    iretain(obj);
    
    SP_EQUAL(meta->current, sizeof(sp_eg_meta_obj) + sizeof(iobj));
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, sizeof(sp_eg_meta_obj) + sizeof(iobj));
    
    irelease(obj);
    
    SP_EQUAL(meta->current, 0);
    SP_EQUAL(meta->freed, sizeof(sp_eg_meta_obj) + sizeof(iobj));
    SP_EQUAL(meta->alloced, sizeof(sp_eg_meta_obj) + sizeof(iobj));
}

typedef struct sp_test_cache_clear{
    irefdeclare;
    int n, m;
}sp_test_cache_clear;

// 注册类型
iimplementregister(sp_test_cache_clear, 1);

SP_CASE(imeta, iaoicacheclear) {
    imeta * meta = imetaof(sp_test_cache_clear);
    
    // 初始状态的meta
    SP_EQUAL(meta->current, 0);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, 0);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 0);
    
    sp_test_cache_clear *clear = iobjmalloc(sp_test_cache_clear);
    iretain(clear);
    
    int size = sizeof(sp_test_cache_clear) + sizeof(iobj);
    
    // 创建一个对象后的meta
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 0);
    
    SP_EQUAL(iobjistype(clear, sp_test_cache_clear), iiok);
    SP_EQUAL(iobjistype(clear, iobj), iino);
    
    SP_EQUAL(meta->current, size);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, size);
    
    // 第一个对象释放后进入缓冲区的meta
    irelease(clear);
    SP_EQUAL(meta->current, size);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, size);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 1);
    
    // 从缓冲区拿到一个对象后的meta
    sp_test_cache_clear *newclear = iobjmalloc(sp_test_cache_clear);
    iretain(newclear);
    
    SP_EQUAL(newclear, clear);
    
    SP_EQUAL(meta->current, size);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, size);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 0);
    
    // 缓冲区已经没有了，重新创建一个对象的meta
    sp_test_cache_clear *nextclear = iobjmalloc(sp_test_cache_clear);
    iretain(nextclear);
    
    SP_EQUAL(meta->current, 2*size);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, 2*size);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 0);
    
    //  把新对象放入缓存后的meta
    irelease(nextclear);
    SP_EQUAL(meta->current, 2*size);
    SP_EQUAL(meta->freed, 0);
    SP_EQUAL(meta->alloced, 2*size);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 1);
   
    // 缓冲区已经满了，直接释放一个对象后的meta
    irelease(newclear);
    SP_EQUAL(meta->current, size);
    SP_EQUAL(meta->freed, size);
    SP_EQUAL(meta->alloced, 2*size);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 1);
    
    // 释放对象缓冲区后的meta
    iaoicacheclear(meta);
    SP_EQUAL(meta->current, 0);
    SP_EQUAL(meta->freed, 2*size);
    SP_EQUAL(meta->alloced, 2*size);
    
    SP_EQUAL(meta->cache.capacity, 1);
    SP_EQUAL(meta->cache.length, 0);
}

SP_CASE(imeta, iaoiistype) {
    sp_test_cache_clear *clear = iobjmalloc(sp_test_cache_clear);
    iretain(clear);
    
    SP_EQUAL(iobjistype(clear, sp_test_cache_clear), iiok);
    SP_EQUAL(iobjistype(clear, iobj), iino);
    
    // 释放对象
    irelease(clear);
    // 清理缓冲区
    iaoicacheclear(imetaof(sp_test_cache_clear));
}

// **********************************************************************************
// iwref
SP_SUIT(iwref);

SP_CASE(iwref, begin) {
    SP_TRUE(1);
    
    /*no memory leak about iwref */
    SP_TRUE(imetaof(iwref)->current == 0);
}

SP_CASE(iwref, iwrefmake) {
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    
    iwref *wref0 = iwrefmake(ref);
    SP_EQUAL(irefcast(wref0->wref), ref);
    iref *strongref = iwrefstrong(wref0);
    SP_EQUAL(strongref, ref);
    irelease(strongref);
    SP_EQUAL(irefcast(wref0->wref), ref);
    
    irelease(ref);
    SP_EQUAL(wref0->wref, NULL);
    strongref = iwrefstrong(wref0);
    SP_EQUAL(strongref, NULL);
    
    irelease(wref0);
    
}

SP_CASE(iwref, iwrefmakeby) {
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    
    iwref *wref0 = iwrefmake(ref);
    SP_EQUAL(irefcast(wref0->wref), ref);
    iref *strongref = iwrefstrong(wref0);
    SP_EQUAL(strongref, ref);
    irelease(strongref);
    SP_EQUAL(irefcast(wref0->wref), ref);
    
    iwref *wref1 = iwrefmakeby(wref0);
    SP_EQUAL(irefcast(wref1->wref), ref);
    SP_EQUAL(wref0, wref1);
    iref *strongref1 = iwrefstrong(wref1);
    SP_EQUAL(strongref1, ref);
    irelease(strongref1);
    SP_EQUAL(irefcast(wref1->wref), ref);
    SP_EQUAL(wref0, wref1);
    
    irelease(ref);
    SP_EQUAL(wref0->wref, NULL);
    strongref = iwrefstrong(wref0);
    SP_EQUAL(strongref, NULL);
    
    SP_EQUAL(wref1->wref, NULL);
    strongref = iwrefstrong(wref1);
    SP_EQUAL(strongref, NULL);
    
    irelease(wref0);
    irelease(wref1);
    
}

SP_CASE(iwref, makenull) {
    iwref *wref0 = iwrefmake(NULL);
    iwref *wref1 = iwrefmake(NULL);
    
    SP_EQUAL(wref0, wref1);
    
    iref *strong = iwrefstrong(wref0);
    SP_EQUAL(strong, NULL);
    
    irelease(wref0);
    irelease(wref1);
    
    iwref *wrefzero = iwrefmakeby(NULL);
    SP_EQUAL(wrefzero, NULL);
    irelease(wrefzero);
}

SP_CASE(iwref, end) {
    SP_TRUE(1);
    
    
    /*no memory leak about iwref */
    SP_TRUE(imetaof(iwref)->current == 0);
}

// **********************************************************************************
// time
SP_SUIT(time);

SP_CASE(time, igetcurmicroandigetcurtick) {
    int64_t micro = igetcurmicro();
    int64_t ticks = igetcurtick();
    SP_EQUAL((micro - ticks * 1000 < 1000), 1);
}

SP_CASE(time, igetnextmicro) {
    int64_t micro0 = igetnextmicro();
    int64_t micro1 = igetnextmicro();
    
    SP_EQUAL(micro0 != micro1, 1);
}

// **********************************************************************************
// ipos
SP_SUIT(ipos);

SP_CASE(ipos, idistancepow2) {
    ipos pos1 = {0, 0};
    ipos pos2 = {3, 4};
    
    SP_EQUAL(25.0, idistancepow2(&pos1, &pos2));
}

// **********************************************************************************
// isize
SP_SUIT(isize);

SP_CASE(isize, nothing) {
    SP_EQUAL(1, 1);
}

// **********************************************************************************
// irect
SP_SUIT(irect);

SP_CASE(irect, irectcontainsTHEself) {
    irect r = {{0,0}, {1,1}};
    
    SP_EQUAL(irectcontains(&r, &r), 1);
}

SP_CASE(irect, irectcontainsTHEsub) {
    irect r = {{0,0}, {2,2}};
    
    irect r0 = {{0,0}, {0,0}};
    
    SP_EQUAL(irectcontains(&r, &r0), 1);
}

SP_CASE(irect, irectcontainsTHEsubNo) {
    irect r = {{0,0}, {2,2}};
    
    irect r0 = {{0,0}, {3,3}};
    
    SP_EQUAL(irectcontains(&r, &r0), 0);
}

SP_CASE(irect, irectcontainsTHEsubNo2) {
    irect r = {{0,0}, {2,2}};
    
    irect r0 = {{1,1}, {2,2}};
    
    SP_EQUAL(irectcontains(&r, &r0), 0);
}

SP_CASE(irect, irectcontainsTHEsubNo3) {
    irect r = {{0,0}, {2,2}};
    
    irect r0 = {{-1,-1}, {2,2}};
    
    SP_EQUAL(irectcontains(&r, &r0), 0);
}

SP_CASE(irect, irectcontainsTHEsubNo4) {
    irect r = {{0,0}, {2,2}};
    
    irect r0 = {{-1,-1}, {5,5}};
    
    SP_EQUAL(irectcontains(&r, &r0), 0);
}

SP_CASE(irect, irectcontainspoint) {
    irect r = {{0,0}, {2,2}};
    ipos p = {0, 0};
    
    SP_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 2, p.y = 2;
    SP_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 2, p.y = 0;
    SP_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 0, p.y = 2;
    SP_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 1, p.y = 1;
    SP_EQUAL(irectcontainspoint(&r, &p), iiok);
    
    p.x = 2, p.y = -1;
    SP_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = 3, p.y = -1;
    SP_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = -1, p.y = -1;
    SP_EQUAL(irectcontainspoint(&r, &p), iino);
    
    p.x = -1, p.y = 1;
    SP_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = -1, p.y = 2;
    SP_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = -1, p.y = 3;
    SP_EQUAL(irectcontainspoint(&r, &p), iino);
}

SP_CASE(irect, irectintersect) {
    irect r = {{0,0}, {2,2}};
    icircle c = {{0, 0}, 1};
    
    SP_EQUAL(irectintersect(&r, &c), iiok);
    
    c.pos.x = 3;
    c.pos.y = 3;
    SP_EQUAL(irectintersect(&r, &c), iino);
    
    c.pos.x = 2.5;
    c.pos.y = 2.5;
    SP_EQUAL(irectintersect(&r, &c), iiok);
    
    c.pos.x = 1;
    c.pos.y = 3;
    SP_EQUAL(irectintersect(&r, &c), iino);
    
    c.pos.x = 1;
    c.pos.y = 3.5;
    SP_EQUAL(irectintersect(&r, &c), iino);
    
    c.pos.x = 1;
    c.pos.y = 2.5;
    SP_EQUAL(irectintersect(&r, &c), iiok);
    
    c.pos.x = 1;
    c.pos.y = 1;
    SP_EQUAL(irectintersect(&r, &c), iiok);
}

// **********************************************************************************
// icircle
SP_SUIT(icircle);

SP_CASE(icircle, icircleintersect) {
    icircle c = {{0, 0}, 1.0};
    
    SP_EQUAL(c.radius, 1.0);
    
    icircle c0 = {{0, 0}, 2.0};
    
    SP_EQUAL(icircleintersect(&c, &c0), 1);
}

SP_CASE(icircle, icircleintersectYES) {
    icircle c = {{0, 0}, 1.0};
    
    SP_EQUAL(c.radius, 1.0);
    
    icircle c0 = {{0, 3}, 2.0};
    
    SP_EQUAL(icircleintersect(&c, &c0), 1);
}

SP_CASE(icircle, icircleintersectNo) {
    icircle c = {{0, 0}, 1.0};
    
    SP_EQUAL(c.radius, 1.0);
    
    icircle c0 = {{3, 3}, 2.0};
    
    SP_EQUAL(icircleintersect(&c, &c0), 0);
}

SP_CASE(icircle, icirclecontains) {
    icircle c = {{0, 0}, 2.0};
    
    SP_EQUAL(icirclecontains(&c, &c), 1);
}

SP_CASE(icircle, icirclecontainsYES) {
    icircle c = {{0, 0}, 2.0};
    
    icircle c0 = {{0, 0}, 1.0};
    
    SP_EQUAL(icirclecontains(&c, &c0), 1);
}

SP_CASE(icircle, icirclecontainsYES1) {
    icircle c = {{0, 0}, 2.0};
    
    SP_EQUAL(icirclecontains(&c, NULL), 1);
}

SP_CASE(icircle, icirclecontainsNO) {
    icircle c = {{0, 0}, 2.0};
    
    SP_EQUAL(icirclecontains(NULL, &c), 0);
}

SP_CASE(icircle, icirclecontainsNO1) {
    icircle c = {{0, 0}, 2.0};
    icircle c0 = {{0, 0}, 3.0};
    
    SP_EQUAL(icirclecontains(&c, &c0), 0);
}

SP_CASE(icircle, icirclecontainsNO2) {
    icircle c = {{0, 0}, 2.0};
    icircle c0 = {{1, 1}, 2.0};
    
    SP_EQUAL(icirclecontains(&c, &c0), 0);
}

SP_CASE(icircle, icirclecontainsNO3) {
    icircle c = {{0, 0}, 2.0};
    icircle c0 = {{5, 5}, 2.0};
    
    SP_EQUAL(icirclecontains(&c, &c0), 0);
}

SP_CASE(icircle, icirclerelation) {
    icircle c = {{0, 0}, 2.0};
    icircle c0 = {{0, 0}, 1.0};
    icircle c1 = {{1, 0}, 1.0};
    icircle c2 = {{3, 0}, 1.0};
    
    SP_EQUAL(icirclerelation(&c, &c), EnumCircleRelationAContainsB);
    SP_EQUAL(icirclerelation(&c, &c0), EnumCircleRelationAContainsB);
    SP_EQUAL(icirclerelation(&c0, &c), EnumCircleRelationBContainsA);
    SP_EQUAL(icirclerelation(&c0, &c1), EnumCircleRelationIntersect);
    SP_EQUAL(icirclerelation(&c0, &c2), EnumCircleRelationNoIntersect);
}

SP_CASE(icircle, icirclecontainspoint) {
    icircle c = {{0, 0}, 2.0};
    ipos p = {0, 0};
    
    SP_EQUAL(icirclecontainspoint(&c, &p), iiok);
    
    p.x = 2, p.y = 0;
    SP_EQUAL(icirclecontainspoint(&c, &p), iiok);
    
    p.x = 3, p.y = 0;
    SP_EQUAL(icirclecontainspoint(&c, &p), iino);
}

// **********************************************************************************
// iname
SP_SUIT(iformat);

SP_CASE(iformat, all) {
    printf("\n");
    
    ipos p = {1, 2};
    ipos *pp = &p;
    printf(__ipos_format"\n", __ipos_value(p));
    printf(__ipos_format"\n", __ipos_value(*pp));
    
    ipos3 p3 = {1, 2, 3};
    ipos3 *pp3 = &p3;
    printf(__ipos3_format"\n", __ipos3_value(p3));
    printf(__ipos3_format"\n", __ipos3_value(*pp3));
    
    isize s = {3, 4};
    isize *ps = &s;
    printf(__isize_format"\n", __isize_value(s));
    printf(__isize_format"\n", __isize_value(*ps));
    
    ivec2 v2 = {{1, 2}};
    ivec2 *pv2 = &v2;
    printf(__ivec2_format"\n", __ivec2_value(v2));
    printf(__ivec2_format"\n", __ivec2_value(*pv2));
    
    ivec3 v3 = {{1, 2, 3}};
    ivec3 *pv3 = &v3;
    printf(__ivec3_format"\n", __ivec3_value(v3));
    printf(__ivec3_format"\n", __ivec3_value(*pv3));
    
    iline2d line = {{2, 4}, {5, 6}};
    iline2d *pline = &line;
    printf(__iline2d_format"\n", __iline2d_value(line));
    printf(__iline2d_format"\n", __iline2d_value(*pline));
    
    iline3d line3d = {{4, 5, 6}, {7, 8, 9}};
    iline3d *pline3d = &line3d;
    printf(__iline3d_format"\n", __iline3d_value(line3d));
    printf(__iline3d_format"\n", __iline3d_value(*pline3d));
    
    iplane plane = {{{1, 2, 3}}, {4, 6, 7}, 5};
    iplane *pplane = &plane;
    printf(__iplane_format"\n", __iplane_value(plane));
    printf(__iplane_format"\n", __iplane_value(*pplane));
    
    irect r = {{2, 6}, {6, 9}};
    irect *pr = &r;
    printf(__irect_format"\n", __irect_value(r));
    printf(__irect_format"\n", __irect_value(*pr));
    
    icircle c = {{8,8}, 7};
    icircle *pc = &c;
    printf(__icircle_format"\n", __icircle_value(c));
    printf(__icircle_format"\n", __icircle_value(*pc));
    
    SP_TRUE(1);
}

// **********************************************************************************
// iname
SP_SUIT(iname);

SP_CASE(iname, nothing) {
    iname name = {{'A', 'B', 'C', 0}};
    
    SP_EQUAL(name.name[0], 'A');
    SP_EQUAL(name.name[1], 'B');
    SP_EQUAL(name.name[2], 'C');
    SP_EQUAL(name.name[3], 0);
}

// **********************************************************************************
// iref
SP_SUIT(iref);

SP_CASE(iref, iretainANDirelease) {
    iref *ref = iobjmalloc(iref);
    
    SP_EQUAL(ref->ref, 0);
    
    iretain(ref);
    
    SP_EQUAL(ref->ref, 1);
    
    iretain(ref);
    
    SP_EQUAL(ref->ref, 2);
    
    irelease(ref);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

static int facereffreecc = 0;
static void fake_ref_free(iref *ref) {
    iobjfree(ref);
    ++facereffreecc;
}

SP_CASE(iref, irelease) {
    iref *ref = iobjmalloc(iref);
    ref->free = fake_ref_free;
    
    SP_EQUAL(ref->ref, 0);
    
    iretain(ref);
    
    SP_EQUAL(ref->ref, 1);
    
    iretain(ref);
    
    SP_EQUAL(ref->ref, 2);
    
    irelease(ref);
    
    SP_EQUAL(ref->ref, 1);
    
    SP_EQUAL(facereffreecc, 0);
    irelease(ref);
    SP_EQUAL(facereffreecc, 1);
}

// **********************************************************************************
// ireflist
SP_SUIT(ireflist);

SP_CASE(ireflist, ireflistmake) {
    ireflist *list = ireflistmake();
    
    ireflistfree(list);
}

SP_CASE(ireflist, ireflistlen) {
    ireflist *list = ireflistmake();
    
    SP_EQUAL(ireflistlen(list), 0);
    
    ireflistfree(list);
}

SP_CASE(ireflist, ireflistfirst) {
    ireflist *list = ireflistmake();
    
    SP_EQUAL(ireflistfirst(list), NULL);
    
    ireflistfree(list);
}

SP_CASE(ireflist, ireflistadd) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SP_CASE(ireflist, ireflistaddANDireflistfirst) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfirst(list), joint);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SP_CASE(ireflist, ireflistfind) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SP_CASE(ireflist, ireflistaddjoint) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    ireflistaddjoint(list, add);
    
    SP_EQUAL(ireflistlen(list), 2);
    
    SP_EQUAL(ireflistfind(list, ref), add);
    
    SP_EQUAL(ireflistfirst(list), add);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SP_CASE(ireflist, ireflistremovejoint) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    SP_EQUAL(add->list, NULL);
    
    ireflistaddjoint(list, add);
    
    SP_EQUAL(add->list, list);
    
    SP_EQUAL(ireflistlen(list), 2);
    
    SP_EQUAL(ireflistfind(list, ref), add);
    
    SP_EQUAL(ireflistfirst(list), add);
    
    ireflistremovejoint(list, add);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    SP_EQUAL(ireflistfirst(list), joint);
    
    irefjointfree(add);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}


SP_CASE(ireflist, ireflistremove) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    ireflistaddjoint(list, add);
    
    SP_EQUAL(ireflistlen(list), 2);
    
    SP_EQUAL(ireflistfind(list, ref), add);
    
    SP_EQUAL(ireflistfirst(list), add);
    
    
    SP_EQUAL(add->list, list);
    
    ireflistremovejoint(list, add);
    
    SP_EQUAL(add->list, NULL);
    
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    SP_EQUAL(ireflistfirst(list), joint);
    
    ireflistremove(list, ref);
    
    SP_EQUAL(ireflistlen(list), 0);
    
    SP_EQUAL(ireflistfind(list, ref), NULL);
    
    SP_EQUAL(ireflistfirst(list), NULL);
    
    irefjointfree(add);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SP_CASE(ireflist, ireflistremoveall) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SP_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SP_EQUAL(ref->ref, 2);
    
    SP_EQUAL(ireflistlen(list), 1);
    
    SP_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    ireflistaddjoint(list, add);
    
    SP_EQUAL(ireflistlen(list), 2);
    
    SP_EQUAL(ireflistfind(list, ref), add);
    
    SP_EQUAL(ireflistfirst(list), add);
    
    ireflistremoveall(list);
    
    SP_EQUAL(ireflistlen(list), 0);
    
    SP_EQUAL(ireflistfind(list, ref), NULL);
    
    SP_EQUAL(ireflistfirst(list), NULL);
    
    ireflistfree(list);
    
    SP_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

// **********************************************************************************
// irefcache
SP_SUIT(irefcache);

static int fakecachenewcc = 0;

void fake_cache_free(iref *ref) {
    --fakecachenewcc;
    iobjfree(ref);
}

iref* fake_cache_new() {
    ++fakecachenewcc;
    iref *ref= iobjmalloc(iref);
    ref->free = fake_cache_free;
    iretain(ref);
    return ref;
}

SP_CASE(irefcache, irefcachemake) {
    irefcache *cache = irefcachemake(2, fake_cache_new);
    
    SP_EQUAL(cache->capacity, 2);
    
    SP_EQUAL(cache->newentry, fake_cache_new);
    
    irefcachefree(cache);
}

SP_CASE(irefcache, irefcachepoll) {
    irefcache *cache = irefcachemake(1, fake_cache_new);
    
    SP_EQUAL(cache->capacity, 1);
    
    SP_EQUAL(cache->newentry, fake_cache_new);
    
    SP_EQUAL(fakecachenewcc, 0);
    iref *ref = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 1);
    iref *ref0 = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref);
    SP_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref0);
    SP_EQUAL(fakecachenewcc, 1);
    
    iref *ref1 = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 1);
    SP_EQUAL(ref1, ref);
    
    irefcachepush(cache, ref1);
    
    irefcachefree(cache);
    
    SP_EQUAL(fakecachenewcc, 0);
}

SP_CASE(irefcache, irefcachepush) {
    irefcache *cache = irefcachemake(1, fake_cache_new);
    
    SP_EQUAL(cache->capacity, 1);
    
    SP_EQUAL(cache->newentry, fake_cache_new);
    
    SP_EQUAL(fakecachenewcc, 0);
    iref *ref = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 1);
    iref *ref0 = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref);
    SP_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref0);
    SP_EQUAL(fakecachenewcc, 1);
    
    iref *ref1 = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 1);
    SP_EQUAL(ref1, ref);
    
    irefcachepush(cache, ref1);
    
    irefcachefree(cache);
    
    SP_EQUAL(fakecachenewcc, 0);
}

SP_CASE(irefcache, irefcachefree) {
    irefcache *cache = irefcachemake(1, fake_cache_new);
    
    SP_EQUAL(cache->capacity, 1);
    
    SP_EQUAL(cache->newentry, fake_cache_new);
    
    SP_EQUAL(fakecachenewcc, 0);
    iref *ref = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 1);
    iref *ref0 = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref);
    SP_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref0);
    SP_EQUAL(fakecachenewcc, 1);
    
    iref *ref1 = irefcachepoll(cache);
    SP_EQUAL(fakecachenewcc, 1);
    SP_EQUAL(ref1, ref);
    
    irefcachepush(cache, ref1);
    
    irefcachefree(cache);
    
    SP_EQUAL(fakecachenewcc, 0);
}

// **********************************************************************************
// icode
SP_SUIT(icode);

SP_CASE(icode, nothing) {
    icode code = {{'A','B', 0}, {0, 0}};
    
    SP_EQUAL(code.code[0], 'A');
}

// **********************************************************************************
// iuserdata
SP_SUIT(iuserdata);

SP_CASE(iuserdata, nothing) {
    iuserdata u = {0, 0, 0, 0, NULL, NULL, NULL, NULL};
    
    SP_EQUAL(u.u1, 0);
    
    SP_EQUAL(u.up1, NULL);
}

// **********************************************************************************
// iunit
SP_SUIT(iunit);

SP_CASE(iunit, imakeunit) {
    iunit *unit = imakeunit(0, 0, 0);
    
    SP_EQUAL(unit->id, 0);
    
    ifreeunit(unit);
}

SP_CASE(iunit, ifreeunit) {
    iunit *unit = imakeunit(0, 0, 0);
    
    SP_EQUAL(unit->id, 0);
    
    ifreeunit(unit);
}

SP_CASE(iunit, ifreeunitlist) {
    iunit *unit = imakeunit(0, 0, 0);
    
    SP_EQUAL(unit->id, 0);
    
    ifreeunitlist(unit);
}

// **********************************************************************************
// inode
SP_SUIT(inode);

typedef struct node_graphics {
    inode *A;
    inode *B;
    inode *C;
    inode *D;
    inode *E;   
}node_graphics;

static void _inode_prepare_graphics(node_graphics *g) {
    g->A = imakenode();
    g->A->code.code[0] = 'A';
    
    g->B = imakenode();
    g->B->code.code[0] = 'B';
    
    g->C = imakenode();
    g->C->code.code[0] = 'C';
    
    g->D = imakenode();
    g->D->code.code[0] = 'D';
    
    g->E = imakenode();
    g->E->code.code[0] = 'E';
}

static void _inode_free_graphics(node_graphics *g) {
    ifreenodekeeper(g->A);
    ifreenodekeeper(g->B);
    ifreenodekeeper(g->C);
    ifreenodekeeper(g->D);
    ifreenodekeeper(g->E);
}

/*
 * A ---> B ---> C ---> D
 *        B ---> E
 * A ---> D
 *                      D ---> B
 *                                  E ---> B
 */
SP_CASE(inode, neighborsadd) {
    node_graphics g;
    _inode_prepare_graphics(&g);
    
    ineighborsadd(icast(irefneighbors, g.A), icast(irefneighbors, g.B));
    
    SP_EQUAL(g.A->neighbors_from, NULL);
    SP_EQUAL(g.B->neighbors_to, NULL);
    SP_EQUAL(ireflistlen(g.A->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.B->neighbors_from), 1);
    SP_EQUAL(ireflistfirst(g.B->neighbors_from)->value, irefcast(g.A));
    SP_EQUAL(ireflistfirst(g.A->neighbors_to)->value, irefcast(g.B));
    
    ineighborsadd(icast(irefneighbors, g.A), icast(irefneighbors, g.D));
    ineighborsadd(icast(irefneighbors, g.B), icast(irefneighbors, g.C));
    ineighborsadd(icast(irefneighbors, g.B), icast(irefneighbors, g.E));
    ineighborsadd(icast(irefneighbors, g.C), icast(irefneighbors, g.D));
    ineighborsadd(icast(irefneighbors, g.D), icast(irefneighbors, g.B));
    ineighborsadd(icast(irefneighbors, g.E), icast(irefneighbors, g.B));
    
    SP_EQUAL(ireflistlen(g.A->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.A->neighbors_from), 0)
    
    SP_EQUAL(ireflistlen(g.B->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.B->neighbors_from), 3);
    
    SP_EQUAL(ireflistlen(g.C->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.C->neighbors_from), 1);
    
    SP_EQUAL(ireflistlen(g.D->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.D->neighbors_from), 2);
    
    SP_EQUAL(ireflistlen(g.E->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.E->neighbors_from), 1);
    
    _inode_free_graphics(&g);
}

SP_CASE(inode, ineighborsdel) {
    
    node_graphics g;
    _inode_prepare_graphics(&g);
    ineighborsadd(icast(irefneighbors, g.A), icast(irefneighbors, g.B));
    ineighborsadd(icast(irefneighbors, g.A), icast(irefneighbors, g.D));
    ineighborsadd(icast(irefneighbors, g.B), icast(irefneighbors, g.C));
    ineighborsadd(icast(irefneighbors, g.B), icast(irefneighbors, g.E));
    ineighborsadd(icast(irefneighbors, g.C), icast(irefneighbors, g.D));
    ineighborsadd(icast(irefneighbors, g.D), icast(irefneighbors, g.B));
    ineighborsadd(icast(irefneighbors, g.E), icast(irefneighbors, g.B));
    SP_EQUAL(ireflistlen(g.A->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.A->neighbors_from), 0)
    
    SP_EQUAL(ireflistlen(g.B->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.B->neighbors_from), 3);
    
    SP_EQUAL(ireflistlen(g.C->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.C->neighbors_from), 1);
    
    SP_EQUAL(ireflistlen(g.D->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.D->neighbors_from), 2);
    
    SP_EQUAL(ireflistlen(g.E->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.E->neighbors_from), 1);
    
    ineighborsdel(icast(irefneighbors, g.A), icast(irefneighbors, g.B));
    SP_EQUAL(ireflistlen(g.A->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.A->neighbors_from), 0)
    
    SP_EQUAL(ireflistlen(g.B->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.B->neighbors_from), 2);
    
    
    _inode_free_graphics(&g);
}

SP_CASE(inode, ineighborsclean) {
    node_graphics g;
    _inode_prepare_graphics(&g);
    ineighborsadd(icast(irefneighbors, g.A), icast(irefneighbors, g.B));
    ineighborsadd(icast(irefneighbors, g.A), icast(irefneighbors, g.D));
    ineighborsadd(icast(irefneighbors, g.B), icast(irefneighbors, g.C));
    ineighborsadd(icast(irefneighbors, g.B), icast(irefneighbors, g.E));
    ineighborsadd(icast(irefneighbors, g.C), icast(irefneighbors, g.D));
    ineighborsadd(icast(irefneighbors, g.D), icast(irefneighbors, g.B));
    ineighborsadd(icast(irefneighbors, g.E), icast(irefneighbors, g.B));
    SP_EQUAL(ireflistlen(g.A->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.A->neighbors_from), 0)
    
    SP_EQUAL(ireflistlen(g.B->neighbors_to), 2)
    SP_EQUAL(ireflistlen(g.B->neighbors_from), 3);
    
    SP_EQUAL(ireflistlen(g.C->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.C->neighbors_from), 1);
    
    SP_EQUAL(ireflistlen(g.D->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.D->neighbors_from), 2);
    
    SP_EQUAL(ireflistlen(g.E->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.E->neighbors_from), 1);
    
    ineighborsclean(icast(irefneighbors, g.B));
    
    SP_EQUAL(ireflistlen(g.A->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.A->neighbors_from), 0)
    
    SP_EQUAL(ireflistlen(g.B->neighbors_to), 0)
    SP_EQUAL(ireflistlen(g.B->neighbors_from), 0);
    
    SP_EQUAL(ireflistlen(g.C->neighbors_to), 1)
    SP_EQUAL(ireflistlen(g.C->neighbors_from), 0);
    
    SP_EQUAL(ireflistlen(g.D->neighbors_to), 0)
    SP_EQUAL(ireflistlen(g.D->neighbors_from), 2);
    
    SP_EQUAL(ireflistlen(g.E->neighbors_to), 0)
    SP_EQUAL(ireflistlen(g.E->neighbors_from), 0);
    
    _inode_free_graphics(&g);
}

SP_CASE(inode, nothing) {
    inode *node = imakenode();
    ifreenodekeeper(node);
    
    SP_EQUAL(1, 1);
}


// **********************************************************************************
// imap
SP_SUIT(imap);

imap *map = NULL; // {0, 0}, {8, 8}, 3
// 是一个左闭右开区间
// AAA: [(0,0) -- (1.0, 1.0))
// AAD: [(1.0, 1.0) --- (2.0, 2.0))
/**
 MAPDETAIL 1" size=4
 |B| D|
 |_____
 |A| C|
 |_____
 MAPDETAIL 2" size=2
 |BB| BD| DB| DD|
 |________________
 |BA| BC| DA| DC|
 |________________
 |AB| AD| CB| CD|
 |________________
 |AA| AC| CA| CC|
 |________________
 MAPDETAIL 3" size=1
 |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
 |_______________________________________
 |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
 |_______________________________________
 |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
 |_______________________________________
 |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
 |_______________________________________
 |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
 |_______________________________________
 |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
 |_______________________________________
 |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
 |_______________________________________
 |AAA| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
 |_______________________________________
  */
void free_test_map() {
    if(map) {
        imapfree(map);
        map = NULL;
    }
}

void make_test_map() {
    free_test_map();
    ipos pos = {0, 0};
    isize size = {8, 8};
    map = imapmake(&pos, &size, 3);
}

SP_CASE(imap, imapmake) {
    make_test_map();
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
    
    SP_EQUAL(map->root != NULL, 1);
    SP_EQUAL(map->root->level, 0);
    SP_EQUAL(map->root->code.code[0], 'R');
    SP_EQUAL(map->root->code.code[1], 'O');
    SP_EQUAL(map->root->code.code[2], 'O');
    SP_EQUAL(map->root->code.code[3], 'T');
    
    SP_EQUAL(map->nodesizes[0].w, 8);
    SP_EQUAL(map->nodesizes[0].h, 8);
    
    SP_EQUAL(map->nodesizes[1].w, 4);
    SP_EQUAL(map->nodesizes[1].h, 4);
    
    SP_EQUAL(map->nodesizes[2].w, 2);
    SP_EQUAL(map->nodesizes[2].h, 2);
    
    SP_EQUAL(map->nodesizes[3].w, 1);
    SP_EQUAL(map->nodesizes[3].h, 1);
    
    SP_EQUAL(map->distances[0], 128);
    SP_EQUAL(map->distances[1], 32);
    SP_EQUAL(map->distances[2], 8);
    SP_EQUAL(map->distances[3], 2);
    
    
}

SP_CASE(imap, imapgencode) {
    icode code;
    
    ipos p = {0, 0};
    
    imapgencode(map, &p, &code);
    
    SP_EQUAL(code.pos.x, p.x);
    SP_EQUAL(code.pos.y, p.y);
    
    SP_EQUAL(code.code[0], 'A');
    SP_EQUAL(code.code[1], 'A');
    SP_EQUAL(code.code[2], 'A');
    SP_EQUAL(code.code[3], 0);
    
    p.x = 1.0;
    p.y = 1.0;
    
    imapgencode(map, &p, &code);
    
    SP_EQUAL(code.code[0], 'A');
    SP_EQUAL(code.code[1], 'A');
    SP_EQUAL(code.code[2], 'D');
    SP_EQUAL(code.code[3], 0);
}

SP_CASE(imap, imapgenpos) {
    
    icode code = {{'A', 'A', 'A', 0}};
    
    ipos p = {0, 0};
    
    imapgenpos(map, &p, &code);
    
    SP_EQUAL(0, p.x);
    SP_EQUAL(0, p.y);
    
    code.code[2] = 'D';
    
    imapgenpos(map, &p, &code);
    
    SP_EQUAL(1.0, p.x);
    SP_EQUAL(1.0, p.y);
}

///简单的单元管理
static int64_t gid = 0;
static iunit* gunits[1000] = {};

#define __preparecnt 20

#define __getmeunit(x, y) imakeunit(gid++, x, y)

#define __hold(u) gunits[u->id] = u; iretain(u)

#define __getunitfor(id) gunits[id]

#define __unhold(id) irelease(gunits[id]); gunits[id] = NULL

#define __setu(xid, xx, xy) do { gunits[xid]->pos.x = xx; gunits[xid]->pos.y = xy; } while(0)

#define __resetu(xid) __setu(xid, 0, 0)

#define __setcode(code, a, b, c) code.code[0] = a; code.code[1] = b; code.code[2] = c

#define __getnode(code, level) imapgetnode(map, &code, level, EnumFindBehaviorAccurate)

// 准备20个在原点的单元
SP_CASE(imap, prepareunit) {
    
    for (int i=0; i<__preparecnt; ++i) {
        iunit *u = __getmeunit(0, 0);
        __hold(u);
        ifreeunit(u);
    }
    
    SP_TRUE(1);
}

SP_CASE(imap, imapaddunit) {
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
    
    imapaddunit(map, __getunitfor(0));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 1);
}

SP_CASE(imap, imapremoveunit) {
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 1);
    
    imapremoveunit(map, __getunitfor(0));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
}

SP_CASE(imap, complexANDimapaddunitANDimapremoveunitANDimapgetnode) {
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
    
    imapaddunit(map, __getunitfor(0));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 1);
    
    __setu(1, 0.5, 0.7);
    __setu(2, 0.99, 0.2);
    
    imapaddunit(map, __getunitfor(1));
    imapaddunit(map, __getunitfor(2));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0, 1, 2]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    SP_EQUAL(map->divide, 3);
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 3);
    
    icode code = {{'A', 'A', 'A', 0}};
    
    inode *node = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate);
    
    SP_EQUAL(node, map->root);
    
    node = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate);
    
    SP_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 0, 1);
    
    node = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate);
    
    SP_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 'A'
                     && node->code.code[2] == 0, 1);
    
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    
    SP_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 'A'
                     && node->code.code[2] == 'A'
                     && node->code.code[3] == 0, 1);
    
    SP_EQUAL(node, __getunitfor(0)->node);
    SP_EQUAL(node, __getunitfor(1)->node);
    SP_EQUAL(node, __getunitfor(2)->node);
    
    __setu(3, 1, 1);
    
    imapaddunit(map, __getunitfor(3));
    
    SP_EQUAL(map->state.nodecount, 4);
    SP_EQUAL(map->state.leafcount, 2);
    SP_EQUAL(map->state.unitcount, 4);
    
    code.code[2] = 'D';
    
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    
    SP_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 'A'
                     && node->code.code[2] == 'D'
                     && node->code.code[3] == 0, 1);
    
    SP_EQUAL(node, __getunitfor(3)->node);
    SP_EQUAL(node->x, 1);
    SP_EQUAL(node->y, 1);
    
    code.code[0] = 'D';
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node, NULL);
    node = imapgetnode(map, &code, 3, EnumFindBehaviorFuzzy);
    SP_EQUAL(node, map->root);
    code.code[0] = 'A';
    code.code[1] = 'A';
    code.code[2] = 'B';
    node = imapgetnode(map, &code, 3, EnumFindBehaviorFuzzy);
    SP_EQUAL(node, map->root->childs[0]->childs[0]);
    code.code[1] = 'D';
    node = imapgetnode(map, &code, 3, EnumFindBehaviorFuzzy);
    SP_EQUAL(node, map->root->childs[0]);
    
    // _aoi_print(map, EnumNodePrintStateAll);
    
    imapremoveunit(map, __getunitfor(3));
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 3);
    
    SP_EQUAL(irefcachesize(map->nodecache), 1);
    
    imapremoveunit(map, __getunitfor(0));
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 2);
    
    imapremoveunit(map, __getunitfor(1));
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 1);
    
    imapremoveunit(map, __getunitfor(2));
    
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
    
    SP_EQUAL(irefcachesize(map->nodecache), 4);
    
    SP_EQUAL(NULL, __getunitfor(0)->node);
    SP_EQUAL(NULL, __getunitfor(1)->node);
    SP_EQUAL(NULL, __getunitfor(2)->node);
    SP_EQUAL(NULL, __getunitfor(3)->node);
    
    __resetu(0);
    __resetu(1);
    __resetu(2);
    __resetu(3);
}


SP_CASE(imap, imapupdateunit) {
    
    __setu(0, 0.8, 0.8);
    
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
    
    imapaddunit(map, __getunitfor(0));
    icode code = {{'A','A','A',0}};
    inode *node = NULL;
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 1);
    SP_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
                     __getunitfor(0)->node);
    
    __setu(0, 0.1, 0.1);
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    imapupdateunit(map, __getunitfor(0));
    SP_EQUAL(map->state.nodecount, 3);
    SP_EQUAL(map->state.leafcount, 1);
    SP_EQUAL(map->state.unitcount, 1);
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node, __getunitfor(0)->node);
    
    SP_EQUAL(node->x, 0);
    SP_EQUAL(node->y, 0);
    
    
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    code.code[0] = 'A'; code.code[1] = 'A'; code.code[2] = 'A';
    int64_t ts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t tsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t tsAA = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    int64_t tsAAA = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    
    __resetu(1);
    imapaddunit(map, __getunitfor(1));
    int64_t nts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t ntsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t ntsAA = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    int64_t ntsAAA = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    
    SP_EQUAL(nts > ts, 1);
    SP_EQUAL(ntsA > tsA, 1);
    SP_EQUAL(ntsAA > tsAA, 1);
    SP_EQUAL(ntsAAA > tsAAA, 1);
    
    __setu(0, 1.0, 1.0);
    imapupdateunit(map, __getunitfor(0));
    
    code.code[0] = 'A';
    code.code[1] = 'A';
    code.code[2] = 'D';
    SP_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
                     __getunitfor(0)->node);
    
    int64_t nnts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t nntsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t nntsAA = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    int64_t nntsAAA = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    SP_EQUAL(nts, nnts);
    SP_EQUAL(ntsA, nntsA);
    SP_EQUAL(ntsAA, nntsAA);
    SP_EQUAL(ntsAAA != nntsAAA, 1);
    code.code[2] = 'D';
    int64_t nntsAAD = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    
    SP_EQUAL(nntsAAD >= ntsAAA, 1);
    
    SP_EQUAL(map->state.nodecount, 4);
    SP_EQUAL(map->state.leafcount, 2);
    SP_EQUAL(map->state.unitcount, 2);
    
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[0]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[1]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    __setu(0, 3.0, 3.0);
    imapupdateunit(map, __getunitfor(0));
    // AAD --> ADD
    code.code[0] = 'A';
    code.code[1] = 'A';
    code.code[2] = 'D';
    
    int64_t nnnts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t nnntsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t nnntsAA = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    SP_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate), NULL);
    
    code.code[0] = 'A';
    code.code[1] = 'D';
    code.code[2] = 'D';
    
    SP_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
                     __getunitfor(0)->node);
    
    int64_t nnnnts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t nnnntsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t nnnntsAD = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    int64_t nnnntsADD = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    
    SP_EQUAL(nnnnts, nnnts);
    SP_EQUAL(nnnntsA, nnntsA);
    SP_EQUAL(nnnntsAD > nnntsAA, 1);
    SP_EQUAL(nnnntsADD > nnntsAA, 1);
    
    imapremoveunit(map, __getunitfor(0));
    imapremoveunit(map, __getunitfor(1));
    
    SP_EQUAL(map->state.nodecount, 0);
    SP_EQUAL(map->state.leafcount, 0);
    SP_EQUAL(map->state.unitcount, 0);
}

SP_CASE(imap, imapmovecodeAndimapgencode) {
    int divide = 20;
    //int randmove = 1024;
    int maxmove = (int)pow(2, divide) - 1;
    ipos pos = {0, 0};
    isize size = {512, 512};
    imap *xxmap = imapmake(&pos, &size, divide);
    
    icode code;
    imapgencode(xxmap, &pos, &code);
    
    int64_t t0 = igetcurmicro();
    for (int i=0; i <maxmove; ++i) {
        imapmovecode(xxmap, &code, EnumCodeMoveUp);
    }
    int64_t e0 = igetcurmicro() - t0;
    
    int64_t t1 = igetcurmicro();
    for (int i=0; i <maxmove; ++i) {
        imapgencode(xxmap, &pos, &code);
    }
    int64_t e1 = igetcurmicro() - t1;
    
    printf("move-code:%lld , gen-code:%lld \n ", e0, e1);
    
    imapfree(xxmap);
    // move code is more fast
    SP_TRUE(e1 > e0);
    
    SP_TRUE(1);
}

SP_CASE(imap, imapmovecode) {
    int divide = 20;
    int randmove = 1024;
    int maxmove = (int)pow(2, divide) - 1;
    ipos pos = {0, 0};
    isize size = {512, 512};
    imap *xxmap = imapmake(&pos, &size, divide); 

    icode code;
    imapgencode(xxmap, &pos, &code);

    printf("code: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
    SP_EQUAL(strlen(code.code), xxmap->divide);

    icode xcode;
    printf("map divide precisions: %f, %f \n", 
            xxmap->nodesizes[xxmap->divide].w, 
            xxmap->nodesizes[xxmap->divide].h);

    // maxmove Up
    printf("Move Up ( step %d)\n", maxmove);
    for(int i=0; i<maxmove; ++i) {

        imapmovecode(xxmap, &code, EnumCodeMoveUp);

        pos.x = pos.x;
        pos.y += xxmap->nodesizes[xxmap->divide].h;
        imapgencode(xxmap, &pos, &xcode);
        //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
        //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

        SP_EQUAL(code.pos.x, pos.x);
        SP_EQUAL(code.pos.y, pos.y);

        SP_TRUE(strcmp(xcode.code, code.code) == 0);
    }

    // Down
    printf("Move Down ( step %d)\n", maxmove);
    for(int i=0; i<maxmove; ++i) {

        imapmovecode(xxmap, &code, EnumCodeMoveDown);

        pos.x = pos.x;
        pos.y -= xxmap->nodesizes[xxmap->divide].h;
        imapgencode(xxmap, &pos, &xcode);
        //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
        //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

        SP_EQUAL(code.pos.x, pos.x);
        SP_EQUAL(code.pos.y, pos.y);

        SP_TRUE(strcmp(xcode.code, code.code) == 0);
    }

    // Right
    printf("Move Right ( step %d)\n", maxmove);
    for(int i=0; i<maxmove; ++i) {

        imapmovecode(xxmap, &code, EnumCodeMoveRight);

        pos.x += xxmap->nodesizes[xxmap->divide].w;
        pos.y = pos.y;
        imapgencode(xxmap, &pos, &xcode);
        //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
        //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

        SP_EQUAL(code.pos.x, pos.x);
        SP_EQUAL(code.pos.y, pos.y);

        SP_TRUE(strcmp(xcode.code, code.code) == 0);
    }

    // Left
    printf("Move Left ( step %d)\n", maxmove);
    for(int i=0; i<maxmove; ++i) {

        imapmovecode(xxmap, &code, EnumCodeMoveLeft);

        pos.x -= xxmap->nodesizes[xxmap->divide].w;
        pos.y = pos.y;
        imapgencode(xxmap, &pos, &xcode);
        //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
        //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

        SP_EQUAL(code.pos.x, pos.x);
        SP_EQUAL(code.pos.y, pos.y);

        SP_TRUE(strcmp(xcode.code, code.code) == 0);
    }

    // 随机左移
    printf("Rand Move Left ( step %d)\n", randmove);
    for (int i=0; i<randmove; ++i) {
        pos.x = rand() % 512;
        pos.y = rand() % 512;

        imapgencode(xxmap, &pos, &xcode);
        //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);
        if ( imapmovecode(xxmap, &xcode, EnumCodeMoveLeft) > 0 ) {
            pos.x -= xxmap->nodesizes[xxmap->divide].w;

            SP_EQUAL(xcode.pos.x, pos.x);
            SP_EQUAL(xcode.pos.y, pos.y);

            imapgencode(xxmap, &pos, &code);
            //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
            //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

            SP_TRUE(strcmp(xcode.code, code.code) == 0);
        }
    }

    // 随机右移
    printf("Rand Move Right ( step %d)\n", randmove);
    for (int i=0; i<randmove; ++i) {
        pos.x = rand() % 512;
        pos.y = rand() % 512;

        imapgencode(xxmap, &pos, &xcode);
        if ( imapmovecode(xxmap, &xcode, EnumCodeMoveRight) > 0 ) {
            pos.x += xxmap->nodesizes[xxmap->divide].w;

            SP_EQUAL(xcode.pos.x, pos.x);
            SP_EQUAL(xcode.pos.y, pos.y);

            imapgencode(xxmap, &pos, &code);
            //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
            //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

            SP_TRUE(strcmp(xcode.code, code.code) == 0);
        }
    }

    // 随机上移
    printf("Rand Move Up ( step %d)\n", randmove);
    for (int i=0; i<randmove; ++i) {
        pos.x = rand() % 512;
        pos.y = rand() % 512;

        imapgencode(xxmap, &pos, &xcode);
        if ( imapmovecode(xxmap, &xcode, EnumCodeMoveUp) > 0 ) {
            pos.y += xxmap->nodesizes[xxmap->divide].h;

            SP_EQUAL(xcode.pos.x, pos.x);
            SP_EQUAL(xcode.pos.y, pos.y);

            imapgencode(xxmap, &pos, &code);
            //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
            //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

            SP_TRUE(strcmp(xcode.code, code.code) == 0);
        }
    }

    // 随机下移
    printf("Rand Move Down ( step %d)\n", randmove);
    for (int i=0; i<randmove; ++i) {
        pos.x = rand() % 512;
        pos.y = rand() % 512;

        imapgencode(xxmap, &pos, &xcode);
        if ( imapmovecode(xxmap, &xcode, EnumCodeMoveDown) > 0 ) {
            pos.y -= xxmap->nodesizes[xxmap->divide].h;

            SP_EQUAL(xcode.pos.x, pos.x);
            SP_EQUAL(xcode.pos.y, pos.y);

            imapgencode(xxmap, &pos, &code);
            //printf("icode: %s, %f, %f\n", code.code, code.pos.x, code.pos.y);
            //printf("xcode: %s, %f, %f\n", xcode.code, xcode.pos.x, xcode.pos.y);

            SP_TRUE(strcmp(xcode.code, code.code) == 0);
        }
    }

    imapfree(xxmap);
}

SP_CASE(imap, imapmovecodeedge) {
    ipos p = {0, 0};
    isize s = {512, 512};

    imap *xxmap = imapmake(&p, &s, 10);

    icode code;
    icode xcode;

    imapgencode(xxmap, &p, &code);
    imapgencode(xxmap, &p, &xcode);

    SP_TRUE(strcmp(code.code, xcode.code) == 0);

    // Can not move left
    { 
    int move = imapmovecode(xxmap, &code, EnumCodeMoveLeft);
    SP_EQUAL(move, 0);
    SP_TRUE(strcmp(code.code, xcode.code) == 0);
    SP_EQUAL(code.pos.x, p.x);
    SP_EQUAL(code.pos.y, p.y);
    }
    // Can not move down
    {
    int move = imapmovecode(xxmap, &code, EnumCodeMoveDown);
    SP_EQUAL(move, 0);
    SP_TRUE(strcmp(code.code, xcode.code) == 0);
    SP_EQUAL(code.pos.x, p.x);
    SP_EQUAL(code.pos.y, p.y);
    }
    // Can Move right
    {
    int move = imapmovecode(xxmap, &code, EnumCodeMoveRight);
    SP_EQUAL(move, 1);
    p.x += xxmap->nodesizes[xxmap->divide].w;
    imapgencode(xxmap, &p, &xcode);
    SP_TRUE(strcmp(code.code, xcode.code) == 0);
    SP_EQUAL(code.pos.x, p.x);
    SP_EQUAL(code.pos.y, p.y);
    }
    // Can Move up
    {
	    int move;
	    p.x = 0;
	    imapgencode(xxmap, &p, &code);
	    move = imapmovecode(xxmap, &code, EnumCodeMoveUp);
	    SP_EQUAL(move, 1);
	    p.y += xxmap->nodesizes[xxmap->divide].h;
	    imapgencode(xxmap, &p, &xcode);
	    SP_TRUE(strcmp(code.code, xcode.code) == 0);
	    SP_EQUAL(code.pos.x, p.x);
	    SP_EQUAL(code.pos.y, p.y);
    }

    imapfree(xxmap);

    SP_TRUE(1)
}

SP_CASE(imap, nodepos) {
    ipos p = {0, 0};
    isize s = {8, 8};
    
    imap *xxmap = imapmake(&p, &s, 3);
    
    SP_EQUAL(xxmap->root->x, 0);
    SP_EQUAL(xxmap->root->y, 0);
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[0]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[1]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    iunit* u0 = imakeunit(0, 1, 1);
    imapaddunit(xxmap, u0);
    icode code = {{'A', 'A', 'D', 0}};
    inode *node = imapgetnode(xxmap, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node->x, 1);
    SP_EQUAL(node->y, 1);
    
    u0->pos.x = 7;
    u0->pos.y = 0;
    imapupdateunit(xxmap, u0);
    code.code[0] = 'C';
    code.code[1] = 'C';
    code.code[2] = 'C';
    node = imapgetnode(xxmap, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node->x, 7);
    SP_EQUAL(node->y, 0);
    
    u0->pos.x = 0;
    u0->pos.y = 7;
    imapupdateunit(xxmap, u0);
    code.code[0] = 'B';
    code.code[1] = 'B';
    code.code[2] = 'B';
    node = imapgetnode(xxmap, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node->x, 0);
    SP_EQUAL(node->y, 7);
    
    u0->pos.x = 7;
    u0->pos.y = 7;
    imapupdateunit(xxmap, u0);
    code.code[0] = 'D';
    code.code[1] = 'D';
    code.code[2] = 'D';
    node = imapgetnode(xxmap, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node->x, 7);
    SP_EQUAL(node->y, 7);
    
    u0->pos.x = 4;
    u0->pos.y = 5;
    imapupdateunit(xxmap, u0);
    code.code[0] = 'D';
    code.code[1] = 'A';
    code.code[2] = 'B';
    node = imapgetnode(xxmap, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(node->x, 4);
    SP_EQUAL(node->y, 5);
    
    
    ifreeunit(u0);
    
    imapfree(xxmap);
}

SP_CASE(imap, end) {
    SP_TRUE(1);
}

// **********************************************************************************
// ifilter
SP_SUIT(ifilter);

SP_CASE(ifilter, ifiltermake) {
    make_test_map();
    
    ifilter *filter = ifiltermake();
    
    SP_EQUAL(ifilterchecksum(map, filter), 0);
    
    ifilterfree(filter);
}

SP_CASE(ifilter, ifiltermake_circle) {
    
    ipos pos = {0, 0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SP_EQUAL(ifilterchecksum(map, filterrange) != 0, 1);
    
    SP_EQUAL(filterrange->s.u.circle.radius, 2.0);
    
    SP_EQUAL(filterrange->s.u.circle.pos.x, 0);
    SP_EQUAL(filterrange->s.u.circle.pos.y, 0);
    
    ifilterfree(filterrange);
}

SP_CASE(ifilter, ifiltermake_rect) {
    ipos pos = {0, 0};
    isize size = {2, 2};
    ifilter *filterrect = ifiltermake_rect(&pos, &size);
    
    SP_EQUAL(ifilterchecksum(map, filterrect) != 0, 1);
    SP_EQUAL(filterrect->s.u.rect.pos.x, 0);
    SP_EQUAL(filterrect->s.u.rect.pos.y, 0);
    SP_EQUAL(filterrect->s.u.rect.size.w, 2);
    SP_EQUAL(filterrect->s.u.rect.size.h, 2);
    
    ifilterfree(filterrect);
}

SP_CASE(ifilter, ifilteradd) {
    ifilter *filter = ifiltermake();
    
    ipos pos = {0, 0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SP_EQUAL(ifilterchecksum(map, filter), 0);
    
    ifilteradd(filter, filterrange);
    
    SP_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

SP_CASE(ifilter, ifilterremove) {
    ifilter *filter = ifiltermake();
    
    ipos pos = {0, 0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SP_EQUAL(ifilterchecksum(map, filter), 0);
    
    SP_EQUAL(ireflistlen(filter->s.list), 0);
    
    ifilteradd(filter, filterrange);
    
    SP_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    SP_EQUAL(ireflistlen(filter->s.list), 1);
    
    ifilterremove(filter, filterrange);
    
    SP_EQUAL(ireflistlen(filter->s.list), 0);
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

SP_CASE(ifilter, ifilterclean) {
    ifilter *filter = ifiltermake();
    
    ipos pos = {0, 0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SP_EQUAL(ifilterchecksum(map, filter), 0);
    
    SP_EQUAL(ireflistlen(filter->s.list), 0);
    
    ifilteradd(filter, filterrange);
    ifilteradd(filter, filterrange);
    
    SP_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    SP_EQUAL(ireflistlen(filter->s.list), 2);
    
    ifilterclean(filter);
    
    SP_EQUAL(ireflistlen(filter->s.list), 0);
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

int __filter_test_forid(imap *map, const ifilter *filter, const iunit *unit) {
    icheckret(filter, iino);
    icheckret(unit, iino);
    
    if (unit->id == filter->s.u.id) {
        return iiok;
    }
    
    return iino;
}

SP_CASE(ifilter, ifilterrun) {
    
    // prepare units in map
    __setu(0, 0.1, 0.1);
    
    imapaddunit(map, __getunitfor(0));
    
    __setu(1, 0.3, 0.3);
    
    imapaddunit(map, __getunitfor(1));
    
    __setu(2, 1.0, 1.0);
    
    imapaddunit(map, __getunitfor(2));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[0]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[2]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
 
    
    ifilter *filter = ifiltermake();
    
    int ok ;
    ok = ifilterrun(map, filter, __getunitfor(0));
    SP_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(1));
    SP_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(2));
    SP_EQUAL(ok, iiok);
    
    ipos pos = {0, 0};
    ifilter *filterrange = ifiltermake_circle(&pos, 1.0);
    
    SP_EQUAL(ifilterchecksum(map, filter), 0);
    
    SP_EQUAL(ireflistlen(filter->s.list), 0);
    
    ifilteradd(filter, filterrange);
    
    SP_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    ok = ifilterrun(map, filter, __getunitfor(0));
    SP_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(1));
    SP_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(2));
    SP_EQUAL(ok, iino);
    
    __setu(0, -0.3, -0.4);
    imapupdateunit(map, __getunitfor(0));
    SP_EQUAL(__getunitfor(0)->node->code.code[2], 'A');
    ok = ifilterrun(map, filter, __getunitfor(0));
    SP_EQUAL(ok, iiok);
    
    __setu(0, 1.3, 1.4);
    imapupdateunit(map, __getunitfor(0));
    SP_EQUAL(__getunitfor(0)->node->code.code[2], 'D');
    ok = ifilterrun(map, filter, __getunitfor(0));
    SP_EQUAL(ok, iino);
    
    // 过滤ID
    ok = ifilterrun(map, filter, __getunitfor(1));
    SP_EQUAL(ok, iiok);
    filter->s.u.id = 2;
    filter->entry = __filter_test_forid;
    ok = ifilterrun(map, filter, __getunitfor(1));
    SP_EQUAL(ok, iino);
    
    // 清理 map
    imapremoveunit(map, __getunitfor(0));
    imapremoveunit(map, __getunitfor(1));
    imapremoveunit(map, __getunitfor(2));
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

SP_CASE(ifilter, imapcollectunit) {
    
    // prepare units in map
    __setu(0, 0.1, 0.0);
    
    imapaddunit(map, __getunitfor(0));
    
    __setu(1, 0.3, 0.4);
    
    imapaddunit(map, __getunitfor(1));
    
    __setu(2, 0.5, 0.0);
    
    imapaddunit(map, __getunitfor(2));
    
    __setu(3, 1.5, 1.2);
    
    imapaddunit(map, __getunitfor(3));
    
    __setu(4, 1.5, 0.2);
    
    imapaddunit(map, __getunitfor(4));
    
    __setu(5, 1.0, 0.0);
    
    imapaddunit(map, __getunitfor(5));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[0]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3,5]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2]| AAC:[4]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    
    ifilter *filter = ifiltermake();
    
    ipos pos = {0, 0};
    ifilter *filterrange = ifiltermake_circle(&pos, 1.0);
    
    SP_EQUAL(ifilterchecksum(map, filter), 0);
    
    SP_EQUAL(ireflistlen(filter->s.list), 0);
    
    ifilteradd(filter, filterrange);
    
    SP_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    ireflist *list = ireflistmake();
    
    icode code = {{'A', 'A', 0}};
    
    inode *node = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate);
    SP_EQUAL(node->x, 0);
    SP_EQUAL(node->y, 0);

#define __range(x) (x)
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (3: 1.5, 1.2) (4: 1.5, 0.2) (5: 1.0, 1.0)
    // ------------------------------------------------------------------------------------
    ireflist *snap = ireflistmake();
    filterrange->s.u.circle.radius = __range(2.0);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (3: 1.5, 1.2) (4: 1.5, 0.2) (5: 1.0, 1.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SP_EQUAL(ireflistlen(list), 6);
    
    ireflistremoveall(list);
    filterrange->s.u.circle.radius = __range(0.1);
    // (0: 0.1, 0.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SP_EQUAL(ireflistlen(list), 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SP_EQUAL(icast(iunit, ireflistfirst(list)->value)->id, 0);
    
    ireflistremoveall(list);
    filterrange->s.u.circle.radius = __range(0.5);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SP_EQUAL(ireflistlen(list), 3);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(1))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(2))) != NULL, 1);
    
    ireflistremoveall(list);
    filterrange->s.u.circle.radius = __range(1.5);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (5: 1.0, 1.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SP_EQUAL(ireflistlen(list), 4);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(1))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(2))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(5))) != NULL, 1);
    
    ireflistremoveall(list);
    filterrange->s.u.circle.radius = __range(1.7);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (4: 1.5, 0.2) (5: 1.0, 1.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SP_EQUAL(ireflistlen(list), 5);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(1))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(2))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(4))) != NULL, 1);
    SP_EQUAL(ireflistfind(list, irefcast(__getunitfor(5))) != NULL, 1);
    
    // 清理 map
    imapremoveunit(map, __getunitfor(0));
    imapremoveunit(map, __getunitfor(1));
    imapremoveunit(map, __getunitfor(2));
    imapremoveunit(map, __getunitfor(3));
    imapremoveunit(map, __getunitfor(4));
    imapremoveunit(map, __getunitfor(5));
    
    ifilterfree(filter);
    ifilterfree(filterrange);
    ireflistfree(list);
    ireflistfree(snap);
}

SP_CASE(ifilter, end) {
    SP_TRUE(1);
}


// **********************************************************************************
// isearchresult
SP_SUIT(isearchresult);

SP_CASE(isearchresult, isearchresultmake) {
    isearchresult *search = isearchresultmake();
    
    SP_EQUAL(search->filter != NULL, 1);
    SP_EQUAL(search->units != NULL, 1);
    
    isearchresultfree(search);
}

SP_CASE(isearchresult, isearchresultattach) {
    isearchresult *search = isearchresultmake();
    
    SP_EQUAL(search->filter != NULL, 1);
    SP_EQUAL(search->units != NULL, 1);
    
    ifilter *filter = ifiltermake();
    isearchresultattach(search, filter);
    SP_EQUAL(search->filter, filter);
    
    isearchresultfree(search);
    ifilterfree(filter);
}

SP_CASE(isearchresult, isearchresultdettach) {
    isearchresult *search = isearchresultmake();
    
    SP_EQUAL(search->filter != NULL, 1);
    SP_EQUAL(search->units != NULL, 1);
    
    ifilter *filter = ifiltermake();
    isearchresultattach(search, filter);
    SP_EQUAL(search->filter, filter);
    
    isearchresultdettach(search);
    
    SP_EQUAL(search->filter, NULL);
    
    isearchresultfree(search);
    ifilterfree(filter);
}

// **********************************************************************************
SP_SUIT(searching);

SP_CASE(searching, imapsearchfromposANDimapsearchfromnode) {
    // prepare units in map
    __setu(0, 0.1, 0.0);
    
    imapaddunit(map, __getunitfor(0));
    
    __setu(1, 0.3, 0.4);
    
    imapaddunit(map, __getunitfor(1));
    
    __setu(2, 0.5, 0.0);
    
    imapaddunit(map, __getunitfor(2));
    
    __setu(3, 1.5, 1.2);
    
    imapaddunit(map, __getunitfor(3));
    
    __setu(4, 1.5, 0.2);
    
    imapaddunit(map, __getunitfor(4));
    
    __setu(5, 1.0, 0.0);
    
    imapaddunit(map, __getunitfor(5));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3,5]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2]| AAC:[4]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    isearchresult *result = isearchresultmake();
    
    SP_EQUAL(result->tick, 0);
    SP_EQUAL(result->checksum, 0);
    
    ipos pos = {0,0};
    icode code = {{'A','A','A',0}};
    imapsearchfrompos(map, &pos, result, 0.1);
    int64_t ts = result->tick;
    int64_t checksum = result->checksum;
    inode *node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(ts, node->tick);
    SP_EQUAL(ireflistlen(result->units), 1);
    SP_EQUAL(checksum != 0, 1);
    
    imapsearchfrompos(map, &pos, result, 0.1);
    ts = result->tick;
    SP_EQUAL(ts, node->tick);
    SP_EQUAL(ireflistlen(result->units), 1);
    SP_EQUAL(checksum, result->checksum);
    
    // add unit 6
    __setu(6, 0.05, 0.05);
    imapaddunit(map, __getunitfor(6));
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3,5]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2,6]| AAC:[4]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    imapsearchfrompos(map, &pos, result, 0.1);
    int64_t nts = result->tick;
    SP_EQUAL(nts, node->tick);
    SP_EQUAL(nts > ts, 1);
    SP_EQUAL(ireflistlen(result->units), 2);
    SP_EQUAL(checksum != result->checksum, 1);
    checksum = result->checksum;
    
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 1.0)    (6: 0.05, 0.05)
    // ------------------------------------------------------------------------------------
    imapsearchfrompos(map, &pos, result, 0.5);
    int64_t nnts = result->tick;
    SP_EQUAL(result->tick, node->tick);
    SP_EQUAL(nnts == nts, 1);
    SP_EQUAL(ireflistlen(result->units), 4);
    SP_EQUAL(checksum != result->checksum, 1);
    
    
    __setu(7, 3.99, 3.99);
    imapaddunit(map, __getunitfor(7));
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[7]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3,5]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2,6]| AAC:[4]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    pos.x = 4;
    pos.y = 4;
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 1.0)    (6: 0.05, 0.05) (7:3.99, 3.99)
    // ------------------------------------------------------------------------------------
    imapsearchfrompos(map, &pos, result, 0.5);
    int64_t nnnts = result->tick;
    __setcode(code, 'A', 'D', 'D');
    node = __getnode(code, 3);
    SP_EQUAL(result->tick, node->tick);
    SP_EQUAL(nnnts == nnts, 0);
    SP_EQUAL(ireflistlen(result->units), 1);
    SP_EQUAL(checksum != result->checksum, 1);
    
    __setu(8, 4.01, 4.01);
    imapaddunit(map, __getunitfor(8));
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA:[8]| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[7]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3,5]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2,6]| AAC:[4]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 1.0)    (6: 0.05, 0.05) (7:3.99, 3.99)  (8: 4.01, 4.01)
    // ------------------------------------------------------------------------------------
    imapsearchfrompos(map, &pos, result, 0.5);
    int64_t nnnnts = result->tick;
    __setcode(code, 'A', 'D', 'D');
    node = __getnode(code, 0);
    SP_EQUAL(result->tick, node->tick);
    SP_EQUAL(nnnnts == nnnts, 0);
    SP_EQUAL(ireflistlen(result->units), 2);
    SP_EQUAL(checksum != result->checksum, 1);
    
    __setu(8, 5.0, 5.0);
    imapupdateunit(map, __getunitfor(8));
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD:[8]| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[7]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3,5]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2,6]| AAC:[4]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 1.0)    (6: 0.05, 0.05) (7:3.99, 3.99)  (8: 5.0, 5.0)
    // ------------------------------------------------------------------------------------
    imapsearchfrompos(map, &pos, result, 0.5);
    int64_t nnnnnts = result->tick;
    __setcode(code, 'A', 'D', 'D');
    node = __getnode(code, 3);
    SP_EQUAL(result->tick, node->tick);
    SP_EQUAL(nnnnnts == nnnnts, 0);
    SP_EQUAL(ireflistlen(result->units), 1);
    SP_EQUAL(checksum != result->checksum, 1);
    
    isearchresultfree(result);
    
    // remove all unit
    for (int i=0; i<=20; ++i) {
        imapremoveunit(map, __getunitfor(i));
    }
}

SP_CASE(searching, imapsearchfromunit) {
    // prepare units in map
    __setu(0, 0.1, 0.0);
    
    imapaddunit(map, __getunitfor(0));
    
    __setu(1, 0.3, 0.4);
    
    imapaddunit(map, __getunitfor(1));
    
    __setu(2, 0.5, 0.0);
    
    imapaddunit(map, __getunitfor(2));
    
    __setu(3, 1.5, 1.2);
    
    imapaddunit(map, __getunitfor(3));
    
    __setu(4, 1.5, 0.2);
    
    imapaddunit(map, __getunitfor(4));
    
    __setu(5, 1.0, 0.0);
    
    imapaddunit(map, __getunitfor(5));
    
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[3]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0,1,2]| AAC:[4,5]| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    
    isearchresult *result = isearchresultmake();
    
    SP_EQUAL(result->tick, 0);
    SP_EQUAL(result->checksum, 0);
    
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 0.0)
    // ------------------------------------------------------------------------------------
    icode code = {{'A','A','A',0}};
    imapsearchfromunit(map, __getunitfor(0), result, 0.45);
    int64_t ts = result->tick;
    int64_t checksum = result->checksum;
    inode *node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SP_EQUAL(ts, node->tick);
    SP_EQUAL(ireflistlen(result->units), 2);
    SP_EQUAL(checksum != 0, 1);
    
    imapsearchfromunit(map, __getunitfor(0), result, 0.45);
    ts = result->tick;
    SP_EQUAL(ts, node->tick);
    SP_EQUAL(ireflistlen(result->units), 2);
    SP_EQUAL(checksum, result->checksum);
    
    isearchresultfree(result);
    
    // _aoi_print(map, EnumNodePrintStateNode);
    
    __setu(5, 1.0, 0.3);
    imapupdateunit(map, __getunitfor(5));
    
    // _aoi_print(map, EnumNodePrintStateNode);
    
    __setu(5, 1.0, -2.0);
    imapupdateunit(map, __getunitfor(5));
    // _aoi_print(map, EnumNodePrintStateNode);
    
    // remove all unit
    for (int i=0; i<=20; ++i) {
        imapremoveunit(map, __getunitfor(i));
    }
}

SP_CASE(searching, utick) {
    /**
     |BBB| BBD| BDB| BDD| DBB| DBD| DDB| DDD|
     |_______________________________________
     |BBA| BBC| BDA| BDC| DBA| DBC| DDA| DDC|
     |_______________________________________
     |BAB| BAD| BCB| BCD| DAB| DAD:[5]| DCB| DCD|
     |_______________________________________
     |BAA| BAC| BCA| BCC| DAA:[4]| DAC| DCA| DCC|
     |_______________________________________
     |ABB| ABD| ADB| ADD:[3]| CBB| CBD| CDB| CDD|
     |_______________________________________
     |ABA| ABC| ADA:[2]| ADC| CBA| CBC| CDA| CDC|
     |_______________________________________
     |AAB| AAD:[1]| ACB| ACD| CAB| CAD| CCB| CCD|
     |_______________________________________
     |AAA:[0]| AAC| ACA| ACC| CAA| CAC| CCA| CCC|
     |_______________________________________
     */
    __setu(0, 0, 0);
    imapaddunit(map, __getunitfor(0));
    
    __setu(1, 1.0, 1.0);
    imapaddunit(map, __getunitfor(1));
    
    __setu(2, 2, 2);
    imapaddunit(map, __getunitfor(2));
    
    __setu(3, 3, 3);
    imapaddunit(map, __getunitfor(3));
    
    __setu(4, 4, 4);
    imapaddunit(map, __getunitfor(4));
    
    __setu(5, 5, 5);
    
    // _aoi_print(map, EnumNodePrintStateNode);
    
    isearchresult *result = isearchresultmake();
    
    imapsearchfromunit(map, __getunitfor(0), result, 0.4);
    
    SP_EQUAL(result->tick, __getunitfor(0)->tick);
    SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 3)->tick);
    SP_EQUAL(ireflistlen(result->units), 0);
    
    __setu(1, 1.1, 1.1);
    imapupdateunit(map, __getunitfor(1));
    // _aoi_print(map, EnumNodePrintStateNode);
    
    // AAA ->tick
    imapsearchfromunit(map, __getunitfor(0), result, 0.4);
    SP_EQUAL(result->tick, __getunitfor(0)->tick);
    SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 3)->tick);
    SP_EQUAL(ireflistlen(result->units), 0);
    
    // AA ->utick
    imapsearchfromunit(map, __getunitfor(0), result, 0.8);
    SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 2)->utick);
    SP_TRUE(result->tick != __getnode(__getunitfor(0)->code, 2)->tick);
    SP_EQUAL(ireflistlen(result->units), 0);
    SP_TRUE(__getnode(__getunitfor(0)->code, 2)->utick == __getnode(__getunitfor(0)->code, 1)->utick);
    
    __setu(2, 2.2, 2.2);
    imapupdateunit(map, __getunitfor(2));
    // _aoi_print(map, EnumNodePrintStateNode);
    
    imapsearchfromunit(map, __getunitfor(0), result, 0.8);
    SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 2)->utick);
    SP_TRUE(result->tick != __getnode(__getunitfor(0)->code, 2)->tick);
    SP_EQUAL(ireflistlen(result->units), 0);
    SP_TRUE(__getnode(__getunitfor(0)->code, 2)->utick != __getnode(__getunitfor(0)->code, 1)->utick);
    
    {
        
        imapsearchfromunit(map, __getunitfor(0), result, 1.1);
        SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 1)->utick);
        
        int64_t uts = __getnode(__getunitfor(0)->code, 0)->utick;
        int64_t auts = __getnode(__getunitfor(0)->code, 1)->utick;
        int64_t aauts = __getnode(__getunitfor(0)->code, 2)->utick;
        int64_t aaauts = __getnode(__getunitfor(0)->code, 3)->utick;
        
        int64_t ts = __getnode(__getunitfor(0)->code, 0)->tick;
        int64_t ats = __getnode(__getunitfor(0)->code, 1)->tick;
        int64_t aats = __getnode(__getunitfor(0)->code, 2)->tick;
        int64_t aaats = __getnode(__getunitfor(0)->code, 3)->tick;
        
        SP_EQUAL(aaauts, aaats);
        SP_EQUAL(aauts > aats, 1);
        SP_EQUAL(auts > ats, 1);
        SP_EQUAL(auts > aauts, 1);
        SP_EQUAL(uts > ts, 1);
        
        __setu(5, 4.2, 4.2);
        imapaddunit(map, __getunitfor(5));
        // _aoi_print(map, EnumNodePrintStateNode);
        
        uts = __getnode(__getunitfor(0)->code, 0)->utick;
        ts = __getnode(__getunitfor(0)->code, 0)->tick;
        
        imapsearchfromunit(map, __getunitfor(0), result, 1.1);
        SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 1)->utick);
        SP_EQUAL(uts == ts, 1);
        
        __setu(5, 0.5, 0.5);
        imapupdateunit(map, __getunitfor(5));
        // _aoi_print(map, EnumNodePrintStateNode);
        
        uts = __getnode(__getunitfor(0)->code, 0)->utick;
        ts = __getnode(__getunitfor(0)->code, 0)->tick;
        auts = __getnode(__getunitfor(0)->code, 1)->utick;
        ats = __getnode(__getunitfor(0)->code, 1)->tick;
        SP_EQUAL(uts > ts, 1);
        SP_EQUAL(auts, ats);
        SP_EQUAL(auts, uts);
        imapsearchfromunit(map, __getunitfor(0), result, 1.1);
        SP_EQUAL(result->tick, __getnode(__getunitfor(0)->code, 1)->utick);
    }
    
    isearchresultfree(result);
}

SP_CASE(searching, end) {
    for (int i=0; i<__preparecnt; ++i) {
        __unhold(i);
    }
    
    SP_TRUE(1);
    
    imapfree(map);
    
    clearalliaoicacheandmemrorystate();
}

SP_SUIT(autoreleasepool);

static int _autoreleasecount = 0;
static void _hook_iref_free(iref *ref) {
    iobjfree(ref);
    ++_autoreleasecount;
}

SP_CASE(autoreleasepool, autorelease) {

    _iautoreleasepool;

    for (int i=0; i<10; ++i) {
        iref *ref = _iautomalloc(iref);
        ref->free = _hook_iref_free;
    }

    for (int i=0; i<10; ++i) {
        inode *node = imakenode();
        node->free = _hook_iref_free;
        _iautorelease(node);
    }

    _iautoreleaseall;

    SP_EQUAL(_autoreleasecount, 20);
}


#if iiradius

SP_SUIT(unit_radius);

SP_CASE(unit_radius, radius) {
    SP_TRUE(1);
    
    static const int MAX_COUNT = 2000;
    static const int MAP_SIZE = 512;
    ipos pos = {0, 0};
    isize size = {MAP_SIZE, MAP_SIZE};
    int divide = 10;
    imap *map = imapmake(&pos, &size, divide);
    SP_EQUAL(map->maxradius, 0);
    int i = 0;
    int maxunit = MAX_COUNT;
    iunit *u = NULL;
    int maxunitrange = 2;
    ireal maxradius = 0;
    
    for (i=0; i<maxunit; ++i) {
        u = imakeunit((iid)i, (ireal)(rand()%MAP_SIZE), (ireal)(rand()%MAP_SIZE));
        /* 给单元加一个随机半径 */
        u->radius = (ireal)(rand()%100)/100*maxunitrange;
        imapaddunit(map, u);
        
        if (maxradius < u->radius) {
            maxradius = u->radius;
        }
        
        ifreeunit(u);
    }
    SP_EQUAL(map->maxradius, maxradius);
    
    u->radius = maxradius * 2;
    imaprefreshunit(map, u);
    SP_EQUAL(map->maxradius, u->radius);
    
    imapfree(map);
}

#endif

SP_SUIT(searching_bench_right) ;

static void silly_search(imap *map, iunit **units, int num, ipos *pos, ireal range, isearchresult *result) {
    ifilter *filter = NULL;
    
    int i =0;
    iunit *unit = NULL;
    
    filter = ifiltermake_circle(pos, range);
    isearchresultclean(result);
    
    for (i=0; i<num; ++i) {
        unit = units[i];
        /* 是否满足条件 */
        if (ifilterrun(map, filter, unit) == iiok) {
            ireflistadd(result->units, irefcast(unit));
        }
    }
    ifilterfree(filter);
}

static int64_t silly_checksum(ireflist *units) {
    irefjoint *joint = ireflistfirst(units);
    iunit *unit = NULL;
    int64_t sum = 0;
    while (joint) {
        unit = icast(iunit, joint->value);
        sum += unit->id;
        joint = joint->next;
        
        /* printf("%4lld,", unit->id); */
    }
    /* printf("===>%9lld\n", sum); */
    return sum;
}

SP_CASE(searching_bench_right, searchpos){
    
    SP_TRUE(1);
    
    static const int MAX_COUNT = 2000;
    static const int MAP_SIZE = 512;
    ipos pos = {0, 0};
    isize size = {MAP_SIZE, MAP_SIZE};
    int divide = 10;
    iarray* units = iarraymakeiref(MAX_COUNT);
    int i = 0;
    int maxunit = MAX_COUNT;
    int bench = 10000;
    int maxrange = 10;
    int minrange = 5;
    int maxunitrange = 2;
    ireal range = 0;
    isearchresult* resultlfs = isearchresultmake();
    isearchresult* resultrfs = isearchresultmake();
    imap *map = imapmake(&pos, &size, divide);
    
    for (i=0; i<maxunit; ++i) {
        iunit * u = imakeunit((iid)i, (ireal)(rand()%MAP_SIZE), (ireal)(rand()%MAP_SIZE));
        /* 给单元加一个随机半径 */
        u->radius = (ireal)(rand()%100)/100*maxunitrange;
        imapaddunit(map, u);
        iarrayadd(units, &u);
        irelease(u);
    }
    
    /* _aoi_print(map, 0xffffff); */
    
    for(i=0;i<bench; ++i) {
        pos.x = (ireal)(rand()%MAP_SIZE);
        pos.y = (ireal)(rand()%MAP_SIZE);
        range = (ireal)(rand()%maxrange + minrange);
        imapsearchfrompos(map, &pos, resultlfs, range);
        silly_search(map, (iunit**)iarraybuffer(units), MAX_COUNT, &pos, range, resultrfs);
        
        SP_EQUAL(ireflistlen(resultrfs->units), ireflistlen(resultlfs->units));
        
        SP_EQUAL(silly_checksum(resultrfs->units), silly_checksum(resultlfs->units));
    }
    
    iarrayfree(units);
    isearchresultfree(resultlfs);
    isearchresultfree(resultrfs);
    imapfree(map);
}

static void silly_search_lineofsight(imap *map,
                                     iunit **units,
                                     int num,
                                     const iline2d *line,
                                     isearchresult *result) {
    ifilter *filter = NULL;
    
    int i =0;
    iunit *unit = NULL;
    
    filter = ifiltermake_line2d(&line->start, &line->end, iepsilon);
    isearchresultclean(result);
    
    for (i=0; i<num; ++i) {
        unit = units[i];
        /* 是否满足条件 */
        if (ifilterrun(map, filter, unit) == iiok) {
            ireflistadd(result->units, irefcast(unit));
        }
    }
    ifilterfree(filter);
}

static void __print_list_unit(const ireflist *list) {
    printf("[");
    irefjoint *first = ireflistfirst(list);
    while (first) {
        iunit * u = icast(iunit, first->value);
        printf("%lld%s", u->id, first->next == NULL ? "" :", ");
        first = first->next;
    }
    printf("]\n");
}

SP_CASE(searching_bench_right, lineofsight_accurate) {
    
    static const int MAX_COUNT = 8;
    static const int MAP_SIZE = 8;
    ipos pos = {0, 0};
    isize size = {MAP_SIZE, MAP_SIZE};
    int divide = 3;
    iarray *units = iarraymakeiref(MAX_COUNT);
    int i = 0;
    int maxunit = MAX_COUNT;
    iline2d line;
    isearchresult* resultlfs = isearchresultmake();
    isearchresult* resultrfs = isearchresultmake();
    imap *map = imapmake(&pos, &size, divide);
    
    for (i=0; i<maxunit; ++i) {
        iunit *u = imakeunit((iid)i, (ireal)(i), (ireal)(i));
        /* 给单元加一个随机半径 */
        u->radius = (ireal)(1);
        imapaddunit(map, u);
        iarrayadd(units, &u);
        
        ifreeunit(u);
    }
    
    printf("**********************\n");
#define __x_set_sight(x0, y0, x1, y1) \
    line.start.x = x0; line.start.y = y0; line.end.x = x1; line.end.y = y1
    
    {
        __x_set_sight(1, 1, 3, 3);
        silly_search_lineofsight(map, (iunit**)iarraybuffer(units), MAX_COUNT, &line, resultrfs);
        __print_list_unit(resultrfs->units);
        imaplineofsight(map, &line.start, &line.end, resultlfs);
        __print_list_unit(resultlfs->units);
    }
    printf("**********************\n");
    
    iarrayfree(units);
    isearchresultfree(resultrfs);
    isearchresultfree(resultlfs);
    imapfree(map);
    
}

SP_CASE(searching_bench_right, lineofsight) {
    SP_TRUE(1);
    
    
    static const int MAX_COUNT = 2000;
    static const int MAP_SIZE = 512;
    ipos pos = {0, 0};
    ipos to = {0, 0};
    iline2d line;
    isize size = {MAP_SIZE, MAP_SIZE};
    int divide = 5;
    iarray *units = iarraymakeiref(MAX_COUNT);
    int i = 0;
    int maxunit = MAX_COUNT;
    int bench = 2000;
    int maxunitrange = 2;
    isearchresult* resultlfs = isearchresultmake();
    isearchresult* resultrfs = isearchresultmake();
    imap *map = imapmake(&pos, &size, divide);
    
    for (i=0; i<maxunit; ++i) {
        iunit * u = imakeunit((iid)i, (ireal)(rand()%MAP_SIZE), (ireal)(rand()%MAP_SIZE));
        /* 给单元加一个随机半径 */
        u->radius = (ireal)(rand()%100)/100*maxunitrange;
        imapaddunit(map, u);
        iarrayadd(units, &u);
        ifreeunit(u);
    }
   
    for(i=0;i<bench; ++i) {
        pos.x = (ireal)(rand()%MAP_SIZE);
        pos.y = (ireal)(rand()%MAP_SIZE);
        to.x = (ireal)(rand()%MAP_SIZE);
        to.y = (ireal)(rand()%MAP_SIZE);
        line.start = pos;
        line.end = to;
        
        silly_search_lineofsight(map, (iunit**)iarraybuffer(units), MAX_COUNT, &line, resultrfs);
        //__print_list_unit(resultrfs->units);
        
        imaplineofsight(map, &pos, &to, resultlfs);
        //__print_list_unit(resultlfs->units);
        
        SP_EQUAL(ireflistlen(resultrfs->units), ireflistlen(resultlfs->units));
        
        SP_EQUAL(silly_checksum(resultrfs->units), silly_checksum(resultlfs->units));
    }
    
    iarrayfree(units);
    isearchresultfree(resultlfs);
    isearchresultfree(resultrfs);
    imapfree(map);
}

SP_SUIT(iarray);

static iarray *_iarray_make_int(size_t size) {
    iarray *arr = iarraymakeint(1);
    iarrayunsetflag(arr, EnumArrayFlagKeepOrder);
    return arr;
}

static void _iarrayaddint(iarray *arr, int i) {
    iarrayadd(arr, &i);
}

SP_CASE(iarray, int) {
    iarray *arr = _iarray_make_int(1);

    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 20000);
    _iarrayaddint(arr, 3);
    
    int *values = (int*)arr->buffer;
    printf("%d-%d-%d \n", values[0], values[1], values[2]);
    SP_EQUAL(values[0], 1);
    SP_EQUAL(values[1], 20000);
    SP_EQUAL(values[2], 3);
    
    iarrayremove(arr, 1);
    SP_EQUAL(values[0], 1);
    SP_EQUAL(values[1], 3);
    
    _iarrayaddint(arr, 2);
    SP_EQUAL(values[2], 2);
    
    iarraysort(arr);
    
    values = (int*)arr->buffer;
    SP_EQUAL(values[0], 1);
    SP_EQUAL(values[1], 2);
    SP_EQUAL(values[2], 3);
    
    iarrayfree(arr);
    SP_TRUE(1);
}

SP_CASE(iarray, iarraylen) {
    iarray *arr = _iarray_make_int(1);
    SP_EQUAL(iarraylen(arr), 0);
    
    _iarrayaddint(arr, 1);
    SP_EQUAL(iarraylen(arr), 1);
    
    _iarrayaddint(arr, 20000);
    SP_EQUAL(iarraylen(arr), 2);
    
    _iarrayaddint(arr, 3);
    SP_EQUAL(iarraylen(arr), 3);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarraycapacity) {
    iarray *arr = _iarray_make_int(1);
    SP_EQUAL(iarraycapacity(arr), 1);
    
    _iarrayaddint(arr, 1);
    SP_EQUAL(iarraylen(arr), 1);
    SP_EQUAL(iarraycapacity(arr), 1);
    
    _iarrayaddint(arr, 20000);
    SP_EQUAL(iarraylen(arr), 2);
    SP_EQUAL(iarraycapacity(arr), 2);
    
    _iarrayaddint(arr, 3);
    SP_EQUAL(iarraylen(arr), 3);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    _iarrayaddint(arr, 4);
    SP_EQUAL(iarraylen(arr), 4);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    _iarrayaddint(arr, 5);
    SP_EQUAL(iarraylen(arr), 5);
    SP_EQUAL(iarraycapacity(arr), 8);
    
    iarrayfree(arr);
}


SP_CASE(iarray, iarrayat) {
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 0);
    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 2);
    _iarrayaddint(arr, 3);
    _iarrayaddint(arr, 4);
    _iarrayaddint(arr, 5);
    
    SP_EQUAL(((int*)iarrayat(arr, 0))[0], 0);
    SP_EQUAL(((int*)iarrayat(arr, 1))[0], 1);
    SP_EQUAL(((int*)iarrayat(arr, 2))[0], 2);
    SP_EQUAL(((int*)iarrayat(arr, 3))[0], 3);
    SP_EQUAL(((int*)iarrayat(arr, 4))[0], 4);
    SP_EQUAL(((int*)iarrayat(arr, 5))[0], 5);
    
    SP_EQUAL(iarrayof(arr, int, 5), 5);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarraybuffer) {
    iarray *arr = _iarray_make_int(1);
    SP_TRUE(iarraybuffer(arr) != NULL);
    iarrayfree(arr);
}

SP_CASE(iarray, iarrayremove) {
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 0);
    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 2);
    _iarrayaddint(arr, 3);
    _iarrayaddint(arr, 4);
    _iarrayaddint(arr, 5);
    
    SP_EQUAL(iarraylen(arr), 6);
    
    iarrayremove(arr, 3);
    SP_EQUAL(iarraylen(arr), 5);
    
    SP_EQUAL(iarrayof(arr, int, 0), 0);
    SP_EQUAL(iarrayof(arr, int, 1), 1);
    SP_EQUAL(iarrayof(arr, int, 2), 2);
    SP_EQUAL(iarrayof(arr, int, 3), 5);
    SP_EQUAL(iarrayof(arr, int, 4), 4);
    
    iarrayremove(arr, 2);
    SP_EQUAL(iarraylen(arr), 4);
    SP_EQUAL(iarrayof(arr, int, 0), 0);
    SP_EQUAL(iarrayof(arr, int, 1), 1);
    SP_EQUAL(iarrayof(arr, int, 2), 4);
    SP_EQUAL(iarrayof(arr, int, 3), 5);
    
    iarrayremove(arr, 3);
    SP_EQUAL(iarraylen(arr), 3);
    SP_EQUAL(iarrayof(arr, int, 0), 0);
    SP_EQUAL(iarrayof(arr, int, 1), 1);
    SP_EQUAL(iarrayof(arr, int, 2), 4);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarrayremoveall) {
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 0);
    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 2);
    
    SP_EQUAL(iarraylen(arr), 3);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    iarrayremoveall(arr);
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    for (int i=0; i < 16; ++i) {
        _iarrayaddint(arr, i);
    }
    SP_EQUAL(iarraylen(arr), 16);
    SP_EQUAL(iarraycapacity(arr), 16);
    
    iarrayremoveall(arr);
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 8);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarraytruncate) {
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 0);
    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 2);
    
    SP_EQUAL(iarraylen(arr), 3);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    iarraytruncate(arr, 1);
    SP_EQUAL(iarraylen(arr), 1);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    iarraytruncate(arr, 0);
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    for (int i=0; i < 20; ++i) {
        _iarrayaddint(arr, i);
    }
    SP_EQUAL(iarraylen(arr), 20);
    SP_EQUAL(iarraycapacity(arr), 32);
    
    iarraytruncate(arr, 6);
    SP_EQUAL(iarraylen(arr), 6);
    SP_EQUAL(iarraycapacity(arr), 12);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarrayshrinkcapacity) {
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 0);
    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 2);
    
    SP_EQUAL(iarraylen(arr), 3);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    SP_EQUAL(iarrayshrinkcapacity(arr, 8), 4);
    SP_EQUAL(iarraycapacity(arr), 4);
    
    SP_EQUAL(iarrayshrinkcapacity(arr, 3), 3);
    SP_EQUAL(iarraycapacity(arr), 3);
    
    SP_EQUAL(iarrayshrinkcapacity(arr, 2), 3);
    SP_EQUAL(iarraycapacity(arr), 3);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarrayinsert) {
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 3);
    _iarrayaddint(arr, 4);
    _iarrayaddint(arr, 5);
    _iarrayaddint(arr, 6);
    
    int inserts[] = {0, 1, 2};
    int appends[] = {7, 8, 9};
    
    SP_EQUAL(iarraylen(arr), 4);
    SP_EQUAL(iarrayof(arr, int, 0), 3);
    SP_EQUAL(iarrayof(arr, int, 1), 4);
    SP_EQUAL(iarrayof(arr, int, 2), 5);
    SP_EQUAL(iarrayof(arr, int, 3), 6);
    
    iarrayinsert(arr, 0, inserts, 3);
    SP_EQUAL(iarraylen(arr), 7);
    SP_EQUAL(iarrayof(arr, int, 0), 0);
    SP_EQUAL(iarrayof(arr, int, 1), 1);
    SP_EQUAL(iarrayof(arr, int, 2), 2);
    SP_EQUAL(iarrayof(arr, int, 3), 3);
    SP_EQUAL(iarrayof(arr, int, 4), 4);
    SP_EQUAL(iarrayof(arr, int, 5), 5);
    SP_EQUAL(iarrayof(arr, int, 6), 6);
    
    iarrayinsert(arr, arr->len, appends, 3);
    SP_EQUAL(iarraylen(arr), 10);
    SP_EQUAL(iarrayof(arr, int, 0), 0);
    SP_EQUAL(iarrayof(arr, int, 1), 1);
    SP_EQUAL(iarrayof(arr, int, 2), 2);
    SP_EQUAL(iarrayof(arr, int, 3), 3);
    SP_EQUAL(iarrayof(arr, int, 4), 4);
    SP_EQUAL(iarrayof(arr, int, 5), 5);
    SP_EQUAL(iarrayof(arr, int, 6), 6);
    SP_EQUAL(iarrayof(arr, int, 7), 7);
    SP_EQUAL(iarrayof(arr, int, 8), 8);
    SP_EQUAL(iarrayof(arr, int, 9), 9);
    
    iarrayfree(arr);
}

SP_CASE(iarray, iarraysort) {
    
    iarray *arr = _iarray_make_int(1);
    _iarrayaddint(arr, 3);
    _iarrayaddint(arr, 1);
    _iarrayaddint(arr, 2);
    _iarrayaddint(arr, 7);
    _iarrayaddint(arr, 0);
    
    iarraysort(arr);
    
    int *values = (int*)iarraybuffer(arr);
    SP_EQUAL(values[0], 0);
    SP_EQUAL(values[1], 1);
    SP_EQUAL(values[2], 2);
    SP_EQUAL(values[3], 3);
    SP_EQUAL(values[4], 7);
    
    iarrayfree(arr);
}

SP_SUIT(iarray_iref);

static int _arr_iref_try_free = 0;
static void _arr_iref_watch(iref *ref) {
    icheck(ref->ref == 0);
    ++_arr_iref_try_free;
}

static iref* _arrirefmake() {
    iref* ref = iobjmalloc(iref);
    iretain(ref);
    ref->watch = _arr_iref_watch;
    // printf("ref : %p\n", ref);
    return ref;
}

static void _arrirefadd(iarray* arr, iref *ref) {
    iarrayadd(arr, &ref);
}

SP_CASE(iarray_iref, iarrayadd) {
    iarray *arr = iarraymakeiref(1);
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 1);
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iref* ref = _arrirefmake();
    _arrirefadd(arr, ref);
    irelease(ref);
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iarrayfree(arr);
    
    SP_EQUAL(_arr_iref_try_free, 1);
    
    _arr_iref_try_free = 0;
}

SP_CASE(iarray_iref, iarrayinsert) {
    iarray *arr = iarraymakeiref(1);
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 1);
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iref* refs[] = {
        _arrirefmake(),
        _arrirefmake(),
        _arrirefmake(),
        _arrirefmake(),
    };
    _arrirefadd(arr, refs[0]);
    _arrirefadd(arr, refs[1]);
    _arrirefadd(arr, refs[2]);
    _arrirefadd(arr, refs[3]);
    
    SP_EQUAL(iarrayof(arr, iref*, 0), refs[0]);
    SP_EQUAL(iarrayof(arr, iref*, 1), refs[1]);
    SP_EQUAL(iarrayof(arr, iref*, 2), refs[2]);
    SP_EQUAL(iarrayof(arr, iref*, 3), refs[3]);
    
    iref* inserts[] = {
        _arrirefmake(),
        _arrirefmake(),
        _arrirefmake(),
    };
    iarrayinsert(arr, 0, inserts, 3);
    SP_EQUAL(iarrayof(arr, iref*, 0), inserts[0]);
    SP_EQUAL(iarrayof(arr, iref*, 1), inserts[1]);
    SP_EQUAL(iarrayof(arr, iref*, 2), inserts[2]);
                                      
    SP_EQUAL(iarrayof(arr, iref*, 3), refs[0]);
    SP_EQUAL(iarrayof(arr, iref*, 4), refs[1]);
    SP_EQUAL(iarrayof(arr, iref*, 5), refs[2]);
    SP_EQUAL(iarrayof(arr, iref*, 6), refs[3]);
    
    iref* appends[] = {
        _arrirefmake(),
        _arrirefmake(),
        _arrirefmake(),
    };
    iarrayinsert(arr, arr->len, appends, 3);
    SP_EQUAL(iarrayof(arr, iref*, 0), inserts[0]);
    SP_EQUAL(iarrayof(arr, iref*, 1), inserts[1]);
    SP_EQUAL(iarrayof(arr, iref*, 2), inserts[2]);
                                      
    SP_EQUAL(iarrayof(arr, iref*, 3), refs[0]);
    SP_EQUAL(iarrayof(arr, iref*, 4), refs[1]);
    SP_EQUAL(iarrayof(arr, iref*, 5), refs[2]);
    SP_EQUAL(iarrayof(arr, iref*, 6), refs[3]);
    
    SP_EQUAL(iarrayof(arr, iref*, 7), appends[0]);
    SP_EQUAL(iarrayof(arr, iref*, 8), appends[1]);
    SP_EQUAL(iarrayof(arr, iref*, 9), appends[2]);
    
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iarrayfree(arr);
    irelease(inserts[0]);
    irelease(inserts[1]);
    irelease(inserts[2]);
    irelease(refs[0]);
    irelease(refs[1]);
    irelease(refs[2]);
    irelease(refs[3]);
    irelease(appends[0]);
    irelease(appends[1]);
    irelease(appends[2]);
    
    SP_EQUAL(_arr_iref_try_free, 10);
    
    _arr_iref_try_free = 0;
}

SP_CASE(iarray_iref, iarrayremove) {
    iarray *arr = iarraymakeiref(1);
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 1);
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iref* ref = _arrirefmake();
    _arrirefadd(arr, ref);
    irelease(ref);
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iarrayremove(arr, 0);
    iarrayremove(arr, -1);
    iarrayremove(arr, 2);
    
    SP_EQUAL(_arr_iref_try_free, 1);
    
    iarrayfree(arr);
    
    SP_EQUAL(_arr_iref_try_free, 1);
    
    _arr_iref_try_free = 0;
}

SP_CASE(iarray_iref, iarrayremove_keeporder) {
    iarray *arr = iarraymakeiref(1);
    iarraysetflag(arr, EnumArrayFlagKeepOrder);
    
    SP_EQUAL(iarraylen(arr), 0);
    SP_EQUAL(iarraycapacity(arr), 1);
    
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iref* ref0 = _arrirefmake();
    _arrirefadd(arr, ref0);
    iref* ref1 = _arrirefmake();
    _arrirefadd(arr, ref1);
    iref* ref2 = _arrirefmake();
    _arrirefadd(arr, ref2);
    iref* ref3 = _arrirefmake();
    _arrirefadd(arr, ref3);
    
    SP_EQUAL(iarraylen(arr), 4);
    SP_EQUAL(iarrayof(arr, iref*, 0), ref0);
    SP_EQUAL(iarrayof(arr, iref*, 1), ref1);
    SP_EQUAL(iarrayof(arr, iref*, 2), ref2);
    SP_EQUAL(iarrayof(arr, iref*, 3), ref3);
    
    iarrayremove(arr, 2);
    
    SP_EQUAL(iarrayof(arr, iref*, 0), ref0);
    SP_EQUAL(iarrayof(arr, iref*, 1), ref1);
    SP_EQUAL(iarrayof(arr, iref*, 2), ref3);
    
    iarrayunsetflag(arr, EnumArrayFlagKeepOrder);
    
    iarrayremove(arr, 0);
    SP_EQUAL(iarrayof(arr, iref*, 0), ref3);
    SP_EQUAL(iarrayof(arr, iref*, 1), ref1);
    
    irelease(ref0);
    irelease(ref1);
    irelease(ref2);
    irelease(ref3);
    
    SP_EQUAL(_arr_iref_try_free, 2);
    
    iarrayfree(arr);
    
    SP_EQUAL(_arr_iref_try_free, 4);
    
    _arr_iref_try_free = 0;
}

SP_CASE(iarray_iref, iarrayset) {
    
    iarray *arr = iarraymakeiref(1);
    
    iref* ref0 = _arrirefmake();
    SP_EQUAL(iarrayset(arr, 0, &ref0), iino);
    SP_EQUAL(iarraylen(arr), 0);
    
    _arrirefadd(arr, ref0);
    SP_EQUAL(iarraylen(arr), 1);
    SP_EQUAL(iarrayset(arr, 0, &ref0), iiok);
    
    irelease(ref0);
    SP_EQUAL(_arr_iref_try_free, 0);
    
    iarrayfree(arr);
    
    SP_EQUAL(_arr_iref_try_free, 1);
    
    _arr_iref_try_free = 0;
}


SP_SUIT(islice);

SP_CASE(islice, islicemake) {
    
    iarray *arr = iarraymakeint(8);
    int values[] = {0, 1, 2};
    iarrayinsert(arr, 0, values, 3);
    
    {
        islice *slice = islicemake(arr, 0, 0, 0);
        SP_EQUAL(islicelen(slice), 0);
        SP_EQUAL(islicecapacity(slice), 0);
        
        islicefree(slice);
    }
    
    {
        islice *slice = islicemake(arr, 0, 1, 1);
        SP_EQUAL(islicelen(slice), 1);
        SP_EQUAL(islicecapacity(slice), 1);
        
        islicefree(slice);
    }
    
    {
        islice *slice = islicemake(arr, 0, 1, 0);
        SP_EQUAL(islicelen(slice), 1);
        SP_EQUAL(islicecapacity(slice), 1);
        
        islicefree(slice);
    }
    
    
    {
        islice *slice = islicemake(arr, 0, 1, 2);
        SP_EQUAL(islicelen(slice), 1);
        SP_EQUAL(islicecapacity(slice), 2);
        
        islicefree(slice);
    }
    
    
    {
        islice *slice = islicemake(arr, 1, 3, 9);
        SP_EQUAL(islicelen(slice), 2);
        SP_EQUAL(islicecapacity(slice), 7);
        
        islicefree(slice);
    }
    
    iarrayfree(arr);
}

SP_CASE(islice, islicemakeby) {
    /* arr[0:8] */
    iarray *arr = iarraymakeint(8);
    int values[] = {0, 1, 2};
    iarrayinsert(arr, 0, values, 3);
    
    /* output array */
    irange(arr, int,
           printf("arr[%d]=%d\n", __key, __value);
           );
    
    /* slice = arr[0:3:8]  */
    islice *slice_0_3_8 = isliced(arr, 0, 3);
    SP_EQUAL(islicelen(slice_0_3_8), 3);
    SP_EQUAL(islicecapacity(slice_0_3_8), 8);
    SP_EQUAL(isliceof(slice_0_3_8, int, 0), 0);
    SP_EQUAL(isliceof(slice_0_3_8, int, 1), 1);
    SP_EQUAL(isliceof(slice_0_3_8, int, 2), 2);
    
    /* output slice */
    irange(slice_0_3_8, int,
           printf("slice_0_3_8-arr[0:3][%d]=%d\n", __key, __value);
           );
    
    /* child = slice[1:2:8] */
    islice *child = islicedby(slice_0_3_8, 1, 2);
    SP_EQUAL(islicelen(child), 1);
    SP_EQUAL(islicecapacity(child), 7);
    SP_EQUAL(isliceof(child, int, 0), 1);
    SP_EQUAL(isliceat(child, 1), NULL);
    
    /*output slice child*/
    irange(child, int,
           printf("slice_0_3_8-child[1:2][%d]=%d\n", __key, __value);
           );
    
    /* sub = child[1:1:7] */
    islice* sub = islicedby(child, 1, 1);
    SP_EQUAL(islicelen(sub), 0);
    SP_EQUAL(islicecapacity(sub), 6);
    SP_EQUAL(isliceat(sub, 0), NULL);
    
    /*output slice child sub*/
    irange(sub, int,
           printf("slice_0_3_8-child[1:2]-sub[1:1][%d]=%d\n", __key, __value);
           );
    
    /* xslice = slice[8:8]
     */
    islice *xslice = isliced(arr, 8, 8);
    
    /*output xslice*/
    irange(xslice, int,
           printf("xslice-arr[8:8][%d]=%d\n", __key, __value);
           );
    
    SP_EQUAL(islicelen(xslice), 0);
    SP_EQUAL(islicecapacity(xslice), 0);
    
    islice *yslice = isliced(arr, 5, 5);
    
    /*output yslice*/
    irange(yslice, int,
           printf("yslice-arr[5:5][%d]=%d\n", __key, __value);
           );
    
    SP_EQUAL(islicelen(yslice), 0);
    SP_EQUAL(islicecapacity(yslice), 3);
    
    /**/
    int aps[] = {100, 200};
    isliceadd(yslice, aps);
    
    SP_EQUAL(islicelen(yslice), 1);
    SP_EQUAL(islicecapacity(yslice), 3);
    SP_EQUAL(isliceof(yslice, int, 0), 100);
    
    /*output yslice*/
    irange(yslice, int,
           printf("yslice-arr[5:5]-add{100} [%d]=%d\n", __key, __value);
           );
    
    SP_EQUAL(iarraylen(arr), 4);
    SP_EQUAL(iarrayof(arr, int, 3), 100);
    
    /* output array */
    irange(arr, int,
           printf("arr-after-yslice-add[%d]=%d\n", __key, __value);
           );
    
    islicefree(yslice);
    
    islicefree(xslice);
    
    islicefree(child);
    
    islicefree(sub);
    
    islicefree(slice_0_3_8);
    
    iarrayfree(arr);
}

#define __slice_arr(arr, ...) islicemakearg(arr, #__VA_ARGS__)

void __slice_print(islice *s) {
    printf("[");
    for (int i=0; i <islicelen(s); ++i) {
        int v = isliceof(s, int, i);
        printf("%d%s", v, i==islicelen(s)-1 ? "" : ", ");
    }
    printf("]");
}

void __slice_println(islice *s) {
    __slice_print(s);
    printf("%s", "\n");
}

SP_CASE(islice, islicelen_islicecapacity) {
    /* arr[0:8] */
    iarray *arr = iarraymakeint(8);
    int values[] = {0, 1, 2};
    iarrayinsert(arr, 0, values, 3);
    
    islice *slice0 = __slice_arr(arr);
    __slice_println(slice0);
    SP_EQUAL(islicelen(slice0), 3);
    SP_EQUAL(islicecapacity(slice0), 8);
    islicefree(slice0);
    
    islice *slice1 = __slice_arr(arr, 2:);
    __slice_println(slice1);
    SP_EQUAL(islicelen(slice1), 1);
    SP_EQUAL(islicecapacity(slice1), 6);
    islicefree(slice1);
    
    islice *slice2 = __slice_arr(arr, :1);
    __slice_println(slice2);
    SP_EQUAL(islicelen(slice2), 1);
    SP_EQUAL(islicecapacity(slice2), 8);
    islicefree(slice2);
    
    islice *slice3 = __slice_arr(arr, :1:5);
    __slice_println(slice3);
    SP_EQUAL(islicelen(slice3), 1);
    SP_EQUAL(islicecapacity(slice3), 5);
    islicefree(slice3);
    
    islice *slice4 = __slice_arr(arr, :0:5);
    __slice_println(slice4);
    SP_EQUAL(islicelen(slice4), 0);
    SP_EQUAL(islicecapacity(slice4), 5);
    islicefree(slice4);
    
    iarrayfree(arr);
}

void __array_print(iarray *s) {
    printf("[");
    irange(s, int,
           printf("%s%d", __key==0 ? "" : ", ", __value);
           );
    printf("]");
}

void __array_println(iarray *s) {
    __array_print(s);
    printf("%s", "\n");
}



SP_SUIT(iheap);

SP_CASE(iheap, iheapbuild) {
    iarray *arr = iarraymakeint(16);
    int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    iarrayinsert(arr, 0, values, 8);
    __array_println(arr);
    iheapbuild(arr);
    __array_println(arr);

    SP_EQUAL(iarrayof(arr, int, 0), 7);

    iarrayfree(arr);
}

SP_CASE(iheap, iheapadd) {
    iarray *arr = iarraymakeint(16);
    int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    iarrayinsert(arr, 0, values, 8);
    __array_println(arr);
    iheapbuild(arr);
    __array_println(arr);

    SP_EQUAL(iarrayof(arr, int, 0), 7);
    
    int v = 8;
    iheapadd(arr, &v);
    __array_println(arr);
    SP_EQUAL(iarrayof(arr, int, 0), 8);

    iarrayfree(arr);
}

SP_CASE(iheap, iheappeek) {
    iarray *arr = iarraymakeint(16);
    int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    iarrayinsert(arr, 0, values, 8);
    __array_println(arr);
    iheapbuild(arr);
    __array_println(arr);
    
    SP_EQUAL(iarrayof(arr, int, 0), 7);
    
    int * u = (int*)iheappeek(arr);
    SP_EQUAL(*u, 7);
    
    iarrayfree(arr);
}


SP_CASE(iheap, iheappop) {
    iarray *arr = iarraymakeint(16);
    int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    iarrayinsert(arr, 0, values, 8);
    __array_println(arr);
    iheapbuild(arr);
    __array_println(arr);
    
    SP_EQUAL(iarrayof(arr, int, 0), 7);
    
    int * u = (int*)iheappeek(arr);
    SP_EQUAL(*u, 7);
    
    iheappop(arr);
    
    u = (int*)iheappeek(arr);
    SP_EQUAL(*u, 6);
    
    iheappop(arr);
    
    u = (int*)iheappeek(arr);
    SP_EQUAL(*u, 5);
    
    
    iarrayfree(arr);
}

SP_CASE(iheap, iheapdelete) {
    iarray *arr = iarraymakeint(16);
    int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    iarrayinsert(arr, 0, values, 8);
    __array_println(arr);
    iheapbuild(arr);
    __array_println(arr);
    
    /*[7, 4, 6, 3, 0, 5, 2, 1]*/
    iheapdelete(arr, 0);
    /*[6, 4, 5, 3, 0, 1, 2]*/
    __array_println(arr);
    SP_EQUAL(*(int*)iheappeek(arr), 6);
    
    /*[6, 4, 5, 3, 0, 1, 2]*/
    iheapdelete(arr, 1);
    /*[6, 3, 5, 2, 0, 1]*/
    SP_EQUAL(iarrayof(arr, int, 1), 3);
    SP_EQUAL(iarrayof(arr, int, 2), 5);
    SP_EQUAL(iarrayof(arr, int, 3), 2);
    SP_EQUAL(iarrayof(arr, int, 4), 0);
    SP_EQUAL(iarrayof(arr, int, 5), 1);
    
    iarrayfree(arr);
}

SP_CASE(iheap, iheapadjust) {
    
    {
        iarray *arr = iarraymakeint(16);
        int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
        iarrayinsert(arr, 0, values, 8);
        __array_println(arr);
        iheapbuild(arr);
        __array_println(arr);
        
        /*[7, 4, 6, 3, 0, 5, 2, 1]*/
        SP_EQUAL(iarrayof(arr, int, 0), 7);
        SP_EQUAL(iarrayof(arr, int, 1), 4);
        SP_EQUAL(iarrayof(arr, int, 2), 6);
        SP_EQUAL(iarrayof(arr, int, 3), 3);
        SP_EQUAL(iarrayof(arr, int, 4), 0);
        SP_EQUAL(iarrayof(arr, int, 5), 5);
        SP_EQUAL(iarrayof(arr, int, 6), 2);
        SP_EQUAL(iarrayof(arr, int, 7), 1);
        
        iarrayof(arr, int, 0) = 1;
        __array_println(arr);
        SP_EQUAL(iarrayof(arr, int, 0), 1);
        
        iheapadjust(arr, 0);
        __array_println(arr);
        SP_EQUAL(iarrayof(arr, int, 0), 6);
        SP_EQUAL(iarrayof(arr, int, 1), 4);
        SP_EQUAL(iarrayof(arr, int, 2), 5);
        SP_EQUAL(iarrayof(arr, int, 3), 3);
        SP_EQUAL(iarrayof(arr, int, 4), 0);
        SP_EQUAL(iarrayof(arr, int, 5), 1);
        SP_EQUAL(iarrayof(arr, int, 6), 2);
        SP_EQUAL(iarrayof(arr, int, 7), 1);
        
        iarrayfree(arr);
    }
    
    {
        iarray *arr = iarraymakeint(16);
        int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
        iarrayinsert(arr, 0, values, 8);
        __array_println(arr);
        iheapbuild(arr);
        __array_println(arr);
        
        /*[7, 4, 6, 3, 0, 5, 2, 1]*/
        SP_EQUAL(iarrayof(arr, int, 0), 7);
        SP_EQUAL(iarrayof(arr, int, 1), 4);
        SP_EQUAL(iarrayof(arr, int, 2), 6);
        SP_EQUAL(iarrayof(arr, int, 3), 3);
        SP_EQUAL(iarrayof(arr, int, 4), 0);
        SP_EQUAL(iarrayof(arr, int, 5), 5);
        SP_EQUAL(iarrayof(arr, int, 6), 2);
        SP_EQUAL(iarrayof(arr, int, 7), 1);
        
        iarrayof(arr, int, 1) = 9;
        iheapadjust(arr, 1);
        SP_EQUAL(iarrayof(arr, int, 0), 9);
        SP_EQUAL(iarrayof(arr, int, 1), 7);
        SP_EQUAL(iarrayof(arr, int, 2), 6);
        SP_EQUAL(iarrayof(arr, int, 3), 3);
        SP_EQUAL(iarrayof(arr, int, 4), 0);
        SP_EQUAL(iarrayof(arr, int, 5), 5);
        SP_EQUAL(iarrayof(arr, int, 6), 2);
        SP_EQUAL(iarrayof(arr, int, 7), 1);
        
        iarrayfree(arr);
    }
}

SP_SUIT(istring);

SP_CASE(istring, istringmake) {
    istring s = istringmake("abcdefg");
    char c;
    
    SP_EQUAL(istringlen(s), 7);
    c = isliceof(s, char, 0);
    SP_EQUAL(isliceof(s, char, 0), 'a');
    c = isliceof(s, char, 1);
    c = isliceof(s, char, 2);
    c = isliceof(s, char, 3);
    SP_EQUAL(isliceof(s, char, 1), 'b');
    SP_EQUAL(isliceof(s, char, 2), 'c');
    SP_EQUAL(isliceof(s, char, 3), 'd');
    SP_EQUAL(isliceof(s, char, 4), 'e');
    SP_EQUAL(isliceof(s, char, 5), 'f');
    SP_EQUAL(isliceof(s, char, 6), 'g');
    
    irelease(s);
}

SP_CASE(istring, istringbuf) {
    
    istring s = istringmake("abcdefg");
    
    SP_EQUAL(istringlen(s), 7);
    
    const char* buf = istringbuf(s);
    
    SP_EQUAL(buf[0], 'a');
    SP_EQUAL(buf[1], 'b');
    
    SP_EQUAL(buf[7], 0);
    
    irelease(s);
}

SP_CASE(istring, istringsplit) {
    
    istring s = istringmake("a:b:c:d:e:f:g");
    
    iarray *ss = istringsplit(s, ":", 1);
    
    const char* buf;
    
    SP_EQUAL(iarraylen(ss), 7);
    
    istring s0 = iarrayof(ss, istring, 0);
    buf = istringbuf(s0);
    SP_EQUAL(isliceof(s0, char, 0), 'a');
    
    istring s1 = iarrayof(ss, istring, 1);
    buf = istringbuf(s1);
    SP_EQUAL(isliceof(s1, char, 0), 'b');
    
    istring s2 = iarrayof(ss, istring, 2);
    buf = istringbuf(s2);
    SP_EQUAL(isliceof(s2, char, 0), 'c');
    
    istring s3 = iarrayof(ss, istring, 3);
    buf = istringbuf(s3);
    SP_EQUAL(isliceof(s3, char, 0), 'd');
    
    istring s6 = iarrayof(ss, istring, 6);
    buf = istringbuf(s6);
    SP_EQUAL(isliceof(s6, char, 0), 'g');
    
    iarrayfree(ss);
    
    irelease(s);
}

SP_CASE(istring, istringjoin) {
    
    istring s = istringmake("a:b:c:d:e:f:g");
    
    iarray *ss = istringsplit(s, ":", 1);
    
    istring js = istringjoin(ss, "-", 1);
    
    SP_TRUE(istringcompare(s, js) != 0);
    
    istring xjs = istringmake("a-b-c-d-e-f-g");
    
    SP_TRUE(istringcompare(xjs, js) == 0);
    
    irelease(s);
    irelease(js);
    irelease(xjs);
    
    iarrayfree(ss);
}

SP_CASE(istring , istringreplace) {
    istring s = istringmake("a:b:c:d:e:f:g");
    
    istring js = istringrepleace(s, ":", "-");
    
    istring xjs = istringmake("a-b-c-d-e-f-g");
    
    SP_TRUE(istringcompare(js, xjs) == 0);
    
    irelease(s);
    irelease(js);
    irelease(xjs);
}

SP_CASE(istring, istringappend) {
    istring s = istringmake("a:b:c:d:e:f:g");
    
    istring js = istringappend(s, "-abcdefg");
    
    istring xjs = istringmake("a:b:c:d:e:f:g-abcdefg");
    
    SP_TRUE(istringcompare(js, xjs) == 0);
    
    irelease(js);
    irelease(xjs);
    irelease(s);
}

SP_CASE(istring, istringcompare) {
    istring sa = istringmake("a");
    istring sb = istringmake("b");
    istring sab = istringmake("ab");
    
    
    irelease(sa);
    irelease(sb);
    irelease(sab);
}

SP_CASE(istring, istringformat) {
    istring s = istringformat("efg %i %f abc", 1, 1.0);
    iarray* ss = istringsplit(s, " ", 1);
    
    ideclarestring(efg, "efg");
    istringlaw(efg);
    
    SP_TRUE(istringcompare(iarrayof(ss, istring, 0), efg) == 0);
    
    int i = istringatoi(iarrayof(ss, istring, 1));
    SP_EQUAL(i, 1);
    
    double d = istringatof(iarrayof(ss, istring, 2));
    SP_TRUE(ireal_equal(d, 1));
    
    ideclarestring(abc, "abc");
    istringlaw(abc);
    
    SP_TRUE(istringcompare(iarrayof(ss, istring, 3), abc) == 0);
    
    irelease(s);
    iarrayfree(ss);
}

SP_SUIT(iline2d);

SP_CASE(iline2d, nothing) {
    SP_TRUE(1);
}

SP_CASE(iline2d, iline2ddirection) {
    iline2d line;
    line.start.x = 1;
    line.start.y = 1;
    
    line.end.x = 2;
    line.end.y = 3;
    
    ivec2 dir = iline2ddirection(&line);
    ireal len = iline2dlength(&line);
    
    SP_TRUE(ireal_equal(dir.v.x, 1/len));
    SP_TRUE(ireal_equal(dir.v.y, 2/len));
}

SP_CASE(iline2d, iline2dnormal) {
    iline2d line;
    line.start.x = 0;
    line.start.y = 0;
    
    line.end.x = 3;
    line.end.y = 4;
    
    /*direction: (0.6, 0.8)*/
    /*direction: (0.8, -0.6)*/
    ivec2 normal = iline2dnormal(&line);
    SP_TRUE(ireal_equal(normal.v.x, 0.8));
    SP_TRUE(ireal_equal(normal.v.y, -0.6));
}

SP_CASE(iline2d, iline2dlength) {
    iline2d line;
    line.start.x = 0;
    line.start.y = 0;
    
    line.end.x = 3;
    line.end.y = 4;
    
    /*direction: (0.6, 0.8)*/
    /*direction: (0.8, -0.6)*/
    ivec2 normal = iline2dnormal(&line);
    SP_TRUE(ireal_equal(normal.v.x, 0.8));
    SP_TRUE(ireal_equal(normal.v.y, -0.6));
    
    ireal len = iline2dlength(&line);
    SP_TRUE(ireal_equal(len, 5));
}

SP_CASE(iline2d, iline2dsigneddistance) {
    iline2d line;
    line.start.x = 0;
    line.start.y = 0;
    
    line.end.x = 3;
    line.end.y = 0;
    
    ipos p0 = {1, 1};
    
    ireal d0 = iline2dsigneddistance(&line, &p0);
    SP_TRUE(ireal_less_zero(d0));
    SP_TRUE(ireal_equal(d0, -1));
    
    ipos p1 = {1, -1};
    ireal d1 = iline2dsigneddistance(&line, &p1);
    SP_TRUE(ireal_greater_zero(d1));
    SP_TRUE(ireal_equal(d1, 1));
}

SP_CASE(iline2d, iline2dclassifypoint) {
    iline2d line;
    line.start.x = 0;
    line.start.y = 0;
    
    line.end.x = 3;
    line.end.y = 0;
    
    ipos p0 = {1, 1};
    
    ireal d0 = iline2dsigneddistance(&line, &p0);
    SP_TRUE(ireal_less_zero(d0));
    SP_TRUE(ireal_equal(d0, -1));
    
    int c0 = iline2dclassifypoint(&line, &p0, iepsilon);
    SP_EQUAL(c0, EnumPointClass_Left);
    
    ipos p1 = {1, -1};
    ireal d1 = iline2dsigneddistance(&line, &p1);
    SP_TRUE(ireal_greater_zero(d1));
    SP_TRUE(ireal_equal(d1, 1));
    
    int c1 = iline2dclassifypoint(&line, &p1, iepsilon);
    SP_EQUAL(c1, EnumPointClass_Right);
    
    ipos pz = {1, 0};
    
    ireal dz = iline2dsigneddistance(&line, &pz);
    SP_TRUE(ireal_equal_zero(dz));
    SP_TRUE(ireal_equal(dz, 0));
    
    int cz = iline2dclassifypoint(&line, &pz, iepsilon);
    SP_EQUAL(cz, EnumPointClass_On);
}

SP_CASE(iline2d, iline2dintersection) {
    iline2d line0;
    line0.start.x = 0;
    line0.start.y = 0;
    line0.end.x = 3;
    line0.end.y = 0;
    
    iline2d line1;
    line1.start.x = 1;
    line1.start.y = 1;
    line1.end.x = 1;
    line1.end.y = -1;
    
    ipos p;
    int pr = iline2dintersection(&line0, &line1, &p);
    SP_EQUAL(pr, EnumLineClass_Segments_Intersect);
    SP_TRUE(ireal_equal(p.x, 1));
    SP_TRUE(ireal_equal(p.y, 0));
    
    iline2d line2;
    line2.start.x = 0;
    line2.start.y = 1;
    line2.end.x = 3;
    line2.end.y = 1;
    pr = iline2dintersection(&line0, &line2, &p);
    SP_EQUAL(pr, EnumLineClass_Paralell);
    
    
    iline2d line3;
    line3.start.x = 0;
    line3.start.y = 0;
    line3.end.x = 3;
    line3.end.y = 0;
    pr = iline2dintersection(&line0, &line3, &p);
    SP_EQUAL(pr, EnumLineClass_Collinear);
    
    {
        iline2d line;
        line.start.x = 4;
        line.start.y = 0;
        line.end.x = 6;
        line.end.y = 0;
        pr = iline2dintersection(&line0, &line, &p);
        SP_EQUAL(pr, EnumLineClass_Collinear);
    }
    
    {
        iline2d line;
        line.start.x = 0;
        line.start.y = 5;
        line.end.x = 4;
        line.end.y = 1;
        pr = iline2dintersection(&line0, &line, &p);
        SP_EQUAL(pr, EnumLineClass_Lines_Intersect);
    }
    
    {
        iline2d line;
        line.start.x = 0;
        line.start.y = 5;
        line.end.x = 1;
        line.end.y = 1;
        pr = iline2dintersection(&line0, &line, &p);
        SP_EQUAL(pr, EnumLineClass_B_Bisects_A);
    }
    
    {
        iline2d line;
        line.start.x = 5;
        line.start.y = 5;
        line.end.x = 5;
        line.end.y = -1;
        pr = iline2dintersection(&line0, &line, &p);
        SP_EQUAL(pr, EnumLineClass_A_Bisects_B);
    }
}

SP_CASE(iline2d, iline2dclosestpoint) {
    iline2d line0;
    line0.start.x = 0;
    line0.start.y = 0;
    line0.end.x = 3;
    line0.end.y = 0;
    
    {
        ipos center;
        center.x = -1;
        center.y = 1;
        
        ipos v = iline2dclosestpoint(&line0, &center, iepsilon);
        SP_TRUE(ireal_equal(v.x, 0));
        SP_TRUE(ireal_equal(v.y, 0));
    }
    
    {
        ipos center;
        center.x = 1;
        center.y = 1;
        
        ipos v = iline2dclosestpoint(&line0, &center, iepsilon);
        SP_TRUE(ireal_equal(v.x, 1));
        SP_TRUE(ireal_equal(v.y, 0));
    }
    
    {
        ipos center;
        center.x = 4;
        center.y = 1;
        
        ipos v = iline2dclosestpoint(&line0, &center, iepsilon);
        SP_TRUE(ireal_equal(v.x, 3));
        SP_TRUE(ireal_equal(v.y, 0));
    }
}

SP_SUIT(inavi);

SP_CASE(inavi, nothing) {
    inavi_mm_init();

    SP_TRUE(1);
}

SP_CASE(inavi, inavimapdescreadfromtextfile) {
    
    clearalliaoicacheandmemrorystate();
    inavimapdesc desc = {{{0}}, 0, 0, 0};
    
    int err = inavimapdescreadfromtextfile(&desc, "./navi.map");
    SP_EQUAL(err, 0);
    
    printf("%s", "\n");
    
    __array_println(desc.polygons);
    __array_println(desc.polygonsindex);
    __array_println(desc.polygonsconnection);
    
    inavimap * map = inavimapmake(8);
    inavimaploadfromdesc(map, &desc);
    inavimapdescfreeresource(&desc);
    
    inavimapfree(map);
    
    clearalliaoicacheandmemrorystate();
    
    __array_println(desc.polygons);
}

/*map: 8*8 */
static void __navidesc_prepare(inavimapdesc *desc) {
    desc->header.size.w = 8;
    desc->header.size.h = 8;
    
    ipos3 points[] = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 0, 3},
        {1, 0, 3},
        {0, 0, 4},
        {1, 0, 4},
        {4, 0, 4},
        {4, 0, 3},
        {5, 0, 3},
        {5, 0, 4},
        {5, 0, 7},
        {4, 0, 7},
    };
    desc->header.points = icountof(points);
    desc->points = iarraymakeipos3(desc->header.points);
    iarrayinsert(desc->points, 0, points, desc->header.points);
    
    int polygons[] = {
        4,4,4,4,4
    };
    desc->header.polygons = icountof(polygons);
    desc->polygons = iarraymakeint(desc->header.polygons);
    iarrayinsert(desc->polygons, 0, polygons, desc->header.polygons);
    
    int polygonsindex[] = {
        0,2,3,1,
        2,4,5,3,
        5,6,7,3,
        7,6,9,8,
        6,11,10,9
    };
    desc->header.polygonsize = icountof(polygonsindex);
    desc->polygonsindex = iarraymakeint(desc->header.polygonsize);
    iarrayinsert(desc->polygonsindex, 0, polygonsindex, desc->header.polygonsize);
    
    int polygonsconnection[] = {
        -1,1,-1,-1,
        -1,-1,2,0,
        -1,3,-1,1,
        2,4,-1,-1,
        -1,-1,-1,3,
    };
    desc->polygonsconnection = iarraymakeint(desc->header.polygonsize);
    iarrayinsert(desc->polygonsconnection, 0, polygonsconnection, desc->header.polygonsize);
    
    ireal polygonscost[] = {
        -1,1,-1,-1,
        -1,-1,2,0,
        -1,3,-1,1,
        2,4,-1,-1,
        -1,-1,-1,3,
    };
    desc->polygonscosts = iarraymakeireal(desc->header.polygonsize);
    iarrayinsert(desc->polygonscosts, 0, polygonscost, desc->header.polygonsize);
}

SP_SUIT(inavimapdesc);

SP_CASE(inavimapdesc, inavimapdescreadfromtextfile) {
    clearalliaoicacheandmemrorystate();
    inavimapdesc desc = {{{0}}, 0, 0, 0};
    
    int err = inavimapdescreadfromtextfile(&desc, "./navi.map");
    SP_EQUAL(err, 0);
    
    printf("%s", "\n");
    
    __array_println(desc.polygons);
    __array_println(desc.polygonsindex);
    __array_println(desc.polygonsconnection);
    __array_println(desc.polygonscosts);
    
    inavimapdescfreeresource(&desc);
    
    clearalliaoicacheandmemrorystate();
}

SP_CASE(inavimapdesc, preparedesc) {
    inavimapdesc desc = {{{0}}, 0, 0, 0};
    
    __navidesc_prepare(&desc);
    
    printf("%s", "\n");
    
    __array_println(desc.polygons);
    __array_println(desc.polygonsindex);
    __array_println(desc.polygonsconnection);
    __array_println(desc.polygonscosts);
    
    inavimapdescfreeresource(&desc);
    
    SP_TRUE(1);
}

SP_SUIT(inavicellconnection);

SP_CASE(inavicellconnection, nothing) {
    SP_TRUE(1);
    
    inavicellconnection *con = inavicellconnectionmake();
    
    inavicellconnectionfree(con);
}

SP_SUIT(inavicell);

SP_CASE(inavicell, inavicellmake) {
    inavimap *map = inavimapmake(8);
    
    inavicell *cell = inavicellmake(map, NULL, NULL, NULL);
    
    inavicellfree(cell);
    inavimapfree(map);
}

SP_CASE(inavicell, test) {
    /*
     (ipos) start = (x = 6, y = 0)
     Printing description of edge.end:
     (ipos) end = (x = 6, y = 2)
     Printing description of line->start:
     (ipos) start = (x = 1.5316666666666667, y = 0.17010416666666667)
     Printing description of line->end:
     (ipos) end = (x = 6, y = 1)
     */
    iline2d edge = {{6,0}, {6,2}};
    iline2d line = {{1.5316666666666667, 0.17010416666666667}, {6, 1}};
    
    int relation_start = iline2dclassifypoint(&edge, &line.start, iepsilon);
    int relation_end = iline2dclassifypoint(&edge, &line.end, iepsilon);
    
    SP_EQUAL(relation_start, EnumPointClass_Left);
    SP_EQUAL(relation_end, EnumPointClass_On);
}

SP_CASE(inavicell, inavimapfind) {
    
    inavimap *map = inavimapmake(8);
    
    inavimapdesc desc = {{{0}}, 0, 0, 0};
    
    int err = inavimapdescreadfromtextfile(&desc, "./navi.map");
    SP_EQUAL(err, 0);
    
    printf("%s", "\n");
    
    __array_println(desc.polygons);
    __array_println(desc.polygonsindex);
    __array_println(desc.polygonsconnection);
    __array_println(desc.polygonscosts);
    
    inavimaploadfromdesc(map, &desc);
    inavimapdescfreeresource(&desc);
    
    ipos3 p = {1.2019791666666666, 0, 0.7873958333333333};
    inavicell *cell = inavimapfind(map, &p);
    
    SP_EQUAL(cell, iarrayof(map->cells, inavicell*, 0));
    
    ipos3 end = {4.775729166666666, 0,1.5808333333333333};
    
    inavipath * path = inavipathmake();
    inavimapfindpath(map, NULL, &p, &end, path);
    
    {
        /* cell 1 to cell 3 */
        ipos3 p0 = {5.718645833333333 ,0 ,0.3851041666666667};
        ipos3 p1 = {7.53375 ,0 ,3.23625};
        
        inavimapfindpath(map, NULL, &p0, &p1, path);
    }
    
    {
        /*cell 0 to cell 2*/
        ipos3 p0 = {1.5316666666666667, 0, 0.17010416666666667};
        ipos3 p1 = {6.384375, 0, 0.6238541666666667};
        
        inavimapfindpath(map, NULL, &p0, &p1, path);
        
        inavimapsmoothpath(map, NULL, path, INT32_MAX);
    }
    
    {
        /*cell 1 to cell 0*/
        ipos3 p0 = {2.7309375, 0, 1.0340625};
        ipos3 p1 = {1.3151041666666667, 0, 0.608125};
        
        inavimapfindpath(map, NULL, &p0, &p1, path);
        
        inavimapsmoothpath(map, NULL, path, INT32_MAX);
    }
    
    {
        /*cell 3 to cell 0*/
        ipos3 p0 = {7.166458333333333, 0,3.536770833333333};
        ipos3 p1 = {1.6123958333333333, 0, 0.51625};
        
        inavimapfindpath(map, NULL, &p0, &p1, path);
    }
    
    {
        /*cell 1 to cell 3*/
        ipos3 p0 = {4.6275, 0, 1.1232291666666667};
        ipos3 p1 = {7.0459375, 0, 8.425520833333334};
        inavimapfindpath(map, NULL, &p0, &p1, path);
    }
    
    {
        /*cell 0 to cell 3*/
        ipos3 p0 = {0.9146875, 0, 0.5341666666666667};
        ipos3 p1 = {6.977083333333334, 0, 9.5159375};
        
        inavimapfindpath(map, NULL, &p0, &p1, path);
        
        inavimapsmoothpath(map, NULL, path, INT32_MAX);
    }
    
    irange(map->cells, inavicell*,
           printf("ref:%d"__icell_format"\n", __value->ref,__icell_value(*__value));
           );
    
    inavipathfree(path);
    
    inavimapfree(map);
}

SP_CASE(inavicell, inavimapcelladd) {
    inavimap *map = inavimapmake(8);
    inavimapdesc desc = {{{0}}, 0, 0, 0};
    int err = inavimapdescreadfromtextfile(&desc, "./navi.map");
    SP_EQUAL(err, 0);
    inavimaploadfromdesc(map, &desc);
    
    ipos pos = {0, 0};
    isize size = {16, 16};
    imap *aoimap = imapmake(&pos, &size, 4);
    
    irange(map->cells, inavicell*,
           printf("map-cell[%d]: "__icell_format"\n", __key, __icell_value(*__value));
           );
   
    /*add cell to aoi map*/
    irangeover(map->cells, int, inavicell*,
               inavimapcelladd(map, __value, aoimap);
               );
    
    ipos3 pos3 = {0, 0, 0};
    iarray *cells = inavimapcellfind(map, aoimap, &pos3);
    printf("\n");
    
    irange(cells, inavicell*,
           printf(__icell_format"\n", __icell_value(*__value));
           );
    
    iarrayfree(cells);
    
    /*remove cell from aoi map*/
    irangeover(map->cells, int, inavicell*,
               inavimapcelldel(map, __value, aoimap);
               );
    
    inavimapdescfreeresource(&desc);
    inavimapfree(map);
    
    imapfree(aoimap);
}

SP_CASE(inavicell, inavimapcelldel) {
    SP_TRUE(1);
}

SP_CASE(inavicell, inavimapcellfind) {
    SP_TRUE(1);
}

SP_SUIT(inaviwaypoint);

SP_CASE(inaviwaypoint, nothing) {
    SP_TRUE(1);
}

SP_SUIT(inavipath);

SP_CASE(inavipath, nothing) {
    SP_TRUE(1);
}

SP_SUIT(inavimap);

SP_CASE(inavimap, nothing) {
    SP_TRUE(1);
}

/*检验一下内存是否有泄露*/
SP_SUIT(memorycheck);

SP_CASE(memorycheck, check) {
    clearalliaoicacheandmemrorystate();
    
    SP_TRUE(1);
    
    /*check the memory leak*/
    SP_TRUE(iaoimemorysize(NULL, EnumAoiMemoerySizeKind_Hold)==0);
    SP_EQUAL(iaoimemorysize(NULL, EnumAoiMemoerySizeKind_Freed),
             iaoimemorysize(NULL, EnumAoiMemoerySizeKind_Alloced));
}


SP_SUIT(navimesh);

SP_CASE(navimesh, tiled) {
    
    /*
     *
     */
    
    printf("\n");
    for (int j=0; j<16; ++j) {
        for (int i=0; i<16; ++i) {
            int b = (i==j|| i-j==1 || j-i==1);
            printf("%s%d", i!=0?",":"\n",b);
        }
    }
    printf("\n");
    
    char __blocks [] = {
        1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    };
    int trytotal = 2 + 2 + 14 * 3;
    
    typedef struct _iwaypos {
        int x, y;
    }_iwaypos;
    
    typedef struct _iwaytry {
        _iwaypos pos;
        int next;
    }_iwaytry;
    
    iarray *__meta_arr = iarraymakeipos(2);
    iarrayentry _waytry_entry = {
        EnumArrayFlagAutoShirk |
        EnumArrayFlagKeepOrder |
        EnumArrayFlagMemsetZero,
        sizeof(_iwaytry),
        __meta_arr->entry->swap,
        __meta_arr->entry->assign,
        NULL,
    };
    iarrayfree(__meta_arr);
    
    iarray *arr = iarraymake(20, &_waytry_entry);
    
    while (trytotal) {
        
    }
    
    iarrayfree(arr);
}

#endif
