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

#ifndef __AOI_H_
#define __AOI_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32

#include <windows.h>
#define snprintf _snprintf

// TODO: figure out a multiplatform version of uint64_t
// - maybe: https://code.google.com/p/msinttypes/
// - or: http://www.azillionmonkeys.com/qed/pstdint.h
typedef _int64 int64_t;
typedef _uint64 uint64_t;
typedef _int32 int32_t;
typedef _uint32 uint32_t;

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif /* end of: offsetof */

#ifndef container_of
#define container_of(ptr, type, member) ({            \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);\
    (type *)( (char *)__mptr - offsetof(type,member) );})
#endif /* end of:container_of */

#else
#include <stdbool.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#endif /* end of: _WIN32 */

/*****
 * AOI: (Area Of Interesting)
 * 1. 适用于大批量的对象管理和查询
 * 2. 采用四叉树来进行管理，可以通过少量改动支持3D
 * 3. 动态节点回收管理，减少内存, 节点的数量不会随着四叉树的层次变得不可控，与对象数量成线性关系
 * 3. 缓存搜索上下文，减少无效搜索，对于零变更的区域，搜索会快速返回
 * 4. 整个搜索过程，支持自定义过滤器进行单元过滤
 * 5. AOI 支持对象的内存缓冲区管理
 * 6. 线程不安全
 * 7. 支持单位半径，不同单位可以定义不同的半径(开启半径感知，会损失一部分性能)
 * 8. 只提供了搜索结构，不同单位的视野区域可以定义不一样
 *
 * 提高内存访问，建议启用 iimeta (1)
 * 如果不需要单位半径感知 建议关闭 iiradius (0)
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* 是否启动元信息： 记录类型对象的内存使用情况, 并加一层对象的内存缓冲区 */
#define iimeta (1)

/* 是否启动单位半径支持 */
#define iiradius (1)
    
/* if we support thread safe in meta system and ref */
#define iithreadsafe (1)

/* 常用布尔宏 */
#define iiyes 1
#define iiok 1
#define iino 0

/* 条件检查，不带assert */
#define icheck(con) do { if(!(con)) return ; } while(0)
#define icheckret(con, ret) do { if(!(con)) return ret; } while(0)

/* flat array count */
#define icountof(arr) (sizeof(arr)/sizeof(arr[0]))
    
/* 常用的宏 */
#define imax(a, b) ((a) > (b) ? (a) : (b))
#define imin(a, b) ((a) < (b) ? (a) : (b))
#define iunused(v) (void)(v)
#define ilog(...) printf(__VA_ARGS__)
    
/* helper for printf isize */
#define __isize_format "(w:%lf, h:%lf)"
#define __isize_value(s) (s).w, (s).h

/* helper for printf ipos */
#define __ipos_format "(x:%lf, y:%lf)"
#define __ipos_value(p) (p).x, (p).y
    
/* helper for printf ipos3 */
#define __ipos3_format "(x:%lf, y:%lf, z:%lf)"
#define __ipos3_value(p) (p).x, (p).y, (p).z
    
/* helper for printf ivec2 */
#define __ivec2_format __ipos_format
#define __ivec2_value(i) __ipos_value((i).v)

/* helper for printf ivec3 */
#define __ivec3_format __ipos3_format
#define __ivec3_value(i) __ipos3_value((i).v)
    
/* helper for printf iline2d */
#define __iline2d_format "[start:"__ipos_format" end:"__ipos_format"]"
#define __iline2d_value(l) __ipos_value((l).start),__ipos_value((l).end)
    
/* helper for printf iline3d */
#define __iline3d_format "[start:"__ipos3_format" end:"__ipos3_format"]"
#define __iline3d_value(l) __ipos3_value((l).start),__ipos3_value((l).end)
    
/* helper for printf iplane */
#define __iplane_format "[normal:"__ivec3_format" pos:"__ipos3_format" dist:%lf]"
#define __iplane_value(p) __ivec3_value((p).normal),__ipos3_value((p).pos),(p).distance

/* helper for printf irect */
#define __irect_format "[pos:"__ipos_format" size:"__isize_format"]"
#define __irect_value(r) __ipos_value((r).pos),__isize_value((r).size)
    
/* helper for printf irect */
#define __icircle_format "[pos:"__ipos_format" radius:%lf]"
#define __icircle_value(c) __ipos_value((c).pos), (c).radius

/* 节点查找行为 */
typedef enum EnumFindBehavior {
    /* 精确查找 */
    EnumFindBehaviorAccurate,
    /* 模糊查找 */
    EnumFindBehaviorFuzzy,
}EnumFindBehavior;

/* 获取当前系统的微秒数 */
int64_t igetcurmicro();
/* 获取当前系统的毫秒数 */
int64_t igetcurtick();
/* 获取系统的下一个唯一的事件微秒数 */
int64_t igetnextmicro();

/* 精度 */
typedef double ireal;
    
/* 默认的 epsilon */
#define iepsilon 0.1e-6
    
/* 浮点比较 */
#define ireal_equal_in(a, b, eps) (fabs((a)-(b)) < (eps))
    
/*浮点数相等比较*/
#define ireal_equal(a, b) ireal_equal_in(a, b, iepsilon)
    
/*与零比较*/
#define ireal_equal_zero(a) ireal_equal(a, 0)
    
/* 比较: 小于 */
#define ireal_less_than(a, b, eps) ((a-b) < -eps)
#define ireal_less(a, b) ireal_less_than(a, b, iepsilon)
    
/* 比较: 大于 */
#define ireal_greater_than(a, b, eps) ((a-b) > eps)
#define ireal_greater(a, b) ireal_greater_than(a, b, iepsilon) 
    
/*小于 0*/
#define ireal_less_zero(a) ireal_less_than(a, 0, iepsilon)
    
/*大于 0*/
#define ireal_greater_zero(a) ireal_greater_than(a, 0, iepsilon)

/* 编号 */
typedef int64_t iid;
    
/* type define */
typedef unsigned char ibyte;
typedef unsigned char ibool;
    
/* return the next pow of two */
int inextpot(int size);
    
/* sleeping the current thread */
void isleep(unsigned int milliseconds);
    
/*************************************************************/
/* imutex                                                    */
/*************************************************************/

/* recursive mutex */
typedef struct imutex {
#ifdef WIN32
    HANDLE _mutex;
#else
    pthread_mutex_t _mutex;
#endif
}imutex;
    
/*create resource*/
void imutexinit(imutex *mutex);
/*release resource*/
void imutexrelease(imutex *mutex);

/*lock mutex*/
void imutexlock(imutex *mx);
/*unlock mutex*/
void imutexunlock(imutex *mx);
    
/*************************************************************/
/* iatomic                                                   */
/*************************************************************/
/* [asf](https://svn.apache.org/repos/asf/avro/trunk/lang/c/src/avro/refcount.h) */
    
/* compare the store with expected, than store the value with desired */
uint32_t iatomiccompareexchange(volatile uint32_t *store, uint32_t expected, uint32_t desired);

/* fetch the old value and store the with add*/
uint32_t iatomicadd(volatile uint32_t *store, uint32_t value);

/* fetch the old value, than do exchange operator */
uint32_t iatomicexchange(volatile uint32_t *store, uint32_t value);
    
/* atomic increment, return the new value */
uint32_t iatomccincrement(volatile uint32_t *store);

/* atomic decrement, return the new value */
uint32_t iatomicdecrement(volatile uint32_t *store);
    
/*************************************************************/
/* ibase64                                                   */
/*************************************************************/

/* caculating enough space for the base64 algorithm */
#define ibase64_encode_out_size(s)	(((s) + 2) / 3 * 4 + 1) /*pendding a zero: c-style-ending*/
#define ibase64_decode_out_size(s)	(((s)) / 4 * 3)
   
/* base64 encode */
int ibase64encode_n(const unsigned char *in, size_t inlen, char *out, size_t *outsize);
/* base64 decode */
int ibase64decode_n(const char *in, size_t inlen, unsigned char *out, size_t *outsize);

/*************************************************************/
/* ipos                                                      */
/*************************************************************/

/* 坐标 */
typedef struct ipos {
    ireal x, y;
}ipos;
    
/* zero point */
extern const ipos kipos_zero;

/* 计算距离的平方 */
ireal idistancepow2(const ipos *p, const ipos *t);
    
/*************************************************************/
/* ipos                                                      */
/*************************************************************/
typedef struct ipos3 {
    ireal x, y, z;
}ipos3;
    
/* zero of ipos3 */
extern const ipos3 kipos3_zero;
    
/* 计算距离的平方 */
ireal idistancepow3(const ipos3 *p, const ipos3 *t);
   
/* get the xy from the p with xz */
void ipos3takexz(const ipos3 *p, ipos *to);

/*************************************************************/
/* ivec2                                                     */
/*************************************************************/

/* 向量,  完善基本的数学方法:
 *  加法 ; 减法 ; 乘法 ; 除法 ; 点积(内积) ; 乘积(外积) ; 长度
 * */
typedef union ivec2 {
    ireal values[2];
    struct {
        ireal x, y;
    } v;
}ivec2;

/* 两点相减得到向量 */
ivec2 ivec2subtractpoint(const ipos *p0, const ipos *p1);
    
/* 把点在这个方向上进行移动 */
ipos ivec2movepoint(const ivec2 *dir, ireal dist, const ipos *p);

/* 加法*/
ivec2 ivec2add(const ivec2 *l, const ivec2 *r);

/* 减法 */
ivec2 ivec2subtract(const ivec2 *l, const ivec2 *r);

/* 乘法 */
ivec2 ivec2multipy(const ivec2 *l, const ireal a);

/* 点积 */
ireal ivec2dot(const ivec2 *l, const ivec2 *r);

/* 乘积 : 二维向量不存在叉积
 * ivec2 ivec2cross(const ivec2 *l, const ivec2 *r);
 * */

/* 长度的平方 */
ireal ivec2lengthsqr(const ivec2 *l);

/* 长度 */
ireal ivec2length(const ivec2 *l);

/* 绝对值 */
ivec2 ivec2abs(const ivec2* l);

/* 归一化 */
ivec2 ivec2normalize(const ivec2 *l);

/* 平行分量, 确保 r 已经归一化 */
ivec2 ivec2parallel(const ivec2 *l, const ivec2 *r);

/* 垂直分量, 确保 r 已经归一化 */
ivec2 ivec2perpendicular(const ivec2 *l, const ivec2 *r);

/*************************************************************/
/* ivec3                                                     */
/*************************************************************/

/* 向量 完善基本的数学方法 */
typedef union ivec3 {
    ireal values[3];
    struct {
        ireal x, y, z;
    }v;
}ivec3;

/* 两点相减得到向量 */
ivec3 ivec3subtractpoint(const ipos3 *p0, const ipos3 *p1);

/* 加法*/
ivec3 ivec3add(const ivec3 *l, const ivec3 *r);

/* 减法 */
ivec3 ivec3subtract(const ivec3 *l, const ivec3 *r);

/* 乘法 */
ivec3 ivec3multipy(const ivec3 *l, const ireal a);

/* 点积 */
ireal ivec3dot(const ivec3 *l, const ivec3 *r);

/* 乘积 */
ivec3 ivec3cross(const ivec3 *l, const ivec3 *r);

/* 长度的平方 */
ireal ivec3lengthsqr(const ivec3 *l);

/* 长度 */
ireal ivec3length(const ivec3 *l);

/* 绝对值 */
ivec3 ivec3abs(const ivec3* l);

/* 归一*/
ivec3 ivec3normalize(const ivec3 *l);

/* 平行分量, 确保 r 已经归一化 */
ivec3 ivec3parallel(const ivec3 *l, const ivec3 *r);

/* 垂直分量, 确保 r 已经归一化 */
ivec3 ivec3perpendicular(const ivec3 *l, const ivec3 *r);

/*************************************************************/
/* iline2d                                                   */
/*************************************************************/
typedef struct iline2d {
    ipos start;
    ipos end;
}iline2d;

/* start ==> end */
ivec2 iline2ddirection(const iline2d *line);

/* start ==> end , rorate -90 */
ivec2 iline2dnormal(const iline2d *line);

/**/
ireal iline2dlength(const iline2d *line);

/*
 * Determines the signed distance from a point to this line. Consider the line as
 * if you were standing on start of the line looking towards end. Posative distances
 * are to the right of the line, negative distances are to the left.
 * */
ireal iline2dsigneddistance(const iline2d *line, const ipos *point);

/*
 * point classify
 * */
typedef enum EnumPointClass{
    EnumPointClass_On,      /* The point is on, or very near, the line  */
    EnumPointClass_Left,    /* looking from endpoint A to B, the test point is on the left */
    EnumPointClass_Right    /* looking from endpoint A to B, the test point is on the right */
}EnumPointClass;

/*
 * Determines the signed distance from a point to this line. Consider the line as
 * if you were standing on PointA of the line looking towards PointB. Posative distances
 * are to the right of the line, negative distances are to the left.
 * */
int iline2dclassifypoint(const iline2d *line, const ipos *point, ireal epsilon); 

/* 
 * line classify
 * */
typedef enum EnumLineClass {
    EnumLineClass_Collinear,			/* both lines are parallel and overlap each other */
    EnumLineClass_Lines_Intersect,      /* lines intersect, but their segments do not */
    EnumLineClass_Segments_Intersect,	/* both line segments bisect each other */
    EnumLineClass_A_Bisects_B,          /* line segment B is crossed by line A */
    EnumLineClass_B_Bisects_A,          /* line segment A is crossed by line B */
    EnumLineClass_Paralell              /* the lines are paralell */
}EnumLineClass;

/*
 * Determines if two segments intersect, and if so the point of intersection. The current
 * member line is considered line AB and the incomming parameter is considered line CD for
 * the purpose of the utilized equations.
 *
 * A = PointA of the member line
 * B = PointB of the member line
 * C = PointA of the provided line
 * D = PointB of the provided line
 * */
int iline2dintersection(const iline2d *line, const iline2d *other,  ipos *intersect);
    
/* Caculating the closest point in the segment to center pos */
ipos iline2dclosestpoint(const iline2d *line, const ipos *center, ireal epsilon);
    
/*************************************************************/
/* iline3d                                                   */
/*************************************************************/
typedef struct iline3d {
    ipos3 start;
    ipos3 end;
}iline3d;

/* start ==> end */
ivec3 iline3ddirection(const iline3d *line);

/**/
ireal iline3dlength(const iline3d *line);
    
/* Caculating the closest point in the segment to center pos */
ipos3 iline3dclosestpoint(const iline3d *line, const ipos3 *center, ireal epsilon);

   
/*************************************************************/
/* iplane                                                    */
/*************************************************************/
    
/*
 * A Plane in 3D Space represented in point-normal form (Ax + By + Cz + D = 0).
 * The convention for the distance constant D is:
 * D = -(A, B, C) dot (X, Y, Z) */
typedef struct iplane {
    ivec3 normal;
    ipos3 pos;
    ireal distance;
}iplane;
    
/* Setup Plane object given a clockwise ordering of 3D points */
void iplaneset(iplane *plane, const ipos3 *a, const ipos3 *b, const ipos3 *c);
    
/* TODO */
ireal iplanesigneddistance(const iplane *plane, const ipos3 *p);
    
/* Given Z and Y, Solve for X on the plane */
ireal iplanesolveforx(iplane *plane, ireal y, ireal z);
    
/* Given X and Z, Solve for Y on the plane */
ireal iplanesolvefory(iplane *plane, ireal x, ireal z);
    
/* Given X and Y, Solve for Z on the plane */
ireal iplanesolveforz(iplane *plane, ireal x, ireal y);

/*************************************************************/
/* isize                                                     */
/*************************************************************/

/* 大小 */
typedef struct isize {
    ireal w, h;
}isize;

/*************************************************************/
/* irect                                                    */
/*************************************************************/

/* 矩形 */
typedef struct irect {
    ipos pos;
    isize size;
}irect;

/* 矩形包含: iiok, iino */
int irectcontains(const irect *con, const irect *r);
/* 矩形包含: iiok, iino */
int irectcontainspoint(const irect *con, const ipos *p);
    
/* down-left pos*/
ipos irectdownleft(const irect *con);
/* down-right pos*/
ipos irectdownright(const irect *con);
/* up-left pos*/
ipos irectupleft(const irect *con);
/* up-right pos*/
ipos irectupright(const irect *con);

/*************************************************************/
/* icircle                                                   */
/*************************************************************/

/* 圆形 */
typedef struct icircle {
    ipos pos;
    ireal radius;
}icircle;

/*圆形的关系 */
typedef enum EnumCircleRelation {
    EnumCircleRelationBContainsA = -2,
    EnumCircleRelationAContainsB = -1,
    EnumCircleRelationNoIntersect = 0,
    EnumCircleRelationIntersect = 1,
} EnumCircleRelation;

/* 圆形相交: iiok, iino */
int icircleintersect(const icircle *con, const icircle *c);
/* 圆形包含: iiok, iino */
int icirclecontains(const icircle *con, const icircle *c);
/* 圆形包含: iiok, iino */
int icirclecontainspoint(const icircle *con, const ipos *p);
/* 圆形的关系: EnumCircleRelationBContainsA(c包含con), */
/*    EnumCircleRelationAContainsB(con包含c), */
/*    EnumCircleRelationIntersect(相交), */
/*    EnumCircleRelationNoIntersect(相离) */
int icirclerelation(const icircle *con, const icircle *c);

/* 矩形与圆是否相交 */
int irectintersect(const irect *con, const icircle *c);
    
/* Caculating the offset that circle should moved to avoid collided with the line */
ivec2 icircleoffset(const icircle* circle, const iline2d* line);

/* 名字的最大长度 */
#define IMaxNameLength 32

/*************************************************************/
/* iname                                                     */
/*************************************************************/

/* 名字 */
typedef struct iname {
    char name[IMaxNameLength+1];
}iname;

/*************************************************************/
/* imeta                                                     */
/*************************************************************/

/* 内存操作 */
#define icalloc(n, size) calloc(n, size)
#define irealloc(ptr, size) realloc(ptr, size)
#define ifree(p) free(p)

#if iimeta  /* if iimeta */

/* 最多支持100个类型对象 */
#define IMaxMetaCountForUser 100

/* 前置声明 */
struct imeta;
struct iobj;
    
/* tracing the iobj alloc */
typedef void (*ientryobjcalloctrace)(struct imeta *meta, struct iobj *obj);
/* tracing the iobj free */
typedef void (*ientryobjfreetrace)(struct imeta *meta, struct iobj *obj);
/* make all iobj has the hash values */
typedef int (*ientryobjhash)(struct imeta *meta, struct iobj *obj);
/* make all iobj can be compare with each other */
typedef int (*ientryobjcompare)(struct imeta *meta, struct iobj *lfs, struct iobj *rfs);

/* 基础的内存对象, 都具备缓冲功能 */
typedef struct iobj {
    int size;
    struct imeta *meta;
    struct iobj *next;
    char addr[];
}iobj;

/* 基础内存对象缓存 */
typedef struct iobjcache {
    struct iobj *root;
    int length;
    int capacity;
}iobjcache;

/* 编码对象的元信息 */
typedef struct imeta {
    const char* name;
    struct iobjcache cache;
    int size;
    int64_t current;
    int64_t alloced;
    int64_t freed;
    
    /* trace all obj calloc */
    ientryobjcalloctrace tracecalloc;
    /* trace all obj free */
    ientryobjfreetrace tracefree;
    
    /*all iobj can be do hash */
    ientryobjhash _hash;
    /*all iobj can be do compare */
    ientryobjcompare _compare;
    
/* support thread safe for meta system */
#if iithreadsafe
    imutex mutex; /*will never release resouce until program ended */
#endif
}imeta;
    
/*Hepler Macro for log*/
#define __imeta_format "Meta-Obj: (%15.15s, %5d) --> alloc: %9lld, free: %9lld, hold: %9lld - count: %8lld - cache(%6d/%6d)"
#define __imeta_value(meta) (meta).name,(meta).size,(meta).alloced,(meta).freed,imax((meta).current, 0),imax((meta).current,0)/((meta).size+sizeof(iobj)),(meta).cache.length,(meta).cache.capacity
    
/* 获取类型的元信息 */
imeta *imetaget(int idx);

/* 也可以手动注册一个元信息来管理自己的对象: 然后就可以通过 iobjmalloc 进行对象内存管理 */
/* 将会返回对象的meta索引 */
int imetaregister(const char* name, int size, int capacity);
/* 获取索引 */
#define imetaindex(type) imeta_##type##_index
/* 注册宏 */
#define iregister(type, capacity) imetaregister(#type, sizeof(type), capacity)
/* 声明 */
#define irealdeclareregister(type) int imetaindex(type)
/* 声明 */
#define iideclareregister(type) extern irealdeclareregister(type)
/* 注册宏 */
#define irealimplementregister(type, capacity) imetaindex(type) = iregister(type, capacity)
/* 注册宏 */
#define iimplementregister(type, capacity) int irealimplementregister(type, capacity)
/* 获取meta */
#define imetaof(type) imetaget(imetaindex(type))


/* 定义所有内部对象的meta索引 */
#define __ideclaremeta(type, capacity) imetaindex(type)

#define __iallmeta                            \
    __ideclaremeta(iobj, 0),                  \
    __ideclaremeta(iref, 0),                  \
    __ideclaremeta(iwref, 0),                 \
    __ideclaremeta(irangeite, 0),             \
    __ideclaremeta(ireflist, 1000),           \
    __ideclaremeta(irefjoint, 200000),        \
    __ideclaremeta(inode, 4000),              \
    __ideclaremeta(iunit, 2000),              \
    __ideclaremeta(imap, 0),                  \
    __ideclaremeta(irefcache, 0),             \
    __ideclaremeta(ifilter, 2000),            \
    __ideclaremeta(isearchresult, 0),         \
    __ideclaremeta(irefautoreleasepool, 0),   \
    __ideclaremeta(iarray, 0),                \
    __ideclaremeta(islice, 0),                \
    __ideclaremeta(iringbuffer, 0),           \
    __ideclaremeta(idict, 0),                 \
    __ideclaremeta(ipolygon3d, 0),            \
    __ideclaremeta(ipolygon2d, 0)
    

/* 定义所有元信息索引 */
typedef enum EnumMetaTypeIndex {
    __iallmeta,
   EnumMetaTypeIndex_imax,
}EnumMetaTypeIndex;

/* 获取响应的内存：会经过Meta的Cache */
void *iaoicalloc(imeta *meta);

/* 释放内存：会经过Meta的Cache */
void iaoifree(void *p);

/* 尽可能的释放Meta相关的Cache */
void iaoicacheclear(imeta *meta);

/* 打印当前内存状态 */
void iaoimemorystate();

/* 获取指定对象的meta信息 */
imeta *iaoigetmeta(const void *p);

/* 指定对象是响应的类型: 一致返回iiok, 否则返回 iino */
int iaoiistype(const void *p, const char* type);

/* Trace the memory size */
typedef enum EnumAoiMemoerySizeKind {
    EnumAoiMemoerySizeKind_Alloced,
    EnumAoiMemoerySizeKind_Freed,
    EnumAoiMemoerySizeKind_Hold,
} EnumAoiMemoerySizeKind;

/*获取当前的总的内存统计*/
int64_t iaoimemorysize(void *meta, int kind);

#define imetacacheclear(type) (iaoicacheclear(imetaof(type)))

#define iobjmalloc(type) ((type*)iaoicalloc(imetaof(type)))
#define iobjfree(p) do { iaoifree(p); p = NULL; } while(0)
#define iobjistype(p, type) iaoiistype((void*)p, #type)
#define iistype(p, type) (iaoigetmeta(p) == imetaof(type))

#else   /* #if iimeta */

/* 打印当前内存状态: 空的内存状态 */
void iaoimemorystate() ;
    
int64_t iaoimemorysize(void * meta, int kind);

#define iobjmalloc(type) ((type*)icalloc(1, sizeof(type)))
#define iobjfree(p) do { ifree(p); p = NULL; } while(0)
#define iobjistype(p, type) iino
#define iistype(p, type) iino

#endif  /* #if iimeta */

/*************************************************************/
/* iref                                                      */
/*************************************************************/

/* 定义引用计数，基础对象 */
#define irefdeclare volatile uint32_t ref; volatile struct iwref * wref; struct irefcache* cache; ientryfree free; ientrywatch watch
/* iref 转换成 target */
#define icast(type, v) ((type*)(v))
/* 转换成iref */
#define irefcast(v) icast(iref, v)

/* 前置声明 */
struct iref;
struct irefcache;
struct iwref;

/* iref 的析构函数 */
typedef void (*ientryfree)(struct iref* ref);

/* iref 的跟踪函数: 引用计数减小的时候会通知 */
typedef void (*ientrywatch)(struct iref* ref);

/* 引用计数结构体 */
typedef struct iref {
    irefdeclare;
}iref;

/* 增加引用计数 */
int irefretain(iref *ref);

/* 释放引用计数 */
void irefrelease(iref *ref);

/* 引用宏 */
#define iretain(p) do { if(p) irefretain((iref*)(p)); } while(0)

/* 释放宏 */
#define irelease(p) do { if(p) irefrelease((iref*)(p)); } while(0)

/* 应用计数的赋值操作 */
#define iassign(dst, src) do { if(src != dst) { irelease(dst); iretain(src); dst = src; } } while(0)
    
/*************************************************************/
/* iwref                                                     */
/*************************************************************/

/* 弱引用: we can do operators as iref: iretain; irelease; iassign */
/* the iwref not thread safe: only use iwref in one thread context */
typedef struct iwref {
    irefdeclare;
}iwref;
    
/* make a weak iref by ref */
iwref *iwrefmake(iref *ref);
    
/* make a weak iref by wref */
iwref *iwrefmakeby(iwref *wref);

/* make strong ref: need call irelease */
iref *iwrefstrong(iwref *wref);
    
/* make strong ref: unneed call irelease */
iref *iwrefunsafestrong(iwref *wref);
   
/* ref assign to weak ref */
#define iwassign(dst, src) do { if (dst && (iref*)(dst->wref) == (iref*)(src)) { \
    break; } irelease(dst); dst = iwrefmake((iref*)(src)); } while(0)

/*************************************************************/
/* irefautoreleasepool                                       */
/*************************************************************/

/* 前置声明 */
struct ireflist;

/* 自动释放池子 */
typedef struct irefautoreleasepool {
    struct ireflist *list;
}irefautoreleasepool;

/* 申请自动释放池子 */
irefautoreleasepool * irefautoreleasebegin();

/* 自动释放 */
iref* irefautorelease(irefautoreleasepool *pool, iref *ref);

/* 结束自动释放 */
void irefautoreleaseend(irefautoreleasepool *pool);

/* 返回值本身的，引用retain */
iref *irefassistretain(iref *ref);

/* 便利宏用来使用autoreleasepool */
#define _iautoreleasepool irefautoreleasepool* pool = irefautoreleasebegin()

#define _iautomalloc(type) ((type*)irefautorelease(pool, irefassistretain((iref*)iobjmalloc(type))))

#define _iautorelease(p) irefautorelease(pool, (iref*)p)

#define _iautoreleaseall irefautoreleaseend(pool)

/*************************************************************/
/* ireflist                                                  */
/*************************************************************/

/* 节点 */
typedef struct irefjoint {
    /* 附加的对象 */
    iref *value;
    
    /* 附加在上的 资源*/
    void *res;

    /* 必要的校验 */
    struct ireflist *list;

    /* 列表节点 */
    struct irefjoint *next;
    struct irefjoint *pre;
}irefjoint;

/* 构造列表节点 */
irefjoint* irefjointmake(iref *value);

/* 释放列表节点 */
void irefjointfree(irefjoint* joint);
    
/* 释放附加在列表节点上的资源 */
typedef void (*irefjoint_entry_res_free)(irefjoint *joint);

/* 引用对象列表 */
typedef struct ireflist {
    irefdeclare;
    
    /* 列表根节点, 也是列表的第一个节点 */
    irefjoint *root;
    /* 列表长度 */
    size_t length;
    /* 时间 */
    int64_t tick;
    /* free the res append in list */
    irefjoint_entry_res_free entry;
}ireflist;

/* 创建列表 */
ireflist *ireflistmake();
    
/* 释放列表 */
void ireflistfree(ireflist *list);
    
/* 创建列表 */
ireflist *ireflistmakeentry(irefjoint_entry_res_free entry);

/* 获取列表长度 */
size_t ireflistlen(const ireflist *list);

/* 获取第一个节点 */
irefjoint* ireflistfirst(const ireflist *list);

/* 从列表里面查找第一个满足要求的节点 */
irefjoint* ireflistfind(const ireflist *list,
        const iref *value);

/* 往列表增加节点: 前置节点 */
irefjoint* ireflistaddjoint(ireflist *list, irefjoint * joint);

/* 往列表增加节点: 前置节点(会增加引用计数) */
irefjoint* ireflistadd(ireflist *list, iref *value);
    
/* 往列表增加节点: 前置节点(会增加引用计数) */
irefjoint* ireflistaddres(ireflist *list, iref *value, void *res);

/* 从节点里面移除节点, 返回下一个节点 */
irefjoint* ireflistremovejoint(ireflist *list, irefjoint *joint);
/* 从节点里面移除节点 , 并且释放当前节点, 并且返回下一个节点 */
irefjoint* ireflistremovejointandfree(ireflist *list, irefjoint *joint);
/* 从节点里面移除节点: 并且会释放节点, 返回下一个节点 */
irefjoint* ireflistremove(ireflist *list, iref *value);

/* 释放所有节点 */
void ireflistremoveall(ireflist *list);

    
/*************************************************************/
/* irefneighbors                                             */
/*************************************************************/
typedef struct irefneighbors {
    irefdeclare;
    /*
     * 构成了一个有向图，可在联通上做单向通行
     * */
    /* 所有可以到达当前节点的邻居 other ===> this */
    ireflist *neighbors_from;
    /* 可走的列表 this ===> other */
    ireflist *neighbors_to;
    
    /* List joint resouce free entry */
    irefjoint_entry_res_free neighbors_resfree;
}irefneighbors;
    
/*macro declare*/
#define irefneighborsdeclare \
    irefdeclare; \
    ireflist *neighbors_from; \
    ireflist *neighbors_to; \
    irefjoint_entry_res_free neighbors_resfree
    
/* 设置邻居间关系描述的 释放符号 */
void ineighborsbuild(irefneighbors *neighbors, irefjoint_entry_res_free entry);
    
/* 从节点图里面移除 */
void ineighborsclean(irefneighbors *neighbors);

/* 在有向图上加上一单向边 */
void ineighborsadd(irefneighbors *from, irefneighbors *to);
    
/* 在有向图上加上一单向边 */
void ineighborsaddvalue(irefneighbors *from, irefneighbors *to, void *from_to, void *to_from);

/* 在有向图上移除一条单向边 */
void ineighborsdel(irefneighbors *from, irefneighbors *to);
    

/*************************************************************/
/* iarray                                                    */
/*************************************************************/
struct iarray;
struct islice;
    
/* 如果是需要跟 kindex_invalid 进行交换就是置0 */
/* invalid index */
extern const int kindex_invalid;

/* 交换两个对象 */
typedef void (*iarray_entry_swap)(struct iarray *arr,
        int i, int j);
/* 比较两个对象 */
typedef int (*iarray_entry_cmp)(struct iarray *arr,
        int i, int j);
/* 赋值  */
typedef void (*iarray_entry_assign)(struct iarray *arr,
        int i, const void *value, int nums);
/* 遍历 访问 */
typedef void (*iarray_entry_visitor)(const struct iarray *arr,
    int i, const void *value);

/* 数组常用控制项 */
typedef enum EnumArrayFlag {
    EnumArrayFlagNone = 0,

    /* 移除元素的时候，
     * 不移动数组，直接从后面替补 * */
    EnumArrayFlagKeepOrder = 1<<1,  /*是否保持有序*/

    /*是否是简单数组,
     *单元不需要通过swap或者assign去释放
     *truncate 的时候可以直接设置长度
     *简单数组实现的assign 函数必须实现 memmove 语义
     *memmove 可以处理内存重叠问题 
     *标志位只有设置在数组的entry才有效，不可被更改*/
    EnumArrayFlagSimple = 1<<2,

    /*自动缩减存储容量*/
    EnumArrayFlagAutoShirk = 1<<3,
    
    /* MemSet Pennding Memory */
    EnumArrayFlagMemsetZero = 1<<4,
    
    /* 数组被slice 操作过, 数组不能执行shirk, remove, truncate 操作
     * 数组的 insert 操作如果导致需要数组扩容也会失败*/
    EnumArrayFlagSliced = 1<<5,
}EnumArrayFlag;

/* 数组基础属性, 类型元信息 */
typedef struct iarrayentry{
    int flag;
    size_t size;
    iarray_entry_swap swap;
    iarray_entry_assign assign;
    iarray_entry_cmp cmp;
} iarrayentry;

/* 通用数组 */
typedef struct iarray {
    irefdeclare;

    size_t capacity;
    size_t len;
    char *buffer;
    int flag;
    iarray_entry_cmp cmp;

    /* 每一种数组类型都需要定义这个 */
    const iarrayentry* entry;
    
    /* user data appending to array*/
    void *userdata;
}iarray;

/* 建立数组*/
iarray *iarraymake(size_t capacity, const iarrayentry *entry);

/* 释放 */
void iarrayfree(iarray *arr);

/* 长度 */
size_t iarraylen(const iarray *arr);

/* 容量*/
size_t iarraycapacity(const iarray *arr);

/* 查询 */
const void* iarrayat(const iarray *arr, int index);

/* 数组内存缓冲区 */
void* iarraybuffer(iarray *arr);

/* 设置标签, 返回操作前的标志 */
int iarraysetflag(iarray *arr, int flag);

/* 清理标签, 返回操作前的标志*/
int iarrayunsetflag(iarray *arr, int flag);

/* 是否具备标签 */
int iarrayisflag(const iarray *arr, int flag);

/* 删除 */
int iarrayremove(iarray *arr, int index);

/* 增加 */
int iarrayadd(iarray *arr, const void* value);
    
 /* 插入 */
int iarrayinsert(iarray *arr, int index, const void *value, int nums);
   
/* 设置 */
int iarrayset(iarray *arr, int index, const void *value);
    
/* 清理数组 */
void iarrayremoveall(iarray *arr);

/* 截断数组 */
void iarraytruncate(iarray *arr, size_t len);

/* 缩减容量 */
size_t iarrayshrinkcapacity(iarray *arr, size_t capacity);
    
/* 扩大容量 */
size_t iarrayexpandcapacity(iarray *arr, size_t capacity);

/* 排序 */
void iarraysort(iarray *arr);
    
/* for each */
void iarrayforeach(const iarray *arr, iarray_entry_visitor visitor);

/*************************************************************/
/* iheap（big heap）                                          */
/*************************************************************/

/*a heap is a array*/
typedef struct iarray iheap;

/* 建立 堆操作 */
void iheapbuild(iheap *heap);
    
/* 堆大小 */
size_t iheapsize(const iheap *heap);

/* 堆操作: 增加一个元素 */
void iheapadd(iheap *heap, const void *value);
    
/* 堆操作: 调整一个元素 */
void iheapadjust(iheap *heap, int index);

/* 堆操作: 获取堆顶元素 */
const void *iheappeek(const iheap *heap);
#define iheappeekof(heap, type) iarrayof(heap, type, 0)

/* 堆操作: 移除堆顶元素*/
void iheappop(iheap *heap);

/* 堆操作: 移除指定的位置的元素, 仍然保持堆 */
void iheapdelete(iheap *heap, int index);
    
/*************************************************************/
/* iarray: int, ireal, int64, char, iref                     */
/* iarray: iref                                              */
/* iarray: icircle, ivec2, ivec3, ipos, isize, irect         */
/*************************************************************/

/* 内置的整数数组 */
iarray* iarraymakeint(size_t capacity);
    
/* 浮点数组 */
iarray* iarraymakeireal(size_t capacity);
    
/* int64 数组*/
iarray* iarraymakeint64(size_t capacity);
    
/* char 数组*/
iarray* iarraymakechar(size_t capacity);
    
/* 用来告知 对象的坐标发生变化 */
typedef void (*irefarray_index_change) (iarray *arr, iref *ref, int index);
    
/* append to iarray with iref entry */
typedef struct irefarrayentry {
    irefarray_index_change indexchange;
} irefarrayentry;
    
/* 内置的引用数组 */
iarray* iarraymakeiref(size_t capacity);
    
/* 内置的引用数组 */
iarray* iarraymakeirefwithentry(size_t capacity, const irefarrayentry *refentry);
    
/* 内置的 ipos 数组*/
iarray* iarraymakeipos(size_t capacity);
    
/* 内置的 ipos3 数组*/
iarray* iarraymakeipos3(size_t capacity);
    
/* 内置的 isize 数组*/
iarray* iarraymakeisize(size_t capacity);
    
/* 内置的 irect 数组*/
iarray* iarraymakeirect(size_t capacity);
    
/* 内置的 icircle 数组*/
iarray* iarraymakeicircle(size_t capacity);
    
/* 内置的 ivec2 数组*/
iarray* iarraymakeivec2(size_t capacity);
    
/* 内置的 ivec3 数组*/
iarray* iarraymakeivec3(size_t capacity);
    
/* 辅助宏，获取*/
#define iarrayof(arr, type, i) (((type *)iarrayat(arr, i))[0])
    
/* Helper-Macro: For-Earch in c89 */
#define irangearraycin(arr, type, begin, end, key, value, wrap) \
    do { \
    for(key=begin; key<end; ++key) {\
        value = iarrayof(arr, type, key);\
        wrap;\
    } } while(0)
    
/* Helper-Macro: For-Earch in c89 */
#define irangearrayc(arr, type, key, value, wrap) \
    irangearraycin(arr, type, 0, iarraylen(arr), key, value, wrap)
    
/* Helper-Macro: For-Earch in cplusplus */
#define irangearrayin(arr, type, begin, end, wrap) \
    do { \
        int __key; \
        type __value; \
        irangearraycin(arr, type, begin, end, __key, __value, wrap); \
    } while(0)

/* Helper-Macro: For-Earch in cplusplus */
#define irangearray(arr, type, wrap) \
    irangearrayin(arr, type, 0, iarraylen(arr), wrap)

/*************************************************************/
/* islice                                                    */
/*************************************************************/
/*make the slice be a string or heap*/
typedef enum EnumSliceFlag {
    EnumSliceFlag_String = 1<<3,
}EnumSliceFlag;

/*
 * 与 slice 搭配的是 array
 * array 是固定容量的数组，容量不变
 * 数组不能执行 remove, shrink, truncate 操作
 **/
typedef struct islice {
    irefdeclare;

    iarray *array;
    
    /*
     * len = end - begin
     * real.capacity = capacity - begin 
     **/
    int begin;
    int end;
    int capacity;
    
    /* special flags */
    int flag;
}islice;
    
/* 遍历 访问 */
typedef void (*islice_entry_visitor)(const islice *slice,
    int i, const void *value);
    
/* 左闭右开的区间 [begin, end) */
islice *islicemake(iarray *arr, int begin, int end, int capacity);

/* 左闭右开的区间 [begin, end) */
islice *islicemakeby(islice *slice, int begin, int end, int capacity);
    
/* 通过参数创建 "begin:end:capacity"
 * islicemakearg(arr, ":")
 * islicemakearg(arr, ":1")
 * islicemakearg(arr, ":1:5")
 * islicemakearg(arr, "3:1:5")
 * islicemakearg(arr, "3:")
 */
islice *islicemakearg(iarray *arr, const char* args);
    
/* 通过参数创建 "begin:end:capacity"
 * islicemakeargby(arr, ":")
 * islicemakeargby(arr, ":1")
 * islicemakeargby(arr, ":1:5")
 * islicemakeargby(arr, "3:1:5")
 * islicemakeargby(arr, "3:")
 */
islice *islicemakeargby(islice *slice, const char* args);
    
  
/* 从array创建slice, 创建的slice 总数享有 父节点的最大容量 */
#define isliced(arr, begin, end) islicemake((arr), (begin), (end), (int)((arr)->capacity))
/* 从slice创建子slice, 创建的slice 总数享有 父节点的最大容量 */
#define islicedby(slice, begin, end) islicemakeby((slice), (begin), (end), islicecapacity(slice))
    
/* 参数 解析 */
void isliceparamsparse(int *params, const char* args, const char delim);
    
/* 释放 */
void islicefree(islice *slice);

/* 长度 */
size_t islicelen(const islice *slice);
    
/* 容量 */
size_t islicecapacity(const islice *slice);
    
/* 附加
 * 用法: slice = isliceappendvalues(slice, values, count); */
islice* isliceappendvalues(islice* slice, const void *values, int count);
    
/* 附加
 * 用法: slice = isliceappend(slice, append); */
islice* isliceappend(islice *slice, const islice *append);
   
/* 
 * 增加元素 
 * 用法 : slice = isliceadd(slice, i); */
islice* isliceadd(islice *slice, const void *value);
    
/* 设置值*/
int isliceset(islice *slice, int index, const void *value);
    
/* 查询 */
const void* isliceat(const islice *slice, int index);
    
/* foreach */
void isliceforeach(const islice *slice, islice_entry_visitor visitor);
    
/* 辅助宏，获取*/
#define isliceof(slice, type, i) (((type *)isliceat(slice, i))[0])
    
/* Helper-Macro: For-Earch in c89 */
#define irangeslicecin(arr, type, begin, end, key, value, wrap) \
    do { \
    for(key=begin; key<end; ++key) {\
        value = isliceof(arr, type, idx);\
        wrap;\
    } } while(0)
    
/* Helper-Macro: For-Each in c89*/
#define irangeslicec(slice, type, key, value, wrap) \
    irangeslicecin(slice, type, 0, islicelen(slice), key, value, wrap)
    
/* Helper-Macro: For-Each in cplusplus*/
#define irangeslicein(slice, type, begin, end, wrap) \
    do {\
        int __key;\
        type __value;\
        irangeslicecin(slice, type, begin, end, __key, __value, wrap);\
    } while(0)

/* Helper-Macro: For-Each in cplusplus*/
#define irangeslice(slice, type, wrap) \
    irangeslicein(slice, type, 0, islicelen(slice), wrap)
    
/*************************************************************/
/* irange                                                    */
/*************************************************************/
struct irangeite;
    
/* tell the container, about irangite create */
typedef void (*_irangeentry_create)(struct irangeite *ite);
/* free all resource about ite */
typedef void (*_irangeentry_free)(struct irangeite *ite);
/* do next move */
typedef int (*_irangeentry_next)(struct irangeite *ite);
/* give me the real value address */
typedef const void* (*_irangeentry_value)(struct irangeite *ite);
/* give me the real key key */
typedef const void* (*_irangeentry_key)(struct irangeite *ite);
    
/* iterator accessor */
typedef struct irangeaccess {
    _irangeentry_create accesscreate;
    _irangeentry_free accessfree;
    _irangeentry_next accessnext;
    _irangeentry_value accessvalue;
    _irangeentry_key accesskey;
    
}irangeaccess;
    
/* iterator over a range container */
typedef struct irangeite {
    irefdeclare;
    
    void *container;
    void *ite;
    
    const irangeaccess *access;
    int __internal;
}irangeite;
    
/* range operator, accept: iarray, islice, istring, iheap */
size_t irangelen(const void *p);

/* range get: accept: iarray, islice, istring, iheap */
const void* irangeat(const void *p, int index);

/* make a range ite over the container */
irangeite* irangeitemake(void* p, const irangeaccess* access);

/* free the range ite */
void irangeitefree(irangeite *ite);
    
/*iiok: iino*/
int irangenext(irangeite *ite);
    
/* returnt the value address */
const void *irangevalue(irangeite *ite);
    
/* returnt the key address */
const void *irangekey(irangeite *ite);
    
/* Helper-Macro: Get-Value */
#define irangeof(con, type, i) (((type *)irangeat(con, i))[0])
    
/* Helper-Macro: For-Earch in c89 */
#define irangecin(arr, type, begin, end, key, value, wrap) \
    do { \
        for(key=begin; key<end; ++key) {\
        value = irangeof(arr, type, key);\
        wrap;\
    } } while(0)

/* Helper-Macro: For-Earch in c89 */
#define irangec(arr, type, key, value, wrap) \
    irangecin(arr, type, 0, irangelen(arr), key, value, wrap)
    
/* Helper-Macro: For-Earch in cplusplus */
#define irangein(arr, type, begin, end, wrap) \
    do { \
        int __key = begin; \
        type __value;\
        irangecin(arr, type, begin, end, __key, __value, wrap); \
    } while(0)

/* Helper-Macro: For-Earch in cplusplus */
#define irange(arr, type, wrap) \
    irangein(arr, type, 0, irangelen(arr), wrap)
    
/*can deal with: iarray, islice, ireflist */
irangeite *irangeitemakefrom(void *p);
    
/* can not over container for delete */
#define irangeover(con, keytype, valuetype, wrap) \
    do {\
        irangeite *ite = irangeitemakefrom((void*)(con));\
        while (irangenext(ite) == iiok) { \
            keytype __key = ((keytype*)irangekey(ite))[0];\
            valuetype __value = ((valuetype*)irangevalue(ite))[0];\
            iunused(__key); iunused(__value); \
            wrap;\
        } \
        irangeitefree(ite);\
    } while(0)
    
/*************************************************************/
/* istring                                                   */
/*************************************************************/

typedef islice* istring;
  
/* declare the string in stack, no need to free */
#define ideclarestring(name, value) \
iarray name##_array = {1, NULL, NULL, NULL, NULL, strlen(value), strlen(value), (char*)value};\
islice name##_slice = {1, NULL, NULL, NULL, NULL, &name##_array, 0, strlen(value)};\
islice * name = & name##_slice
    
/*Make a string by c-style string */
istring istringmake(const char* s);
    
/*Make a string by s and len*/
istring istringmakelen(const char* s, size_t len);
    
/*Make a copy of s with c-style string*/
istring istringdup(const istring s);

/*Return the string length */
size_t istringlen(const istring s);

/*visit the real string buffer*/
const char* istringbuf(const istring s);
    
/*set the entry for stack string */
istring istringlaw(istring s);

/*format the string and return the value*/
/* This function is similar to sdscatprintf, but much faster as it does
 * not rely on sprintf() family functions implemented by the libc that
 * are often very slow. Moreover directly handling the sds string as
 * new data is concatenated provides a performance improvement.
 *
 * However this function only handles an incompatible subset of printf-alike
 * format specifiers:
 *
 * %s - C String
 * %i - signed int
 * %I - 64 bit signed integer (long long, int64_t)
 * %u - unsigned int
 * %U - 64 bit unsigned integer (unsigned long long, uint64_t)
 * %v - istring
 * %V - istring
 * %% - Verbatim "%" character.
 */
istring istringformat(const char* format, ...);
    
/*compare the two istring*/
int istringcompare(const istring lfs, const istring rfs);
    
/*find the index in istring */
int istringfind(const istring rfs, const char *sub, int len, int index);
    
/*sub string*/
istring istringsub(const istring s, int begin, int end);
    
/*return the array of istring*/
iarray* istringsplit(const istring s, const char* split, int len);
   
/*return the array of sting joined by dealer */
istring istringjoin(const iarray* ss, const char* join, int len);
    
/*return the new istring with new component*/
istring istringrepleace(const istring s, const char* olds, const char* news);
  
/*return the new istring append with value*/
istring istringappend(const istring s, const char* append);
    
/*baisc wrap for ::atoi */
int istringatoi(const istring s);
 
/*[cocos2dx](https://github.com/cocos2d/cocos2d-x/blob/v3/cocos/base/ccUtils.h)*/
/** Same to ::atof, but strip the string, remain 7 numbers after '.' before call atof.
 * Why we need this? Because in android c++_static, atof ( and std::atof ) 
 * is unsupported for numbers have long decimal part and contain
 * several numbers can approximate to 1 ( like 90.099998474121094 ), it will return inf. 
 * This function is used to fix this bug.
 * @param str The string be to converted to double.
 * @return Returns converted value of a string.
 */
double istringatof(const istring s);
    
    
/*************************************************************/
/* iringbuffer                                               */
/*************************************************************/
/* ringbuffer control flags */
typedef enum EnumRingBufferFlag {
    EnumRingBufferFlag_BlockRead = 1,  /* Block Read Will Got What We Needed */
    EnumRingBufferFlag_BlockWrite = 1<<1, /* Block Write Util We Got Empty Spaces */
    
    EnumRingBufferFlag_Override = 1<<2,   /* Override The Ring Buffer */
    
    EnumRingBufferFlag_ReadChannelShut = 1<<3,
    EnumRingBufferFlag_WriteChannelShut = 1<<4,
    
    EnumRingBufferFlag_ReadSleep = 1<<5,
    EnumRingBufferFlag_WriteSleep = 1<<6,
}EnumRingBufferFlag;
    
/* ring buffer */
/*[the ring buffer](https://github.com/JerryZhou/ringbuffer)*/
typedef struct iringbuffer {
    irefdeclare;
    
    /* char array */
    iarray *buf;
    /* the control flags */
    int flag;
    /* write cursor */
    size_t wcursor;
    /* read cursor */
    size_t rcursor;
    /* total write length */
    int64_t wlen;
    /* total read length */
    int64_t rlen;
} iringbuffer;
    
/* Make a ring buffer */
iringbuffer *iringbuffermake(size_t capacity, int flag);

/* release the ring buffer */
void iringbufferfree(iringbuffer *r);

/* close the ring buffer: can not read and write */
void iringbufferclose(iringbuffer *r);

/* shutdown the ringbuffer, to forbid the ringbuffer behavior */
void iringbuffershut(iringbuffer *r, int channel);

/* write s to ringbuffer, return count of write */
size_t iringbufferwrite(iringbuffer *r, const char* s, size_t len);

/* read to dst, until read full of dst, return the realy readed count */
size_t iringbufferread(iringbuffer *r, char * dst, size_t len);

/* return the ready count of content */
size_t iringbufferready(iringbuffer *r);

/* giving the raw buffer address, unsafe behavior */
const char* iringbufferraw(iringbuffer *r);

/* Print to rb: support
 * %s(c null end string),
 * %i(signed int),
 * %I(signed 64 bit),
 * %u(unsigned int),
 * %U(unsigned 64bit) */
size_t iringbufferfmt(iringbuffer *rb, const char * fmt, ...);
    
/*************************************************************/
/*  idict                                                    */
/*************************************************************/
/* [redis](https://github.com/antirez/redis) */
/* see details dict.h dict.c in redis */
    
/* dict type */
typedef struct idicttype {
    unsigned int (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} idicttype;

/* dict entry save in hash table */
typedef struct idictentry {
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct idictentry *next;
} idictentry;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct idicthashtable {
    idictentry **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
    
}idicthashtable;
    
/* Real has table structure */
typedef struct idict {
    irefdeclare;
    
    idicttype *type;
    void *privdata;
    idicthashtable ht[2];
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
    unsigned long iterators; /* number of iterators currently running */
} idict;
    
/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
typedef struct idictiterator {
    idict *d;
    long index;
    int table, safe;
    idictentry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    int64_t fingerprint;
} idictiterator;
    
/* the dict foreach scanner */
typedef void (*idictscanfunction)(void *privdata, const idictentry *de);
    
/* make a dict with type and privdata */
idict *idictmake(idicttype *type, void *privdata);

/* get value by key */
void *idictget(idict *d, const void *key);

/* set value by key */
/* Add an element, discarding the old if the key already exists.
 * Return 1 if the key was added from scratch, 0 if there was already an
 * element with such key and dictReplace() just performed a value update
 * operation. */
int idictset(idict *d, const void *key, void *value);
    
/* Search and remove an element */
int idictremove(idict *d, const void *key);

/* get the dict size */
size_t idictsize(idict *d);
    
/* clear all the key-values */
void idictclear(idict *d);
   
/*Resize the table to the minimal size that contains all the elements,
* but with the invariant of a USED/BUCKETS ratio near to <= 1*/
int idictresize(idict *d);

/* Rehash for an amount of time between ms milliseconds and ms+1 milliseconds */
int idictrehashmilliseconds(idict *d, int ms);

void idictenableresize(void);
void idictdisableresize(void);
    
/* if the dict has the key-value */
int idicthas(idict *d, const void *key);
    
/* scan the dict */
unsigned long idictscan(idict *d, unsigned long v, idictscanfunction fn, void *privdata);

/*all details idict: */
unsigned int idictgenhashfunction(const void *key, int len);
    
void idictsethashfunctionseed(uint32_t seed);
uint32_t idictgethashfunctionseed(void);
    
int idictdelete(idict *ht, const void *key);
int idictdeletenofree(idict *ht, const void *key);
    
idictentry *idictreplaceraw(idict *d, void *key);
    
idictiterator * idictgetsafeiterator(idict *d);
idictentry *idictNext(idictiterator *iter);
void idictreleaseiterator(idictiterator *iter);
    
idictentry *idictgetrandomkey(idict *d);
unsigned int idictgetsomekeys(idict *d, idictentry **des, unsigned int count);

/*************************************************************/
/* ipolygon3d                                                */
/*************************************************************/
    
/* polygon 3d definition */
typedef struct ipolygon3d {
    irefdeclare;
    
    /*ipos3 slice*/
    islice *pos;
    /* max point */
    ipos3 max;
    /* min point */
    ipos3 min;
    /* center point */
    ipos3 center;
    
    /* accumulating the pos */
    ipos3 accumulating;
    
    /* the polygon plane */
    iplane plane;
}ipolygon3d;
    
/* create a polygon 3d*/
ipolygon3d *ipolygon3dmake(size_t capacity);
    
/* free a polygon 3d*/
void ipolygon3dfree(ipolygon3d *);
    
/* add ivec3 to polygon*/
void ipolygon3dadd(ipolygon3d *poly, const ipos3 *v, int nums);
    
/* caculating the center of polygon3d  */
void ipolygon3dfinish(ipolygon3d *poly);
    
/* take the polygon3d as a wrap buffer of pos */
const ipos3 * ipolygon3dpos3(ipolygon3d *polygon, int index);
    
/* take the polygon3d pos (x, z) as a wrap buffer of pos */
ipos ipolygon3dposxz(ipolygon3d *polygon, int index);
    
/* the the edge (center-middle) point*/
ipos3 ipolygon3dedgecenter(ipolygon3d *polygon, int index);

/* if the point in polygon, just like 2d contains*/
/* Left Hand System
 * y     z
 * ^     ^
 * |    /
 * |   /
 * |  /
 * | /
 * |---------> x
 * */
int ipolygon3dincollum(const ipolygon3d *poly, const ipos3 *v);
    
/* give the projection rect in xz-plane */
void ipolygon3dtakerectxz(const ipolygon3d *poly, irect *r);
   
/*************************************************************/
/* ipolygon2d                                                */
/*************************************************************/
 
/* polygon 2d definition */
typedef struct ipolygon2d {
    irefdeclare;
    
    /*ivec2 slice*/
    islice *slice;
    ivec2 max;
    ivec2 min;
}ipolygon2d;
    
/* create a polygon 2d*/
ipolygon2d *ipolygon2dmake(size_t capacity);
    
/* free a polygon 2d*/
void ipolygon2dfree(ipolygon2d *);
    
/* add ivec2 to polygon*/
void ipolygon2dadd(ipolygon2d *poly, const ivec2 *v, int nums);
    
/* if the point in polygon*/
int ipolygon2dcontains(const ipolygon2d *poly, const ivec2 *v);

/*************************************************************/
/* irefcache                                                 */
/*************************************************************/

/* 构造函数 */
typedef iref* (*icachenewentry)();

/* 加入缓存的函数 */
typedef void (*icacheaddentry)(iref *ref);

/* 缓存弃守接口: 缓冲区放不下了就会调用这个 */
typedef void (*icacheenvictedentry)(iref *ref);

/* Cache */
/* 从缓存里面拿的东西，是需要释放的 */
/* 缓存参与引用计数对象的管理 */
typedef struct irefcache{
    iname name;

    ireflist* cache;
    size_t capacity;

    icachenewentry newentry;
    icacheenvictedentry envicted;
    icacheaddentry whenadd;
}irefcache;

/* 创造一个cache */
irefcache *irefcachemake(size_t capacity, icachenewentry newentry);

/* 从缓存里面取一个 */
iref *irefcachepoll(irefcache *cache);

/* 释放到缓存里面: 只有 ref 真正没有被其他用到的时候才会回收到缓冲区重复使用 */
/* 可以用来代替 irelease 的调用 */
/* 即使不是通过irefcachepoll 获取的对象也可以放入缓存里面只要类型一致 */
void irefcachepush(irefcache *cache, iref *ref);

/* 当前缓冲区清理, 不能直接操作里面的list */
void irefcacheclear(irefcache *cache);

/* 释放缓存 */
void irefcachefree(irefcache *cache);

/* 当前缓冲区的存储的对象个数 */
size_t irefcachesize(irefcache *cache);

/* 用宏处理缓存接口: 拿 */
#define icache(cache, type) ((type*)irefcachepoll(cache))
/* 用宏处理缓存接口: 放回 */
#define icacheput(cache, ref) irefcachepush(cache, (iref*)(ref))

/* 最大的分割次数 */
#define IMaxDivide 32

/*************************************************************/
/* icode                                                     */
/*************************************************************/

/* 编码, 以0结尾，c 风格字符串 */
typedef struct icode {
    char code[IMaxDivide+1];
    ipos pos;
}icode;

/*icode printf format*/
#define __icode_format "[code: %s pos:"__ipos_format"]"
#define __icode_value(c) (c).code,__ipos_value((c).pos)

/*************************************************************/
/* iuserdata                                                 */
/*************************************************************/

/* 自定义数据 */
typedef struct iuserdata {
    int u1, u2, u3, u4;
    void *up1, *up2, *up3, *up4;
}iuserdata;

/*************************************************************/
/* iunit                                                     */
/*************************************************************/

/* 前置声明 */
struct inode;
struct imap;

/* 基本单元状态 */
typedef enum EnumUnitState {
    EnumUnitStateNone = 0,
    EnumUnitStateDead = 1 << 3,
    EnumUnitStateFlying = 1<< 5,
    EnumUnitStateMoving = 1<< 6,
    EnumUnitStateSearching = 1<<10,
}EnumUnitState;
    
    /*内置的单元标志*/
typedef enum EnumUnitFlag {
    EnumUnitFlagNone = 0,
    EnumUnitFlagSkipSearching = 1<<2, /*单元需要跳过搜索*/
} EnumUnitFlag;

/* 单元 */
typedef struct iunit {
    irefdeclare;

    /* 名称 */
    iname name;
    /* ID */
    iid   id;

    /* 状态 */
    int64_t state;
    /* 更新时间(毫秒) */
    int64_t tick;
    /* 标志 */
    int64_t flag;

    /* 坐标 */
    ipos  pos;
    icode code;

#if iiradius
    /* 半径 */
    ireal radius;
#endif

    /* 用户数据 */
    iuserdata userdata;

    /* 在同一单元内部列表的关系 */
    struct iunit* next;
    struct iunit* pre;

    /* 所在的叶子节点 */
    struct inode* node;
}iunit;

/* 构造一个基本单元 */
iunit * imakeunit(iid id, ireal x, ireal y);

/* 重载：构造一个基本单元 */
iunit * imakeunitwithradius(iid id, ireal x, ireal y, ireal radius);

/* 释放一个基本单元 */
void ifreeunit(iunit *unit);

/* 释放链表 */
void ifreeunitlist(iunit *unit);

/*************************************************************/
/* inode                                                     */
/*************************************************************/

/* 节点分割的次数 */
#define IMaxChilds 4

/* 分割次数 */
#define IDivide 2

/* 二维空间 */
#define IDimension 2

/* 节点的状态 */
typedef enum EnumNodeState {
    EnumNodeStateNone = 0,
    /* 查找标记，在搜索的时候会短暂的标记 */
    EnumNodeStateSearching = 1<<5,

    /* 寻路的时候代表不可走，需要避开 */
    EnumNodeStateNoWalkingAble = 1<<12,
    /* 寻路的时候代表整个可以走 */
    EnumNodeStateWalkingAble = 1<<13,

    /* 节点需要Hold 住，不能释放 */
    EnumNodeStateStatic = 1<<23,
    /* 节点可不可以附加单元 */
    EnumNodeStateNoUnit = 1<<24,
}EnumNodeState;

/* 是否追踪更新时间戳 */
#define open_node_utick (1)

/* 节点 */
typedef struct inode {
    /* 声明为iref-neighbors */
    irefneighborsdeclare;
    /* 节点层级: 从1 开始（根节点为0） */
    int   level;
    /* 节点对应的起点坐标编码 code[level-1] */
    icode code;
    /* 相对父节点的索引 */
    int codei;
    /* 继承坐标系 */
    int x, y;

    /* 节点状态 */
    int64_t state;
    /* 管理时间戳(增加单元，减少单元，增加子节点，减少子节点会更新这个时间戳) */
    int64_t tick;
#if open_node_utick
    /* 更新时间戳(管理时间戳 + 单元移动时间戳) */
    int64_t utick;
#endif

    /* 四叉树 */
    struct inode *parent;
    struct inode *childs[IMaxChilds];

    /* 附加在上面的单元 */
    struct iunit *units;

    /* 记录节点状态 */
    int childcnt;
    int unitcnt;

    /* 所有叶子节点都串起来了: */
    /* 维护一个有序的叶子节点列表非常有用 */
    struct inode *pre;
    struct inode *next;

}inode;
    
/* inode format */
#define __inode_format "[code:"__icode_format" size:"__isize_format" level:%d]"
#define __inode_value(m, n) __icode_value((n).code),__isize_value((m).nodesizes[(n).level]),(n).level

/* 节点内存管理 */
inode * imakenode();

/* 释放节点本身 */
void ifreenode(inode *node);

/* 释放节点以及节点关联的单元 */
void ifreenodekeeper(inode *node);

/* 释放节点组成的树 */
void ifreenodetree(inode *node);
    
/*坐标是否在节点里面*/
int inodecontains(const struct imap *map, const inode *node, const ipos *pos);

/* 从子节点的时间戳更新父节点 */
int inodeupdatparenttick(inode *node);
    
/* 从单元身上获取更新时间戳*/
int inodeupdatetickfromunit(inode *node, iunit *unit);

/*************************************************************/
/* imap                                                     */
/*************************************************************/

/* 地图状态信息，统计数据 */
typedef struct imapstate {
    int64_t nodecount;
    int64_t leafcount;
    int64_t unitcount;
}imapstate;

/* 地图 */
typedef struct imap {
    /* 地图名称 */
    iname name;

    /* 地图基本信息，世界坐标系统的位置，以及大小 */
    ipos  pos;
    isize size;

#if iiradius
    /* 单位的最大半径 */
    ireal maxradius;
#endif

    /* 地图分割的层 */
    /* 512 的地图分割24 次以后的精度为：0.00003052 */
    int   divide;

    /* 地图编码库, 以及地图的精度信息 */
    char  code[IDimension][IDivide];
    isize nodesizes[IMaxDivide+1];
    /* 地图精度的平方: 用来做距离筛选 */
    ireal distances[IMaxDivide+1];

    /* 四叉树的根节点 */
    inode *root;

    /* 节点缓存 */
    irefcache *nodecache;

    /* 叶子节点 */
    inode *leaf;

    /* 地图状态信息 */
    imapstate state;

    /* 存储地图的原始阻挡的位图信息 bits-map*/
    char *blocks;
}imap;

/* 节点加入地图 */
int imapaddunitto(imap *map, inode *node, iunit *unit, int idx);

/* 从地图上移除 */
int imapremoveunitfrom(imap *map, inode *node, iunit *unit, int idx, inode *stop);

/* 尽可能的回收节点 */
int imaprecyclenodeaspossible(imap *map, inode *node, inode *stop);

/* 根据坐标生成code */
int imapgencode(const imap *map, const ipos *pos, icode *code);
int imapgencodewithlevel(const imap *map, const ipos *pos, icode *code, int level);

/* 计算Code */
/* y */
/* ^ */
/* | (B , D) */
/* | (A , C) */
/* -----------> x  */
/* y */
/* ^ */
/* | ((0, 1) , (1, 1)) */
/* | ((0, 0) , (1, 0)) */
/* -----------> x
 */
/* 从编码生成坐标 */
int imapgenpos(imap *map, ipos *pos, const icode *code);

/* 编码移动方向 */
typedef enum EnumCodeMove {
    EnumCodeMoveLeft,
    EnumCodeMoveRight,
    EnumCodeMoveDown,
    EnumCodeMoveUp,
    EnumCodeMoveMax
}EnumCodeMove;
/* 移动编码: 失败返回0, 成功返回移动的步骤数 */
int imapmovecode(const imap *map, icode *code, int way);

/* 建议 divide 不要大于 10*/
/* 生成一张地图数据 */
imap *imapmake(const ipos *pos, const isize *size, int divide);

/* 地图信息类型 */
typedef enum EenumMapState {
    EnumMapStateHead = 1,
    EnumMapStateTail = 1<<1,
    EnumMapStateBasic = 1<<2,
    EnumMapStatePrecisions = 1<<3,
    EnumMapStateNode = 1<<4,
    EnumMapStateUnit = 1<<5,

    /* 所有信息 */
    EnumMapStateAll = 0x7fffffff,
    /* 除了头部和尾部 */
    EnumMapStateAllNoHeadTail = 0x7fffffff
    & ~EnumMapStateHead
    & ~EnumMapStateTail,
    /* 不打印信息 */
    EnumMapStateNone = 0
}EenumMapState;

/* 打印地图状态信息 */
void imapstatedesc(const imap *map, int require,
                   const char* intag, const char *inhead);

/* 释放地图数据，释放附加在地图上的单元数据 */
void imapfree(imap *map);

/* 生成地图数据 */
int imapgen(imap *map);

/* 增加一个单元到地图上 */
int imapaddunit(imap *map, iunit *unit);
    
/* 把单元加入地图， 到指定的分割层次 */
int imapaddunittolevel(imap *map, iunit *unit, int level);

/* 从地图上移除一个单元 */
int imapremoveunit(imap *map, iunit *unit);
    
/* direct remove the unit from the unit->node */
int imapremoveunitdirect(imap *map, iunit *unit);

/* 从地图上检索节点 */
inode *imapgetnode(const imap *map, const icode *code,
        int level, int find);

/* 更新一个单元在地图上的数据 */
int imapupdateunit(imap *map, iunit *unit);

/* 更新一个单元的附加信息到地图数据上：现阶段就只更新了单元的半径信息 */
/* 如果单元改变了半径，需要调用这个函数刷新一下，才回立刻生效，不然等单位移动单元格后才生效*/
void imaprefreshunit(imap *map, const iunit *unit);

/* 设置块的状态 */
void imapsetblock(imap *map, int x, int y, int state);

/* 加载位图阻挡信息 sizeof(blocks) == (divide*divide + 7 ) / 8 */
void imaploadblocks(imap *map, char* blocks);

/* 获取块的状态 */
int imapgetblock(const imap *map, int x, int y);
    
/* 返回地图水平的分割层次：当前层次的分割单元大于给定的大小, 那么轴对齐的线段的断点最多在两个分割单元内部*/
int imapcontainslevelw(const imap *map, int level, ireal w);
/* 返回地图垂直的分割层次：当前层次的分割单元大于给定的大小, 那么轴对齐的线段的断点最多在两个分割单元内部*/
int imapcontainslevelh(const imap *map, int level, ireal h);
/* 返回地图的分割层次，当前分割层次可以把矩形包含进最多4个分割单元内*/
int imapcontainslevel(const imap *map, const irect* r);

/*************************************************************/
/* ifilter                                                   */
/*************************************************************/

/* 前置声明 */
struct ifilter;

/* 过滤器行为 */
typedef enum EnumFilterBehavior {
    EnumFilterBehaviorChooseRangeNode = 1,
}EnumFilterBehavior;

/* 过滤器入口函数 */
typedef int (*ientryfilter)(imap *map, const struct ifilter *filter, const iunit* unit);

/* 过滤器指纹入口 */
typedef int64_t (*ientryfilterchecksum)(imap *map, const struct ifilter *filter);
    
/* 包装线段过滤器参数 */
typedef  struct ifilterline {
    iline2d line;
    ireal epsilon;
}ifilterline;

/* 过滤器上下文 */
typedef struct ifilter {
    irefdeclare;
    /* 过滤器上下文 */
    struct {
        /* 基础属性 */
        union {
            icircle circle;
            irect rect;
            icode code;
            ifilterline line;
            int64_t id;
        }u;
        /* 复合过滤器 */
        ireflist *list;
    }s;

    /* 过滤器入口 */
    ientryfilter entry;
    /* 指纹 */
    ientryfilterchecksum entrychecksum;
    /* 选择合适的子节点 */
    int behavior;
}ifilter;

/* 指纹识别 */
int64_t ifilterchecksum(imap *map, const ifilter *d);

/* 释放节点o */
void ifilterfree(ifilter *filter);

/* 基础过滤器 */
ifilter *ifiltermake();

/* 往一个已有的过滤器里面添加过滤器 */
void ifilteradd(ifilter *filter, ifilter *added);

/* 移除子过滤器 */
void ifilterremove(ifilter *filter, ifilter *sub);

/* 移除所有子过滤器 */
void ifilterclean(ifilter *filter);

/* 通用过滤器入口 */
int ifilterrun(imap *map, const ifilter *filter, const iunit *unit);

/* circle 过滤器 */
ifilter *ifiltermake_circle(const ipos *pos, ireal range);

/* rect 过滤器 */
ifilter *ifiltermake_rect(const ipos *pos, const isize *size);
    
/* line2d 过滤器 */
ifilter *ifiltermake_line2d(const ipos *from, const ipos *to, ireal epsilon);

/* 搜集树上的所有单元, 调用完后必须调用imapcollectcleanunittag */
void imapcollectunit(imap *map, const inode *node, ireflist *list, const ifilter *filter, ireflist *snap);
/* 清除搜索结果标记 */
void imapcollectcleanunittag(imap *map, const ireflist *list);
/* 清除搜索结果标记 */
void imapcollectcleannodetag(imap *map, const ireflist *list);

/*************************************************************/
/* isearchresult                                             */
/*************************************************************/

/* 搜索结果 */
typedef struct isearchresult {
    /* 单元 */
    ireflist* units;
    /* 过滤器 */
    ifilter * filter;

    /* 时间点 */
    int64_t tick;
    /* 上下文校验 */
    int64_t checksum;
    /* 快照 */
    ireflist* snap;
}isearchresult;

/* 创建搜索结果 */
isearchresult* isearchresultmake();

/* 对接过滤器 */
void isearchresultattach(isearchresult* result, ifilter *filter);

/* 移除过滤器 */
void isearchresultdettach(isearchresult *result);

/* 释放所有节点 */
void isearchresultfree(isearchresult *result);

/* 清理搜索的内容，后续的搜索会从新开始搜索 */
void isearchresultclean(isearchresult *result);

/* 从快照里面从新生成新的结果 */
void isearchresultrefreshfromsnap(imap *map, isearchresult *result);

/* 收集包含指定矩形局域的节点(最多4个) */
void imapsearchcollectnode(imap *map, const irect *rect, ireflist *list);
    
/* Collecting nodes that intersected with line with map radius */
void imapsearchcollectline(imap *map, const iline2d *line, ireflist *list);

/* 计算给定节点列表里面节点的最小公共父节点 */
inode *imapcaculatesameparent(imap *map, const ireflist *collects);

/* 从地图上搜寻单元 irect{pos, size{rangew, rangeh}}, 并附加条件 filter */
void imapsearchfromrectwithfilter(imap *map, const irect *rect,
                                  isearchresult *result, ifilter *filter);
    
/* Collecting units from nodes(collects) with the filter */
void imapsearchfromcollectwithfilter(imap *map, const ireflist* collects,
                                     isearchresult *result, ifilter *filter);

/* 从地图上搜寻单元 */
void imapsearchfrompos(imap *map, const ipos *pos,
                       isearchresult *result, ireal range);

/* 从地图上搜寻单元, 不包括自己 */
void imapsearchfromunit(imap *map, iunit *unit,
                        isearchresult *result, ireal range);

/* 搜索: 最后的搜索都会经过这里 */
void imapsearchfromnode(imap *map, const inode *node,
                        isearchresult* result, const ireflist *innodes);

/* 从地图上搜寻单元: 视野检测, 没有缓存的*/
void imaplineofsight(imap *map, const ipos *from,
                     const ipos *to, isearchresult *result);
    
/* 计算节点列表的指纹信息 */
int64_t imapchecksumnodelist(imap *map, const ireflist *list, int64_t *maxtick, int64_t *maxutick);

/*************************************************************/
/* print helper                                              */
/*************************************************************/

/* 打印树的时候携带的信息 */
typedef enum EnumNodePrintState {
    EnumNodePrintStateTick = 1,
    EnumNodePrintStateUnits = 1 << 1,
    EnumNodePrintStateMap = 1 << 2,
    EnumNodePrintStateNode = EnumNodePrintStateTick | EnumNodePrintStateUnits,
    EnumNodePrintStateAll = 0x7fffffff,
}EnumNodePrintState;

/* 打印地图 */
void _aoi_print(imap *map, int require);
/* 打印指定的节点*/
void _aoi_printnode(int require, const inode *node, const char* prefix, int tail);

/* 测试 */
int _aoi_test(int argc, char** argv);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* __AOI_H_ */


