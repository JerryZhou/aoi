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
#include "simpletest.h"


// **********************************************************************************
// time
SIMPLETEST_SUIT(time);

SIMPLETEST_CASE(time, igetcurnanoandigetcurtick) {
    int64_t nano = igetcurnano();
    int64_t ticks = igetcurtick();
    SIMPLETEST_EQUAL((nano - ticks * 1000 < 1000), 1);
}

SIMPLETEST_CASE(time, igetnextnano) {
    int64_t nano0 = igetnextnano();
    int64_t nano1 = igetnextnano();
    
    SIMPLETEST_EQUAL(nano0 != nano1, 1);
}

// **********************************************************************************
// ipos
SIMPLETEST_SUIT(ipos);

SIMPLETEST_CASE(ipos, idistancepow2) {
    ipos pos1 = {.x=0, .y=0};
    ipos pos2 = {.x=3, .y=4};
    
    SIMPLETEST_EQUAL(25.0, idistancepow2(&pos1, &pos2));
}

// **********************************************************************************
// isize
SIMPLETEST_SUIT(isize);

SIMPLETEST_CASE(isize, nothing) {
    SIMPLETEST_EQUAL(1, 1);
}

// **********************************************************************************
// irect
SIMPLETEST_SUIT(irect);

SIMPLETEST_CASE(irect, irectcontainsTHEself) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=1,.h=1}};
    
    SIMPLETEST_EQUAL(irectcontains(&r, &r), 1);
}

SIMPLETEST_CASE(irect, irectcontainsTHEsub) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=2,.h=2}};
    
    irect r0 = {.pos={.x=0,.y=0}, .size={.w=0,.h=0}};
    
    SIMPLETEST_EQUAL(irectcontains(&r, &r0), 1);
}

SIMPLETEST_CASE(irect, irectcontainsTHEsubNo) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=2,.h=2}};
    
    irect r0 = {.pos={.x=0,.y=0}, .size={.w=3,.h=3}};
    
    SIMPLETEST_EQUAL(irectcontains(&r, &r0), 0);
}

SIMPLETEST_CASE(irect, irectcontainsTHEsubNo2) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=2,.h=2}};
    
    irect r0 = {.pos={.x=1,.y=1}, .size={.w=2,.h=2}};
    
    SIMPLETEST_EQUAL(irectcontains(&r, &r0), 0);
}

SIMPLETEST_CASE(irect, irectcontainsTHEsubNo3) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=2,.h=2}};
    
    irect r0 = {.pos={.x=-1,.y=-1}, .size={.w=2,.h=2}};
    
    SIMPLETEST_EQUAL(irectcontains(&r, &r0), 0);
}

SIMPLETEST_CASE(irect, irectcontainsTHEsubNo4) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=2,.h=2}};
    
    irect r0 = {.pos={.x=-1,.y=-1}, .size={.w=5,.h=5}};
    
    SIMPLETEST_EQUAL(irectcontains(&r, &r0), 0);
}

SIMPLETEST_CASE(irect, irectcontainspoint) {
    irect r = {.pos={.x=0,.y=0}, .size={.w=2,.h=2}};
    ipos p = {.x=0, .y=0};
    
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 2, p.y = 2;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 2, p.y = 0;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 0, p.y = 2;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iiok);
    p.x = 1, p.y = 1;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iiok);
    
    p.x = 2, p.y = -1;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = 3, p.y = -1;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = -1, p.y = -1;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iino);
    
    p.x = -1, p.y = 1;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = -1, p.y = 2;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iino);
    p.x = -1, p.y = 3;
    SIMPLETEST_EQUAL(irectcontainspoint(&r, &p), iino);
}

// **********************************************************************************
// icircle
SIMPLETEST_SUIT(icircle);

SIMPLETEST_CASE(icircle, icircleintersect) {
    icircle c = {.pos={.x=0, .y=0}, .radis=1.0};
    
    SIMPLETEST_EQUAL(c.radis, 1.0);
    
    icircle c0 = {.pos={.x=0, .y=0}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icircleintersect(&c, &c0), 1);
}

SIMPLETEST_CASE(icircle, icircleintersectYES) {
    icircle c = {.pos={.x=0, .y=0}, .radis=1.0};
    
    SIMPLETEST_EQUAL(c.radis, 1.0);
    
    icircle c0 = {.pos={.x=0, .y=3}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icircleintersect(&c, &c0), 1);
}

SIMPLETEST_CASE(icircle, icircleintersectNo) {
    icircle c = {.pos={.x=0, .y=0}, .radis=1.0};
    
    SIMPLETEST_EQUAL(c.radis, 1.0);
    
    icircle c0 = {.pos={.x=3, .y=3}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icircleintersect(&c, &c0), 0);
}

SIMPLETEST_CASE(icircle, icirclecontains) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icirclecontains(&c, &c), 1);
}

SIMPLETEST_CASE(icircle, icirclecontainsYES) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    
    icircle c0 = {.pos={.x=0, .y=0}, .radis=1.0};
    
    SIMPLETEST_EQUAL(icirclecontains(&c, &c0), 1);
}

SIMPLETEST_CASE(icircle, icirclecontainsYES1) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icirclecontains(&c, NULL), 1);
}

SIMPLETEST_CASE(icircle, icirclecontainsNO) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icirclecontains(NULL, &c), 0);
}

SIMPLETEST_CASE(icircle, icirclecontainsNO1) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    icircle c0 = {.pos={.x=0, .y=0}, .radis=3.0};
    
    SIMPLETEST_EQUAL(icirclecontains(&c, &c0), 0);
}

SIMPLETEST_CASE(icircle, icirclecontainsNO2) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    icircle c0 = {.pos={.x=1, .y=1}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icirclecontains(&c, &c0), 0);
}

SIMPLETEST_CASE(icircle, icirclecontainsNO3) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    icircle c0 = {.pos={.x=5, .y=5}, .radis=2.0};
    
    SIMPLETEST_EQUAL(icirclecontains(&c, &c0), 0);
}

SIMPLETEST_CASE(icircle, icirclerelation) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    icircle c0 = {.pos={.x=0, .y=0}, .radis=1.0};
    icircle c1 = {.pos={.x=1, .y=0}, .radis=1.0};
    icircle c2 = {.pos={.x=3, .y=0}, .radis=1.0};
    
    SIMPLETEST_EQUAL(icirclerelation(&c, &c), EnumCircleRelationAContainsB);
    SIMPLETEST_EQUAL(icirclerelation(&c, &c0), EnumCircleRelationAContainsB);
    SIMPLETEST_EQUAL(icirclerelation(&c0, &c), EnumCircleRelationBContainsA);
    SIMPLETEST_EQUAL(icirclerelation(&c0, &c1), EnumCircleRelationIntersect);
    SIMPLETEST_EQUAL(icirclerelation(&c0, &c2), EnumCircleRelationNoIntersect);
}

SIMPLETEST_CASE(icircle, icirclecontainspoint) {
    icircle c = {.pos={.x=0, .y=0}, .radis=2.0};
    ipos p = {.x=0, .y=0};
    
    SIMPLETEST_EQUAL(icirclecontainspoint(&c, &p), iiok);
    
    p.x = 2, p.y = 0;
    SIMPLETEST_EQUAL(icirclecontainspoint(&c, &p), iiok);
    
    p.x = 3, p.y = 0;
    SIMPLETEST_EQUAL(icirclecontainspoint(&c, &p), iino);
}

// **********************************************************************************
// iname
SIMPLETEST_SUIT(iname);

SIMPLETEST_CASE(iname, nothing) {
    iname name = {.name = {'A', 'B', 'C', 0}};
    
    SIMPLETEST_EQUAL(name.name[0], 'A');
    SIMPLETEST_EQUAL(name.name[1], 'B');
    SIMPLETEST_EQUAL(name.name[2], 'C');
    SIMPLETEST_EQUAL(name.name[3], 0);
}

// **********************************************************************************
// iref
SIMPLETEST_SUIT(iref);

SIMPLETEST_CASE(iref, iretainANDirelease) {
    iref *ref = iobjmalloc(iref);
    
    SIMPLETEST_EQUAL(ref->ref, 0);
    
    iretain(ref);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    iretain(ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    irelease(ref);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

static int facereffreecc = 0;
static void fake_ref_free(iref *ref) {
    iobjfree(ref);
    ++facereffreecc;
}

SIMPLETEST_CASE(iref, irelease) {
    iref *ref = iobjmalloc(iref);
    ref->free = fake_ref_free;
    
    SIMPLETEST_EQUAL(ref->ref, 0);
    
    iretain(ref);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    iretain(ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    irelease(ref);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    SIMPLETEST_EQUAL(facereffreecc, 0);
    irelease(ref);
    SIMPLETEST_EQUAL(facereffreecc, 1);
}

// **********************************************************************************
// ireflist
SIMPLETEST_SUIT(ireflist);

SIMPLETEST_CASE(ireflist, ireflistmake) {
    ireflist *list = ireflistmake();
    
    ireflistfree(list);
}

SIMPLETEST_CASE(ireflist, ireflistlen) {
    ireflist *list = ireflistmake();
    
    SIMPLETEST_EQUAL(ireflistlen(list), 0);
    
    ireflistfree(list);
}

SIMPLETEST_CASE(ireflist, ireflistfirst) {
    ireflist *list = ireflistmake();
    
    SIMPLETEST_EQUAL(ireflistfirst(list), NULL);
    
    ireflistfree(list);
}

SIMPLETEST_CASE(ireflist, ireflistadd) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SIMPLETEST_CASE(ireflist, ireflistaddANDireflistfirst) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), joint);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SIMPLETEST_CASE(ireflist, ireflistfind) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SIMPLETEST_CASE(ireflist, ireflistaddjoint) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    ireflistaddjoint(list, add);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 2);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), add);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), add);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SIMPLETEST_CASE(ireflist, ireflistremovejoint) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    SIMPLETEST_EQUAL(add->list, NULL);
    
    ireflistaddjoint(list, add);
    
    SIMPLETEST_EQUAL(add->list, list);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 2);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), add);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), add);
    
    ireflistremovejoint(list, add);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), joint);
    
    irefjointfree(add);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}


SIMPLETEST_CASE(ireflist, ireflistremove) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    ireflistaddjoint(list, add);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 2);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), add);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), add);
    
    
    SIMPLETEST_EQUAL(add->list, list);
    
    ireflistremovejoint(list, add);
    
    SIMPLETEST_EQUAL(add->list, NULL);
    
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), joint);
    
    ireflistremove(list, ref);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 0);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), NULL);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), NULL);
    
    irefjointfree(add);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

SIMPLETEST_CASE(ireflist, ireflistremoveall) {
    ireflist *list = ireflistmake();
    
    iref *ref = iobjmalloc(iref);
    iretain(ref);
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irefjoint *joint = ireflistadd(list, ref);
    
    SIMPLETEST_EQUAL(ref->ref, 2);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), joint);
    
    irefjoint *add = irefjointmake(ref);
    
    ireflistaddjoint(list, add);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 2);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), add);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), add);
    
    ireflistremoveall(list);
    
    SIMPLETEST_EQUAL(ireflistlen(list), 0);
    
    SIMPLETEST_EQUAL(ireflistfind(list, ref), NULL);
    
    SIMPLETEST_EQUAL(ireflistfirst(list), NULL);
    
    ireflistfree(list);
    
    SIMPLETEST_EQUAL(ref->ref, 1);
    
    irelease(ref);
}

// **********************************************************************************
// irefcache
SIMPLETEST_SUIT(irefcache);

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

SIMPLETEST_CASE(irefcache, irefcachemake) {
    irefcache *cache = irefcachemake(2, fake_cache_new);
    
    SIMPLETEST_EQUAL(cache->capacity, 2);
    
    SIMPLETEST_EQUAL(cache->newentry, fake_cache_new);
    
    irefcachefree(cache);
}

SIMPLETEST_CASE(irefcache, irefcachepoll) {
    irefcache *cache = irefcachemake(1, fake_cache_new);
    
    SIMPLETEST_EQUAL(cache->capacity, 1);
    
    SIMPLETEST_EQUAL(cache->newentry, fake_cache_new);
    
    SIMPLETEST_EQUAL(fakecachenewcc, 0);
    iref *ref = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    iref *ref0 = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref);
    SIMPLETEST_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref0);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    
    iref *ref1 = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    SIMPLETEST_EQUAL(ref1, ref);
    
    irefcachepush(cache, ref1);
    
    irefcachefree(cache);
    
    SIMPLETEST_EQUAL(fakecachenewcc, 0);
}

SIMPLETEST_CASE(irefcache, irefcachepush) {
    irefcache *cache = irefcachemake(1, fake_cache_new);
    
    SIMPLETEST_EQUAL(cache->capacity, 1);
    
    SIMPLETEST_EQUAL(cache->newentry, fake_cache_new);
    
    SIMPLETEST_EQUAL(fakecachenewcc, 0);
    iref *ref = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    iref *ref0 = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref);
    SIMPLETEST_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref0);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    
    iref *ref1 = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    SIMPLETEST_EQUAL(ref1, ref);
    
    irefcachepush(cache, ref1);
    
    irefcachefree(cache);
    
    SIMPLETEST_EQUAL(fakecachenewcc, 0);
}

SIMPLETEST_CASE(irefcache, irefcachefree) {
    irefcache *cache = irefcachemake(1, fake_cache_new);
    
    SIMPLETEST_EQUAL(cache->capacity, 1);
    
    SIMPLETEST_EQUAL(cache->newentry, fake_cache_new);
    
    SIMPLETEST_EQUAL(fakecachenewcc, 0);
    iref *ref = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    iref *ref0 = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref);
    SIMPLETEST_EQUAL(fakecachenewcc, 2);
    irefcachepush(cache, ref0);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    
    iref *ref1 = irefcachepoll(cache);
    SIMPLETEST_EQUAL(fakecachenewcc, 1);
    SIMPLETEST_EQUAL(ref1, ref);
    
    irefcachepush(cache, ref1);
    
    irefcachefree(cache);
    
    SIMPLETEST_EQUAL(fakecachenewcc, 0);
}

// **********************************************************************************
// icode
SIMPLETEST_SUIT(icode);

SIMPLETEST_CASE(icode, nothing) {
    icode code = {.code={'A','B', 0}, .pos={.x=0, .y=0}};
    
    SIMPLETEST_EQUAL(code.code[0], 'A');
}

// **********************************************************************************
// iuserdata
SIMPLETEST_SUIT(iuserdata);

SIMPLETEST_CASE(iuserdata, nothing) {
    iuserdata u = {.u1=0, .up1=NULL};
    
    SIMPLETEST_EQUAL(u.u1, 0);
    
    SIMPLETEST_EQUAL(u.up1, NULL);
}

// **********************************************************************************
// iunit
SIMPLETEST_SUIT(iunit);

SIMPLETEST_CASE(iunit, imakeunit) {
    iunit *unit = imakeunit(0, 0, 0);
    
    SIMPLETEST_EQUAL(unit->id, 0);
    
    ifreeunit(unit);
}

SIMPLETEST_CASE(iunit, ifreeunit) {
    iunit *unit = imakeunit(0, 0, 0);
    
    SIMPLETEST_EQUAL(unit->id, 0);
    
    ifreeunit(unit);
}

SIMPLETEST_CASE(iunit, ifreeunitlist) {
    iunit *unit = imakeunit(0, 0, 0);
    
    SIMPLETEST_EQUAL(unit->id, 0);
    
    ifreeunitlist(unit);
}

// **********************************************************************************
// inode
SIMPLETEST_SUIT(inode);

SIMPLETEST_CASE(inode, nothing) {
    inode *node = imakenode();
    ifreenodekeeper(node);
    
    SIMPLETEST_EQUAL(1, 1);
}


// **********************************************************************************
// imap
SIMPLETEST_SUIT(imap);

imap *map; // {.x=0, .y=0}, {.w=8, .h=8}, 3
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
SIMPLETEST_CASE(imap, imapmake) {
    ipos pos = {.x=0, .y=0};
    isize size = {.w=8, .h=8};
    map = imapmake(&pos, &size, 3);
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
    
    SIMPLETEST_EQUAL(map->root != NULL, 1);
    SIMPLETEST_EQUAL(map->root->level, 0);
    SIMPLETEST_EQUAL(map->root->code.code[0], 'R');
    SIMPLETEST_EQUAL(map->root->code.code[1], 'O');
    SIMPLETEST_EQUAL(map->root->code.code[2], 'O');
    SIMPLETEST_EQUAL(map->root->code.code[3], 'T');
    
    SIMPLETEST_EQUAL(map->nodesizes[0].w, 8);
    SIMPLETEST_EQUAL(map->nodesizes[0].h, 8);
    
    SIMPLETEST_EQUAL(map->nodesizes[1].w, 4);
    SIMPLETEST_EQUAL(map->nodesizes[1].h, 4);
    
    SIMPLETEST_EQUAL(map->nodesizes[2].w, 2);
    SIMPLETEST_EQUAL(map->nodesizes[2].h, 2);
    
    SIMPLETEST_EQUAL(map->nodesizes[3].w, 1);
    SIMPLETEST_EQUAL(map->nodesizes[3].h, 1);
    
    SIMPLETEST_EQUAL(map->distances[0], 128);
    SIMPLETEST_EQUAL(map->distances[1], 32);
    SIMPLETEST_EQUAL(map->distances[2], 8);
    SIMPLETEST_EQUAL(map->distances[3], 2);
}

SIMPLETEST_CASE(imap, imapgencode) {
    
    icode code;
    
    ipos p = {.x = 0, .y = 0};
    
    imapgencode(map, &p, &code);
    
    SIMPLETEST_EQUAL(code.pos.x, p.x);
    SIMPLETEST_EQUAL(code.pos.y, p.y);
    
    SIMPLETEST_EQUAL(code.code[0], 'A');
    SIMPLETEST_EQUAL(code.code[1], 'A');
    SIMPLETEST_EQUAL(code.code[2], 'A');
    SIMPLETEST_EQUAL(code.code[3], 0);
    
    p.x = 1.0;
    p.y = 1.0;
    
    imapgencode(map, &p, &code);
    
    SIMPLETEST_EQUAL(code.code[0], 'A');
    SIMPLETEST_EQUAL(code.code[1], 'A');
    SIMPLETEST_EQUAL(code.code[2], 'D');
    SIMPLETEST_EQUAL(code.code[3], 0);
}

SIMPLETEST_CASE(imap, imapgenpos) {
    
    icode code = {.code={'A', 'A', 'A', 0}};
    
    ipos p = {.x = 0, .y = 0};
    
    imapgenpos(map, &p, &code);
    
    SIMPLETEST_EQUAL(0, p.x);
    SIMPLETEST_EQUAL(0, p.y);
    
    code.code[2] = 'D';
    
    imapgenpos(map, &p, &code);
    
    SIMPLETEST_EQUAL(1.0, p.x);
    SIMPLETEST_EQUAL(1.0, p.y);
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
SIMPLETEST_CASE(imap, prepareunit) {
    
    for (int i=0; i<__preparecnt; ++i) {
        iunit *u = __getmeunit(0, 0);
        __hold(u);
        ifreeunit(u);
    }
    
    SIMPLETEST_TRUE();
}

SIMPLETEST_CASE(imap, imapaddunit) {
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
    
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
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 1);
}

SIMPLETEST_CASE(imap, imapremoveunit) {
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 1);
    
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
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
}

SIMPLETEST_CASE(imap, complexANDimapaddunitANDimapremoveunitANDimapgetnode) {
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
    
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
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 1);
    
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
    
    SIMPLETEST_EQUAL(map->divide, 3);
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 3);
    
    icode code = {.code={'A', 'A', 'A', 0}};
    
    inode *node = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate);
    
    SIMPLETEST_EQUAL(node, map->root);
    
    node = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate);
    
    SIMPLETEST_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 0, 1);
    
    node = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate);
    
    SIMPLETEST_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 'A'
                     && node->code.code[2] == 0, 1);
    
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    
    SIMPLETEST_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 'A'
                     && node->code.code[2] == 'A'
                     && node->code.code[3] == 0, 1);
    
    SIMPLETEST_EQUAL(node, __getunitfor(0)->node);
    SIMPLETEST_EQUAL(node, __getunitfor(1)->node);
    SIMPLETEST_EQUAL(node, __getunitfor(2)->node);
    
    __setu(3, 1, 1);
    
    imapaddunit(map, __getunitfor(3));
    
    SIMPLETEST_EQUAL(map->state.nodecount, 4);
    SIMPLETEST_EQUAL(map->state.leafcount, 2);
    SIMPLETEST_EQUAL(map->state.unitcount, 4);
    
    code.code[2] = 'D';
    
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    
    SIMPLETEST_EQUAL(node != NULL
                     && node->code.code[0] == 'A'
                     && node->code.code[1] == 'A'
                     && node->code.code[2] == 'D'
                     && node->code.code[3] == 0, 1);
    
    SIMPLETEST_EQUAL(node, __getunitfor(3)->node);
    
    code.code[0] = 'D';
    node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SIMPLETEST_EQUAL(node, NULL);
    node = imapgetnode(map, &code, 3, EnumFindBehaviorFuzzy);
    SIMPLETEST_EQUAL(node, map->root);
    code.code[0] = 'A';
    code.code[1] = 'A';
    code.code[2] = 'B';
    node = imapgetnode(map, &code, 3, EnumFindBehaviorFuzzy);
    SIMPLETEST_EQUAL(node, map->root->childs[0]->childs[0]);
    code.code[1] = 'D';
    node = imapgetnode(map, &code, 3, EnumFindBehaviorFuzzy);
    SIMPLETEST_EQUAL(node, map->root->childs[0]);
    
    imapremoveunit(map, __getunitfor(3));
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 3);
    
    SIMPLETEST_EQUAL(ireflistlen(map->nodecache->cache), 1);
    
    imapremoveunit(map, __getunitfor(0));
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 2);
    
    imapremoveunit(map, __getunitfor(1));
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 1);
    
    imapremoveunit(map, __getunitfor(2));
    
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
    
    SIMPLETEST_EQUAL(ireflistlen(map->nodecache->cache), 4);
    
    SIMPLETEST_EQUAL(NULL, __getunitfor(0)->node);
    SIMPLETEST_EQUAL(NULL, __getunitfor(1)->node);
    SIMPLETEST_EQUAL(NULL, __getunitfor(2)->node);
    SIMPLETEST_EQUAL(NULL, __getunitfor(3)->node);
    
    __resetu(0);
    __resetu(1);
    __resetu(2);
    __resetu(3);
}


SIMPLETEST_CASE(imap, imapupdateunit) {
    
    __setu(0, 0.8, 0.8);
    
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
    
    imapaddunit(map, __getunitfor(0));
    icode code = {.code={'A','A','A',0}};
    
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
    
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 1);
    SIMPLETEST_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
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
    SIMPLETEST_EQUAL(map->state.nodecount, 3);
    SIMPLETEST_EQUAL(map->state.leafcount, 1);
    SIMPLETEST_EQUAL(map->state.unitcount, 1);
    SIMPLETEST_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
                     __getunitfor(0)->node);
    
    
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
    
    SIMPLETEST_EQUAL(nts > ts, 1);
    SIMPLETEST_EQUAL(ntsA > tsA, 1);
    SIMPLETEST_EQUAL(ntsAA > tsAA, 1);
    SIMPLETEST_EQUAL(ntsAAA > tsAAA, 1);
    
    __setu(0, 1.0, 1.0);
    imapupdateunit(map, __getunitfor(0));
    
    code.code[0] = 'A';
    code.code[1] = 'A';
    code.code[2] = 'D';
    SIMPLETEST_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
                     __getunitfor(0)->node);
    
    int64_t nnts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t nntsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t nntsAA = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    int64_t nntsAAA = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    SIMPLETEST_EQUAL(nts, nnts);
    SIMPLETEST_EQUAL(ntsA, nntsA);
    SIMPLETEST_EQUAL(ntsAA, nntsAA);
    SIMPLETEST_EQUAL(ntsAAA != nntsAAA, 1);
    code.code[2] = 'D';
    int64_t nntsAAD = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    
    SIMPLETEST_EQUAL(nntsAAD >= ntsAAA, 1);
    
    SIMPLETEST_EQUAL(map->state.nodecount, 4);
    SIMPLETEST_EQUAL(map->state.leafcount, 2);
    SIMPLETEST_EQUAL(map->state.unitcount, 2);
    
    
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
    SIMPLETEST_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate), NULL);
    
    code.code[0] = 'A';
    code.code[1] = 'D';
    code.code[2] = 'D';
    
    SIMPLETEST_EQUAL(imapgetnode(map, &code, 3, EnumFindBehaviorAccurate),
                     __getunitfor(0)->node);
    
    int64_t nnnnts = imapgetnode(map, &code, 0, EnumFindBehaviorAccurate)->tick;
    int64_t nnnntsA = imapgetnode(map, &code, 1, EnumFindBehaviorAccurate)->tick;
    int64_t nnnntsAD = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate)->tick;
    int64_t nnnntsADD = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate)->tick;
    
    SIMPLETEST_EQUAL(nnnnts, nnnts);
    SIMPLETEST_EQUAL(nnnntsA, nnntsA);
    SIMPLETEST_EQUAL(nnnntsAD > nnntsAA, 1);
    SIMPLETEST_EQUAL(nnnntsADD > nnntsAA, 1);
    
    imapremoveunit(map, __getunitfor(0));
    imapremoveunit(map, __getunitfor(1));
    
    SIMPLETEST_EQUAL(map->state.nodecount, 0);
    SIMPLETEST_EQUAL(map->state.leafcount, 0);
    SIMPLETEST_EQUAL(map->state.unitcount, 0);
}

SIMPLETEST_CASE(imap, end) {
    
    ireflistremoveall(map->nodecache->cache);
    
    SIMPLETEST_TRUE();
}

// **********************************************************************************
// ifilter
SIMPLETEST_SUIT(ifilter);

SIMPLETEST_CASE(ifilter, ifiltermake) {
    ifilter *filter = ifiltermake();
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter), 0);
    
    ifilterfree(filter);
}

SIMPLETEST_CASE(ifilter, ifiltermake_circle) {
    
    ipos pos = {.x=0, .y=0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filterrange) != 0, 1);
    
    SIMPLETEST_EQUAL(filterrange->circle.radis, 2.0);
    
    SIMPLETEST_EQUAL(filterrange->circle.pos.x, 0);
    SIMPLETEST_EQUAL(filterrange->circle.pos.y, 0);
    
    ifilterfree(filterrange);
}

SIMPLETEST_CASE(ifilter, ifiltermake_rect) {
    ipos pos = {.x=0, .y=0};
    isize size = {.w=2, .h=2};
    ifilter *filterrect = ifiltermake_rect(&pos, &size);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filterrect) != 0, 1);
    SIMPLETEST_EQUAL(filterrect->rect.pos.x, 0);
    SIMPLETEST_EQUAL(filterrect->rect.pos.y, 0);
    SIMPLETEST_EQUAL(filterrect->rect.size.w, 2);
    SIMPLETEST_EQUAL(filterrect->rect.size.h, 2);
    
    ifilterfree(filterrect);
}

SIMPLETEST_CASE(ifilter, ifilteradd) {
    ifilter *filter = ifiltermake();
    
    ipos pos = {.x=0, .y=0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter), 0);
    
    ifilteradd(filter, filterrange);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

SIMPLETEST_CASE(ifilter, ifilterremove) {
    ifilter *filter = ifiltermake();
    
    ipos pos = {.x=0, .y=0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter), 0);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 0);
    
    ifilteradd(filter, filterrange);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 1);
    
    ifilterremove(filter, filterrange);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 0);
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

SIMPLETEST_CASE(ifilter, ifilterclean) {
    ifilter *filter = ifiltermake();
    
    ipos pos = {.x=0, .y=0};
    ifilter *filterrange = ifiltermake_circle(&pos, 2.0);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter), 0);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 0);
    
    ifilteradd(filter, filterrange);
    ifilteradd(filter, filterrange);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 2);
    
    ifilterclean(filter);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 0);
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

int __filter_test_forid(imap *map, ifilter *filter, iunit *unit) {
    icheckret(filter, iino);
    icheckret(unit, iino);
    
    if (unit->id == filter->id) {
        return iiok;
    }
    
    return iino;
}

SIMPLETEST_CASE(ifilter, ifilterrun) {
    
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
    SIMPLETEST_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(1));
    SIMPLETEST_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(2));
    SIMPLETEST_EQUAL(ok, iiok);
    
    ipos pos = {.x=0, .y=0};
    ifilter *filterrange = ifiltermake_circle(&pos, 1.0);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter), 0);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 0);
    
    ifilteradd(filter, filterrange);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    ok = ifilterrun(map, filter, __getunitfor(0));
    SIMPLETEST_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(1));
    SIMPLETEST_EQUAL(ok, iiok);
    ok = ifilterrun(map, filter, __getunitfor(2));
    SIMPLETEST_EQUAL(ok, iino);
    
    __setu(0, -0.3, -0.4);
    imapupdateunit(map, __getunitfor(0));
    SIMPLETEST_EQUAL(__getunitfor(0)->node->code.code[2], 'A');
    ok = ifilterrun(map, filter, __getunitfor(0));
    SIMPLETEST_EQUAL(ok, iiok);
    
    __setu(0, 1.3, 1.4);
    imapupdateunit(map, __getunitfor(0));
    SIMPLETEST_EQUAL(__getunitfor(0)->node->code.code[2], 'D');
    ok = ifilterrun(map, filter, __getunitfor(0));
    SIMPLETEST_EQUAL(ok, iino);
    
    // 过滤ID
    ok = ifilterrun(map, filter, __getunitfor(1));
    SIMPLETEST_EQUAL(ok, iiok);
    filter->id = 2;
    filter->entry = __filter_test_forid;
    ok = ifilterrun(map, filter, __getunitfor(1));
    SIMPLETEST_EQUAL(ok, iino);
    
    // 清理 map
    imapremoveunit(map, __getunitfor(0));
    imapremoveunit(map, __getunitfor(1));
    imapremoveunit(map, __getunitfor(2));
    
    ifilterfree(filter);
    ifilterfree(filterrange);
}

SIMPLETEST_CASE(ifilter, imapcollectunit) {
    
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
    
    ipos pos = {.x=0, .y=0};
    ifilter *filterrange = ifiltermake_circle(&pos, 1.0);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter), 0);
    
    SIMPLETEST_EQUAL(ireflistlen(filter->list), 0);
    
    ifilteradd(filter, filterrange);
    
    SIMPLETEST_EQUAL(ifilterchecksum(map, filter) != 0, 1);
    
    ireflist *list = ireflistmake();
    
    icode code = {.code={'A', 'A', 0}};
    
    inode *node = imapgetnode(map, &code, 2, EnumFindBehaviorAccurate);

#define __range(x) (x)
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (3: 1.5, 1.2) (4: 1.5, 0.2) (5: 1.0, 1.0)
    // ------------------------------------------------------------------------------------
    ireflist *snap = ireflistmake();
    filterrange->circle.radis = __range(2.0);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (3: 1.5, 1.2) (4: 1.5, 0.2) (5: 1.0, 1.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SIMPLETEST_EQUAL(ireflistlen(list), 6);
    
    ireflistremoveall(list);
    filterrange->circle.radis = __range(0.1);
    // (0: 0.1, 0.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SIMPLETEST_EQUAL(ireflistlen(list), 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SIMPLETEST_EQUAL(icast(iunit, ireflistfirst(list)->value)->id, 0);
    
    ireflistremoveall(list);
    filterrange->circle.radis = __range(0.5);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SIMPLETEST_EQUAL(ireflistlen(list), 3);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(1))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(2))) != NULL, 1);
    
    ireflistremoveall(list);
    filterrange->circle.radis = __range(1.5);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (5: 1.0, 1.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SIMPLETEST_EQUAL(ireflistlen(list), 4);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(1))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(2))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(5))) != NULL, 1);
    
    ireflistremoveall(list);
    filterrange->circle.radis = __range(1.7);
    // (0: 0.1, 0.0) (1: 0.3, 0.4) (2: 0.5, 0.0) (4: 1.5, 0.2) (5: 1.0, 1.0)
    imapcollectunit(map, node, list, filter, snap);
    imapcollectcleanunittag(map, snap);
    SIMPLETEST_EQUAL(ireflistlen(list), 5);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(0))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(1))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(2))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(4))) != NULL, 1);
    SIMPLETEST_EQUAL(ireflistfind(list, irefcast(__getunitfor(5))) != NULL, 1);
    
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

SIMPLETEST_CASE(ifilter, end) {
    ireflistremoveall(map->nodecache->cache);
    SIMPLETEST_TRUE();
}


// **********************************************************************************
// isearchresult
SIMPLETEST_SUIT(isearchresult);

SIMPLETEST_CASE(isearchresult, isearchresultmake) {
    isearchresult *search = isearchresultmake();
    
    SIMPLETEST_EQUAL(search->filter != NULL, 1);
    SIMPLETEST_EQUAL(search->units != NULL, 1);
    
    isearchresultfree(search);
}

SIMPLETEST_CASE(isearchresult, isearchresultattach) {
    isearchresult *search = isearchresultmake();
    
    SIMPLETEST_EQUAL(search->filter != NULL, 1);
    SIMPLETEST_EQUAL(search->units != NULL, 1);
    
    ifilter *filter = ifiltermake();
    isearchresultattach(search, filter);
    SIMPLETEST_EQUAL(search->filter, filter);
    
    isearchresultfree(search);
    ifilterfree(filter);
}

SIMPLETEST_CASE(isearchresult, isearchresultdettach) {
    isearchresult *search = isearchresultmake();
    
    SIMPLETEST_EQUAL(search->filter != NULL, 1);
    SIMPLETEST_EQUAL(search->units != NULL, 1);
    
    ifilter *filter = ifiltermake();
    isearchresultattach(search, filter);
    SIMPLETEST_EQUAL(search->filter, filter);
    
    isearchresultdettach(search);
    
    SIMPLETEST_EQUAL(search->filter, NULL);
    
    isearchresultfree(search);
    ifilterfree(filter);
}

// **********************************************************************************
SIMPLETEST_SUIT(searching);

SIMPLETEST_CASE(searching, imapsearchfromposANDimapsearchfromnode) {
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
    
    SIMPLETEST_EQUAL(result->tick, 0);
    SIMPLETEST_EQUAL(result->checksum, 0);
    
    ipos pos = {.x=0,.y=0};
    icode code = {.code={'A','A','A',0}};
    imapsearchfrompos(map, &pos, result, 0.1);
    int64_t ts = result->tick;
    int64_t checksum = result->checksum;
    inode *node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SIMPLETEST_EQUAL(ts, node->tick);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 1);
    SIMPLETEST_EQUAL(checksum != 0, 1);
    
    imapsearchfrompos(map, &pos, result, 0.1);
    ts = result->tick;
    SIMPLETEST_EQUAL(ts, node->tick);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 1);
    SIMPLETEST_EQUAL(checksum, result->checksum);
    
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
    SIMPLETEST_EQUAL(nts, node->tick);
    SIMPLETEST_EQUAL(nts > ts, 1);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 2);
    SIMPLETEST_EQUAL(checksum != result->checksum, 1);
    checksum = result->checksum;
    
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 1.0)    (6: 0.05, 0.05)
    // ------------------------------------------------------------------------------------
    imapsearchfrompos(map, &pos, result, 0.5);
    int64_t nnts = result->tick;
    SIMPLETEST_EQUAL(result->tick, node->tick);
    SIMPLETEST_EQUAL(nnts == nts, 1);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 4);
    SIMPLETEST_EQUAL(checksum != result->checksum, 1);
    
    
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
    SIMPLETEST_EQUAL(result->tick, node->tick);
    SIMPLETEST_EQUAL(nnnts == nnts, 0);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 1);
    SIMPLETEST_EQUAL(checksum != result->checksum, 1);
    
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
    SIMPLETEST_EQUAL(result->tick, node->tick);
    SIMPLETEST_EQUAL(nnnnts == nnnts, 0);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 2);
    SIMPLETEST_EQUAL(checksum != result->checksum, 1);
    
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
    SIMPLETEST_EQUAL(result->tick, node->tick);
    SIMPLETEST_EQUAL(nnnnnts == nnnnts, 0);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 1);
    SIMPLETEST_EQUAL(checksum != result->checksum, 1);
    
    isearchresultfree(result);
    
    // remove all unit
    for (int i=0; i<=20; ++i) {
        imapremoveunit(map, __getunitfor(i));
    }
}

SIMPLETEST_CASE(searching, imapsearchfromunit) {
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
    
    SIMPLETEST_EQUAL(result->tick, 0);
    SIMPLETEST_EQUAL(result->checksum, 0);
    
    // ------------------------------------------------------------------------------------
    // (0: 0.1, 0.0)    (1: 0.3, 0.4)   (2: 0.5, 0.0)   (3: 1.5, 1.2)   (4: 1.5, 0.2)
    // (5: 1.0, 1.0)
    // ------------------------------------------------------------------------------------
    icode code = {.code={'A','A','A',0}};
    imapsearchfromunit(map, __getunitfor(0), result, 0.45);
    int64_t ts = result->tick;
    int64_t checksum = result->checksum;
    inode *node = imapgetnode(map, &code, 3, EnumFindBehaviorAccurate);
    SIMPLETEST_EQUAL(ts, node->tick);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 2);
    SIMPLETEST_EQUAL(checksum != 0, 1);
    
    imapsearchfromunit(map, __getunitfor(0), result, 0.45);
    ts = result->tick;
    SIMPLETEST_EQUAL(ts, node->tick);
    SIMPLETEST_EQUAL(ireflistlen(result->units), 2);
    SIMPLETEST_EQUAL(checksum, result->checksum);
    
    isearchresultfree(result);
    
    // remove all unit
    for (int i=0; i<=20; ++i) {
        imapremoveunit(map, __getunitfor(i));
    }
}

SIMPLETEST_CASE(searching, end) {
    for (int i=0; i<__preparecnt; ++i) {
        __unhold(i);
    }
    
    SIMPLETEST_TRUE();
    
    imapfree(map);
    
    // out put all the memory information after unit test
    iaoimemorystate();
}

#endif
