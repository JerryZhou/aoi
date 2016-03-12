/*!
  Area Of Interest In Game Developing
  Copyright Jerryzhou@outlook.com
Licence: Apache 2.0

Project: https://github.com/JerryZhou/aoi.git
Purpose: Resolve the AOI problem in game developing
with high run fps
with minimal memory cost,

Please see examples for more details.
*/

#include "aoi.h"
#include <math.h>

#ifdef _WIN32
#ifdef __cplusplus
extern "C" {
#endif
static int gettimeofday(struct timeval *tp, void *tzp)
{
        time_t clock;
        struct tm tm;
        SYSTEMTIME wtm;

        GetLocalTime(&wtm);
        tm.tm_year = wtm.wYear - 1900;
        tm.tm_mon = wtm.wMonth - 1;
        tm.tm_mday = wtm.wDay;
        tm.tm_hour = wtm.wHour;
        tm.tm_min = wtm.wMinute;
        tm.tm_sec = wtm.wSecond;
        tm.tm_isdst = -1;
        clock = mktime(&tm);
        tp->tv_sec = clock;
        tp->tv_usec = wtm.wMilliseconds * 1000;

        return 0;
}
#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*WIN32*/

/* 日志开关 */
#define open_log_code		(0) /* 关于编码生成的日志 */
#define open_log_gencode	(0)
#define open_log_node		(0) /* 关于节点生成的日志 */
#define open_log_node_get	(0) /* 关于节点查找的日志 */
#define open_log_unit		(0)
#define open_log_unit_update (0)
#define open_log_filter		(0)
#define open_log_unit_add	(0)
#define open_log_unit_remove (0)
#define open_log_profile	(0)
#define open_log_map_construct  (0)

/* 常用的宏 */
#define imax(a, b) ((a) > (b) ? (a) : (b))
#define imin(a, b) ((a) < (b) ? (a) : (b))
#define iunused(v) (void)(v)
#define ilog(...) printf(__VA_ARGS__)

/* 状态操作宏 */
#define _state_add(value, state) value |= state
#define _state_remove(value, state) value &= ~state
#define _state_is(value, state) (value & state)

#if iimeta
/* 内存统计 */
int64_t gcallocsize = 0;
int64_t gfreesize = 0;
int64_t gholdsize = 0;

#undef __ideclaremeta
/* #define __ideclaremeta(type, cap) {.name=#type, .size=sizeof(type), .current=0, .alloced=0, .freed=0, .cache={.root=NULL, .length=0, .capacity=cap}}*/
#define __ideclaremeta(type, cap) {#type, {NULL, 0, cap}, sizeof(type), 0, 0, 0}
/* 所有类型的元信息系统 */
imeta gmetas[] = {__iallmeta,
	__ideclaremeta(imeta, 0)
};

/* 所有自定义的类型原系统 */
imeta gmetasuser[IMaxMetaCountForUser] = {{0}};

#undef __ideclaremeta
#define __ideclaremeta(type, cap) EnumMetaTypeIndex_##type

#ifndef __countof
#define __countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef __max
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* 内置的meta个数 */
static int const gmetacount = __countof(gmetas);
static int gmetacountuser = 0;

/* 获取类型的元信息 */
imeta *imetaget(int idx) {
	if (idx < 0) {
		return NULL;
	}
	if (idx < gmetacount) {
		return &gmetas[idx];
	}
	idx -= gmetacount;
	if (idx <  gmetacountuser ) {
		return &gmetasuser[idx];
	}
	return NULL;
}

/* 也可以手动注册一个元信息来管理自己的对象: 然后就可以通过 iobjmallocuser 进行对象内存管理 */
int imetaregister(const char* name, int size, int capacity) {
	gmetasuser[gmetacountuser].name = name;
	gmetasuser[gmetacountuser].size = size;
	gmetasuser[gmetacountuser].cache.capacity = capacity;
	return gmetacount + gmetacountuser++;
}

/**
 * 尝试从缓冲区拿对象
 */
static iobj *_imetapoll(imeta *meta) {
	iobj *obj = NULL;
	if (meta->cache.length) {
		obj = meta->cache.root;
		meta->cache.root = meta->cache.root->next;
		obj->next = NULL;
		--meta->cache.length;
		memset(obj->addr, 0, obj->meta->size);
	}else {
		int newsize = sizeof(iobj) + meta->size;
		obj = (iobj*)icalloc(1, newsize);
		obj->meta = meta;
		obj->size = newsize;
		obj->meta->alloced += newsize;
		obj->meta->current += newsize;
		gcallocsize += newsize;
		gholdsize += newsize;
	}

	return obj;
}

/* 释放对象 */
void imetaobjfree(iobj *obj) {
	obj->meta->current -= obj->size;
	obj->meta->freed += obj->size;
	gfreesize += obj->size;
	gholdsize -= obj->size;
	ifree(obj);
}

/* Meta 的缓冲区管理 */
void imetapush(iobj *obj) {
	if (obj->meta->cache.length < obj->meta->cache.capacity) {
		obj->next = obj->meta->cache.root;
		obj->meta->cache.root = obj;
		++obj->meta->cache.length;
	} else {
		imetaobjfree(obj);
	}
}

/* 获取响应的内存：会经过Meta的Cache */
void *iaoicalloc(imeta *meta) {
	iobj *obj = _imetapoll(meta);
	return obj->addr;
}

/* 偏移一下获得正确的内存对象 */
#define __iobj(p) (iobj*)((char*)(p) - sizeof(iobj))

/* 释放内存：会经过Meta的Cache */
void iaoifree(void *p) {
	iobj *newp = __iobj(p);
	imetapush(newp);
}

/* 尽可能的释放Meta相关的Cache */
void iaoicacheclear(imeta *meta) {
	iobj *next = NULL;
	iobj *cur = NULL;
	icheck(meta->cache.length);
	cur = meta->cache.root;
	while (cur) {
		next = cur->next;
		imetaobjfree(cur);
		cur = next;
	}
	meta->cache.root = NULL;
	meta->cache.length = 0;
}

/* 打印当前内存状态 */
void iaoimemorystate() {
	int count = __countof(gmetas);
	int i;
	ilog("[AOI-Memory] *************************************************************** Begin\n");
	ilog("[AOI-Memory] Total---> new: %lld, free: %lld, hold: %lld \n", gcallocsize, gfreesize, gholdsize);

	for (i=0; i<count; ++i) {
		ilog("[AOI-Memory] Obj: (%s, %d) ---> alloc: %lld, free: %lld, hold: %lld - count: %lld\n",
				gmetas[i].name,	gmetas[i].size,
				gmetas[i].alloced, gmetas[i].freed, gmetas[i].current,
				gmetas[i].current/(gmetas[i].size+sizeof(iobj)));
		if (gmetas[i].cache.capacity) {
			ilog("[AOI-Memory] Obj: (%s, %d) ---> cache: (%d/%d) \n",
					gmetas[i].name,	gmetas[i].size,
					gmetas[i].cache.length, gmetas[i].cache.capacity);
		}
	}
	ilog("[AOI-Memory] *************************************************************** End\n");
}

/* 获取指定对象的meta信息 */
imeta *iaoigetmeta(const void *p) {
	iobj *obj = NULL;
	icheckret(p, NULL);
	obj = __iobj(p);
	return obj->meta;
}

/* 指定对象是响应的类型 */
int iaoiistype(const void *p, const char* type) {
	imeta *meta = NULL;
	icheckret(type, iino);
	meta = iaoigetmeta(p);
	icheckret(meta, iino);

	if (strncmp(type, meta->name, __max(strlen(meta->name), strlen(type))) == 0) {
		return iiok;
	}
	return iino;
}

#else

void iaoimemorystate() {
	ilog("[AOI-Memory] Not Implement IMeta \n");
}

#endif

/* 获取当前系统的微秒数 */
int64_t igetcurmicro() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000*1000 + tv.tv_usec;
}

/* 获取当前系统的毫秒数 */
int64_t igetcurtick() {
	return igetcurmicro()/1000;
}

/* 获取系统的下一个唯一的事件纳秒数 */
int64_t igetnextmicro(){
	static int64_t gseq = 0;
	int64_t curseq = igetcurmicro();
	if (curseq > gseq) {
		gseq = curseq;
	}else {
		++gseq;
	}
	return gseq;
}

/* 计算距离的平方 */
ireal idistancepow2(const ipos *p, const ipos *t) {
	ireal dx = p->x - t->x;
	ireal dy = p->y - t->y;
	return dx*dx + dy*dy;
}

/* 两点相减得到向量 */
ivec2 ivec2subtractpoint(const ipos *p0, const ipos *p1) {
	ivec2 vec;
	vec.u.v.x = p1->x - p0->x;
	vec.u.v.y = p1->y - p0->y;
	return vec;
}

/* 点积 */
ireal ivec2dot(const ivec2 *l, const ivec2 *r) {
	icheckret(l, 0);
	icheckret(r, 0);
	return l->u.v.x * r->u.v.x + l->u.v.y + r->u.v.y;
}

/* 减法 */
ivec2 ivec2subtract(const ivec2 *l, const ivec2 *r) {
	ivec2 vec;
	vec.u.v.x = r->u.v.x - l->u.v.x;
	vec.u.v.y = r->u.v.y - l->u.v.y;
	return vec;
}

/* 加法*/
ivec2 ivec2add(const ivec2 *l, const ivec2 *r) {
	ivec2 vec;
	vec.u.v.x = r->u.v.x + l->u.v.x;
	vec.u.v.y = r->u.v.y + l->u.v.y;
	return vec;
}

/* 乘法 */
ivec2 ivec2multipy(const ivec2 *l, const ireal a) {
	ivec2 vec;
	vec.u.v.x = l->u.v.x * a;
	vec.u.v.y = l->u.v.y * a;
	return vec;
}

/* 绝对值 */
ivec2 ivec2abs(const ivec2* l) {
	ivec2 vec;
	vec.u.v.x = fabs(l->u.v.x);
	vec.u.v.y = fabs(l->u.v.y);
	return vec;
}

/* 归一*/
ivec2 ivec2normalize(const ivec2 *l) {
	ireal len = ivec2length(l);
	return len > 0 ? ivec2multipy(l, 1.0/len) : *l;
}

/* 长度的平方 */
ireal ivec2lengthsqr(const ivec2 *l) {
	return ivec2dot(l, l);
}

/* 长度 */
ireal ivec2length(const ivec2 *l) {
	return sqrtf(ivec2dot(l, l));
}

/* 平行分量, 确保 r 已经归一化 */
ivec2 ivec2parallel(const ivec2 *l, const ivec2 *r) {
	ireal projection = ivec2dot (l, r);
	return ivec2multipy(r, projection);
}

/* 垂直分量, 确保 r 已经归一化 */
ivec2 ivec2perpendicular(const ivec2 *l, const ivec2 *r) {
	ivec2 p = ivec2parallel(l, r);
	return ivec2subtract(l, &p);
}

/* 加法*/
ivec3 ivec3add(const ivec3 *l, const ivec3 *r) {
	ivec3 vec;
	vec.u.v.x = r->u.v.x + l->u.v.x;
	vec.u.v.y = r->u.v.y + l->u.v.y;
	vec.u.v.z = r->u.v.z + l->u.v.z;
	return vec;
}

/* 减法 */
ivec3 ivec3subtract(const ivec3 *l, const ivec3 *r) {
	ivec3 vec;
	vec.u.v.x = r->u.v.x - l->u.v.x;
	vec.u.v.y = r->u.v.y - l->u.v.y;
	vec.u.v.z = r->u.v.z - l->u.v.z;
	return vec;
}

/* 乘法 */
ivec3 ivec3multipy(const ivec3 *l, ireal a) {
	ivec3 vec;
	vec.u.v.x = l->u.v.x * a;
	vec.u.v.y = l->u.v.y * a;
	vec.u.v.z = l->u.v.z * a;
	return vec;
}

/* 点积
 * https://en.wikipedia.org/wiki/Dot_product
 * */
ireal ivec3dot(const ivec3 *l, const ivec3 *r) {
	return l->u.v.x * r->u.v.x
		+ l->u.v.y * r->u.v.y
		+ l->u.v.z * r->u.v.z;
}

/* 乘积
 * https://en.wikipedia.org/wiki/Cross_product
 * */
ivec3 ivec3cross(const ivec3 *l, const ivec3 *r) {
	ivec3 vec;
	vec.u.v.x = l->u.v.y * r->u.v.z - l->u.v.z * r->u.v.y;
	vec.u.v.y = l->u.v.z * r->u.v.x - l->u.v.x * r->u.v.z;
	vec.u.v.z = l->u.v.x * r->u.v.y - l->u.v.y * r->u.v.x;
	return vec;
}

/* 长度的平方 */
ireal ivec3lengthsqr(const ivec3 *l) {
	return ivec3dot(l, l);
}

/* 长度 */
ireal ivec3length(const ivec3 *l) {
	return sqrtf(ivec3dot(l, l));
}

/* 绝对值 */
ivec3 ivec3abs(const ivec3* l) {
	ivec3 vec;
	vec.u.v.x = fabs(l->u.v.x);
	vec.u.v.y = fabs(l->u.v.y);
	vec.u.v.z = fabs(l->u.v.z);
	return vec;
}

/* 归一*/
ivec3 ivec3normalize(const ivec3 *l) {
	ireal len = ivec3length(l);
	return len > 0 ? ivec3multipy(l, 1.0/len) : *l;
}

/* 平行分量, 确保 r 已经归一化 */
ivec3 ivec3parallel(const ivec3 *l, const ivec3 *r) {
	ireal projection = ivec3dot (l, r);
	return ivec3multipy(r, projection);
}

/* 垂直分量, 确保 r 已经归一化 */
ivec3 ivec3perpendicular(const ivec3 *l, const ivec3 *r) {
	ivec3 p = ivec3parallel(l, r);
	return ivec3subtract(l, &p);
}

/* 判断矩形包含关系 */
int irectcontains(const irect *con, const irect *r) {
	icheckret(con, iino);
	icheckret(r, iiok);

	if (con->pos.x <= r->pos.x && con->pos.y <= r->pos.y
			&& con->pos.x + con->size.w >= r->pos.x + r->size.w
			&& con->pos.y + con->size.h >= r->pos.y + r->size.h) {
		return iiok;
	}

	return iino;
}

/* 判断矩形与点的包含关系 */
int irectcontainspoint(const irect *con, const ipos *p) {
	icheckret(con, iino);
	icheckret(p, iiok);

	if (con->pos.x <= p->x && con->pos.y <= p->y
			&& con->pos.x + con->size.w >= p->x
			&& con->pos.y + con->size.h >= p->y) {
		return iiok;
	}

	return iino;
}

/* 矩形与圆是否相交 */
int irectintersect(const irect *con, const icircle *c) {
	icheckret(con, iino);
	icheckret(c, iiok);

	/* https://www.zhihu.com/question/24251545  */
	/*
	 * bool BoxCircleIntersect(Vector2 c, Vector2 h, Vector2 p, float r) {
	 * Vector2 v = abs(p - c);    // 第1步：转换至第1象限
	 * Vector2 u = max(v - h, 0); // 第2步：求圆心至矩形的最短距离矢量
	 * return dot(u, u) <= r * r; // 第3步：长度平方与半径平方比较
	 * }

	 * 作者：Milo Yip
	 * 链接：https://www.zhihu.com/question/24251545/answer/27184960
	 */

	/*
	 * ivec2 c = {con->pos.x, con->pos.y};
	 * ivec2 p = {c->pos.x, c->pos.y};
	 */
	do {
		ivec2 v = {{{fabs(c->pos.x - con->pos.x), fabs(c->pos.y - con->pos.y)}}};
		ivec2 h = {{{con->size.w, con->size.h}}};
		ivec2 u =  {{{v.u.v.x - h.u.v.x, v.u.v.y - h.u.v.y}}};
		u.u.v.x = u.u.v.x < 0 ? 0 : u.u.v.x;
		u.u.v.y = u.u.v.y < 0 ? 0 : u.u.v.y;
		return u.u.v.x * u.u.v.x + u.u.v.y * u.u.v.y < c->radius * c->radius;
	} while(0);
	return 0;
}

/* 圆形相交: iiok, iino */
int icircleintersect(const icircle *con, const icircle *c) {
	ireal ds = 0.0;
	icheckret(con, iino);
	icheckret(c, iiok);

	ds = (con->radius + c->radius);

	if (idistancepow2(&con->pos, &c->pos) <= ds * ds) {
		return iiok;
	}

	return iino;
}

/* 圆形包含: iiok, iino */
int icirclecontains(const icircle *con, const icircle *c) {
	ireal ds = 0.0;
	icheckret(con, iino);
	icheckret(c, iiok);
	ds = (con->radius - c->radius);
	icheckret(ds >= 0, iino);

	if (idistancepow2(&con->pos, &c->pos) <= ds * ds) {
		return iiok;
	}

	return iino;
}

/* 圆形包含: iiok, iino */
int icirclecontainspoint(const icircle *con, const ipos *p) {
	ireal ds = 0.0;
	icheckret(con, iino);
	icheckret(p, iiok);
	ds = (con->radius);
	icheckret(ds >= 0, iino);

	if (idistancepow2(&con->pos, p) <= ds * ds) {
		return iiok;
	}

	return iino;
}

/* 圆形的关系: EnumCircleRelationBContainsA(c包含con), */
/*	  EnumCircleRelationAContainsB(con包含c), */
/*	  EnumCircleRelationIntersect(相交), */
/*	  EnumCircleRelationNoIntersect(相离) */
int icirclerelation(const icircle *con, const icircle *c) {
	ireal minusds = 0.0;
	ireal addds = 0.0;
	ireal ds = 0.0;
	icheckret(con, EnumCircleRelationBContainsA);
	icheckret(c, EnumCircleRelationAContainsB);

	minusds = con->radius - c->radius;
	addds = con->radius + c->radius;
	ds = idistancepow2(&con->pos, &c->pos);

	if (ds <= minusds * minusds) {
		if (minusds >= 0) {
			return EnumCircleRelationAContainsB;
		}else {
			return EnumCircleRelationBContainsA;
		}
	} else if (ds <= addds *addds) {
		return EnumCircleRelationIntersect;
	} else {
		return EnumCircleRelationNoIntersect;
	}
}

/* 一堆时间宏，常用来做性能日志 */
#define __ProfileThreashold 10

#define __ShouldLog(t) (t > __ProfileThreashold)

#define __Millis igetcurtick()

#define __Micros  igetcurmicro()

#define __Since(t) (__Micros - t)

#define iplogwhen(t, when, ...) do { if(open_log_profile && t > when) {printf("[PROFILE] Take %lld micros ", t); printf(__VA_ARGS__); } } while (0)

#define iplog(t, ...) iplogwhen(t, __ProfileThreashold, __VA_ARGS__)

/* iref 转换成 target */
#define icast(type, v) ((type*)(v))
/* 转换成iref */
#define irefcast(v) icast(iref, v)

/* 增加引用计数 */
int irefretain(iref *ref) {
	return ++ref->ref;
}

/* 释放引用计数 */
void irefrelease(iref *ref) {
	--ref->ref;
	/* 通知 watch */
	if (ref->watch) {
		ref->watch(ref);
	}
	/* 没有引用了，析构对象 */
	if (ref->ref == 0) {
		if (ref->free) {
			ref->free(ref);
		}else {
			iobjfree(ref);
		}
	}
}

/* 申请自动释放池子 */
irefautoreleasepool * irefautoreleasebegin() {
	return iobjmalloc(irefautoreleasepool);
}

/* 自动释放 */
iref* irefautorelease(irefautoreleasepool *pool, iref *ref) {
	icheckret(pool, NULL);
	if(!pool->list) {
		pool->list = ireflistmake();
	}

	ireflistadd(pool->list, ref);
	return ref;
}

/* 结束自动释放 */
void irefautoreleaseend(irefautoreleasepool *pool) {
	irefjoint *joint = NULL;
	icheck(pool);
	icheck(pool->list);

	joint = ireflistfirst(pool->list);
	while(joint) {
		irelease(joint->value);
		joint = joint->next;
	}
	ireflistfree(pool->list);
	iobjfree(pool);
}

/* 返回值本身的引用retain */
iref *irefassistretain(iref *ref) {
	iretain(ref);
	return ref;
}


/* 列表操作 */
#define list_add_front(root, node)                \
	do {                                      \
	        if (root == NULL) {               \
	                root = node;              \
	        }else {                           \
	                node->next = root;        \
	                root->pre = node;         \
	                root = node;              \
	        }                                 \
	}while(0)

#define list_remove(root, node)                              \
	do {                                                 \
	        if (root == node) {                          \
	                root = node->next;                   \
	        }                                            \
	        if (node->next) {                            \
	                node->next->pre = node->pre;         \
	        }                                            \
	                                                     \
	        if (node->pre) {                             \
	                node->pre->next = node->next;        \
	        }                                            \
	                                                     \
	        node->pre = NULL;                            \
	        node->next = NULL;                           \
	}while(0)

/* 构造列表节点 */
irefjoint* irefjointmake(iref *value) {
	irefjoint *joint = iobjmalloc(irefjoint);
	iassign(joint->value, value);
	return joint;
}

/* 释放列表节点 */
void irefjointfree(irefjoint* joint) {
	icheck(joint);
	irelease(joint->value);
	iobjfree(joint);
}

/* 创建列表 */
ireflist *ireflistmake() {
	return	iobjmalloc(ireflist);
}

/* 获取列表长度 */
int ireflistlen(const ireflist *list) {
	icheckret(list, 0);
	return list->length;
}

/* 获取第一个节点 */
irefjoint* ireflistfirst(const ireflist *list) {
	icheckret(list, NULL);
	return list->root;
}

/* 从列表里面查找第一个满足要求的节点 */
irefjoint* ireflistfind(const ireflist *list, const iref *value) {
	irefjoint* joint = ireflistfirst(list);
	while(joint) {
		if (joint->value == value) {
			break;
		}
		joint = joint->next;
	}
	return joint;
}

/* 往列表增加节点: 前置节点 */
irefjoint* ireflistaddjoint(ireflist *list, irefjoint * joint) {
	joint->list = list;
	list_add_front(list->root, joint);
	++list->length;
	return joint;
}

/* 往列表增加节点: 前置节点(会增加引用计数) */
irefjoint* ireflistadd(ireflist *list, iref *value) {
	irefjoint *joint = NULL;
	icheckret(list, NULL);
	joint = irefjointmake(value);
	return ireflistaddjoint(list, joint);
}

/* 从节点里面移除节点 , 并且释放当前节点, 并且返回下一个节点 */
static irefjoint* _ireflistremovejointwithfree(ireflist *list, irefjoint *joint, int withfree) {
	irefjoint *next = NULL;
	icheckret(list, next);
	icheckret(joint, next);
	icheckret(joint->list == list, next);
	joint->list = NULL;

	next = joint->next;

	list_remove(list->root, joint);
	--list->length;

	if (withfree) {
		irefjointfree(joint);
	}

	return next;
}

/* 从节点里面移除节点 , 并且释放当前节点, 并且返回下一个节点 */
irefjoint* ireflistremovejointandfree(ireflist *list, irefjoint *joint) {
	return _ireflistremovejointwithfree(list, joint, 1);
}

/* 从节点里面移除节点 */
irefjoint * ireflistremovejoint(ireflist *list, irefjoint *joint) {
	return _ireflistremovejointwithfree(list, joint, 0);
}

/* 从节点里面移除节点: 并且会释放节点 */
irefjoint* ireflistremove(ireflist *list, iref *value) {
	irefjoint *joint = NULL;
	icheckret(list, NULL);
	joint = ireflistfind(list, value);
	return _ireflistremovejointwithfree(list, joint, 1);
}

/* 释放所有节点 */
void ireflistremoveall(ireflist *list) {
	irefjoint *joint = NULL;
	irefjoint *next = NULL;
	icheck(list);
	joint = ireflistfirst(list);
	while(joint) {
		next = joint->next;
		irefjointfree(joint);
		joint = next;
	}
	list->root = NULL;
	list->length = 0;
}

/* 释放列表 */
void ireflistfree(ireflist *list) {
	icheck(list);

	/* 释放列表节点 */
	ireflistremoveall(list);

	/* 释放list 本身 */
	iobjfree(list);
}

/* 释放数组相关的资源 */
static void _iarray_entry_free(struct iref* ref) {
    iarray *array = (iarray *)ref;

    /* 释放资源的时候把shrink关掉 */
    iarrayunsetflag(array, EnumArrayFlagAutoShirk);
    /* 释放资源的时候把slice 标志位清理掉 */
    iarrayunsetflag(array, EnumArrayFlagSliced);
    
    /* 释放 */
    iarraytruncate(array, 0);
    /* 释放内存 */
    ifree(array->buffer);
    /*
    array->buffer = NULL;
    array->len = 0;
    array->capacity = 0;
    array->entry = NULL;
    array->free = NULL;
    array->flag = 0;
    */

    iobjfree(ref);
}

/* 建立数组*/
iarray *iarraymake(size_t capacity, const iarrayentry *entry) {
	iarray *array = (iarray *)iobjmalloc(iarray);
    array->capacity = capacity;
    array->len = 0;
    array->buffer = (char*)icalloc(capacity, entry->size);
    array->free = _iarray_entry_free;
    array->entry = entry;
    array->flag = entry->flag;
    iretain(array);

    return array;
}

/* 释放 */
void iarrayfree(iarray *arr) {
    irelease(arr);
}

/* 长度 */
size_t iarraylen(const iarray *arr) {
    icheckret(arr, 0);
    return arr->len;
}

/* 容量*/
size_t iarraycapacity(const iarray *arr) {
    icheckret(arr, 0);
    return arr->capacity;
}

/* 查询 */
#define __arr_i(arr, i) ((void*)((arr)->buffer + (i) * (arr)->entry->size))
const void* iarrayat(iarray *arr, int index) {
    icheckret(arr, NULL);
    icheckret(index>=0 && index<arr->len, NULL);

    return  __arr_i(arr, index);
}

/* 数组内存缓冲区 */
void* iarraybuffer(iarray *arr) {
    return arr->buffer;
}

/* 设置标签 */
int iarraysetflag(iarray *arr, int flag) {
    int old = arr->flag; 
    arr->flag |= flag;
    return old;
}

/* 清理标签 */
int iarrayunsetflag(iarray *arr, int flag){
    int old = arr->flag; 
    arr->flag &= (~flag);
    return old;
}

/* 是否具备标签 */
int iarrayisflag(iarray *arr, int flag) {
    return arr->flag & flag;
}

/* 删除 */
int iarrayremove(iarray *arr, int index) {
    int i;

    icheckret(arr, iino);
    icheckret(index>=0 && index<arr->len, iino);
    /* sliced array can not be removed */
    icheckret(!iarrayisflag(arr, EnumArrayFlagSliced), iino);
    
    if (!(arr->entry->flag & EnumArrayFlagSimple)) {
        arr->entry->swap(arr, index, arr_invalid);
    }

    if (arr->flag & EnumArrayFlagKeepOrder) {
        /* 移除一项就慢慢的移 */
        for(i=index; i<arr->len-1; ++i) {
            arr->entry->swap(arr, i, i+1);
        }
    } else if (arr->len-1 != index){
        /* 直接交换最后一项 */
        arr->entry->swap(arr, index, arr->len-1);
    }
    --arr->len;
    return iiok;
}

/* NB!! 外部确保变小的容量对象已经被释放了 */
static size_t _iarray_just_capacity(iarray *arr, size_t newcapacity) {
    char* newbuffer;
    newbuffer = irealloc(arr->buffer, newcapacity * arr->entry->size);
    icheckret(newbuffer, arr->capacity);
    /* 清理新加的内存 */
    if (arr->flag & EnumArrayFlagMemsetZero && newcapacity > arr->capacity) {
        memset(newbuffer + arr->capacity * arr->entry->size,
           0,
           (newcapacity-arr->capacity) * arr->entry->size);
    }

    arr->buffer = newbuffer;
    arr->capacity = newcapacity;
    return arr->capacity;
}

/* 确保 arr->capacity >= capacity , 返回调整后的容量*/
static size_t _iarray_be_capacity(iarray *arr, size_t capacity) {
    size_t newcapacity;

    icheckret(arr->capacity < capacity, arr->capacity);

    /* 新的容量 */
    newcapacity = arr->capacity;
    do {
        newcapacity = newcapacity * 2;
    } while(newcapacity < capacity);

    return _iarray_just_capacity(arr, newcapacity);
}

/* 自动缩小容量 */
static void _iarrayautoshrink(iarray *arr) {
    size_t suppose = imax(arr->len * 2, 8);
    if (arr->capacity > suppose) {
        iarrayshrinkcapacity(arr, suppose);
    }
}

/* 增加 */
int iarrayadd(iarray *arr, const void* value) {
    return iarrayinsert(arr, arr->len, value, 1);
}

/* 插入 */
int iarrayinsert(iarray *arr, int index, const void *value, int nums) {
    int i;
    
    /* check if we need do insert */
    icheckret(nums > 0, iiok);
    /* check if the index belong to [0, arr->len] */
    icheckret(index>=0 && index<=arr->len, iino);
    /* the sliced array can not extend capacity */
    icheckret(!iarrayisflag(arr, EnumArrayFlagSliced)
              || (arr->len + nums) <= arr->capacity, iino);
    /* be sure the capacity is enough */
    _iarray_be_capacity(arr, arr->len + nums);
    /* check if we have been got enough space to do inserting*/
    icheckret(arr->capacity >= arr->len + nums, iino);
    
    /*swap after*/
    if (index != arr->len) {
        /* if the array is simple one, we can just do memove */
        /* simple flag is only for inner use */
        if (arr->entry->flag & EnumArrayFlagSimple) {
            arr->entry->assign(arr, index + nums,
                               __arr_i(arr, index),
                               arr->len - index);
        } else {
            /* swap one by one */
            i = arr->len - 1;
            while (i >= index) {
                arr->entry->swap(arr, i, i+nums);
                --i;
            }    
        }
    }
    /* assign it */
    arr->entry->assign(arr, index, value, nums);
    arr->len += nums;
    return iiok;
}

/* 设置 */
int iarrayset(iarray *arr, int index, const void *value) {
    icheckret(index >=0 && index<arr->len, iino);
    arr->entry->assign(arr, index, value, 1);
    return iiok;
}

/* 清理数组 */
void iarrayremoveall(iarray *arr) {
    iarraytruncate(arr, 0);
}

/* 截断数组 */
void iarraytruncate(iarray *arr, size_t len) {
    int i;

    icheck(arr);
    icheck(arr->len > len);
    /* sliced array can not be truncate */
    icheck(!iarrayisflag(arr, EnumArrayFlagSliced));
    
    if (arr->entry->flag & EnumArrayFlagSimple) {
        /* direct set the length*/
        arr->len = len;
        /* auto shirk */
        if (arr->flag & EnumArrayFlagAutoShirk) {
            _iarrayautoshrink(arr);
        }
    } else {
        /* remove one by one*/
        for(i=arr->len; i>len; i--) {
            iarrayremove(arr, i-1);
        }
    }
}

/* 缩减容量  */
size_t iarrayshrinkcapacity(iarray *arr, size_t capacity) {
    icheckret(arr->capacity > capacity, arr->capacity);
    
    /* sliced array can not be shrink */
    icheckret(!iarrayisflag(arr, EnumArrayFlagSliced), arr->capacity);

    capacity = imax(arr->len, capacity);
    return _iarray_just_capacity(arr, capacity);
}

/* 堆排序 - 堆调整 */
static void _iarray_heap_shift(iarray *arr,
                      int ind, int end) {

    int i = ind;
    int c = 2 * i + 1;

    while(c <= end) {
        if (c +1 <=end && arr->entry->cmp(arr, c, c+1) < 0 ) {
            c++;
        }
        if (arr->entry->cmp(arr, i, c) > 0) {
            break;
        } else {
            arr->entry->swap(arr, i, c);

            i = c;
            c = 2*i + 1;
        }
    }
}

/* 堆排序 */
static void _iarray_sort_heap(iarray *arr,
                int start, int end) {
    int i, j;
    for (i=(end-1)/2; i>=start; i--) {
        _iarray_heap_shift(arr, i, end);
    }

    for (j=start; j<=end; ++j) {
        arr->entry->swap(arr, start, end-start-j);
        _iarray_heap_shift(arr, start, end - start - j - 1);
    }
}

/* 排序 */
void iarraysort(iarray *arr) {
    icheck(arr->len);

    _iarray_sort_heap(arr, 0, arr->len-1);
}

/*************************************************************/
/* iarray - copy                                             */
/*************************************************************/

/* 赋值 */
static void _iarray_entry_assign_copy(struct iarray *arr,
                            int i, const void *value, int nums) {
    memmove(arr->buffer + i * arr->entry->size,
           value,
           nums * arr->entry->size);
}

/* 交换两个对象 */
static void _iarray_entry_swap_copy(struct iarray *arr,
                          int i, int j) {
    /* 空对象 */
    char buffer[256];
    char *tmp;
    if (arr->entry->size > 256) {
        tmp = icalloc(1, arr->entry->size);
    } else {
        tmp = buffer;
    }
    
    if (j == arr_invalid) {
        /* arr_int[i] = 0;
        may call assign */
        _iarray_entry_assign_copy(arr, i, tmp, 1);
    } else if (i == arr_invalid) {
        /* arr_int[j] = 0;
        may call assign */
        _iarray_entry_assign_copy(arr, j, tmp, 1);
    } else {
        memmove(tmp, __arr_i(arr, i), arr->entry->size);
        memmove(__arr_i(arr, i), __arr_i(arr, j), arr->entry->size);
        memmove(__arr_i(arr, j), tmp, arr->entry->size);
    }
}

/* 比较两个对象 */
static int _iarray_entry_cmp_int(struct iarray *arr,
                         int i, int j) {
    int *arr_int = (int *)arr->buffer;
    return arr_int[i] - arr_int[j];
}

/* 定义int 数组 */
static const iarrayentry _arr_entry_int = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagSimple |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(int),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    _iarray_entry_cmp_int,
};

/* 内置的整数数组 */
iarray* iarraymakeint(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_int);
}

/* 比较两个对象 */
static int _iarray_entry_cmp_ireal(struct iarray *arr,
                                 int i, int j) {
    ireal *arrs = (ireal *)arr->buffer;
    return arrs[i] - arrs[j];
}

/* 定义ireal 数组 */
static const iarrayentry _arr_entry_ireal = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagSimple |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ireal),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    _iarray_entry_cmp_ireal,
};

/* 浮点数组 */
iarray* iarraymakeireal(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_ireal);
}

/* 比较两个对象 */
static int _iarray_entry_cmp_int64(struct iarray *arr,
                                   int i, int j) {
    int64_t *arrs = (int64_t *)arr->buffer;
    return arrs[i] - arrs[j];
}

/* 定义int64 数组 */
static const iarrayentry _arr_entry_int64 = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagSimple |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ireal),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    _iarray_entry_cmp_int64,
};

/* int64 数组*/
iarray* iarraymakeint64(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_int64);
}

/* 比较两个对象 */
static int _iarray_entry_cmp_char(struct iarray *arr,
                                   int i, int j) {
    char *arrs = (char *)arr->buffer;
    return arrs[i] - arrs[j];
}

/* 定义char 数组 */
static const iarrayentry _arr_entry_char = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagSimple |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ireal),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    _iarray_entry_cmp_char,
};

/* char 数组*/
iarray* iarraymakechar(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_char);
}

/*************************************************************/
/* iarray - iref                                             */
/*************************************************************/

/* 赋值 */
static void _iarray_entry_assign_iref(struct iarray *arr,
                            int i, const void *value, int nums) {
    iref* *arrs = (iref* *)arr->buffer;
    iref* *refvalue = (iref* *)value;
    iref* ref = NULL;
    int j = 0;
    
    /* 附加很多个 */
    while (j < nums) {
        /* realloc not set zero to pending memory */
        if (i >= arr->len) {
            arrs[i] = NULL;
        }
        if (refvalue) {
            ref = refvalue[j];
        }
        
        iassign(arrs[i], ref);
        ++j;
        ++i;
    }
}

/* 交换两个对象 */
static void _iarray_entry_swap_iref(struct iarray *arr,
                          int i, int j) {
    iref* tmp;
    iref* *arrs = (iref* *)arr->buffer;
    if (j == arr_invalid) {
        /* arr_int[i] = 0;
         * may call assign */
        _iarray_entry_assign_iref(arr, i, 0, 1);
    } else if (i == arr_invalid) {
        /* arr_int[j] = 0;
         * may call assign */
        _iarray_entry_assign_iref(arr, j, 0, 1);
    } else {
        tmp = arrs[i];
        arrs[i] = arrs[j];
        arrs[j] = tmp;    
    }
}

/* 比较两个对象 */
static int _iarray_entry_cmp_iref(struct iarray *arr,
                         int i, int j) {
    iref* *arrs = (iref* *)arr->buffer;
    return arrs[i] - arrs[j];
}

/* 定义iref 数组 */
static const iarrayentry _arr_entry_iref = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(iref*),
    _iarray_entry_swap_iref,
    _iarray_entry_assign_iref,
    _iarray_entry_cmp_iref,
};

/* 内置的引用数组 */
iarray* iarraymakeiref(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_iref);
}

/* 定义ipos 数组 */
static const iarrayentry _arr_entry_ipos = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ipos),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};
/* 内置的 ipos 数组*/
iarray* iarraymakeipos(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_ipos);
}

/* 定义isize 数组 */
static const iarrayentry _arr_entry_isize = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(isize),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};
/* 内置的 isize 数组*/
iarray* iarraymakeisize(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_isize);
}

/* 定义irect 数组 */
static const iarrayentry _arr_entry_irect = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(irect),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};
/* 内置的 irect 数组*/
iarray* iarraymakeirect(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_irect);
}

/* 定义icircle 数组 */
static const iarrayentry _arr_entry_icircle = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(icircle),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};   
/* 内置的 icircle 数组*/
iarray* iarraymakeicircle(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_icircle);
}

/* 定义ivec2 数组 */
static const iarrayentry _arr_entry_ivec2 = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ivec2),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};
/* 内置的 ivec2 数组*/
iarray* iarraymakeivec2(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_ivec2);
}

/* 定义ivec3 数组 */
static const iarrayentry _arr_entry_ivec3 = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ivec3),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};
/* 内置的 ivec3 数组*/
iarray* iarraymakeivec3(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_ivec3);
}

/*************************************************************/
/* islice                                                    */
/*************************************************************/

/* free slice and slice-array */
static void _islice_entry_free(iref *ref) {
    islice *slice = (islice*)ref;
    irelease(slice->array);
    iobjfree(slice);
}

/* 左闭右开的区间 [begin, end) */
islice *islicemake(iarray *arr, int begin, int end, int capacity) {
    islice* slice;
    int difflen = imax(end-begin, 0);
    int diffcap = imax(capacity-begin, 0);
    icheckret(arr, NULL);
    
    /*begin = [0, arr->len] */
    begin = imax(0, begin);
    begin = imin(begin, (arr->len));
    /*end = [begin, arr->len] */
    end = begin + difflen;
    end = imin(end, arr->len);
    /*capacity = [end, arr->capacity] */
    capacity = imax(end, begin + diffcap);
    capacity = imin(capacity, arr->capacity);
    
    /* arr marked sliced, 
     * the array will become the const array with capacity 
     * can not do operators: iarrayremove, iarrayshirnkcapacity, iarraytruncate
     * */
    iarraysetflag(arr, EnumArrayFlagSliced);
    
    slice = iobjmalloc(islice);
    slice->free = _islice_entry_free;
    slice->begin = begin;
    slice->end = end;
    slice->capacity = capacity;
    slice->array = arr;
    
    iretain(arr);
    iretain(slice);
    return slice;
}

/* 左闭右开的区间 [begin, end) */
islice *islicemakeby(islice *sli, int begin, int end, int capacity) {
    icheckret(sli, NULL);
    /* real slice */
    return islicemake(sli->array,
                      sli->begin + begin,
                      sli->begin + end,
                      sli->begin + capacity);
}


/* 通过参数创建 "begin:end:capacity"
 * islicemakearg(arr, ":")
 * islicemakearg(arr, ":1")
 * islicemakearg(arr, ":1:5")
 * islicemakearg(arr, "3:1:5")
 * islicemakearg(arr, "3:")
 */
islice *islicemakearg(iarray *arr, const char* args) {
    int params[3] = {0, (int)iarraylen(arr), (int)iarraycapacity(arr)};
    isliceparamsparse(params, args, ':');
    return islicemake(arr, params[0], params[1], params[2]);
}

/* 通过参数创建 "begin:end:capacity"
 * islicemakeargby(arr, ":")
 * islicemakeargby(arr, ":1")
 * islicemakeargby(arr, ":1:5")
 * islicemakeargby(arr, "3:1:5")
 * islicemakeargby(arr, "3:")
 */
islice *islicemakeargby(islice *slice, const char* args) {
    int params[3] = {0, (int)islicelen(slice), (int)islicecapacity(slice)};
    isliceparamsparse(params, args, ':');
    return islicemakeby(slice, params[0], params[1], params[2]);
}

/* 参数 解析 */
void isliceparamsparse(int *params, const char* args, const char delim) {
    int n=0;
    int m=0;
    int i=0;
    int len=0;
    char buffer[256]= {0};
    char *arg;
    if (args == NULL || strlen(args) == 0) {
        return;
    }
    len = strlen(args);
    if (len > 256) {
        arg = (char*)calloc(1, len);
    } else {
        arg = buffer;
    }
    strcpy(arg, args);
    
    for (m=0; m<len; ++m) {
        if (args[m] == delim) {
            /* found
            * [n, m) */
            if (m > n) {
                params[i] = atoi(arg+n);
            }
            n = m+1;
            ++i;
        }
    }
    if (m > n) {
        params[i] = atoi(args+n);
    }
    
    if (arg != buffer) {
        free(arg);
    }
}

/* 释放 */
void islicefree(islice *slice) {
    irelease(slice);
}

/* 长度 */
size_t islicelen(const islice *slice) {
    icheckret(slice, 0);
    icheckret(slice->end > slice->begin, 0);
    return slice->end - slice->begin;
}

/* 容量 */
size_t islicecapacity(const islice *slice) {
    icheckret(slice, 0);
    icheckret(slice->array, 0);
    icheckret(slice->capacity > slice->begin, 0);
    return slice->capacity - slice->begin;
}

/* 附加
 * 用法: slice = isliceappendvalues(slice, values, count); */
islice* isliceappendvalues(islice* slice, const void *values, int count) {
    int appendcount = count;
    int needcapacity = (slice->end+appendcount-1);
    int index = slice->end;
    int indexappend = 0;
    size_t newcapacity = 0;
    islice *newslice;
    iarray * newarray;
    
    
#define __value_i(values, i, size) (const void *)((char*)values + i * size)
    
    /* if we got enough space*/
    if (needcapacity < slice->capacity) {
        /* set */
        while (index < slice->array->len
               && indexappend < appendcount) {
            iarrayset(slice->array, index,
                      __value_i(values, indexappend, slice->array->entry->size));
            ++index;
            ++indexappend;
        }
        /* append */
        if (indexappend < appendcount) {
            iarrayinsert(slice->array, index,
                         __value_i(values, indexappend, slice->array->entry->size),
                         appendcount - indexappend);
        }
        /* update the slice end cursor */
        slice->end += appendcount;
    } else {
        newcapacity = islicecapacity(slice);
        newcapacity = imax(newcapacity, 4);
        needcapacity = islicelen(slice) + appendcount;
        
        /* caculate the new capacity*/
        while (newcapacity < needcapacity) {
            if (newcapacity < 1000) {
                newcapacity = (size_t)(newcapacity * 2);
            } else {
                newcapacity = (size_t)(newcapacity * 6 / 5);
            }
        }
        /*new array*/
        newarray = iarraymake(newcapacity, slice->array->entry);
        /*copy from old slice*/
        iarrayinsert(newarray, 0, __arr_i(slice->array, slice->begin), islicelen(slice));
        /*add new values*/
        iarrayinsert(newarray, newarray->len, values, appendcount);
        /*make new slice*/
        newslice = islicemake(newarray, 0, needcapacity, newcapacity);
        
        /* free the new array ref*/
        iarrayfree(newarray);
        /* free the old slice*/
        islicefree(slice);
        
        /* set slice to return*/
        slice = newslice;
    }
    return slice;
}

/* 附加 */
islice* isliceappend(islice *slice, const islice *append) {
    /* should be the same slice entry */
    icheckret(slice->array->entry == slice->array->entry, slice);
    return isliceappendvalues(slice,
                              __arr_i(append->array, append->begin),
                              islicelen(append));
}

/* 增加元素 */
islice* isliceadd(islice *slice, const void *value) {
    return isliceappendvalues(slice, value, 1);
}

/* 设置值*/
int isliceset(islice *slice, int index, const void *value) {
    icheckret(slice, iino);
    icheckret(slice->array, iino);
    icheckret(index>=0 && index<islicelen(slice), iino);
    
    return iarrayset(slice->array, slice->begin + index, value);
}

/* 查询 */
const void* isliceat(const islice *slice, int index) {
    /* can not read more than slice can reached */
    icheckret(index>=0 && index<islicelen(slice), NULL);
    /* read from array */
    return iarrayat(slice->array, slice->begin+index);
}

/* cache 的 绑定在 ref 上的回调 */
void _ientrywatch_cache(iref *ref) {
	irefcache *cache = NULL;
	int len = 0;
	icheck(ref->ref == 0);

	/* only move ref to live cache */
	cache = ref->cache;

	len = ireflistlen(cache->cache);
	if (len < cache->capacity) {
		ireflistadd(cache->cache, ref);
	}else if (cache->envicted){
		cache->envicted(ref);
	}
}

/* 创造一个cache */
irefcache *irefcachemake(int capacity, icachenewentry newentry) {
	irefcache *cache = iobjmalloc(irefcache);
	cache->cache = ireflistmake();
	cache->capacity = capacity;
	cache->newentry = newentry;
	return cache;
}

/* 从缓存里面取一个 */
iref *irefcachepoll(irefcache *cache) {
	iref *ref = NULL;
	irefjoint* joint = ireflistfirst(cache->cache);
	if (joint) {
		ireflistremovejoint(cache->cache, joint);

		iassign(ref, joint->value);

		irefjointfree(joint);

		if (ref->ref != 1) {
			ilog("[IMAP-RefCache] Poll - %d\n", ref->ref);
		}
		return ref;
	}

	ref = cache->newentry();
	ref->cache = cache;
	ref->watch = _ientrywatch_cache;
	return ref;
}

/* 释放到缓存里面 */
void irefcachepush(irefcache *cache, iref *ref) {
	icheck(ref);
	/* just make sure we should be cacheable */
	if (ref->cache != cache) {
		ref->cache = cache;
		if (ref->cache) {
			ref->watch = _ientrywatch_cache;
		} else {
			ref->watch = NULL;
		}
	}
	/* release the ref */
	irelease(ref);
}

/* 清理缓冲区 */
void irefcacheclear(irefcache *cache) {
	int oldcapacity = 0;
	icheck(cache);
	oldcapacity = cache->capacity;
	cache->capacity = 0;
	ireflistremoveall(cache->cache);
	cache->capacity = oldcapacity;
}

/* 释放缓存 */
void irefcachefree(irefcache *cache) {
	icheck(cache);
	/* 因为Cache会劫持对象的释放，所以释放对象的时候应该停掉Cache的劫持 */
	cache->capacity = 0;
	/* 释放缓存的引用对象 */
	ireflistfree(cache->cache);
	/* 释放缓存自己 */
	iobjfree(cache);
}

/* 当前缓冲区的存储的对象个数 */
int irefcachesize(irefcache *cache) {
	return ireflistlen(cache->cache);
}

/* 用宏处理缓存接口: 拿 */
#define icache(cache, type) ((type*)irefcachepoll(cache))
/* 用宏处理缓存接口: 放回 */
#define icacheput(cache, ref) irefcachepush(cache, (iref*)(ref))

/* 拷贝一个ABCD编码 */
#define copycode(dst, src, count)                         \
	do {                                              \
	        memset(dst.code, 0, IMaxDivide);          \
	        memcpy(dst.code, src.code, count);        \
	} while(0)

/* 构造一个基本单元 */
iunit * imakeunit(iid id, ireal x, ireal y) {
	iunit *unit = iobjmalloc(iunit);
	iretain(unit);

	unit->id = id;
	unit->pos.x = x;
	unit->pos.y = y;
	return unit;
}

/* 构造一个基本单元 */
iunit * imakeunitwithradius(iid id, ireal x, ireal y, ireal radius) {
	iunit *unit = imakeunit(id, x, y);
	unit->radius = radius;
	return unit;
}


/* 释放一个基本单元 */
void ifreeunit(iunit *unit) {
	irelease(unit);
}

/* 释放链表 */
void ifreeunitlist(iunit *unit) {
	iunit *next = NULL;
	icheck(unit);
	while (unit) {
		next = unit->next;
		ifreeunit(unit);
		unit = next;
	}
}

/* 维度索引 */
#define DimensionW 0
#define DimensionH 1


/* 析构函数 */
static void _ientryfree_node(struct iref* ref) {
	iobjfree(ref);
    /* TODO: do some check if neighbors == NULL and units == NULL */
}

/* 节点内存管理 */
inode * imakenode(){
	inode *node = iobjmalloc(inode);
	iretain(node);
	/* 析构函数在这里  */
	node->free = _ientryfree_node;

	return node;
}

/* 释放节点 */
void ifreenode(inode *node){
	irelease(node);
}

/* 释放节点包含的单元 */
void ifreenodekeeper(inode *node) {
	ifreeunitlist(node->units);
	node->units = NULL;
	node->unitcnt = 0;

	ineighborsclean(node);
	ifreenode(node);
}

/* 释放节点组成的树 */
void ifreenodetree(inode *node) {
	int i;
	icheck(node);

	for (i=0; i<IMaxChilds; ++i) {
		ifreenodetree(node->childs[i]);
	}
	ifreenodekeeper(node);
}

/* 增加叶子节点 */
void justaddleaf(imap *map, inode *node) {
	icheck(node);
	icheck(node->pre == NULL && node->next == NULL);

	list_add_front(map->leaf, node);
	iretain(node);
}

/* 移除叶子节点 */
void justremoveleaf(imap *map, inode *node) {
	icheck(node);
	icheck(node->pre != NULL || node->next != NULL || node == map->leaf);

	list_remove(map->leaf, node);
	irelease(node);
}

/* 把单元加入叶子节点 */
int justaddunit(imap *map, inode *node, iunit *unit){
	icheckret(node && unit, iino);
	icheckret(unit->node == NULL, iino);
	icheckret(node->level == map->divide, iino);

	unit->node = node;
	unit->tick = igetnextmicro();

	node->unitcnt++;
	node->tick = unit->tick;
#if open_node_utick
	node->utick = unit->tick;
#endif

#if open_log_unit
	ilog("[IMAP-Unit] Add Unit (%lld, %s) To Node (%d, %s)\n",
			unit->id, unit->code.code, node->level, node->code.code);
#endif
	list_add_front(node->units, unit);
	/* add ref as we save units in list */
	iretain(unit);

	/* refresh the radius to map */
	imaprefreshunit(map, unit);

	return iiok;
}

/* 从叶子节点移除单元 */
int justremoveunit(imap *map, inode *node, iunit *unit) {
	icheckret(node && unit, iino);
	icheckret(unit->node == node, iino);
	icheckret(node->level == map->divide, iino);

#if open_log_unit
	ilog("[IMAP-Unit] Remove Unit (%lld, %s) From Node (%d, %s)\n",
			unit->id, unit->code.code, node->level, node->code.code);
#endif

	node->unitcnt--;
	node->tick = igetnextmicro();

#if open_node_utick
	node->utick = node->tick;
#endif

	unit->node = NULL;
	unit->tick = node->tick;

	list_remove(node->units, unit);
	irelease(unit);

	return iiok;
}

/* 打印节点信息 */
#define _print_node(node)                                                   \
	do {                                                                \
	        ilog("[IMAP-Node] Node (%d, %s, %p)\n",                     \
	                        node->level, node->code.code, node);        \
	}while(0)

/* 打印单元加入节点的操作 */
#define _print_unit_add(node, unit, idx) \
	do { \
		ilog("[IMAP-Unit-Add] Unit(%lld, %s, x: %.3f, y: %.3f) To Node (%d, %s, %p) \n", \
				unit->id, unit->code.code, unit->code.pos.x, unit->code.pos.y, node->level, node->code.code, node); \
	}while(0)

/* 打印单元移除出节点的操作 */
#define _print_unit_remove(node, unit, idx) \
	do {\
		ilog("[IMAP-Unit-Remove] Unit(%lld, %s, x: %.3f, y: %.3f) From Node (%d, %s, %p) \n", \
				unit->id, unit->code.code, unit->code.pos.x, unit->code.pos.y, node->level, node->code.code, node);\
	}while(0)

/* 节点坐标生成 */
typedef struct ipos2i {
	int x, y;
}ipos2i;
static const ipos2i __node_offset[] =
{
	{0, 0},
	{0, 1},
	{1, 0},
	{1, 1}
};

/* 加入新的节点 */
static inode* _iaddnodetoparent(imap *map, inode *node, int codei, int idx, const icode *code) {
	/* 创造一个节点 */
	inode* child = icache(map->nodecache, inode);
	child->level = idx+1;
	child->parent = node;
	child->codei = codei;
	/* y */
	/* ^ */
	/* | ((0, 1) , (1, 1)) */
	/* | ((0, 0) , (1, 0)) */
	/* -----------> x
	*/
	child->x = node->x * 2 + __node_offset[codei].x;
	child->y = node->y * 2 + __node_offset[codei].y;

	/* 生成节点编码信息 */
	copycode(child->code, (*code), child->level);
	imapgenpos(map, &child->code.pos, &child->code);

	/* 加入父节点 */
	node->childcnt++;
	node->childs[codei] = child;

#if open_log_node
	ilog("[IMAP-Node] Add Node (%d, %s, %p) To Node (%d, %s, %p) in %d\n",
			child->level, child->code.code, child,
			node->level, node->code.code, node, codei);
#endif

	/* 增加节点统计 */
	++map->state.nodecount;
	/* 增加叶子节点统计 */
	if (child->level == map->divide) {
		/* 叶子节点出现 */
		justaddleaf(map, child);
		/* 叶子节点统计 */
		++map->state.leafcount;
	}
	return child;
}

/*
 * 把节点从 有向图里面拿出来， 没有任何一个节点可以到他
 */
void ineighborsclean(inode *node) {
	irefjoint* joint = NULL;
	inode *neighbor = NULL;
	icheck(node);

	/* disconnect to others */
	joint = ireflistfirst(node->neighbors_walkable);
	while (joint) {
		neighbor = icast(inode, joint->value);
		ireflistremove(neighbor->neighbors, irefcast(node));
		joint = joint->next;
	}
	ireflistfree(node->neighbors_walkable);
	node->neighbors_walkable = NULL;

	/* disconnect from others */
	joint = ireflistfirst(node->neighbors);
	while (joint) {
		neighbor = icast(inode, joint->value);
		ireflistremove(neighbor->neighbors_walkable, irefcast(node));
		joint = joint->next;
	}
	ireflistfree(node->neighbors);
	node->neighbors = NULL;
}

/*
 * 没有做重复性的检查
 * 让 node ==> to
 */
void ineighborsadd(inode *node, inode *to) {
    if (!node->neighbors_walkable) {
        node->neighbors_walkable = ireflistmake();
    }
    ireflistadd(node->neighbors_walkable, irefcast(to));
    if (!to->neighbors) {
        to->neighbors = ireflistmake();
    }
    ireflistadd(to->neighbors, irefcast(node));
}

/*
 * 没有做重复性的检查
 * 让 node !==> to
 */
void ineighborsdel(inode *node, inode *to) {
    ireflistremove(node->neighbors_walkable, irefcast(to));
    ireflistremove(to->neighbors, irefcast(node));
}

/* 移除节点 */
static int _iremovenodefromparent(imap *map, inode *node) {
	icheckret(node, iino);
	icheckret(node->parent, iino);

	/* 减少节点统计 */
	--map->state.nodecount;
	/* 减少叶子节点统计 */
	if (node->level == map->divide) {
		/* 叶子节点移除 */
		justremoveleaf(map, node);
		/* 叶子节点统计 */
		--map->state.leafcount;
	}

	/* 移除出父节点 */
	node->parent->childcnt--;
	node->parent->childs[node->codei] = NULL;
	node->parent = NULL;

	/* reset */
	node->level = 0;
	node->codei = 0;
	node->code.code[0] = 0;
	node->tick = 0;
	node->x = node->y = 0;
	node->state = 0;

	/* 清理 邻居 节点*/
	ineighborsclean(node);

#if open_node_utick
	node->utick = 0;
#endif

	/* 释放节点 */
	icacheput(map->nodecache, node);

	return iiok;
}

/* 节点加入地图 */
int imapaddunitto(imap *map, inode *node, iunit *unit, int idx) {
	char code;
	int codei;
	int ok = iino;
	inode *child = NULL;

	icheckret(idx <= map->divide, iino);

	/* 如果节点不能附加单元到上面则直接返回失败 */
	if (_state_is(node->state, EnumNodeStateNoUnit)) {
		return ok;
	}

	code = unit->code.code[idx];
#if open_log_code
	ilog("[IMAP-Code] (%lld, %s, %p) Code %d, Idx: %d, Divide: %d\n", unit->id, unit->code.code, unit, code, idx, map->divide);
#endif

	/* 节点不需要查找了，或者以及达到最大层级 */
	if (code == 0 || idx >= map->divide) {
		/* add unit */
		justaddunit(map, node, unit);
		/* log it */
#if open_log_unit_add
		_print_unit_add(node, unit, idx);
		/* log it */
#endif

		++map->state.unitcount;
		ok = iiok;
	}else {
		/* 定位节点所在的子节点 */
		codei = code - 'A';
		icheckret(codei>=0 && codei<IMaxChilds, iino);
		child = node->childs[codei];

#if open_log_node_get
		ilog("[IMAP-Node-Get] Get Node %p from (%d, %s, %p) in %d\n",
				child, node->level, node->code.code, node, codei);
#endif
		if (child == NULL) {
			/* 创造一个节点 */
			child = _iaddnodetoparent(map, node, codei, idx, &unit->code);
		}

		ok = imapaddunitto(map, child, unit, ++idx);
	}

	/* 更新节点信息 */
	if (ok == iiok) {
		if (child) {
			node->tick = child->tick;
#if open_node_utick
			node->utick = child->tick;
#endif
		}else {
			/* 已经在 justaddunit 更新了节点时间戳 */
		}
	}
	return ok;
}

/* 从地图上移除 */
int imapremoveunitfrom(imap *map, inode *node, iunit *unit, int idx, inode *stop) {
	char code;
	int codei;
	inode *child = NULL;
	int ok = 0;
	icheckret(idx <= map->divide, iino);
	icheckret(node, iino);

	ok = iino;

	code = unit->code.code[idx];
	if (code == 0 || idx >= map->divide) {
		/* log it */
#if open_log_unit_remove
		_print_unit_remove(node, unit, idx);
#endif
		/* 移除单元 */
		justremoveunit(map, node, unit);
		/* 移除单元统计 */
		--map->state.unitcount;
		/* 移除成功 */
		ok = iiok;
	}else {
		codei = code - 'A';
		if (codei >= 0 && codei<IMaxChilds) {
			child = node->childs[codei];
		}

		ok = imapremoveunitfrom(map, child, unit, ++idx, stop);
	}

	if (ok == iiok){
		/* 更新时间 */
		if (child) {
			node->tick = child->tick;
#if open_node_utick
			node->utick = child->tick;
#endif
		}else {
			/* 已经在 justremoveunit 更新了时间戳 */
		}
		/* 回收可能的节点 */
		if (node != stop /* 不是停止节点 */
				&& !_state_is(node->state, EnumNodeStateStatic) /* 不是静态节点 */
				&& node->childcnt == 0 /* 孩子节点为0 */
				&& node->unitcnt == 0 /* 上面绑定的单元节点也为空 */
		   ) {
			_iremovenodefromparent(map, node);
		}
	}
	return ok;
}

/* 根据坐标生成code */
int imapgencode(const imap *map, const ipos *pos, icode *code) {
	/* init value */
	int i = 0;
	int iw, ih;
	ireal threshold = 0.0;
	/*
	ipos np = {.x=pos->x-map->pos.x,
		.y=pos->y-map->pos.y};
	*/
	ipos np = {pos->x-map->pos.x, pos->y-map->pos.y};

	/* assign pos: keep it */
	code->pos.x = pos->x;
	code->pos.y = pos->y;

	/* 计算Code */
	/* y */
	/* ^ */
	/* | (B , D) */
	/* | (A , C) */
	/* -----------> x */
	for( i=0; i<map->divide; ++i) {
#if open_log_gencode
		ilog("[IMAP-GenCode] (%.3f, %.3f) ", np.x, np.y);
#endif
		/* 宽度 */
		iw = 0;
		threshold = map->nodesizes[i+1].w;
#if open_log_gencode
		ilog("threshold-w %.3f ", threshold);
#endif
		if (np.x >= threshold) {
			iw = 1;
			np.x -= threshold;
		}
		ih = 0;
		threshold = map->nodesizes[i+1].h;
#if open_log_gencode
		ilog("threshold-h %.3f ", threshold);
#endif
		if (np.y >= threshold) {
			ih = 1;
			np.y -= threshold;
		}
		code->code[i] = map->code[iw][ih];
#if open_log_gencode
		ilog("map(%d, %d)-%c\n", iw, ih, code->code[i]);
#endif
	}
	/* end it */
	code->code[map->divide] = 0;

	return iiok;
}

/* 从编码生成坐标 */
int imapgenpos(imap *map, ipos *pos, const icode *code) {
	/* init value */
	int i = 0;
	/* ipos np = {.x=0, .y=0}; */
	ipos np = {0, 0};

	/* 计算pos */
	/* y */
	/* ^ */
	/* | (B , D) */
	/* | (A , C) */
	/* -----------> x */
	for( i=0; i<map->divide; ++i) {
		char codei = code->code[i];
		if (codei == 'A') {
		} else if (codei == 'B') {
			np.y += map->nodesizes[i+1].h;
		} else if (codei == 'C') {
			np.x += map->nodesizes[i+1].w;
		} else if (codei == 'D') {
			np.x += map->nodesizes[i+1].w;
			np.y += map->nodesizes[i+1].h;
		} else if (codei == 0) {
			/* we got all precisions */
			break;
		} else {
			ilog("ERROR CODE(%d)=%c in %s \n", i, codei, code->code);
		}
	}
	/* 通过地图原地偏移进行修正 */
	pos->x = np.x + map->pos.x;
	pos->y = np.y + map->pos.y;

	return iiok;
}


/* 编码移动步骤 */
typedef struct movestep {
	char from;
	char to;
	int next; /*iiok 代表还需要继续往父级节点移动*/
}movestep;
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
/* 移动编码具体步骤 */
static movestep gmovesteps[][IMaxChilds] = {
	{/* EnumCodeMoveLeft */
		{'A', 'C', iiok},
		{'B', 'D', iiok},
		{'C', 'A', iino},
		{'D', 'B', iino},
	},
	{/* EnumCodeMoveRight */
		{'A', 'C', iino},
		{'B', 'D', iino},
		{'C', 'A', iiok},
		{'D', 'B', iiok},
	},
	{/* EnumCodeMoveDown */
		{'A', 'B', iiok},
		{'B', 'A', iino},
		{'C', 'D', iiok},
		{'D', 'C', iino},
	},
	{/* EnumCodeMoveUp */
		{'A', 'B', iino},
		{'B', 'A', iiok},
		{'C', 'D', iino},
		{'D', 'C', iiok},
	}
};
/* 坐标移动符号 */
static int gmoveposstep[][2] = {
	/*EnumCodeMoveLeft */
	{-1, 0},
	/*EnumCodeMoveRight */
	{1, 0},
	/*EnumCodeMoveDown */
	{0, -1},
	/*EnumCodeMoveUp */
	{0, 1},
};

/*	边缘部分是不能进行某些操作的 */
static int gmoveforbid[][4] = {
	/* EnumCodeMoveLeft */
	/* C && D Can Be Moved Left*/
	{'A',	'B',	0,		0},
	/* EnumCodeMoveRight */
	/* A && B Can Be Moved Right*/
	{0,		0,		'C',	'D'},
	/* EnumCodeMoveDown */
	/* B && D Can Be Moved Down*/
	{'A',	0,		'C',	0},
	/* EnumCodeMoveUp */
	/* A && C Can Be Moved Up*/
	{0,		'B',	0,		'D'},
};

/* 移动编码 */
int imapmovecode(const imap *map, icode *code, int way) {
	int moves = 0;
	size_t level = 0;
	size_t when = 0;
	icheckret(code, iino);
	icheckret(way>=0 && way<EnumCodeMoveMax, iino);

	level = strlen(code->code);
	when = level;
	/* 确定编码移动没有超出边界 */
	for (;when>0; --when) {
		char at = code->code[when-1];
		icheckret(at>='A' && at<='D', iino);
		if(gmoveforbid[way][at-'A'] == 0 ) {
			break;
		}
	}
	/* 确实可以移动编码 */
	if (when > 0) {
		/* 第一个不同的级别 */
		for (;level > 0; --level) {
			/* get code */
			char at = code->code[level-1];
			movestep * step = NULL;
			icheckret(at>='A' && at<='D', iino);
			/* move it */
			step = &gmovesteps[way][at-'A'];
			code->code[level-1] =step->to;
			moves++;
			/* do need next step */
			if (step->next == iino) {
				--level; /* oldlevel = level + movies; */
				break;
			}
		}
	}

	/* 坐标移动 */
	if (moves) {
		code->pos.x += gmoveposstep[way][0] * map->nodesizes[level+moves].w;
		code->pos.y += gmoveposstep[way][1] * map->nodesizes[level+moves].h;
	}

	return moves;
}

/* 生成一张地图数据 */
imap *imapmake(const ipos *pos, const isize *size, int divide) {
	imap *map = iobjmalloc(imap);
	map->pos = *pos;
	map->size = *size;
	map->divide = divide;

	imapgen(map);

	return map;
}

/* 打印地图状态信息 */
void imapstatedesc(const imap *map, int require,
		const char* intag, const char *inhead) {
	/* 设置Tag */
	const char* tag = "[IMAP-State]";

	icheck(require);
	icheck(map);

	if (intag) {
		tag = intag;
	}

	/* 状态头 */
	if (require & EnumMapStateHead || inhead) {
		ilog("%s ********************************** [Begin]\n",
				inhead?inhead:tag);
	}

	/* 基本信息 */
	if (require & EnumMapStateBasic) {
		ilog("%s Map: -OriginX=%f, OriginY=%f,"
				"-Width=%f, Height=%f, -Divide=%d\n",
				tag,
				map->pos.x, map->pos.y,
				map->size.w, map->size.h,
				map->divide);
	}
	/* 精度信息 */
	if (require & EnumMapStatePrecisions) {
		ilog("%s Precisions: (%.8f, %.8f) \n",
				tag,
				map->nodesizes[map->divide].w,
				map->nodesizes[map->divide].h);
	}
	/* 节点信息 */
	if (require & EnumMapStateNode) {
		ilog("%s Node: Count=%lld\n", tag, map->state.nodecount);
		ilog("%s Node-Leaf: Count=%lld\n", tag, map->state.leafcount);
		ilog("%s Node-Cache: Count=%d\n", tag, irefcachesize(map->nodecache));
	}
	/* 单元信息 */
	if (require & EnumMapStateUnit) {
		ilog("%s Unit: Count=%lld\n", tag, map->state.unitcount);
	}
	/* 状态尾 */
	if (require & EnumMapStateTail || inhead) {
		ilog("%s ********************************** [End]\n",
				inhead?inhead:tag);

	}
}

/* 释放地图数据，不会释放附加在地图上的单元数据 */
void imapfree(imap *map) {
#if open_log_map_construct
	/* 打印状态 */
	imapstatedesc(map, EnumMapStateAll, "[MAP-Free]", "[MAP-Free]");
#endif
	/* 释放四叉树 */
	ifreenodetree(map->root);
	/* 释放叶子节点链表 */
	while (map->leaf) {
		justremoveleaf(map, map->leaf);
	}
	/* 释放节点缓冲区 */
	irefcachefree(map->nodecache);

	/* 释放阻挡位图 */
	if (map->blocks) {
		ifree(map->blocks);
	}

	/* 释放地图本身 */
	iobjfree(map);
}

/* 缓存入口: 生成节点 */
iref *_cache_newnode() {
	return (iref*)imakenode();
}

/* 生成地图数据 */
int imapgen(imap *map) {
	int i = 0;
	ireal iw, ih;

	icheckret(map->divide>=0 && map->divide<IMaxDivide, iino);

	/* 根节点 */
	map->root = imakenode();
	map->root->level = 0;
	map->root->code.code[0] = 'R';
	map->root->code.code[1] = 'O';
	map->root->code.code[2] = 'O';
	map->root->code.code[3] = 'T';

	/* 节点缓存 */
	map->nodecache = irefcachemake(1000, _cache_newnode);

	/* 四分的编码集合 */
	map->code[0][0] = 'A';
	map->code[0][1] = 'B';
	map->code[1][0] = 'C';
	map->code[1][1] = 'D';

	/*宽度的阀值表 高度的阀值表 */
	map->nodesizes[0].w = map->size.w;
	map->nodesizes[0].h = map->size.h;
	map->distances[i] = map->size.w * map->size.w + map->size.h * map->size.h;
	for(i=1; i<=map->divide; ++i){
		iw = map->nodesizes[i-1].w/2;
		ih = map->nodesizes[i-1].h/2;

		map->nodesizes[i].w = iw;
		map->nodesizes[i].h = ih;
		map->distances[i] =  iw * iw + ih *ih;
	}

#if open_log_map_construct
	imapstatedesc(map, EnumMapStateAll, "[MAP-Gen]", "[MAP-Gen]");
#endif

	return iiok;
}

/* 增加一个单元到地图上 */
int imapaddunit(imap *map, iunit *unit) {
	int ok;
	int64_t micro;

	icheckret(unit, iino);
	icheckret(map, iino);

	/* move out side */
	imapgencode(map, &unit->pos, &unit->code);

	/* log it */
#if open_log_unit
	ilog("[IMAP-Unit] Add Unit: %lld - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
#endif
	micro = __Micros;
	ok = imapaddunitto(map, map->root, unit, 0);
	iplog(__Since(micro), "[IMAP-Unit] Add Unit: %lld - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
	return ok;
}

/* 从地图上移除一个单元 */
int imapremoveunit(imap *map, iunit *unit) {
	int ok;
	int64_t micro;

	icheckret(unit, iino);
	icheckret(unit->node, iino);
	icheckret(map, iino);

	/* log it */
#if open_log_unit
	ilog("[IMAP-Unit] Remove Unit: %lld - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
#endif
	micro = __Micros;
	ok = imapremoveunitfrom(map, map->root, unit, 0, map->root);
	iplog(__Since(micro), "[IMAP-Unit] Remove Unit: "
			"%lld - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);

	return ok;
}


/* 从地图上检索节点 */
inode *imapgetnode(const imap *map, const icode *code, int level, int find) {
	int64_t micro = __Micros;
	inode *node = NULL;

	icheckret(map, NULL);
	icheckret(code, NULL);
	icheckret(level<=map->divide, NULL);

	node = map->root;
	while(node->level < level) {
		int codei = code->code[node->level] - 'A';
		/* 孩子超出了 */
		if (!(codei>=0 && codei<IMaxChilds)) {
			break;
		}
		/* 尽可能的返回根节点 */
		if (node->childs[codei] == NULL) {
			break;
		}
		node = node->childs[codei];
	}
	iplog(__Since(micro), "[IMAP-Node] Find Node (%d, %s) With Result %p\n",
			level, code->code, node);

	/* 尽可能的返回 */
	if (find == EnumFindBehaviorFuzzy) {
		return node;
	}
	/* 没有找到 find == EnumFindBehaviorAccurate */
	return node->level == level ? node : NULL;
}

/* 刷新更新时间点 */
#if open_node_utick
static void _imaprefreshutick(inode *node, int64_t utick) {
	icheck(node);
	while (node) {
		node->utick = utick;
		node = node->parent;
	}
}
#endif

/* 更新一个单元在地图上的数据 */
int imapupdateunit(imap *map, iunit *unit) {
	int ok;
	int offset;
	icode code;
	int64_t micro;
	inode *impact;
	inode *removeimpact;
	inode *addimpact;
#if open_node_utick
	int64_t utick = 0;
#endif
	ireal dsx = 0.0;
	ireal dsy = 0.0;

	icheckret(map, iino);
	icheckret(unit->node, iino);

	/* 判断一下坐标变更是否够大 */
	/* 寻找可能的变化范围，定位父亲节点 */
	/* 从公共的父级别节点开始往下找 */
	/* 计算新的偏移量，如果偏移量相对于当前叶子节点原点偏移量小于节点大小 */
	dsx = unit->pos.x - unit->node->code.pos.x;
	dsy = unit->pos.y - unit->node->code.pos.y;
	if (dsx>=0 && dsx<map->nodesizes[unit->node->level].w
			&& dsy>=0 && dsy<map->nodesizes[unit->node->level].h) {
#if open_log_unit_update
		ilog("[MAP-Unit] Update:  Change Do Not Need Trace (%.3f, %.3f)\n", dsx, dsy);
#endif
		/* 如果需要支持utick 依然需要更新所有父级节点的utick */
		/* 获取影响的顶级节,  更新父亲节点影响节点的utick点 */
#if open_node_utick
		_imaprefreshutick(unit->node, igetnextmicro());
#endif
		return iiok;
	}

	micro = __Micros;
	/* 生成新的编码 */
	imapgencode(map, &unit->pos, &code);
	/* 获取新编码的变更顶层节点位置, 并赋值新的编码 */
	/* 这里只能从0级别开始，code 可能后部分一致，但是前部不一致 */
	for(offset=0; offset<map->divide; ++offset) {
		if (code.code[offset] != unit->code.code[offset]) {
			break;
		}else {
			/*unit->code.code[offset] = code.code[offset]; */
		}
	}
	/* 没有任何变更: 虽然距离很大，但是编码不变（在地图边界外部会出现这种情况） */
	if (offset == map->divide) {
		/* 如果需要支持utick 依然需要更新所有父级节点的utick */
		/* 获取影响的顶级节点 */
		/* 更新父亲节点影响节点的utick */
#if open_node_utick
		_imaprefreshutick(unit->node, igetnextmicro());
#endif
		return iiok;
	}
	/* 设置编码里面的坐标 */
	unit->code.pos.x = unit->pos.x;
	unit->code.pos.y = unit->pos.y;
	/* 获取影响的顶级节点 */
	impact = imapgetnode(map, &code, offset, EnumFindBehaviorFuzzy);

	/* 从影响的顶级节点的具体子节点移除 */
	removeimpact = impact->childs[unit->code.code[offset]-'A'];
	ok = imapremoveunitfrom(map, removeimpact, unit, removeimpact->level, impact);
	/* 新的编码 */
	copycode(unit->code, code, map->divide);
	/* 加入到新的节点单元 */
	if (ok == iiok) {
		int codei = 0;
#if open_node_utick
		utick = removeimpact->utick;
#endif
		/* 从当前顶级节点开始加入节点 */
		codei = unit->code.code[offset]-'A';
		addimpact = impact->childs[codei];
		if (!addimpact) {
			addimpact = _iaddnodetoparent(map, impact, codei, impact->level, &code);
		}

		ok = imapaddunitto(map, addimpact, unit, addimpact->level);
#if open_node_utick
		if (ok == iiok) {
			utick = addimpact->utick;
		}
#endif
	}
	/* 更新父亲节点影响节点的utick */
#if open_node_utick
	if (utick) {
        _imaprefreshutick(impact, utick);
	}
#endif
	iplog(__Since(micro), "[MAP-Unit] Update  Unit(%lld) To (%s, %.3f, %.3f)\n",
			unit->id, code.code, code.pos.x, code.pos.y);

	return ok;
}

/* 更新一个单元的附加信息到地图数据上：现阶段就只更新了单元的半径信息 */
void imaprefreshunit(imap *map, const iunit *unit) {
	iunused(map);
	iunused(unit);

#if iiradius
	if (map->maxradius < unit->radius) {
		map->maxradius = unit->radius;
	}
#endif
}

/* 建议 divide 不要大于 10*/
/* 加载位图阻挡信息 sizeof(blocks) == (divide*divide + 7 ) / 8 */
void imaploadblocks(imap *map, char* blocks) {
	/* new memory */
	size_t size = (map->divide*map->divide + 7)/8;
	if (map->blocks == NULL) {
		map->blocks = icalloc(1, size);
	}
	memcpy(map->blocks, blocks, size);
}

/* 设置块的状态 */
void imapsetblock(imap *map, int x, int y, int state) {
	int cur = x * map->divide + y;
	int idx = cur / 8;
	int offset = cur & 8;
	if (state == iiok) {
		map->blocks[idx] = map->blocks[idx] | (1<<offset);
	} else {
		map->blocks[idx] = map->blocks[idx] & (~(1<<offset));
	}
}

/* 获取块的状态 */
int imapgetblock(const imap *map, int x, int y) {
	int cur = x * map->divide + y;
	int idx = cur / 8;
	int offset = cur & 8;
	return (map->blocks[idx] >> offset) & 0x01;
}

/* 对一个数字做Hash:Redis */
static void _ihash(int64_t *hash, int64_t v) {
	*hash += v;
	/* For the hashing step we use Tomas Wang's 64 bit integer hash. */
	*hash = (~*hash) + (*hash << 21); /* hash = (hash << 21) - hash - 1; */
	*hash = *hash ^ (*hash >> 24);
	*hash = (*hash + (*hash << 3)) + (*hash << 8); /* hash * 265 */
	*hash = *hash ^ (*hash >> 14);
	*hash = (*hash + (*hash << 2)) + (*hash << 4); /* hash * 21 */
	*hash = *hash ^ (*hash >> 28);
	*hash = *hash + (*hash << 31);
}

#define __realint(r) ((int64_t)(r*10000000))

/* 默认的过滤器指纹计算方法 */
static int64_t _entryfilterchecksumdefault(imap *map, const struct ifilter *d) {
	int64_t hash = 0;

	/* circle */
	_ihash(&hash, __realint(d->s.u.circle.pos.x));
	_ihash(&hash, __realint(d->s.u.circle.pos.y));
	_ihash(&hash, __realint(d->s.u.circle.radius));

	/* rect */
	_ihash(&hash, __realint(d->s.u.rect.pos.x));
	_ihash(&hash, __realint(d->s.u.rect.pos.y));
	_ihash(&hash, __realint(d->s.u.rect.size.w));
	_ihash(&hash, __realint(d->s.u.rect.size.h));

	/* id */
	_ihash(&hash, d->s.u.id);

	/* code */
	_ihash(&hash, *(int64_t *)(d->s.u.code.code));
	_ihash(&hash, *(int64_t *)(d->s.u.code.code + sizeof(int64_t)));
	_ihash(&hash, *(int64_t *)(d->s.u.code.code + sizeof(int64_t) * 2));
	_ihash(&hash, *(int64_t *)(d->s.u.code.code + sizeof(int64_t) * 3));

	return hash;
}

/* 指纹识别 */
int64_t ifilterchecksum(imap *map, const ifilter *d) {
	/* 依赖 IMaxDivide */
	int64_t hash = 0;
	irefjoint *sub;

	hash = d->entrychecksum ? d->entrychecksum(map, d) : _entryfilterchecksumdefault(map, d);

	/* Subs */
	if (d->s.list) {
		sub = ireflistfirst(d->s.list);
		while (sub) {
			_ihash(&hash, ifilterchecksum(map, icast(ifilter, sub->value)));
			sub = sub->next;
		}
	}

	return hash;
}

/* 过滤器的析构入口 */
static void __ifilterfreeentry(iref *ref) {
	ifilter *filter = icast(ifilter, ref);
	ireflistfree(filter->s.list);
	filter->s.list = NULL;
	iobjfree(filter);
}

/* 释放节点o */
void ifilterfree(ifilter *filter) {
	irelease(filter);
}

/* 基础过滤器 */
ifilter *ifiltermake() {
	ifilter *filter = iobjmalloc(ifilter);
	filter->free = __ifilterfreeentry;
	filter->entrychecksum = _entryfilterchecksumdefault;
	iretain(filter);
	return filter;
}

/* 往一个已有的过滤器里面添加过滤器 */
void ifilteradd(ifilter *filter, ifilter *added) {
	icheck(filter);
	icheck(added);
	if (!filter->s.list) {
		filter->s.list = ireflistmake();
	}
	ireflistadd(filter->s.list, irefcast(added));
}

/* 移除子过滤器 */
void ifilterremove(ifilter *filter, ifilter *sub) {
	icheck(filter);
	icheck(filter->s.list);

	ireflistremove(filter->s.list, irefcast(sub));
}

/* 移除所有子过滤器 */
void ifilterclean(ifilter *filter) {
	icheck(filter);
	ireflistremoveall(filter->s.list);
}

/* 组合过滤器 */
static int _ientryfilter_compose(imap *map, const ifilter *filter, const iunit* unit) {
	irefjoint *joint = NULL;
	icheckret(unit, iino);
	icheckret(filter->s.list, iino);

	/* 遍历所有的过滤器 */
	joint = ireflistfirst(filter->s.list);
	while(joint) {
		ifilter* subfilter = NULL;
		icheckret(joint->value, iino);
		/* 子过滤器 */
		subfilter = icast(ifilter, joint->value);
		if (subfilter->entry(map, subfilter, unit) == iino) {
			return iino;
		}
		/* 下一个过滤器 */
		joint = joint->next;
	}

	return iiok;
}

/* 通用过滤器入口 */
int ifilterrun(imap *map, const ifilter *filter, const iunit *unit) {
	int ok = 0;
	icheckret(filter, iiok);

	ok = iiok;
	/* 组合过滤模式 */
	if (filter->s.list) {
		ok = _ientryfilter_compose(map, filter, unit);
	}
	/* 单一过滤模式 */
	if (ok == iiok && filter->entry &&
			filter->entry != _ientryfilter_compose) {
		ok = filter->entry(map, filter, unit);
	}
	return ok;
}

/* 距离过滤器 */
int _ientryfilter_circle(imap *map, const ifilter *filter, const iunit* unit) {
	icircle ucircle;
	(void) ucircle;
	icheckret(unit, iino);
	iunused(map);

#if iiradius
	ucircle.pos = unit->pos;
	ucircle.radius = unit->radius;
	/* 距离超出范围 */
	if (icircleintersect(&filter->s.u.circle, &ucircle) == iino) {
#if open_log_filter
		ilog("[MAP-Filter] NO : Unit: %lld (%.3f, %.3f : %.3f) - (%.3f, %.3f: %.3f)\n",
				unit->id,
				unit->pos.x, unit->pos.y,
				unit->radius,
				filter->s.u.circle.pos.x, filter->s.u.circle.pos.y, filter->s.u.circle.radis);
#endif /* open_log_filter */
		return iino;
	}
#else
	/* 距离超出范围 */
	if (icirclecontainspoint(&filter->s.u.circle, &unit->pos) == iino) {
#if open_log_filter
		ilog("[MAP-Filter] NO : Unit: %lld (%.3f, %.3f) - (%.3f, %.3f: %.3f)\n",
				unit->id,
				unit->pos.x, unit->pos.y,
				filter->s.u.circle.pos.x, filter->s.u.circle.pos.y, filter->s.u.circle.radis);
#endif /* open_log_filter */
		return iino;
	}
#endif /* iiradius */

	return iiok;
}

/* 圆形过滤器的指纹信息 */
static int64_t _ientryfilechecksum_circile(imap *map, const ifilter *d) {
	int64_t hash = 0;

	/* circle */
	_ihash(&hash, __realint(d->s.u.circle.pos.x));
	_ihash(&hash, __realint(d->s.u.circle.pos.y));
	_ihash(&hash, __realint(d->s.u.circle.radius));
	return hash;
}

/* range过滤器 */
ifilter *ifiltermake_circle(const ipos *pos, ireal range) {
	ifilter *filter = ifiltermake();
	filter->s.u.circle.pos = *pos;
	filter->s.u.circle.radius = range;
	filter->entry = _ientryfilter_circle;
	filter->entrychecksum = _ientryfilechecksum_circile;
	return filter;
}

/* 距离过滤器 */
static int _ientryfilter_rect(imap *map, const ifilter *filter, const iunit* unit) {
	icircle c;
	(void) c;
	icheckret(unit, iino);
	iunused(map);

#if iiradius
	/* 距离超出范围 */
	c.pos = unit->pos;
	c.radius = unit->radius;
	if (irectintersect(&filter->s.u.rect, &c) == iino) {
#if open_log_filter
		ilog("[MAP-Filter] NO : Unit: %lld (%.3f, %.3f: %.3f)"
				" Not In Rect (%.3f, %.3f:%.3f, %.3f) \n",
				unit->id,
				unit->pos.x, unit->pos.y, unit->radius,
				filter->s.u.rect.pos.x, filter->s.u.rect.pos.y,
				filter->s.u.rect.size.w, filter->s.u.rect.size.h);
#endif
		return iino;
	}
#else

	/* 距离超出范围 */
	if (irectcontainspoint(&filter->s.u.rect, &unit->pos) == iino) {
#if open_log_filter
		ilog("[MAP-Filter] NO : Unit: %lld (%.3f, %.3f)"
				" Not In Rect (%.3f, %.3f:%.3f, %.3f) \n",
				unit->id,
				unit->pos.x, unit->pos.y,
				filter->s.u.rect.pos.x, filter->s.u.rect.pos.y,
				filter->s.u.rect.size.w, filter->s.u.rect.size.h);
#endif
		return iino;
	}
#endif /* iiradius*/

	return iiok;
}

/* 方向过滤器的指纹信息 */
static int64_t _ientryfilterchecksum_rect(imap *map, const ifilter *d) {
	int64_t hash = 0;

	/* rect */
	_ihash(&hash, __realint(d->s.u.rect.pos.x));
	_ihash(&hash, __realint(d->s.u.rect.pos.y));
	_ihash(&hash, __realint(d->s.u.rect.size.w));
	_ihash(&hash, __realint(d->s.u.rect.size.h));

	return hash;
}

/* 矩形过滤器 */
ifilter *ifiltermake_rect(const ipos *pos, const isize *size) {
	ifilter *filter = ifiltermake();
	filter->s.u.rect.pos = *pos;
	filter->s.u.rect.size = *size;
	filter->entry = _ientryfilter_rect;
	filter->entrychecksum = _ientryfilterchecksum_rect;
	return filter;
}

/* 搜集树上的所有单元 */
void imapcollectunit(imap *map, const inode *node, ireflist *list, const ifilter *filter, ireflist *snap) {
	int i;
	iunit* unit = NULL;
	icheck(node);

	/* 增加节点单元 */
	unit = node->units;
	for (;unit;unit=unit->next) {
		/* 是否已经处理过 */
		int havestate = _state_is(unit->state, EnumUnitStateSearching);
		if (havestate) {
			continue;
		}
		/* 是否满足条件 */
		if (ifilterrun(map, filter, unit) == iiok) {
			ireflistadd(list, irefcast(unit));
		}
		/* 保存快照 */
		ireflistadd(snap, irefcast(unit));
		_state_add(unit->state, EnumUnitStateSearching);
	}

	/* 遍历所有的孩子节点 */
	icheck(node->childs);
	icheck(node->childcnt);
	for (i=0; i<IMaxChilds; ++i) {
		imapcollectunit(map, node->childs[i], list, filter, snap);
	}
}

/* 清除搜索结果标记 */
void imapcollectcleanunittag(imap *map, const ireflist *list) {
	irefjoint *joint = ireflistfirst(list);
	while (joint) {
		iunit *u = icast(iunit, joint->value);
		_state_remove(u->state, EnumUnitStateSearching);
		joint = joint->next;
	}
}

/* 清除搜索结果标记 */
void imapcollectcleannodetag(imap *map, const ireflist *list) {
	irefjoint *joint = ireflistfirst(list);
	while (joint) {
		inode *u = icast(inode, joint->value);
		_state_remove(u->state, EnumNodeStateSearching);
		joint = joint->next;
	}
}

/* 创建搜索结果 */
isearchresult* isearchresultmake() {
	isearchresult *result = iobjmalloc(isearchresult);
	result->units = ireflistmake();
	result->filter = ifiltermake();
	result->snap = ireflistmake();
	return result;
}

/* 对接过滤器 */
void isearchresultattach(isearchresult* result, ifilter *filter) {
	icheck(result);
	icheck(result->filter != filter);

	iassign(result->filter, filter);
}

/* 移除过滤器 */
void isearchresultdettach(isearchresult *result) {
	icheck(result);
	icheck(result->filter);

	irelease(result->filter);
	result->filter = NULL;
}

/* 释放所有节点 */
void isearchresultfree(isearchresult *result) {
	icheck(result);
	/* 释放快照 */
	ireflistfree(result->snap);
	/* 释放所有单元 */
	ireflistfree(result->units);
	/* 释放过滤器 */
	isearchresultdettach(result);
	/* 释放结果本身 */
	iobjfree(result);
}

/* 清理中间态的结果 */
void isearchresultclean(isearchresult *result) {
	icheck(result);

	ireflistremoveall(result->units);
	ireflistremoveall(result->snap);
	result->checksum = 0;
	result->tick = 0;
}

/* 从快照里面从新生成新的结果 */
void isearchresultrefreshfromsnap(imap *map, isearchresult *result) {
	irefjoint* joint = NULL;
	icheck(result);

	/* 移除旧的结果 */
	ireflistremoveall(result->units);

	/* 增加节点单元 */
	joint = ireflistfirst(result->snap);
	for (;joint;joint=joint->next) {
		iunit *unit = icast(iunit, joint->value);
		/* 是否已经处理过 */
		if (_state_is(unit->state, EnumUnitStateSearching)) {
			continue;
		}
		/* 是否满足条件 */
		if (ifilterrun(map, result->filter, unit) == iiok) {
			ireflistadd(result->units, irefcast(unit));
		}
		/* 保存快照 */
		_state_add(unit->state, EnumUnitStateSearching);
	}

	imapcollectcleanunittag(map, result->snap);
}

/* 计算节点列表的指纹信息 */
int64_t imapchecksumnodelist(imap *map, const ireflist *list, int64_t *maxtick, int64_t *maxutick) {
	int64_t hash = 0;
#if open_node_utick
	int64_t utick = 0;
#endif
	int64_t tick = 0;
	irefjoint *joint = ireflistfirst(list);
	while (joint) {
		inode *node = icast(inode, joint->value);
#if open_node_utick
		if (node->utick > utick) {
			utick = node->utick;
		}
#endif
		if (node->tick > tick) {
			tick = node->tick;
		}

		_ihash(&hash, node->tick);
		joint = joint->next;
	}
	if (maxtick) {
		*maxtick = tick;
	}
	if (maxutick) {
#if open_node_utick
		*maxutick = utick;
#else
		*maxutick = 0;
#endif
	}
	return hash;
}

/* 搜索 */
void imapsearchfromnode(imap *map, const inode *node,
		isearchresult* result, ireflist *innodes) {
	irefjoint *joint;
	inode *searchnode;
	int64_t micro = __Micros;
	/* 校验码 */
	int64_t maxtick;
	int64_t maxutick;
	int64_t checksum = ifilterchecksum(map, result->filter);
	int64_t nodechecksum = imapchecksumnodelist(map, innodes, &maxtick, &maxutick);
	_ihash(&checksum, nodechecksum);
	if (result->checksum == checksum) {
		/* 上下文的环境一样，而且没有人动过或者变更过相关属性，直接返回了 */
		/* 如果不支持 utick , 则maxutick == 0 */
		if (result->tick >= maxutick) {
			return;
		}

		/* 有人动过，但动作幅度不大 ： 直接从快照里面找吧 */
		maxtick = maxtick > node->tick ? maxtick : node->tick;
		if (result->tick >= maxtick) {
			isearchresultrefreshfromsnap(map, result);
			return;
		}
	}
	/* 重新搜索赋予最大的tick */
	maxtick = __max(maxtick, maxutick);

	/* 释放旧的单元，重新搜索 */
	isearchresultclean(result);

	/* 搜索结果 */
	joint = ireflistfirst(innodes);
	while (joint) {
		searchnode = icast(inode, joint->value);

		imapcollectunit(map, searchnode, result->units, result->filter, result->snap);
		joint = joint->next;
	}
	imapcollectcleanunittag(map, result->snap);

	/* 更新时间戳 */
	result->tick = maxtick;
	/* 更新校验码 */
	result->checksum = checksum;
	/* 性能日志 */
	iplog(__Since(micro),
			"[MAP-Unit-Search] Search-Node "
			"(%d, %s) --> Result: %d Units \n",
			node->level, node->code.code, ireflistlen(result->units));
}

/* 收集包含指定矩形区域的节点(最多4个) */
void imapsearchcollectnode(imap *map, const irect *rect, ireflist *collects) {
	icode code;
	ipos tpos;
	inode *tnode = NULL;
	int i;
	int level = map->divide;
	/*
	 *^
	 *|(1)(2)
	 *|(0)(3)
	 *|__________>
	 */
	ireal offsets[] = {rect->pos.x, rect->pos.y,
		rect->pos.x, rect->pos.y + rect->size.h,
		rect->pos.x + rect->size.w, rect->pos.y + rect->size.h,
		rect->pos.x + rect->size.w, rect->pos.y };

	/* 可能的节点层级 */
	while (level > 0 && map->nodesizes[level].h < rect->size.h) {
		--level;
	}
	while (level > 0 && map->nodesizes[level].w < rect->size.w) {
		--level;
	}

	/* 获取可能存在数据的节点 */
	for (i=0; i<4; ++i) {
		tpos.x =  offsets[2*i];
		tpos.y =  offsets[2*i+1];
		/* 跟刚刚一样的节点 */
		if (tnode && tpos.x >= tnode->code.pos.x
			&& (tnode->code.pos.x + map->nodesizes[tnode->level].w > tpos.x)
			&& tpos.y >= tnode->code.pos.y
			&& (tnode->code.pos.y + map->nodesizes[tnode->level].h > tpos.y)) {
			continue;
		}

		/* TODO: can use the imapmovecode to optimaze*/
		imapgencode(map, &tpos, &code);
		tnode = imapgetnode(map, &code, level, EnumFindBehaviorAccurate);
		if (tnode && !_state_is(tnode->state, EnumNodeStateSearching)) {
			_state_add(tnode->state, EnumNodeStateSearching);
			ireflistadd(collects, irefcast(tnode));
		}
	}

	/* 清理标记 */
	imapcollectcleannodetag(map, collects);
}

/* 计算给定节点列表里面节点的最小公共父节点 */
inode *imapcaculatesameparent(imap *map, const ireflist *collects) {
	inode *node;
	inode *tnode;
	irefjoint *joint;

	/* 通过模糊查找响应的节点 */
	joint = ireflistfirst(collects);
	node = icast(inode, joint->value);

	/* 追溯最大的公共父节点 */
	while(joint->next) {
		joint = joint->next;
		tnode = icast(inode, joint->value);
		while (tnode != node) {
			if (tnode->level == node->level) {
				tnode = tnode->parent;
				node = node->parent;
			} else if(tnode->level < node->level) {
				node = node->parent;
			} else {
				tnode = tnode->parent;
			}
		}
	}
	return node;
}

/* 从地图上搜寻单元 irect{pos, size{rangew, rangeh}}, 并附加条件 filter */
void imapsearchfromrectwithfilter(imap *map, const irect *rect,
		isearchresult *result, ifilter *filter) {
	int64_t micro = __Micros;
	inode *node;
	ireflist *collects;

	icheck(result);
	icheck(result->filter);

	/* 获取可能存在数据的节点 */
	collects = ireflistmake();

	/* 收集可能的候选节点 */
	imapsearchcollectnode(map, rect, collects);

	/* 没有任何潜在的节点 */
	if (ireflistlen(collects) == 0) {
		ireflistfree(collects);
        /* 清理以前的搜索结果 */
        isearchresultclean(result);
		return;
	}

	/* 计算父节点 */
	node = imapcaculatesameparent(map, collects);

	/* 造一个距离过滤器 */
	ifilteradd(result->filter, filter);

	/* 从节点上搜寻结果 */
	imapsearchfromnode(map, node, result, collects);

	/* 移除范围过滤器 */
	ifilterremove(result->filter, filter);

	/* 释放所有候选节点 */
	ireflistfree(collects);

	/* 性能日志 */
	iplogwhen(__Since(micro), 10, "[MAP-Unit-Search] Search-Node-Range From: "
			"(%d, %s: (%.3f, %.3f)) In Rect (%.3f, %.3f , %.3f, %.3f) "
			"---> Result : %d Units \n",
			node->level, node->code.code,
			node->code.pos.x, node->code.pos.y,
			rect->pos.x, rect->pos.y, rect->size.w, rect->size.h,
			ireflistlen(result->units));
}

/* 从地图上搜寻单元, 并附加条件 filter */
void imapsearchfrompos(imap *map, const ipos *pos,
		isearchresult *result, ireal range) {
	ireal rectrange = range + iiradius * map->maxradius; /* 扩大搜索区域，以支持单元的半径搜索 */
	/* 目标矩形 */
	/*irect rect = {.pos={.x=pos->x-rectrange, .y=pos->y-rectrange}, .size={.w=2*rectrange, .h=2*rectrange}};*/
	irect rect = {{pos->x-rectrange, pos->y-rectrange}, {2*rectrange, 2*rectrange}};

	ifilter *filter = NULL;

	icheck(result);
	icheck(result->filter);


	/* 造一个距离过滤器 */
	filter = ifiltermake_circle(pos, range);

	imapsearchfromrectwithfilter(map, &rect, result, filter);

	ifilterfree(filter);
}

/* 从地图上搜寻单元 */
void imapsearchfromunit(imap *map, iunit *unit,
		isearchresult *result, ireal range) {
	icheck(result->filter);

	_state_add(unit->state, EnumUnitStateSearching);
	/* 从节点范围搜索 */
	imapsearchfrompos(map, &unit->pos, result, range);
	_state_remove(unit->state, EnumUnitStateSearching);
}

/* 找一种方式打印出地图的树结构 */
/* ************************************************************ */
/* ************************************************************ */
/**
  └── [ROOT] tick: 1432538485114727, utick: 1432538485114727
  └── [A] tick: 1432538485114727, utick: 1432538485114727
  └── [AA] tick: 1432538485114727, utick: 1432538485114727
  ├── [AAA] tick: 1432538485114725, utick: 1432538485114725 units(2,1,0)
  └── [AAD] tick: 1432538485114727, utick: 1432538485114727 units(3)
  */
/* 打印节点 */
void _aoi_printnode(int require, const inode *node, const char* prefix, int tail) {
	/* 前面 */
	ilog("%s", prefix);
	if (tail) {
		ilog("%s", "└── ");
	}else {
		ilog("%s", "├── ");
	}
	/* 打印节点 */
	ilog("[%s]", node->code.code);
	/* 打印节点时间戳 */
	if (require & EnumNodePrintStateTick) {
		ilog(" tick(%lld", node->tick);
#if open_node_utick
		ilog(",%lld", node->utick);
#endif
		ilog(")");
	}
	/* 打印节点单元 */
	if ((require & EnumNodePrintStateUnits) && node->units) {
		iunit *u = node->units;
		ilog(" units(");
		while (u) {
			ilog("%lld%s", u->id, u->next ? ",":")");
			u= u->next;
		}
	}
	ilog("\n");

	if (node->childcnt) {
		int cur = 0;
		int i;
		inode *child = NULL;
		for (i=0; i<node->childcnt; ++i) {
			char snbuf[1024] = {0};
			memset(snbuf, 0, 1024);

			while (child == NULL && cur < IMaxChilds) {
				child = node->childs[cur];
				cur++;
			}

			snprintf(snbuf, 1023, "%s%s", prefix, tail ? "	  " : "│   ");
			if (i == node->childcnt - 1) {
				_aoi_printnode(require, child, snbuf, iiok);
			}else {
				_aoi_printnode(require, child, snbuf, iino);
			}
			child = NULL;
		}
	}
}

/* 打印出地图树 */
void _aoi_print(imap *map, int require) {
	if (require & EnumNodePrintStateMap) {
		imapstatedesc(map, EnumMapStateAll, "Print", "Map-Printing");
	}

	_aoi_printnode(require, map->root, "", iiok);
}

/* 测试 */
int _aoi_test(int argc, char** argv) {
	int i;
	/*
	ipos pos = {.x = 0, .y = 0};
	isize size = {.w = 512, .h = 512};
	*/
	ipos pos = {0, 0};
	isize size = {512, 512};

	int divide = 1;/*10; */
	int randcount = 2000;

	imap *map = NULL;
	iunit* units[10] = {0};


	isearchresult *result = NULL;

	iunused(argc);
	iunused(argv);

	if (argc >= 2) {
		divide = atoi(argv[1]);
	}
	if (argc >= 3) {
		randcount = atoi(argv[2]);
	}

	map = imapmake(&pos, &size, divide);

	for (i=0; i<10; ++i) {
		units[i] = imakeunit((iid)i, (ireal)i, (ireal)i);
		imapaddunit(map, units[i]);
	}

	imapstatedesc(map, EnumMapStateAll, NULL, "[Check]");

	for (i=0; i<10; ++i) {
		imapremoveunit(map, units[i]);
	}

	imapstatedesc(map, EnumMapStateAll, NULL, "[Check]");

	for (i=0; i<10; ++i) {
		imapaddunit(map, units[i]);
	}

	imapstatedesc(map, EnumMapStateAll, NULL, "[Check]");

	ilog("[Test GenCode-GenPos]******************\n");
	for(i=0; i<10; ++i) {
		ipos pos;
		imapgenpos(map, &pos, &units[i]->code);
		ilog("GenPos[%d] (%.3f, %.3f) - %s - (%.3f, %.3f)\n",
				i,
				units[i]->pos.x, units[i]->pos.y,
				units[i]->code.code,
				pos.x, pos.y);
	}
	ilog("[Test GenCode-GenPos]******************\n");

	/* put seed now */
	srand((unsigned)time(NULL));

	for (i=0; i<randcount; ++i) {
		iunit *u = imakeunit(i, (ireal)(rand()%512), (ireal)(rand()%512));
		imapaddunit(map, u);
		ifreeunit(u);
	}

	imapstatedesc(map, EnumMapStateAll, NULL, "[Check-After-Add-2000-Rand]");

	/* update unit */
	for (i=0; i<100; ++i) {
		units[3]->pos.x = (ireal)(rand()%512);
		units[3]->pos.y = (ireal)(rand()%512);
		imapupdateunit(map, units[3]);
	}

	imapstatedesc(map, EnumMapStateAll, NULL, "[Check-After-Update-100-Rand]");

	/* 搜索节点 */
	result = isearchresultmake();

	for (i=0; i<1000; ++i) {
		/*ipos pos = {.x=(ireal)(rand()%512), .y=(ireal)(rand()%512)};*/
		ipos pos = {(ireal)(rand()%512), (ireal)(rand()%512)};
		imapsearchfrompos(map, &pos, result, (ireal)(rand()%10));
	}
	isearchresultfree(result);

	imapstatedesc(map, EnumMapStateAll, NULL, "[Check-After-Search-100-Rand]");

	/* free all units */
	for (i=0; i<10; ++i) {
		ifreeunit(units[i]);
		units[i] = NULL;
	}

	imapfree(map);
	return 0;
}
