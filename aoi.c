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
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

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


/* 状态操作宏 */
#define _state_add(value, state) value |= state
#define _state_remove(value, state) value &= ~state
#define _state_is(value, state) (value & state)

#ifndef __max
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

#if (iimeta)
/* 内存统计 */
volatile int64_t gcallocsize = 0;
volatile int64_t gfreesize = 0;
volatile int64_t gholdsize = 0;

#if (iithreadsafe)
static imutex *_imeta_mutex() {
    static imutex realmutex;
    static imutex *mutex = NULL;
    if (mutex == NULL) {
        mutex = &realmutex;
        imutexinit(mutex);
    }
    return mutex;
}
#define _imeta_global_lock imutexlock(_imeta_mutex())
#define _imeta_global_unlock imutexunlock(_imeta_mutex())
#define _imeta_lock imutexlock(&meta->mutex)
#define _imeta_unlock imutexunlock(&meta->mutex)
#else
#define _imeta_global_lock (void)0
#define _imeta_global_unlock (void)0
#define _imeta_lock (void)meta
#define _imeta_unlock (void)meta
#endif

#undef __ideclaremeta
#define __ideclaremeta(type, cap) {#type, {NULL, 0, cap}, sizeof(type), -1, 0, 0, NULL, NULL},
/* 所有类型的元信息系统 */
imeta gmetas[] = {__iallmeta(__ideclaremeta)
	__ideclaremeta(imeta, 0)
};

/* 所有自定义的类型原系统 */
imeta gmetasuser[IMaxMetaCountForUser] = {{0}};

#undef __ideclaremeta
#define __ideclaremeta(type, cap) EnumMetaTypeIndex_##type

#ifndef __countof
#define __countof(array) (sizeof(array)/sizeof(array[0]))
#endif

/* 内置的meta个数 */
static int const gmetacount = __countof(gmetas);
static int gmetacountuser = 0;

/* 获取类型的元信息 */
imeta *imetaget(int idx) {
    imeta *meta = NULL;
    
	if (idx < 0) {
		return NULL;
	}
	if (idx < gmetacount) {
		meta = &gmetas[idx];
        /*take current as the mark for first time*/
        if (meta->current == -1) {
            meta->current = 0;
#if (iithreadsafe)
            imutexinit(&meta->mutex);
#endif
        }
        return meta;
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
    gmetasuser[gmetacountuser].tracecalloc = NULL;
    gmetasuser[gmetacountuser].tracefree = NULL;
#if (iithreadsafe)
    imutexinit(&gmetasuser[gmetacountuser].mutex);
#endif
	return gmetacount + gmetacountuser++;
}

/*calloc memory for meta*/
static iobj* _imetaobjcalloc(imeta *meta) {
    int newsize = sizeof(iobj) + meta->size;
    iobj *obj = (iobj*)icalloc(1, newsize);
    obj->meta = meta;
    obj->size = newsize;
    
    _imeta_lock;
    obj->meta->alloced += newsize;
    obj->meta->current += newsize;
    _imeta_unlock;
    
    _imeta_global_lock;
    gcallocsize += newsize;
    gholdsize += newsize;
    _imeta_global_unlock;
    
    /* tracing the alloc */
    if (meta->tracecalloc) {
        meta->tracecalloc(meta, obj);
    }
    
    return obj;
}

/* 释放对象 */
static void _imetaobjfree(iobj *obj) {
    imeta *meta = obj->meta;
    
    /* tracing the alloc */
    if (meta->tracefree) {
        meta->tracefree(meta, obj);
    }
    
    _imeta_lock;
    obj->meta->current -= obj->size;
    obj->meta->freed += obj->size;
    _imeta_unlock;
    
    _imeta_global_lock;
    gfreesize += obj->size;
    gholdsize -= obj->size;
    _imeta_global_unlock;
    
    ifree(obj);
}

/**
 * 尝试从缓冲区拿对象
 */
static iobj *_imetapoll(imeta *meta) {
	iobj *obj = NULL;
   
    _imeta_lock;
	if (meta->cache.length) {
		obj = meta->cache.root;
		meta->cache.root = meta->cache.root->next;
		obj->next = NULL;
		--meta->cache.length;
		memset(obj->addr, 0, obj->meta->size);
        _imeta_unlock;
    } else {
        _imeta_unlock;
        
        obj = _imetaobjcalloc(meta);
    }
    
	return obj;
}

/* Meta 的缓冲区管理 */
void imetapush(iobj *obj) {
    imeta *meta = obj->meta;
    
    _imeta_lock;
	if (obj->meta->cache.length < obj->meta->cache.capacity) {
		obj->next = obj->meta->cache.root;
		obj->meta->cache.root = obj;
		++obj->meta->cache.length;
        _imeta_unlock;
	} else {
        _imeta_unlock;
        
		_imetaobjfree(obj);
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
    
    _imeta_lock;
	cur = meta->cache.root;
	while (cur) {
		next = cur->next;
		_imetaobjfree(cur);
		cur = next;
	}
	meta->cache.root = NULL;
	meta->cache.length = 0;
    _imeta_unlock;
}

/* 打印当前内存状态 */
void iaoimemorystate() {
	int i;
	ilog("[AOI-Memory] *************************************************************** Begin\n");
	ilog("[AOI-Memory] Total---> new: %" PRId64 ", free: %" PRId64 ", hold: %" PRId64 " \n", gcallocsize, gfreesize, gholdsize);
   
	for (i=0; i<gmetacount; ++i) {
        ilog("[AOI-Memory] "__imeta_format"\n", __imeta_value(gmetas[i]));
	}
    for (i=0; i<gmetacountuser; ++i) {
        ilog("[AOI-Memory] "__imeta_format"\n", __imeta_value(gmetasuser[i]));
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
    
    if (strlen(type) != strlen(meta->name)) {
        return iino;
    }

	if (strncmp(type, meta->name, strlen(meta->name)) == 0) {
		return iiok;
	}
	return iino;
}

/*获取当前的总的内存统计*/
int64_t iaoimemorysize(void *meta, int kind) {
    switch (kind) {
        case EnumAoiMemoerySizeKind_Alloced:
            return meta ? ((imeta*)(meta))->alloced : gcallocsize;
            break;
        case EnumAoiMemoerySizeKind_Freed:
            return meta ? ((imeta*)(meta))->freed : gfreesize;
            break;
        case EnumAoiMemoerySizeKind_Hold:
            return meta ? ((imeta*)(meta))->current : gholdsize;
            break;
        default:
            break;
    }
    return 0;
}

#else

void iaoimemorystate() {
	ilog("[AOI-Memory] Not Implement IMeta \n");
}

/*获取当前的总的内存统计*/
int64_t iaoimemorysize(void *meta, int kind) {
    return 0;
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

/* return the next pow of two */
int inextpot(int x) {
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >>16);
    return x + 1;
}

/* sleeping the current thread */
void isleep(unsigned int milliseconds) {
#ifdef WIN32
    Sleep((DWORD)milliseconds);
#else
    sleep(milliseconds);
#endif
}

/*create resource*/
void imutexinit(imutex *mutex) {
#ifdef WIN32
    mutex->_mutex = CreateMutex(NULL, 0, NULL);
#else
    /* recursive mutex */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex->_mutex, &attr);
#endif
}
/*release resource*/
void imutexrelease(imutex *mutex) {
#ifdef WIN32
    CloseHandle(mutex->_mutex);
#else
    pthread_mutex_destroy(&mutex->_mutex);
#endif
}

/*lock mutex*/
void imutexlock(imutex *mx) {
#ifdef WIN32
    WaitForSingleObject(mx->_mutex, 0);
#else
    pthread_mutex_lock(&mx->_mutex);
#endif
}

/*unlock mutex*/
void imutexunlock(imutex *mx) {
#ifdef WIN32
    ReleaseMutex(mx->_mutex);
#else
    pthread_mutex_unlock(&mx->_mutex);
#endif   
}

/*************************************************************/
/* iatomic                                                    */
/*************************************************************/
/* compare the store with expected, than store the value with desired */
uint32_t iatomiccompareexchange(volatile uint32_t *store, uint32_t expected, uint32_t desired) {
#ifdef WIN32
    return _InterlockedCompareExchange((volatile LONG*)store, expected, desired);
#else
    return __sync_val_compare_and_swap_4(store, expected, desired);
#endif
}

/* fetch the old value and store the with add*/
uint32_t iatomicadd(volatile uint32_t *store, uint32_t add) {
#ifdef WIN32
   return _InterlockedExchangeAdd((volatile LONG*)store, add);
#else
    return __sync_add_and_fetch_4(store, add);
#endif
}

/* fetch the old value, than do exchange operator */
uint32_t iatomicexchange(volatile uint32_t *store, uint32_t value) {
#ifdef WIN32
    return _InterlockedExchange((volatile LONG*)store, value);
#else
    return __sync_lock_test_and_set_4(store, value);
#endif
}

/* atomic increment, return the new value */
uint32_t iatomicincrement(volatile uint32_t *store) {
#ifdef WIN32
    return _InterlockedIncrement((volatile LONG*)store);
#else
    return __sync_add_and_fetch_4(store, 1);
#endif         
}

uint32_t iatomicdecrement(volatile uint32_t *store) {
#ifdef WIN32
    return _InterlockedDecrement((volatile LONG*)store);
#else
    return __sync_sub_and_fetch_4(store, 1);
#endif
}

static const char _base64EncodingTable[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const short _base64DecodingTable[256] = {
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -1, -1, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
    -2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
    -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
};

int ibase64encode_n(const unsigned char *in, size_t inlen, char *out, size_t *outsize) {
    const unsigned char * objRawData = in;
    char * objPointer;
    char * strResult;
    
    /* Get the Raw Data length and ensure we actually have data */
    int intLength = inlen;
    if (intLength == 0) return iino;
    
    /* Setup the String-based Result placeholder and pointer within that placeholder */
    strResult = out;
    objPointer = strResult;
    
    /* Iterate through everything */
    while (inlen > 2) { /* keep going until we have less than 24 bits */
        *objPointer++ = _base64EncodingTable[objRawData[0] >> 2];
        *objPointer++ = _base64EncodingTable[((objRawData[0] & 0x03) << 4) + (objRawData[1] >> 4)];
        *objPointer++ = _base64EncodingTable[((objRawData[1] & 0x0f) << 2) + (objRawData[2] >> 6)];
        *objPointer++ = _base64EncodingTable[objRawData[2] & 0x3f];
        
        /* we just handled 3 octets (24 bits) of data */
        objRawData += 3;
        inlen -= 3;
    }
    
    /* now deal with the tail end of things */
    if (intLength != 0) {
        *objPointer++ = _base64EncodingTable[objRawData[0] >> 2];
        if (intLength > 1) {
            *objPointer++ = _base64EncodingTable[((objRawData[0] & 0x03) << 4) + (objRawData[1] >> 4)];
            *objPointer++ = _base64EncodingTable[(objRawData[1] & 0x0f) << 2];
            *objPointer++ = '=';
        } else {
            *objPointer++ = _base64EncodingTable[(objRawData[0] & 0x03) << 4];
            *objPointer++ = '=';
            *objPointer++ = '=';
        }
    }
    
    /* Terminate the string-based result */
    *objPointer = '\0';
    
    /*Give Me the Length*/
    if (outsize) {
        *outsize = strlen(objPointer);
    }
    
    return iiok;
}

int ibase64decode_n(const char *in, size_t inlen, unsigned char *out, size_t *outsize) {
    const char *objPointer = in;
    size_t intLength = inlen;
    int intCurrent;
    int i = 0, j = 0, k;
    unsigned char *objResult = out;
    
    /* Run through the whole string, converting as we go */
    while ( ((intCurrent = *objPointer++) != '\0') && (intLength-- > 0) ) {
        if (intCurrent == '=') {
            if (*objPointer != '=' && ((i % 4) == 1)) {/* || (intLength > 0)) { */
                /* the padding character is invalid at this point -- so this entire string is invalid */
                return iino;
            }
            continue;
        }
        
        intCurrent = _base64DecodingTable[intCurrent];
        if (intCurrent == -1) {
            /* we're at a whitespace -- simply skip over */
            continue;
        } else if (intCurrent == -2) {
            /* we're at an invalid character */
            return iino;
        }
        
        switch (i % 4) {
            case 0:
                objResult[j] = intCurrent << 2;
                break;
                
            case 1:
                objResult[j++] |= intCurrent >> 4;
                objResult[j] = (intCurrent & 0x0f) << 4;
                break;
                
            case 2:
                objResult[j++] |= intCurrent >>2;
                objResult[j] = (intCurrent & 0x03) << 6;
                break;
                
            case 3:
                objResult[j++] |= intCurrent;
                break;
        }
        i++;
    }
    
    /* mop things up if we ended on a boundary */
    k = j;
    if (intCurrent == '=') {
        switch (i % 4) {
            case 1:
                /* Invalid state */
                return iino;
                
            case 2:
                k++;
                /* flow through */
            case 3:
                objResult[k] = 0;
        }
    }

    /*give me the outsize*/
    if (outsize) {
        *outsize = k;
    }
    
    return iiok;
}


/* zero point */
const ipos kipos_zero = {0,0};

/* 计算距离的平方 */
ireal idistancepow2(const ipos *p, const ipos *t) {
	ireal dx = p->x - t->x;
	ireal dy = p->y - t->y;
	return dx*dx + dy*dy;
}

/* zero point */
const ipos3 kipos3_zero = {0, 0, 0};

/* 计算距离的平方 */
ireal idistancepow3(const ipos3 *p, const ipos3 *t) {
    ireal dx = p->x - t->x;
	ireal dy = p->y - t->y;
    ireal dz = p->z - t->z;
	return dx*dx + dy*dy + dz*dz;
}

/* get the xy from the p with xz */
void ipos3takexz(const ipos3 *p, ipos *to) {
    to->x = p->x;
    to->y = p->z;
}

/* 把点在这个方向上进行移动 */
ipos ivec2movepoint(const ivec2 *dir, ireal dist, const ipos *p) {
    ipos to = *p;
    to.x += dir->v.x * dist;
    to.y += dir->v.y * dist;
    return to;
}

/* 两点相减得到向量 */
ivec2 ivec2subtractpoint(const ipos *p0, const ipos *p1) {
	ivec2 vec;
	vec.v.x = p0->x - p1->x;
	vec.v.y = p0->y - p1->y;
	return vec;
}

/* 点积 */
ireal ivec2dot(const ivec2 *l, const ivec2 *r) {
	icheckret(l, 0);
	icheckret(r, 0);
	return l->v.x * r->v.x + l->v.y * r->v.y;
}

/* 减法 */
ivec2 ivec2subtract(const ivec2 *l, const ivec2 *r) {
	ivec2 vec;
	vec.v.x = r->v.x - l->v.x;
	vec.v.y = r->v.y - l->v.y;
	return vec;
}

/* 加法*/
ivec2 ivec2add(const ivec2 *l, const ivec2 *r) {
	ivec2 vec;
	vec.v.x = r->v.x + l->v.x;
	vec.v.y = r->v.y + l->v.y;
	return vec;
}

/* 乘法 */
ivec2 ivec2multipy(const ivec2 *l, const ireal a) {
	ivec2 vec;
	vec.v.x = l->v.x * a;
	vec.v.y = l->v.y * a;
	return vec;
}

/* 绝对值 */
ivec2 ivec2abs(const ivec2* l) {
	ivec2 vec;
	vec.v.x = fabs(l->v.x);
	vec.v.y = fabs(l->v.y);
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

/*************************************************************/
/* ivec3                                                     */
/*************************************************************/

/* 两点相减得到向量 */
ivec3 ivec3subtractpoint(const ipos3 *p0, const ipos3 *p1) {
    ivec3 v;
    v.v.x = p0->x - p1->x;
    v.v.y = p0->y - p1->y;
    v.v.z = p0->z - p1->z;
    return v;
}

/* 加法*/
ivec3 ivec3add(const ivec3 *l, const ivec3 *r) {
	ivec3 vec;
	vec.v.x = r->v.x + l->v.x;
	vec.v.y = r->v.y + l->v.y;
	vec.v.z = r->v.z + l->v.z;
	return vec;
}

/* 减法 */
ivec3 ivec3subtract(const ivec3 *l, const ivec3 *r) {
	ivec3 vec;
	vec.v.x = r->v.x - l->v.x;
	vec.v.y = r->v.y - l->v.y;
	vec.v.z = r->v.z - l->v.z;
	return vec;
}

/* 乘法 */
ivec3 ivec3multipy(const ivec3 *l, ireal a) {
	ivec3 vec;
	vec.v.x = l->v.x * a;
	vec.v.y = l->v.y * a;
	vec.v.z = l->v.z * a;
	return vec;
}

/* 点积
 * https://en.wikipedia.org/wiki/Dot_product
 * */
ireal ivec3dot(const ivec3 *l, const ivec3 *r) {
	return l->v.x * r->v.x
		+ l->v.y * r->v.y
		+ l->v.z * r->v.z;
}

/* 乘积
 * https://en.wikipedia.org/wiki/Cross_product
 * */
ivec3 ivec3cross(const ivec3 *l, const ivec3 *r) {
	ivec3 vec;
	vec.v.x = l->v.y * r->v.z - l->v.z * r->v.y;
	vec.v.y = l->v.z * r->v.x - l->v.x * r->v.z;
	vec.v.z = l->v.x * r->v.y - l->v.y * r->v.x;
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
	vec.v.x = fabs(l->v.x);
	vec.v.y = fabs(l->v.y);
	vec.v.z = fabs(l->v.z);
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

/*************************************************************/
/* iline2d                                                   */
/*************************************************************/

/* start ==> end */
ivec2 iline2ddirection(const iline2d *line) {
    ivec2 v = ivec2subtractpoint(&line->end, &line->start);
    return ivec2normalize(&v);
}

/* start ==> end , rorate -90 */
ivec2 iline2dnormal(const iline2d *line) {
    ireal y;
    ivec2 v = iline2ddirection(line);
    y = v.v.y;
    v.v.y = -v.v.x;
    v.v.x = y;
    return v;
}

/**/
ireal iline2dlength(const iline2d *line) {
    ivec2 v = ivec2subtractpoint(&line->end, &line->start);
    return ivec2length(&v);
}

/*
 * Determines the signed distance from a point to this line. Consider the line as
 * if you were standing on start of the line looking towards end. Posative distances
 * are to the right of the line, negative distances are to the left.
 * */
ireal iline2dsigneddistance(const iline2d *line, const ipos *point) {
    ivec2 v = ivec2subtractpoint(point, &line->start);
    ivec2 normal = iline2dnormal(line);
    return ivec2dot(&v, &normal);
}

/*
 * Determines the signed distance from a point to this line. Consider the line as
 * if you were standing on start of the line looking towards end. Posative distances
 * are to the right of the line, negative distances are to the left.
 * */
int iline2dclassifypoint(const iline2d *line, const ipos *point, ireal epsilon) {
    int      result = EnumPointClass_On;
    ireal      distance = iline2dsigneddistance(line, point);
    
    if (distance > epsilon) {
        result = EnumPointClass_Right;
    } else if (distance < -epsilon) {
        result = EnumPointClass_Left;
    }
    
    return result;
}

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
int iline2dintersection(const iline2d *line, const iline2d *other,  ipos *intersect) {
    ireal Ay_minus_Cy = line->start.y - other->start.y;
    ireal Dx_minus_Cx = other->end.x - other->start.x;
    ireal Ax_minus_Cx = line->start.x - other->start.x;
    ireal Dy_minus_Cy = other->end.y - other->start.y;
    ireal Bx_minus_Ax = line->end.x - line->start.x;
    ireal By_minus_Ay = line->end.y - line->start.y;
    
    ireal Numerator = (Ay_minus_Cy * Dx_minus_Cx) - (Ax_minus_Cx * Dy_minus_Cy);
    ireal Denominator = (Bx_minus_Ax * Dy_minus_Cy) - (By_minus_Ay * Dx_minus_Cx);
    
    ireal FactorAB, FactorCD;
    
    /* if lines do not intersect, return now */
    if (!Denominator)
    {
        if (!Numerator)
        {
            return EnumLineClass_Collinear;
        }
        
        return EnumLineClass_Paralell;
    }
    
    FactorAB = Numerator / Denominator;
    FactorCD = ((Ay_minus_Cy * Bx_minus_Ax) - (Ax_minus_Cx * By_minus_Ay)) / Denominator;
    
    /* posting (hitting a vertex exactly) is not allowed, shift the results
     * if they are within a minute range of the end vertecies */
    /*	
     if (fabs(FactorCD) < 1.0e-6f) {
        FactorCD = 1.0e-6f;
     } if (fabs(FactorCD - 1.0f) < 1.0e-6f) {
        FactorCD = 1.0f - 1.0e-6f;
     }
     */
    
    /* if an interection point was provided, fill it in now */
    if (intersect)
    {
        intersect->x = (line->start.x + (FactorAB * Bx_minus_Ax));
        intersect->y = (line->start.y + (FactorAB * By_minus_Ay));
    }
    
    /* now determine the type of intersection */
    if ((FactorAB >= 0.0f) && (FactorAB <= 1.0f) && (FactorCD >= 0.0f) && (FactorCD <= 1.0f)) {
        return EnumLineClass_Segments_Intersect;
    } else if ((FactorCD >= 0.0f) && (FactorCD <= 1.0f)) {
        return (EnumLineClass_A_Bisects_B);
    } else if ((FactorAB >= 0.0f) && (FactorAB <= 1.0f)) {
        return (EnumLineClass_B_Bisects_A);
    }
    
    return EnumLineClass_Lines_Intersect;
}

/* Caculating the closest point in the segment to center pos */
ipos iline2dclosestpoint(const iline2d *line, const ipos *center, ireal epsilon) {
    /*@see http://doswa.com/2009/07/13/circle-segment-intersectioncollision.html */
    ipos closest;
    
    ivec2 start_to_center = ivec2subtractpoint(center, &line->start);
    ivec2 line_direction = iline2ddirection(line);
    ireal line_len = iline2dlength(line);
    
    ireal projlen = ivec2dot(&start_to_center, &line_direction);
    if (projlen <= 0) {
        closest = line->start;
    } else if ( ireal_greater_than(projlen, line_len, epsilon)){
        closest = line->end;
    } else {
        closest.x = line->start.x + line_direction.v.x * projlen;
        closest.y = line->start.y + line_direction.v.y * projlen;
    }
    
    return closest;
}

/*************************************************************/
/* iline3d                                                   */
/*************************************************************/

/* start ==> end */
ivec3 iline3ddirection(const iline3d *line) {
    ivec3 v = ivec3subtractpoint(&line->end, &line->start);
    return ivec3normalize(&v);
}

/**/
ireal iline3dlength(const iline3d *line) {
    ivec3 v = ivec3subtractpoint(&line->end, &line->start);
    return ivec3length(&v);
}

/* find the closest point in line */
ipos3 iline3dclosestpoint(const iline3d *line, const ipos3 *center, ireal epsilon) {
    /*@see http://doswa.com/2009/07/13/circle-segment-intersectioncollision.html */
    ipos3 closest;
    
    ivec3 start_to_center = ivec3subtractpoint(center, &line->start);
    ivec3 line_direction = iline3ddirection(line);
    ireal line_len = iline3dlength(line);
    
    ireal projlen = ivec3dot(&start_to_center, &line_direction);
    if (projlen <= 0) {
        closest = line->start;
    } else if ( ireal_greater_than(projlen, line_len, epsilon)){
        closest = line->end;
    } else {
        closest.x = line->start.x + line_direction.v.x * projlen;
        closest.y = line->start.y + line_direction.v.y * projlen;
        closest.z = line->start.z + line_direction.v.z * projlen;
    }
    
    return closest;
}

/*************************************************************/
/* iplane                                                    */
/*************************************************************/

/* Setup Plane object given a clockwise ordering of 3D points */
void iplaneset(iplane *plane, const ipos3 *a, const ipos3 *b, const ipos3 *c) {
    ivec3 ab = ivec3subtractpoint(a, b);
    ivec3 ac = ivec3subtractpoint(a, c);
    ivec3 normal = ivec3cross(&ab, &ac);
    ivec3 p = {{a->x, a->y, a->z}}; /*change to vec3*/
    
    plane->normal = ivec3normalize(&normal);
    plane->pos = *a;
    plane->distance = ivec3dot(&p, &plane->normal);
}

/* TODO: */
ireal iplanesigneddistance(const iplane *plane, const ipos3 *p) {
    return 0;
}

/* Given Z and Y, Solve for X on the plane */
ireal iplanesolveforx(iplane *plane, ireal y, ireal z) {
    /*
     * Ax + By + Cz + D = 0
     * Ax = -(By + Cz + D)
     * x = -(By + Cz + D)/A */
    
    if (plane->normal.values[0] ) {
        return ( -(plane->normal.values[1]*y
                   + plane->normal.values[2]*z
                   + plane->distance) / plane->normal.values[0] );
    }
    
    return (0.0f);
}
    
/* Given X and Z, Solve for Y on the plane */
ireal iplanesolvefory(iplane *plane, ireal x, ireal z) {
    /*
     * Ax + By + Cz + D = 0
     * By = -(Ax + Cz + D)
     * y = -(Ax + Cz + D)/B */
    
    if (plane->normal.values[1]) {
        return ( -(plane->normal.values[0]*x
                   + plane->normal.values[2]*z
                   + plane->distance) / plane->normal.values[1] );
    }
    
    return (0.0f);
 
}
    
/* Given X and Y, Solve for Z on the plane */
ireal iplanesolveforz(iplane *plane, ireal x, ireal y) {
    /*Ax + By + Cz + D = 0
     * Cz = -(Ax + By + D)
     * z = -(Ax + By + D)/C */
    
    if (plane->normal.values[2]) {
        return ( -(plane->normal.values[0]*x
                   + plane->normal.values[1]*y
                   + plane->distance) / plane->normal.values[2] );
    }
    
    return (0.0f);
}

/*************************************************************/
/* irect                                                    */
/*************************************************************/

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
		ivec2 v = {{fabs(c->pos.x - con->pos.x), fabs(c->pos.y - con->pos.y)}};
		ivec2 h = {{con->size.w, con->size.h}};
		ivec2 u =  {{v.v.x - h.v.x, v.v.y - h.v.y}};
		u.v.x = u.v.x < 0 ? 0 : u.v.x;
		u.v.y = u.v.y < 0 ? 0 : u.v.y;
		return u.v.x * u.v.x + u.v.y * u.v.y < c->radius * c->radius;
	} while(0);
	return 0;
}

/* down-left pos*/
ipos irectdownleft(const irect *con) {
    return con->pos;
}

/* down-right pos*/
ipos irectdownright(const irect *con) {
    ipos p = con->pos;
    p.x += con->size.w;
    return p;
}
/* up-left pos*/
ipos irectupleft(const irect *con) {
    ipos p = con->pos;
    p.y += con->size.h;
    return p;
}

/* up-right pos*/
ipos irectupright(const irect *con) {
    ipos p = con->pos;
    p.x += con->size.w;
    p.y += con->size.h;
    return p;
}

/* Caculating the offset that circle should moved to avoid collided with the line */
ivec2 icircleoffset(const icircle* circle, const iline2d* line) {
    /*@see http://doswa.com/2009/07/13/circle-segment-intersectioncollision.html */
    ipos closest = iline2dclosestpoint(line, &circle->pos, iepsilon);
    ivec2 dist = ivec2subtractpoint(&circle->pos, &closest);
    ireal distlen = ivec2length(&dist);
    ivec2 offset = {{0,0}};
    if (ireal_greater(distlen, circle->radius)) {
        return offset;
    }else if(ireal_equal_zero(distlen)) {
       /*the circle center is on line: move normal in line */
        offset = iline2dnormal(line);
        offset = ivec2multipy(&offset, circle->radius);
        return offset;
    }
    /* normalize(dist) * (radius - distlen) */
    return ivec2multipy(&dist, (circle->radius - distlen) / distlen);
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

#define iplogwhen(t, when, ...) do { if(open_log_profile && t > when) {printf("[PROFILE] Take %" PRId64 " micros ", t); printf(__VA_ARGS__); } } while (0)

#define iplog(t, ...) iplogwhen(t, __ProfileThreashold, __VA_ARGS__)

/* iref 转换成 target */
#define icast(type, v) ((type*)(v))
/* 转换成iref */
#define irefcast(v) icast(iref, v)

/* 增加引用计数 */
int irefretain(iref *ref) {
#if (iithreadsafe)
    return iatomicincrement(&ref->ref);
#else
	return ++ref->ref;
#endif
}

/* 释放引用计数 */
void irefrelease(iref *ref) {
	
	/* 没有引用了，析构对象 */
#if (iithreadsafe)
    if (iatomicdecrement(&ref->ref) == 0) {
#else
    if (--ref->ref == 0) {
#endif
        while (true) {
        /*printf("atomic-after-decrement %p - %d \n", (void*)ref, ref->ref); */
        /* release the hold by wref and ref */
        if (ref->wref) {
            ref->wref->wref = NULL;
            ref->wref = NULL;
        }
        /* 通知 watch */
    	if (ref->watch) {
    		ref->watch(ref);
    	}
        if (ref->ref != 0) {
            break;
        }
        /* release resources */
		if (ref->free) {
			ref->free(ref);
		}else {
            /* just release memory */
			iobjfree(ref);
		}
        break;
        }
	}
}

/*************************************************************/
/* iwref                                                     */
/*************************************************************/

/* zero wref */
static iwref kzero_wref = {1, NULL};

/* make a weak iref by ref */
iwref *iwrefmake(iref *ref) {
    volatile iwref *wref;
    if (ref == NULL) {
        wref = (iwref*)(&kzero_wref);
    } else if (ref->wref == NULL) {
        /* total new wref */
        ref->wref = iobjmalloc(iwref);
        ref->wref->wref = icast(iwref, ref);
        wref = ref->wref;
    } else {
        /* extis */
        wref = ref->wref;
    }
    
    iretain(wref);
    return (iwref*)wref;
}

/* make a weak iref by wref */
iwref *iwrefmakeby(iwref *wref) {
    icheckret(wref, NULL);
    return iwrefmake(irefcast(wref->wref));
}

/* make strong ref: need call irelease */
iref *iwrefstrong(iwref *wref) {
    return irefassistretain(iwrefunsafestrong(wref));
}

/* make strong ref: unneed call irelease */
iref *iwrefunsafestrong(iwref *wref) {
    icheckret(wref, NULL);
    icheckret(wref != &kzero_wref, NULL);
    
    return icast(iref, wref->wref);
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
    /* release the resouce */
    if (joint->list && joint->list->entry) {
        joint->list->entry(joint);
    }
	iobjfree(joint);
}

/* reflist entry for free */
static void _ireflist_entry_free(iref *ref) {
    ireflist *list = icast(ireflist, ref);
    
	/* 释放列表节点 */
	ireflistremoveall(list);

	/* 释放list 本身 */
	iobjfree(list);
}

/* 创建列表 */
ireflist *ireflistmake() {
	ireflist *list = iobjmalloc(ireflist);
    list->free = _ireflist_entry_free;
    iretain(list);
    return list;
}

/* 创建列表 */
ireflist *ireflistmakeentry(irefjoint_entry_res_free entry) {
    ireflist *list = ireflistmake();
    list->entry = entry;
    return list;
}
    
/* 释放列表 */
void ireflistfree(ireflist *list) {
    irelease(list);
}

/* 获取列表长度 */
size_t ireflistlen(const ireflist *list) {
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

/* 往列表增加节点: 前置节点(会增加引用计数) */
irefjoint* ireflistaddres(ireflist *list, iref *value, void *res) {
    irefjoint *joint = ireflistadd(list, value);
    icheckret(joint, NULL);
    joint->res = res;
    return joint;
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


/*************************************************************/
/* irefneighbors                                             */
/*************************************************************/

/* 设置邻居间关系描述的 释放符号 */
void ineighborsbuild(irefneighbors *neighbors, irefjoint_entry_res_free entry) {
    icheck(neighbors);
    neighbors->neighbors_resfree = entry;
}

/*
 * 把节点从 有向图里面拿出来， 没有任何一个节点可以到他
 */
void ineighborsclean(irefneighbors *node) {
    irefjoint* joint = NULL;
    irefneighbors *neighbor = NULL;
    icheck(node);
    
    /* disconnect to others */
    joint = ireflistfirst(node->neighbors_to);
    while (joint) {
        neighbor = icast(irefneighbors, joint->value);
        ireflistremove(neighbor->neighbors_from, irefcast(node));
        joint = joint->next;
    }
    ireflistfree(node->neighbors_to);
    node->neighbors_to = NULL;
    
    /* disconnect from others */
    joint = ireflistfirst(node->neighbors_from);
    while (joint) {
        neighbor = icast(irefneighbors, joint->value);
        ireflistremove(neighbor->neighbors_to, irefcast(node));
        joint = joint->next;
    }
    ireflistfree(node->neighbors_from);
    node->neighbors_from = NULL;
}

/*
 * 没有做重复性的检查
 * 让 from ==> to
 */
void ineighborsadd(irefneighbors *from, irefneighbors *to) {
    ineighborsaddvalue(from, to, NULL, NULL);
}

/* 在有向图上加上一单向边 */
void ineighborsaddvalue(irefneighbors *from, irefneighbors *to, void *from_to, void *to_from) {
    icheck(from);
    icheck(to);
    
    if (!from->neighbors_to) {
        from->neighbors_to = ireflistmakeentry(from->neighbors_resfree);
    }
    ireflistaddres(from->neighbors_to, irefcast(to), from_to);
    if (!to->neighbors_from) {
        to->neighbors_from = ireflistmakeentry(to->neighbors_resfree);
    }
    ireflistaddres(to->neighbors_from, irefcast(from), to_from);
}

/*
 * 没有做重复性的检查
 * 让 from !==> to
 */
void ineighborsdel(irefneighbors *from, irefneighbors *to) {
    ireflistremove(from->neighbors_to, irefcast(to));
    ireflistremove(to->neighbors_from, irefcast(from));
}

/*invalid index */
const int kindex_invalid = -1;

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
    array->buffer = capacity > 0 ? (char*)icalloc(capacity, entry->size) : NULL;
    array->free = _iarray_entry_free;
    array->entry = entry;
    array->flag = entry->flag;
    array->cmp = entry->cmp;
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
const void* iarrayat(const iarray *arr, int index) {
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
int iarrayisflag(const iarray *arr, int flag) {
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
        arr->entry->swap(arr, index, kindex_invalid);
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

/* 扩大容量 */
size_t iarrayexpandcapacity(iarray *arr, size_t capacity) {
    icheckret(arr->capacity < capacity, arr->capacity);
    
    return _iarray_just_capacity(arr, capacity);
}

/* 堆排序 - 堆调整 */
static void _iarray_heap_shift(iarray *arr,
                      int ind, int end) {

    int i = ind;
    int c = 2 * i + 1;

    while(c <= end) {
        if (c+1 <=end && arr->cmp(arr, c, c+1) < 0 ) {
            c++;
        }
        if (arr->cmp(arr, i, c) > 0) {
            break;
        } else {
            arr->entry->swap(arr, i, c);

            i = c;
            c = 2*i + 1;
        }
    }
}

/* 在 [start, end] 上建立堆 */
static void _iarray_heap_build(iarray *arr, int start, int end) {
    int i;
    for (i=(end-1)/2; i>=start; i--) {
        _iarray_heap_shift(arr, i, end);
    }
}

/* 堆排序 */
static void _iarray_sort_heap(iarray *arr,
                int start, int end) {
    int i;
    _iarray_heap_build(arr, start, end);

    for (i=start; i<=end; ++i) {
        arr->entry->swap(arr, start, end-start-i);
        _iarray_heap_shift(arr, start, end - start - i - 1);
    }
}

/* 排序 */
void iarraysort(iarray *arr) {
    icheck(arr->len);

    _iarray_sort_heap(arr, 0, arr->len-1);
}
    
/* for each */
void iarrayforeach(const iarray *arr, iarray_entry_visitor visitor) {
    size_t size = iarraylen(arr);
    size_t idx = 0;
    for (; idx < size; ++idx) {
        visitor(arr, idx, iarrayat(arr, idx));
    }
}

/*************************************************************/
/* iheap                                                     */
/*************************************************************/


/* 建立 堆操作 */
void iheapbuild(iheap *heap) {
    _iarray_heap_build(heap, 0, iarraylen(heap));
}

/* 堆大小 */
size_t iheapsize(const iheap *heap) {
    return iarraylen(heap);
}

/* 向下调整堆 */
static void _iheapadjustup(iheap *heap, int start, int index) {
    int parent;
    while(index > start) {
        parent = (index-1) / 2;
        if ( heap->cmp(heap, index, parent) > 0) {
            heap->entry->swap(heap, index, parent);
            index = parent;
        } else {
            break;
        }
    }
}

/* 向下调整堆 */
static void _iheapadjustdown(iheap *heap, int index, int end) {
    _iarray_heap_shift(heap, index, end);
}

/* 建立 堆操作 */
void iheapadd(iheap *heap, const void *value) {
    int index = iarraylen(heap);
    iarrayadd(heap, value);
    
    /*adjust up*/
    _iheapadjustup(heap, 0, index);
}

/* 堆操作: 调整一个元素 */
void iheapadjust(iheap *heap, int index) {
    int i = index;
    int c = 2*i + 1;
    int start = 0;
    int end = (int)iheapsize(heap);
    
    if (c+1<=end && heap->cmp(heap, c, c+1) < 0) {
        c++;
    }
    if (c <= end && heap->cmp(heap, c, i) > 0) {
        _iheapadjustdown(heap, index, end);
    } else {
        _iheapadjustup(heap, start, index);
    }
}

/* 堆操作: 获取堆顶元素 */
const void *iheappeek(const iheap *heap) {
    icheckret(iarraylen(heap) > 0, NULL);
    return iarrayat(heap, 0);
}

/* 堆操作: 移除堆顶元素*/
void iheappop(iheap *heap) {
    iheapdelete(heap, 0);
}

/* 堆操作: 移除指定的位置的元素, 仍然保持堆 */
void iheapdelete(iheap *heap, int index) {
    icheck(index>=0 && index<iarraylen(heap));

    /*swap last one*/
    heap->entry->swap(heap, index, iarraylen(heap)-1);
    /*array remove it*/
    iarrayremove(heap, iarraylen(heap)-1);

    /*adjust the heap to be still on*/
    if (iarraylen(heap) > 0 ) {
        _iarray_heap_shift(heap, index, iarraylen(heap)-1);
    }
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
    static char buffer[256];
    char *tmp;
    if (arr->entry->size > 256) {
        tmp = icalloc(1, arr->entry->size);
    } else {
        tmp = buffer;
    }
    
    if (j == kindex_invalid) {
        /* arr_int[i] = 0;
        may call assign */
        _iarray_entry_assign_copy(arr, i, tmp, 1);
    } else if (i == kindex_invalid) {
        /* arr_int[j] = 0;
        may call assign */
        _iarray_entry_assign_copy(arr, j, tmp, 1);
    } else {
        memmove(tmp, __arr_i(arr, i), arr->entry->size);
        memmove(__arr_i(arr, i), __arr_i(arr, j), arr->entry->size);
        memmove(__arr_i(arr, j), tmp, arr->entry->size);
    }
    if (tmp != buffer) {
        ifree(tmp);
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
    sizeof(int64_t),
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
    sizeof(char),
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
    irefarrayentry *entry = (irefarrayentry*)arr->userdata;
    
    /* 附加很多个 */
    while (j < nums) {
        /* realloc not set zero to pending memory */
        if (i >= arr->len) {
            arrs[i] = NULL;
        }
        if (refvalue) {
            ref = refvalue[j];
        }
        
        /* watch the index change */
        if (arrs[i] && entry && entry->indexchange) {
            entry->indexchange(arr, arrs[i], kindex_invalid);
        }
        
        iassign(arrs[i], ref);
        
        /* watch the index change */
        if (ref && entry && entry->indexchange) {
            entry->indexchange(arr, ref, i);
        }
        ++j;
        ++i;
    }
}

/* 交换两个对象 */
static void _iarray_entry_swap_iref(struct iarray *arr,
                          int i, int j) {
    iref* tmp;
    iref* *arrs = (iref* *)arr->buffer;
    irefarrayentry *entry = (irefarrayentry*)arr->userdata;
    
    if (j == kindex_invalid) {
        /* arr_int[i] = 0;
         * may call assign */
        _iarray_entry_assign_iref(arr, i, 0, 1);
    } else if (i == kindex_invalid) {
        /* arr_int[j] = 0;
         * may call assign */
        _iarray_entry_assign_iref(arr, j, 0, 1);
    } else {
        tmp = arrs[i];
        arrs[i] = arrs[j];
        arrs[j] = tmp;
        
        /* watch the index change */
        if (entry && entry->indexchange) {
            entry->indexchange(arr, arrs[i], i);
            entry->indexchange(arr, arrs[j], j);
        }
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
    return iarraymakeirefwithentry(capacity, NULL);
}

/* 内置的引用数组 */
iarray* iarraymakeirefwithentry(size_t capacity, const irefarrayentry *refentry) {
    iarray*  arr = iarraymake(capacity, &_arr_entry_iref);
    arr->userdata = (void*)refentry;
    return arr;
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

/* 定义ipos 数组 */
static const iarrayentry _arr_entry_ipos3 = {
    EnumArrayFlagAutoShirk |
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(ipos3),
    _iarray_entry_swap_copy,
    _iarray_entry_assign_copy,
    NULL,
};
/* 内置的 ipos3 数组*/
iarray* iarraymakeipos3(size_t capacity) {
    return iarraymake(capacity, &_arr_entry_ipos3);
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
    
/* foreach */
void isliceforeach(const islice *slice, islice_entry_visitor visitor) {
    size_t size = islicelen(slice);
    size_t idx = 0;
    for (; idx<size; ++idx) {
        visitor(slice, idx, isliceat(slice, idx));
    }
}
    
/*************************************************************/
/* irange                                                    */
/*************************************************************/
    
/*save a state in ite*/
typedef enum EnumRangeIteState {
    EnumRangeIteState_Invalid = 1,
}EnumRangeIteState;
    
/* range operator, accept: iarray, islice, istring, iheap */
size_t irangelen(const void *p) {
    if (iistype(p, iarray)) {
        return iarraylen((const iarray*)(p));
    } else if (iistype(p, islice)) {
        return islicelen((const islice*)(p));
    } else {
        return 0;
    }
}

/* range get: accept: iarray, islice, istring, iheap */
const void* irangeat(const void *p, int index) {
    if (iistype(p, iarray)) {
        return iarrayat((const iarray*)(p), index);
    } else if (iistype(p, islice)) {
        return isliceat((const islice*)(p), index);
    } else {
        return NULL;
    }   
}
    
/* free the irangeite */
static void _irangeite_entry_free(iref *ref) {
    irangeite *ite = icast(irangeite, ref);
    ite->access->accessfree(ite);
    iobjfree(ite);
}
    
/* make a range ite over the container */
irangeite* irangeitemake(void* p, const irangeaccess *access) {
    irangeite *ite = iobjmalloc(irangeite);
    ite->free = _irangeite_entry_free;
    ite->container = p;
    ite->access = access;
    ite->access->accesscreate(ite);
    
    iretain(ite);
    return ite;
}
    
/* free the range ite */
void irangeitefree(irangeite *ite) {
    irelease(ite);
}
    
/*iiok: iino*/
int irangenext(irangeite *ite) {
    icheckret(ite, iino);
    icheckret(ite->__internal & EnumRangeIteState_Invalid, iino); /* invalid ite */
    
    return ite->access->accessnext(ite);
}
    
/* returnt the value address */
const void *irangevalue(irangeite *ite) {
    icheckret(ite, NULL);
    icheckret(ite->__internal & EnumRangeIteState_Invalid, NULL); /* invalid ite */
    
    return ite->access->accessvalue(ite);
}

/* returnt the key: may be key , may be key address */
const void *irangekey(irangeite *ite) {
    icheckret(ite, NULL);
    icheckret(ite->__internal & EnumRangeIteState_Invalid, NULL); // invalid ite
    
    return ite->access->accesskey(ite);
}
    
/*************************************************************/
/* irange-array                                              */
/*************************************************************/
/* range on array: create */
static void _irangeite_access_create_array(irangeite *ite) {
}
    
/* range on array: next */
static int _irangeite_access_next_array(irangeite *ite) {
    icheckret(ite->container, iino);
    
    if (ite->ite == NULL) {
        ite->ite = icalloc(1, sizeof(int));
        *(int*)ite->ite = 0;
    } else {
        ++(*(int*)ite->ite);
    }
    return (*(int*)(ite->ite)) < iarraylen(icast(iarray, ite->container));
}

/* range on array: free */
static void _irangeite_access_free_array(irangeite *ite) {
    if (ite->ite) {
        ifree(ite->ite);
        ite->ite = NULL;
    }
}

/* range on array: value */
static const void * _irangeite_access_value_array(irangeite *ite) {
    icheckret(ite->ite, NULL);
    return iarrayat(icast(iarray, ite->container), *(int*)ite->ite);
}

/* range on array: key */
static const void * _irangeite_access_key_array(irangeite *ite) {
    return ite->ite;
}

/* range on array */
static const irangeaccess _irangeaccess_array = {
    _irangeite_access_create_array,
    _irangeite_access_free_array,
    _irangeite_access_next_array,
    _irangeite_access_value_array,
    _irangeite_access_key_array,
};
    
/*************************************************************/
/* irange-slice                                              */
/*************************************************************/
/* range on slice: create */
static void _irangeite_access_create_slice(irangeite *ite) {
}
    
/* range on slice: next */
static int _irangeite_access_next_slice(irangeite *ite) {
    icheckret(ite->container, iino);
    
    if (ite->ite == NULL) {
        ite->ite = icalloc(1, sizeof(int));
        *(int*)ite->ite = 0;
    } else {
        ++(*(int*)ite->ite);
    }
    return (*(int*)(ite->ite)) < islicelen(icast(islice, ite->container));
}

/* range on slice: free */
static void _irangeite_access_free_slice(irangeite *ite) {
    if (ite->ite) {
        ifree(ite->ite);
        ite->ite = NULL;
    }
}

/* range on slice: value */
static const void * _irangeite_access_value_slice(irangeite *ite) {
    icheckret(ite->ite, NULL);
    return isliceat(icast(islice, ite->container), *(int*)ite->ite);
}

/* range on slice: key */
static const void * _irangeite_access_key_slice(irangeite *ite) {
    return ite->ite;
}

/* range on slice */
static const irangeaccess _irangeaccess_slice = {
    _irangeite_access_create_slice,
    _irangeite_access_free_slice,
    _irangeite_access_next_slice,
    _irangeite_access_value_slice,
    _irangeite_access_key_slice,
};
    
/*************************************************************/
/* irange-reflist                                            */
/*************************************************************/
/* range on reflist: create */
static void _irangeite_access_create_reflist(irangeite *ite) {
}
    
/* range on reflist: next */
static int _irangeite_access_next_reflist(irangeite *ite) {
    icheckret(ite->container, iino);
    
    if (ite->ite == NULL) {
        ite->ite = icalloc(1, sizeof(irefjoint*));
        ((irefjoint**)ite->ite)[0] = ireflistfirst(icast(ireflist, ite->container));
    } else {
        ((irefjoint**)ite->ite)[0] = ((irefjoint**)ite->ite)[0]->next;
    }
    return ite->ite != NULL;
}

/* range on reflist: free */
static void _irangeite_access_free_reflist(irangeite *ite) {
    if (ite->ite) {
        ifree(ite->ite);
        ite->ite = NULL;
    }
}

/* range on reflist: value */
static const void * _irangeite_access_value_reflist(irangeite *ite) {
    icheckret(ite->ite, NULL);
    return icast(irefjoint, ite->ite)->value;
}

/* range on reflist: key */
static const void * _irangeite_access_key_reflist(irangeite *ite) {
    return ite->ite;
}

/* range on reflist */
static const irangeaccess _irangeaccess_reflist = {
    _irangeite_access_create_reflist,
    _irangeite_access_free_reflist,
    _irangeite_access_next_reflist,
    _irangeite_access_value_reflist,
    _irangeite_access_key_reflist,
};
    
/* make the rangeite for array or slice or reflist */
irangeite *irangeitemakefrom(void *p) {
    if (iistype(p, iarray)) {
        return irangeitemake(p, &_irangeaccess_array);
    } else if (iistype(p, islice)) {
        return irangeitemake(p, &_irangeaccess_slice);
    } else if (iistype(p, ireflist)) {
        return irangeitemake(p, &_irangeaccess_reflist);
    }
    
    return NULL;
}
    
/*************************************************************/
/* istring                                                   */
/*************************************************************/

ideclarestring(kstring_zero, "");

/*Make a string by c-style string */
istring istringmake(const char* s) {
    return istringmakelen(s, strlen(s));
}

/*Make a string by s and len*/
istring istringmakelen(const char* s, size_t len) {
    islice *str;
    iarray *arr = iarraymakechar(len+1);
    iarrayinsert(arr, 0, s, len);
    ((char*)iarraybuffer(arr))[len] = 0;
    
    str = islicemake(arr, 0, len, 0);
    iarrayfree(arr);
    return istringlaw(str);
}

/*Make a copy of s with c-style string*/
istring istringdup(const istring s) {
    return istringmakelen(istringbuf(s), istringlen(s));
}

/*Return the string length */
size_t istringlen(const istring s) {
    icheckret(s, 0);
    return islicelen(s);
}

/*visit the real string buffer*/
const char* istringbuf(const istring s) {
    return (const char*)isliceat(s, 0);
}

/*set the entry for stack string */
istring istringlaw(istring s) {
    if (s->array->entry == NULL) {
        s->array->entry = &_arr_entry_char;
    }
    s->flag |= EnumSliceFlag_String;
    return s;
}

/* Helper for irg_print() doing the actual number -> string
 * conversion. 's' must point to a string with room for at least
 * _IRB_LLSTR_SIZE bytes.
 *
 * The function returns the length of the null-terminated string
 * representation stored at 's'. */
#define _IRB_LLSTR_SIZE 256

size_t _ill2str(char *s, int64_t value) {
    char *p, aux;
    uint64_t v;
    size_t l;
    
    /* Generate the string representation, this method produces
     * an reversed string. */
    v = (value < 0) ? -value : value;
    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);
    if (value < 0) *p++ = '-';
    
    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';
    
    /* Reverse the string. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

/* Identical _ill2str(), but for unsigned long long type. */
size_t _iull2str(char *s, uint64_t v) {
    char *p, aux;
    size_t l;
    
    /* Generate the string representation, this method produces
     * an reversed string. */
    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);
    
    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';
    
    /* Reverse the string. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

size_t _idouble2str(char *s, double d) {
    size_t n = snprintf(s, 256, "%.4lf", d);
    s[n] = 0;
    return n;
}

/*format the string and return the value*/
istring istringformat(const char* fmt, ...) {
    istring s;
    iarray *arr = iarraymakechar(strlen(fmt)*2);
    const char *f = fmt;
    size_t i;
    double d;
    va_list ap;
    
    char next, *str;
    size_t l;
    int64_t num;
    uint64_t unum;
    
    char buf[_IRB_LLSTR_SIZE];
    
    va_start(ap,fmt);
    f = fmt;    /* Next format specifier byte to process. */
    i = 0;
    while(*f) {
        
        /* Make sure there is always space for at least 1 char. */
        switch(*f) {
            case '%':
                next = *(f+1);
                f++;
                switch(next) {
                    case 's':
                    case 'S':
                        str = va_arg(ap,char*);
                        l = strlen(str);/*(next == 's') ?  : sdslen(str);*/
                        iarrayinsert(arr, iarraylen(arr), str, l);
                        i += l;
                        break;
                    case 'v':
                    case 'V':
                        s = va_arg(ap, istring);
                        l = istringlen(s);
                        iarrayinsert(arr, iarraylen(arr), istringbuf(s), l);
                        i += l;
                        break;
                    case 'f':
                    case 'F':
                        d = va_arg(ap, double);
                        l = _idouble2str(buf, d);
                        iarrayinsert(arr, iarraylen(arr), buf, l);
                        i += l;
                        break;
                    case 'i':
                    case 'I':
                        if (next == 'i')
                            num = va_arg(ap,int);
                        else
                            num = va_arg(ap,int64_t);
                        {
                        l = _ill2str(buf, num);
                        iarrayinsert(arr, iarraylen(arr), buf, l);
                        i += l;
                        }
                        break;
                    case 'u':
                    case 'U':
                        if (next == 'u')
                            unum = va_arg(ap,unsigned int);
                        else
                            unum = va_arg(ap,uint64_t);
                        {
                        l = _iull2str(buf, unum);
                        iarrayinsert(arr, iarraylen(arr), buf, l);
                        i += l;
                        }
                        break;
                    default: /* Handle %% and generally %<unknown>. */
                        iarrayinsert(arr, iarraylen(arr), f, 1);
                        break;
                }
                break;
            default:
                iarrayinsert(arr, iarraylen(arr), f, 1);
                break;
        }
        f++;
    }
    va_end(ap);
    
    s = islicemakearg(arr, ":");
    iarrayfree(arr);
    return istringlaw(s);
}

/*compare the two istring*/
int istringcompare(const istring lfs, const istring rfs) {
    size_t lfslen = istringlen(lfs);
    size_t rfslen = istringlen(rfs);
    int n = strncmp(istringbuf(lfs), istringbuf(rfs), imin(lfslen, rfslen));
    if (n) {
        return n;
    }
    return lfslen - rfslen;
}

/*find the index in istring */
/*https://en.wikipedia.org/wiki/String_searching_algorithm*/
/*[Rabin-Karp]http://mingxinglai.com/cn/2013/08/pattern-match/*/
/*[Sunday](http://blog.163.com/yangfan876@126/blog/static/80612456201342205056344)*/
/*[Boyer-Moore](http://blog.jobbole.com/52830/) */
/*[Knuth-Morris-Pratt](http://www.ruanyifeng.com/blog/2013/05/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm.html)*/

/*Boyer-Moore Algorithm*/
static void _istringfind_prebmbc(unsigned char *pattern, int m, int bmBc[]) {
    int i;
    
    for(i = 0; i < 256; i++) {
        bmBc[i] = m;
    }
    
    for(i = 0; i < m - 1; i++) {
        bmBc[(int)pattern[i]] = m - 1 - i;
    }
}
    
/*
static void _istringfind_suffix_old(unsigned char *pattern, int m, int suff[]) {
    int i, j;
    suff[m - 1] = m;
    
    for(i = m - 2; i >= 0; i--){
        j = i;
        while(j >= 0 && pattern[j] == pattern[m - 1 - i + j]) j--;
        suff[i] = i - j;
    }
}
*/

static void _istringfind_suffix(unsigned char *pattern, int m, int suff[]) {
    int f, g, i;
    
    suff[m - 1] = m;
    g = m - 1;
    for (i = m - 2; i >= 0; --i) {
        if (i > g && suff[i + m - 1 - f] < i - g)
            suff[i] = suff[i + m - 1 - f];
        else {
            if (i < g)
                g = i;
            f = i;
            while (g >= 0 && pattern[g] == pattern[g + m - 1 - f])
                --g;
            suff[i] = f - g;
        }
    }
}

static void _istringfind_prebmgs(unsigned char *pattern, int m, int bmGs[])
{
    int i, j;
    int suff[256];
    
    _istringfind_suffix(pattern, m, suff);

    for(i = 0; i < m; i++) {
        bmGs[i] = m;
    }
    
    j = 0;
    for(i = m - 1; i >= 0; i--) {
        if(suff[i] == i + 1) {
            for(; j < m - 1 - i; j++) {
                if(bmGs[j] == m)
                    bmGs[j] = m - 1 - i;
            }
        }
    }
    
    for(i = 0; i <= m - 2; i++) {
        bmGs[m - 1 - suff[i]] = m - 1 - i;
    }
}
iarray* _istringfind_boyermoore(unsigned char *pattern, int m, unsigned char *text, int n, int num)
{
    int i, j, bmBc[256], bmGs[256];
    iarray *indexs = iarraymakeint(num);
    
    /* Preprocessing */
    _istringfind_prebmbc(pattern, m, bmBc);
    _istringfind_prebmgs(pattern, m, bmGs);
    
    /* Searching */
    j = 0;
    while(j <= n - m && num) {
        for(i = m - 1; i >= 0 && pattern[i] == text[i + j]; i--);
        if(i < 0) {
            iarrayadd(indexs, &j);
            /*printf("Find it, the position is %d\n", j); */
            j += bmGs[0];
            --num;
        }else {
            j += imax(bmBc[text[i + j]] - m + 1 + i, bmGs[i]);
        }
    }
    
    return indexs;
}

/*find the index in istring */
int istringfind(const istring rfs, const char *sub, int len, int index) {
    iarray *indexs;
    icheckret(index>=0 && index<istringlen(rfs), kindex_invalid);
    
    indexs = _istringfind_boyermoore((unsigned char*)sub, len,
                                   (unsigned char*) (istringbuf(rfs) + index),
                                   istringlen(rfs)-index,
                                     1);
    
    if (iarraylen(indexs)) {
        index = iarrayof(indexs, int, 0);
        iarrayfree(indexs);
    }  else {
        index = kindex_invalid;
    }
    return index;
}

/*sub string*/
istring istringsub(const istring s, int begin, int end) {
    return istringlaw(islicedby(s, begin, end));
}

/*return the array of istring*/
iarray* istringsplit(const istring s, const char* split, int len) {
    int subindex = 0;
    int lastsubindex = 0;
    int i;
    size_t size = istringlen(s);
    istring sub;
    iarray* arr = iarraymakeiref(8);
    
    /*find all the sub*/
    iarray* indexs = _istringfind_boyermoore((unsigned char*)split, len,
                                             (unsigned char*) (istringbuf(s)),
                                             size,
                                             size);
    if (iarraylen(indexs)) {
        /*make enough space for subs*/
        iarrayexpandcapacity(arr, iarraylen(indexs));
        /*go through the sub*/
        for (i=0; i<iarraylen(indexs); ++i) {
            subindex = iarrayof(indexs, int, i);
            sub = istringsub(s, lastsubindex, subindex);
            lastsubindex = subindex + len;
            
            iarrayadd(arr, &sub);
            irelease(sub);
        }
        
        /*the last sub*/
        subindex = iarrayof(indexs, int, 0);
        sub = istringsub(s, lastsubindex, size);
        iarrayadd(arr, &sub);
        irelease(sub);
    } else {
        /*add the original string as the first sub*/
        iarrayadd(arr, &s);
    }
    
    /*free the find indexs*/
    iarrayfree(indexs);
    
    return arr;
}

/*return the array of sting joined by dealer */
istring istringjoin(const iarray* ss, const char* join, int len) {
    iarray *joined = iarraymakechar(8);
    istring s;
    size_t i = 0;
    size_t num = iarraylen(ss);
    /*the first one*/
    if (num) {
        s = iarrayof(ss, istring, 0);
        iarrayinsert(joined, 0, istringbuf(s), istringlen(s));
    }
    /*the after childs*/
    for (i=1; i<num; ++i) {
        iarrayinsert(joined, iarraylen(joined), join, len);
        s = iarrayof(ss, istring, i);
        iarrayinsert(joined, iarraylen(joined), istringbuf(s), istringlen(s));
    }
    
    /*make slice*/
    s = islicemakearg(joined, ":");
    iarrayfree(joined);
    return istringlaw(s);
}

/*return the new istring with new component*/
istring istringrepleace(const istring s, const char* olds, const char* news) {
    iarray *splits = istringsplit(s, olds, strlen(olds));
    istring ns = istringjoin(splits, news, strlen(news));
    iarrayfree(splits);
    
    return ns;
}

/*return the new istring append with value*/
istring istringappend(const istring s, const char* append) {
    istring ns;
    iarray *arr = iarraymakechar(istringlen(s) + strlen(append));
    iarrayinsert(arr, 0, istringbuf(s), istringlen(s));
    iarrayinsert(arr, iarraylen(arr), append, strlen(append));
    
    ns = islicemakearg(arr, ":");
    iarrayfree(arr);
    return istringlaw(ns);
}

/*baisc wrap for ::atoi */
int istringatoi(const istring s) {
    char buf[256+1] = {0};
    size_t size = istringlen(s);
    icheckret(size, 0);
    strncpy(buf, istringbuf(s), imin(256, size));
    
    return atoi(buf);
}

/*[cocos2dx](https://github.com/cocos2d/cocos2d-x/blob/v3/cocos/base/ccUtils.h)*/
/** Same to ::atof, but strip the string, remain 7 numbers after '.' before call atof.
 * Why we need this? Because in android c++_static, atof ( and std::atof )
 * is unsupported for numbers have long decimal part and contain
 * several numbers can approximate to 1 ( like 90.099998474121094 ), it will return inf.
 * This function is used to fix this bug.
 * @param str The string be to converted to double.
 * @return Returns converted value of a string.
 */
double istringatof(const istring s) {
    char buf[256+1] = {0};
    char* dot = NULL;
    size_t size = istringlen(s);
    
    icheckret(size, 0.0);
    strncpy(buf, istringbuf(s), imin(256, size));
    
    /* strip string, only remain 7 numbers after '.' */
    dot = strchr(buf, '.');
    if (dot != NULL && dot - buf + 8 < 256) {
        dot[8] = '\0';
    }
    
    return atof(buf);
}

/*************************************************************/
/* iringbuffer                                               */
/*************************************************************/

static void _iringbuffer_entry_free(iref *ref) {
    iringbuffer *r = icast(iringbuffer, ref);
    iarrayfree(r->buf);
    iobjfree(r);
}

/* Make a ring buffer */
iringbuffer *iringbuffermake(size_t capacity, int flag) {
    iringbuffer *r = iobjmalloc(iringbuffer);
    r->buf = iarraymakechar(capacity+1);
    r->buf->len = capacity;
    r->free = _iringbuffer_entry_free;
    r->flag = flag;
    
    iretain(r);
    return r;
}

/* release the ring buffer */
void iringbufferfree(iringbuffer *r) {
    irelease(r);
}

/* close the ring buffer: can not read and write */
void iringbufferclose(iringbuffer *r) {
    r->flag |= EnumRingBufferFlag_ReadChannelShut;
    r->flag |= EnumRingBufferFlag_WriteChannelShut;
}

/* shutdown the ringbuffer, to forbid the ringbuffer behavior */
void iringbuffershut(iringbuffer *r, int channel) {
    r->flag |= channel;
}

/* write s to ringbuffer, return count of write */
size_t iringbufferwrite(iringbuffer *rb, const char* s, size_t length) {
    size_t empty;
    size_t write;
    size_t finish = 0;
    size_t content;
    size_t capacity = iarraycapacity(rb->buf)-1;
    
    /* write to shut down rb */
    if (rb->flag & EnumRingBufferFlag_WriteChannelShut) {
        return -1;
    }
    
    if (finish < length) do {
        /* should break when got shut down */
        if (rb->flag & EnumRingBufferFlag_WriteChannelShut) {
            break;
        }
        /* current content size */
        content = rb->wlen - rb->rlen;
        
        /* overide buffer */
        if (rb->flag & EnumRingBufferFlag_Override) {
            empty = length - finish;
        } else {
            empty = imin(capacity - content, length - finish);
        }
        
        /* no space continue */
        if (empty == 0) {
            /* the write channel have been shutdown */
            if (rb->flag & EnumRingBufferFlag_WriteChannelShut) {
                break;
            }
            /* not block mode, will return right now */
            if (!(rb->flag & EnumRingBufferFlag_BlockWrite)) {
                break;
            }
            /* need sleep */
            if (rb->flag & EnumRingBufferFlag_WriteSleep) {
                isleep(0);
            }
            /* can be dynamic resize the space */
            continue;
        }
        
        /* write data */
        if (empty > 0) do {
            write = capacity - rb->wcursor;
            write = imin(write, empty);
            
            memcpy(rb->buf->buffer + rb->wcursor, s + finish, write);
            rb->wcursor += write;
            rb->wlen += write;
            if (rb->wcursor >= capacity) {
                rb->wcursor = 0;
            }
            
            finish += write;
            empty -= write;
        } while(empty > 0);
        
    } while(finish < length && (rb->flag & EnumRingBufferFlag_BlockWrite));
    
    return finish;
}

/* read to dst, until read full of dst, return the realy readed count */
size_t iringbufferread(iringbuffer *rb, char * dst, size_t length) {
    size_t full;
    size_t read;
    size_t finish = 0;
    size_t capacity = iarraycapacity(rb->buf)-1;
    
    /* write to shut down rb */
    if (rb->flag & EnumRingBufferFlag_ReadChannelShut) {
        return -1;
    }
    
    if (finish < length) do {
        /* write to shut down rb */
        if (rb->flag & EnumRingBufferFlag_ReadChannelShut) {
            break;
        }
        
        /* write override it */
        if (rb->flag & EnumRingBufferFlag_Override) {
            full = length - finish;
        } else {
            full = imin((rb->wlen - rb->rlen), length - finish);
        }
        
        /* no content continue */
        if (full == 0) {
            /* the write channel have been shutdown */
            if (rb->flag & EnumRingBufferFlag_WriteChannelShut) {
                break;
            }
            /* the read operating is not block, will return right now */
            if (!(rb->flag & EnumRingBufferFlag_BlockRead)) {
                break;
            }
            /* need sleep */
            if (rb->flag & EnumRingBufferFlag_ReadSleep) {
                isleep(0);
            }
            /* can be dynamic resize the space */
            continue;
        }
        
        /* read data */
        if (full > 0) do {
            read = capacity - rb->rcursor;
            read = imin(read, full);
            
            memcpy(dst + finish, rb->buf->buffer + rb->rcursor, read);
            rb->rcursor += read;
            rb->rlen += read;
            if (rb->rcursor >= capacity) {
                rb->rcursor = 0;
            }
            
            finish += read;
            full -= read;
        } while(full > 0);
        
    } while(finish < length && (rb->flag & EnumRingBufferFlag_BlockRead));
    
    return finish;
}

/* return the ready count of content */
size_t iringbufferready(iringbuffer *r) {
    return r->wlen - r->rlen;
}

/* giving the raw buffer address, unsafe behavior */
const char* iringbufferraw(iringbuffer *r) {
    return (const char*)iarraybuffer(r->buf);
}

/* Print to rb: support
 * %s(c null end string),
 * %i(signed int),
 * %I(signed 64 bit),
 * %u(unsigned int),
 * %U(unsigned 64bit) */
size_t iringbufferfmt(iringbuffer *rb, const char * fmt, ...) {
    const char *f = fmt;
    size_t i;
    va_list ap;
    
    char next, *str;
    size_t l;
    int64_t num;
    uint64_t unum;
    
    char buf[_IRB_LLSTR_SIZE];
    
    va_start(ap,fmt);
    f = fmt;    /* Next format specifier byte to process. */
    i = 0;
    while(*f) {
        
        /* Make sure there is always space for at least 1 char. */
        switch(*f) {
            case '%':
                next = *(f+1);
                f++;
                switch(next) {
                    case 's':
                    case 'S':
                        str = va_arg(ap,char*);
                        l = strlen(str);/*(next == 's') ?  : sdslen(str); */
                        iringbufferwrite(rb, str, l);
                        i += l;
                        break;
                    case 'i':
                    case 'I':
                        if (next == 'i')
                            num = va_arg(ap,int);
                        else
                            num = va_arg(ap,int64_t);
                    {
                        l = _ill2str(buf,num);
                        iringbufferwrite(rb, buf, l);
                        i += l;
                    }
                        break;
                    case 'u':
                    case 'U':
                        if (next == 'u')
                            unum = va_arg(ap,unsigned int);
                        else
                            unum = va_arg(ap,uint64_t);
                    {
                        l = _iull2str(buf,unum);
                        iringbufferwrite(rb, buf, l);
                        i += l;
                    }
                        break;
                    default: /* Handle %% and generally %<unknown>. */
                        iringbufferwrite(rb, f, 1);
                        break;
                }
                break;
            default:
                iringbufferwrite(rb, f, 1);
                break;
        }
        f++;
    }
    va_end(ap);
    
    return i;
}

/*************************************************************/
/*  idict                                                    */
/*************************************************************/
/*[redis-dict] (https://github.com/antirez/redis/blob/unstable/src/dict.c) */
/* Hash Tables Implementation.
 *
 * This file implements in memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Using idictenableresize() / dictdisableresize() we make possible to
 * enable/disable resizing of the hash table as needed. This is very important
 * for Redis, as we use copy-on-write and don't want to move too much memory
 * around when there is a child performing saving operations.
 *
 * Note that even when dict_can_resize is set to 0, not all resizes are
 * prevented: a hash table is still allowed to grow if the ratio between
 * the number of elements and the buckets > dict_force_resize_ratio. */
static int dict_can_resize = 1;
static unsigned int dict_force_resize_ratio = 5;

#define DICT_OK 0
#define DICT_ERR 1
#define DICT_HT_INITIAL_SIZE 4

/* ------------------------------- Macros ------------------------------------*/
#define idictfreeval(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

#define idictsetval(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        entry->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        entry->v.val = (_val_); \
    } while(0)

#define idictsetsignedintegerval(entry, _val_) \
do { entry->v.s64 = _val_; } while(0)

#define idictsetunsignedintegerval(entry, _val_) \
do { entry->v.u64 = _val_; } while(0)

#define idictsetdoubleval(entry, _val_) \
do { entry->v.d = _val_; } while(0)

#define idictfreekey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define idictsetkey(d, entry, _key_) do { \
        if ((d)->type->keyDup) \
            entry->key = (d)->type->keyDup((d)->privdata, _key_); \
        else \
            entry->key = (_key_); \
    } while(0)

#define idictcomparekeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
    (d)->type->keyCompare((d)->privdata, key1, key2) : \
    (key1) == (key2))

#define idicthashkey(d, key) (d)->type->hashFunction(key)
#define idictgetkey(he) ((he)->key)
#define idictgetval(he) ((he)->v.val)
#define idictgetsignedintegerval(he) ((he)->v.s64)
#define idictgetunsignedintegerval(he) ((he)->v.u64)
#define idictgetdoubleval(he) ((he)->v.d)
#define idictslots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define _idictsize(d) ((d)->ht[0].used+(d)->ht[1].used)
#define idictisrehashing(d) ((d)->rehashidx != -1)

/* -------------------------- private prototypes ---------------------------- */

static int _idictexpandifneeded(idict *ht);
static unsigned long _idictnextpower(unsigned long size);
static int _idictkeyindex(idict *d, const void *key);
static int _idictinit(idict *ht, idicttype *type, void *privDataPtr);
static idictentry *_idictfind(idict *d, const void *key);
static idictentry *_idictaddraw(idict *d, void *key);
static void *_idictfetchvalue(idict *d, const void *key);

/* -------------------------- private prototypes ---------------------------- */

/* Expand or create the hash table */
static int _idictexpand(idict *d, unsigned long size);

/* -------------------------- hash functions -------------------------------- */

/* Thomas Wang's 32 bit Mix Function */
/*
static unsigned int _idictinthashfunction(unsigned int key) {
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}
*/

static uint32_t dict_hash_function_seed = 5381;

void idictsethashfunctionseed(uint32_t seed) {
    dict_hash_function_seed = seed;
}

uint32_t idictgethashfunctionseed(void) {
    return dict_hash_function_seed;
}

/* MurmurHash2, by Austin Appleby
 * Note - This code makes a few assumptions about how your machine behaves -
 * 1. We can read a 4-byte value from any address without crashing
 * 2. sizeof(int) == 4
 *
 * And it has a few limitations -
 *
 * 1. It will not work incrementally.
 * 2. It will not produce the same results on little-endian and big-endian
 *    machines.
 */
unsigned int idictgenhashfunction(const void *key, int len) {
    /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
    uint32_t seed = dict_hash_function_seed;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    
    /* Initialize the hash to a 'random' value */
    uint32_t h = seed ^ len;
    
    /* Mix 4 bytes at a time into the hash */
    const unsigned char *data = (const unsigned char *)key;
    
    while(len >= 4) {
        uint32_t k = *(uint32_t*)data;
        
        k *= m;
        k ^= k >> r;
        k *= m;
        
        h *= m;
        h ^= k;
        
        data += 4;
        len -= 4;
    }
    
    /* Handle the last few bytes of the input array  */
    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0]; h *= m;
    };
    
    /* Do a few final mixes of the hash to ensure the last few
     * bytes are well-incorporated. */
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    
    return (unsigned int)h;
}

/* And a case insensitive hash function (based on djb hash) */
//static unsigned int _idictgencasehashfunction(const unsigned char *buf, int len) {
//    unsigned int hash = (unsigned int)dict_hash_function_seed;
//
//    while (len--)
//        hash = ((hash << 5) + hash) + (tolower(*buf++)); /* hash * 33 + c */
//    return hash;
//}

/* ----------------------------- API implementation ------------------------- */

/* Reset a hash table already initialized with ht_init().
 * NOTE: This function should only be called by ht_destroy(). */
static void _idictreset(idicthashtable *ht) {
    ht->table = NULL;
    ht->size = 0;
    ht->sizemask = 0;
    ht->used = 0;
}

/* Initialize the hash table */
static int _idictinit(idict *d, idicttype *type,
              void *privDataPtr) {
    _idictreset(&d->ht[0]);
    _idictreset(&d->ht[1]);
    d->type = type;
    d->privdata = privDataPtr;
    d->rehashidx = -1;
    d->iterators = 0;
    return DICT_OK;
}

/* Resize the table to the minimal size that contains all the elements,
 * but with the invariant of a USED/BUCKETS ratio near to <= 1 */
int idictresize(idict *d) {
    int minimal;
    
    if (!dict_can_resize || idictisrehashing(d)) return DICT_ERR;
    minimal = d->ht[0].used;
    if (minimal < DICT_HT_INITIAL_SIZE)
        minimal = DICT_HT_INITIAL_SIZE;
    return _idictexpand(d, minimal);
}

/* Expand or create the hash table */
static int _idictexpand(idict *d, unsigned long size) {
    idicthashtable n; /* the new hash table */
    unsigned long realsize = _idictnextpower(size);
    
    /* the size is invalid if it is smaller than the number of
     * elements already inside the hash table */
    if (idictisrehashing(d) || d->ht[0].used > size)
        return DICT_ERR;
    
    /* Rehashing to the same table size is not useful. */
    if (realsize == d->ht[0].size) return DICT_ERR;
    
    /* Allocate the new hash table and initialize all pointers to NULL */
    n.size = realsize;
    n.sizemask = realsize-1;
    n.table = icalloc(1, realsize*sizeof(idictentry*));
    n.used = 0;
    
    /* Is this the first initialization? If so it's not really a rehashing
     * we just set the first hash table so that it can accept keys. */
    if (d->ht[0].table == NULL) {
        d->ht[0] = n;
        return DICT_OK;
    }
    
    /* Prepare a second hash table for incremental rehashing */
    d->ht[1] = n;
    d->rehashidx = 0;
    return DICT_OK;
}

/* Performs N steps of incremental rehashing. Returns 1 if there are still
 * keys to move from the old to the new hash table, otherwise 0 is returned.
 *
 * Note that a rehashing step consists in moving a bucket (that may have more
 * than one key as we use chaining) from the old to the new hash table, however
 * since part of the hash table may be composed of empty spaces, it is not
 * guaranteed that this function will rehash even a single bucket, since it
 * will visit at max N*10 empty buckets in total, otherwise the amount of
 * work it does would be unbound and the function may block for a long time. */
static int _idictrehash(idict *d, int n) {
    int empty_visits = n*10; /* Max number of empty buckets to visit. */
    if (!idictisrehashing(d)) return 0;
    
    while(n-- && d->ht[0].used != 0) {
        idictentry *de, *nextde;
        
        /* Note that rehashidx can't overflow as we are sure there are more
         * elements because ht[0].used != 0 */
        assert(d->ht[0].size > (unsigned long)d->rehashidx);
        while(d->ht[0].table[d->rehashidx] == NULL) {
            d->rehashidx++;
            if (--empty_visits == 0) return 1;
        }
        de = d->ht[0].table[d->rehashidx];
        /* Move all the keys in this bucket from the old to the new hash HT */
        while(de) {
            unsigned int h;
            
            nextde = de->next;
            /* Get the index in the new hash table */
            h = idicthashkey(d, de->key) & d->ht[1].sizemask;
            de->next = d->ht[1].table[h];
            d->ht[1].table[h] = de;
            d->ht[0].used--;
            d->ht[1].used++;
            de = nextde;
        }
        d->ht[0].table[d->rehashidx] = NULL;
        d->rehashidx++;
    }
    
    /* Check if we already rehashed the whole table... */
    if (d->ht[0].used == 0) {
        ifree(d->ht[0].table);
        d->ht[0] = d->ht[1];
        _idictreset(&d->ht[1]);
        d->rehashidx = -1;
        return 0;
    }
    
    /* More to rehash... */
    return 1;
}

/* Rehash for an amount of time between ms milliseconds and ms+1 milliseconds */
int idictrehashmilliseconds(idict *d, int ms) {
    int64_t start = igetcurmicro();
    int rehashes = 0;
    
    while(_idictrehash(d,100)) {
        rehashes += 100;
        if (igetcurmicro()-start > ms) break;
    }
    return rehashes;
}

/* This function performs just a step of rehashing, and only if there are
 * no safe iterators bound to our hash table. When we have iterators in the
 * middle of a rehashing we can't mess with the two hash tables otherwise
 * some element can be missed or duplicated.
 *
 * This function is called by common lookup or update operations in the
 * dictionary so that the hash table automatically migrates from H1 to H2
 * while it is actively used. */
static void _idictrehashstep(idict *d) {
    if (d->iterators == 0) _idictrehash(d,1);
}

/* Add an element to the target hash table */
static int _idictadd(idict *d, void *key, void *val)
{
    idictentry *entry = _idictaddraw(d,key);
    
    if (!entry) return DICT_ERR;
    idictsetval(d, entry, val);
    return DICT_OK;
}

/* Low level add. This function adds the entry but instead of setting
 * a value returns the dictEntry structure to the user, that will make
 * sure to fill the value field as he wishes.
 *
 * This function is also directly exposed to the user API to be called
 * mainly in order to store non-pointers inside the hash value, example:
 *
 * entry = dictAddRaw(dict,mykey);
 * if (entry != NULL) dictSetSignedIntegerVal(entry,1000);
 *
 * Return values:
 *
 * If key already exists NULL is returned.
 * If key was added, the hash entry is returned to be manipulated by the caller.
 */
static idictentry *_idictaddraw(idict *d, void *key) {
    int index;
    idictentry *entry;
    idicthashtable *ht;
    
    if (idictisrehashing(d)) _idictrehashstep(d);
    
    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if ((index = _idictkeyindex(d, key)) == -1)
        return NULL;
    
    /* Allocate the memory and store the new entry.
     * Insert the element in top, with the assumption that in a database
     * system it is more likely that recently added entries are accessed
     * more frequently. */
    ht = idictisrehashing(d) ? &d->ht[1] : &d->ht[0];
    entry = icalloc(1, sizeof(*entry));
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;
    
    /* Set the hash entry fields. */
    idictsetkey(d, entry, key);
    return entry;
}

/* Add an element, discarding the old if the key already exists.
 * Return 1 if the key was added from scratch, 0 if there was already an
 * element with such key and dictReplace() just performed a value update
 * operation. */
static int _idictreplace(idict *d, void *key, void *val) {
    idictentry *entry, auxentry;
    
    /* Try to add the element. If the key
     * does not exists dictAdd will suceed. */
    if (_idictadd(d, key, val) == DICT_OK)
        return 1;
    /* It already exists, get the entry */
    entry = _idictfind(d, key);
    /* Set the new value and free the old one. Note that it is important
     * to do that in this order, as the value may just be exactly the same
     * as the previous one. In this context, think to reference counting,
     * you want to increment (set), and then decrement (free), and not the
     * reverse. */
    auxentry = *entry;
    idictsetval(d, entry, val);
    idictfreeval(d, &auxentry);
    return 0;
}

/* dictReplaceRaw() is simply a version of dictAddRaw() that always
 * returns the hash entry of the specified key, even if the key already
 * exists and can't be added (in that case the entry of the already
 * existing key is returned.)
 *
 * See dictAddRaw() for more information. */
idictentry *idictreplaceraw(idict *d, void *key) {
    idictentry *entry = _idictfind(d,key);
    
    return entry ? entry : _idictaddraw(d,key);
}

/* Search and remove an element */
static int _idictgenericdelete(idict *d, const void *key, int nofree) {
    unsigned int h, idx;
    idictentry *he, *prevHe;
    int table;
    
    if (d->ht[0].size == 0) return DICT_ERR; /* d->ht[0].table is NULL */
    if (idictisrehashing(d)) _idictrehashstep(d);
    h = idicthashkey(d, key);
    
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        prevHe = NULL;
        while(he) {
            if (idictcomparekeys(d, key, he->key)) {
                /* Unlink the element from the list */
                if (prevHe)
                    prevHe->next = he->next;
                else
                    d->ht[table].table[idx] = he->next;
                if (!nofree) {
                    idictfreekey(d, he);
                    idictfreeval(d, he);
                }
                ifree(he);
                d->ht[table].used--;
                return DICT_OK;
            }
            prevHe = he;
            he = he->next;
        }
        if (!idictisrehashing(d)) break;
    }
    return DICT_ERR; /* not found */
}

int idictdelete(idict *ht, const void *key) {
    return _idictgenericdelete(ht,key,0);
}

int idictdeletenofree(idict *ht, const void *key) {
    return _idictgenericdelete(ht,key,1);
}

/* Destroy an entire dictionary */
static int _idictclear(idict *d, idicthashtable *ht, void(callback)(void *)) {
    unsigned long i;
    
    /* Free all the elements */
    for (i = 0; i < ht->size && ht->used > 0; i++) {
        idictentry *he, *nextHe;
        
        if (callback && (i & 65535) == 0) callback(d->privdata);
        
        if ((he = ht->table[i]) == NULL) continue;
        while(he) {
            nextHe = he->next;
            idictfreekey(d, he);
            idictfreeval(d, he);
            ifree(he);
            ht->used--;
            he = nextHe;
        }
    }
    /* Free the table and the allocated cache structure */
    ifree(ht->table);
    /* Re-initialize the table */
    _idictreset(ht);
    return DICT_OK; /* never fails */
}

static idictentry *_idictfind(idict *d, const void *key) {
    idictentry *he;
    unsigned int h, idx, table;
    
    if (d->ht[0].size == 0) return NULL; /* We don't have a table at all */
    if (idictisrehashing(d)) _idictrehashstep(d);
    h = idicthashkey(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        while(he) {
            if (idictcomparekeys(d, key, he->key))
                return he;
            he = he->next;
        }
        if (!idictisrehashing(d)) return NULL;
    }
    return NULL;
}

static void *_idictfetchvalue(idict *d, const void *key) {
    idictentry *he;
    
    he = _idictfind(d,key);
    return he ? idictgetval(he) : NULL;
}

/* A fingerprint is a 64 bit number that represents the state of the dictionary
 * at a given time, it's just a few dict properties xored together.
 * When an unsafe iterator is initialized, we get the dict fingerprint, and check
 * the fingerprint again when the iterator is released.
 * If the two fingerprints are different it means that the user of the iterator
 * performed forbidden operations against the dictionary while iterating. */
static uint64_t _idictfingerprint(idict *d) {
    uint64_t integers[6], hash = 0;
    int j;
    
    integers[0] = (long) d->ht[0].table;
    integers[1] = d->ht[0].size;
    integers[2] = d->ht[0].used;
    integers[3] = (long) d->ht[1].table;
    integers[4] = d->ht[1].size;
    integers[5] = d->ht[1].used;
    
    /* We hash N integers by summing every successive integer with the integer
     * hashing of the previous sum. Basically:
     *
     * Result = hash(hash(hash(int1)+int2)+int3) ...
     *
     * This way the same set of integers in a different order will (likely) hash
     * to a different number. */
    for (j = 0; j < 6; j++) {
        hash += integers[j];
        /* For the hashing step we use Tomas Wang's 64 bit integer hash. */
        hash = (~hash) + (hash << 21); /* hash = (hash << 21) - hash - 1; */
        hash = hash ^ (hash >> 24);
        hash = (hash + (hash << 3)) + (hash << 8); /* hash * 265 */
        hash = hash ^ (hash >> 14);
        hash = (hash + (hash << 2)) + (hash << 4); /* hash * 21 */
        hash = hash ^ (hash >> 28);
        hash = hash + (hash << 31);
    }
    return hash;
}

static idictiterator *_idictgetiterator(idict *d) {
    idictiterator *iter = icalloc(1, sizeof(*iter));
    
    iter->d = d;
    iter->table = 0;
    iter->index = -1;
    iter->safe = 0;
    iter->entry = NULL;
    iter->nextEntry = NULL;
    return iter;
}

idictiterator * idictgetsafeiterator(idict *d) {
    idictiterator *i = _idictgetiterator(d);
    
    i->safe = 1;
    return i;
}

idictentry *idictNext(idictiterator *iter) {
    while (1) {
        if (iter->entry == NULL) {
            idicthashtable *ht = &iter->d->ht[iter->table];
            if (iter->index == -1 && iter->table == 0) {
                if (iter->safe)
                    iter->d->iterators++;
                else
                    iter->fingerprint = _idictfingerprint(iter->d);
            }
            iter->index++;
            if (iter->index >= (long) ht->size) {
                if (idictisrehashing(iter->d) && iter->table == 0) {
                    iter->table++;
                    iter->index = 0;
                    ht = &iter->d->ht[1];
                } else {
                    break;
                }
            }
            iter->entry = ht->table[iter->index];
        } else {
            iter->entry = iter->nextEntry;
        }
        if (iter->entry) {
            /* We need to save the 'next' here, the iterator user
             * may delete the entry we are returning. */
            iter->nextEntry = iter->entry->next;
            return iter->entry;
        }
    }
    return NULL;
}

void idictreleaseiterator(idictiterator *iter) {
    if (!(iter->index == -1 && iter->table == 0)) {
        if (iter->safe)
            iter->d->iterators--;
        else
            assert(iter->fingerprint == _idictfingerprint(iter->d));
    }
    ifree(iter);
}

/* Return a random entry from the hash table. Useful to
 * implement randomized algorithms */
idictentry *idictgetrandomkey(idict *d) {
    idictentry *he, *orighe;
    unsigned int h;
    int listlen, listele;
    
    if (idictsize(d) == 0) return NULL;
    if (idictisrehashing(d)) _idictrehashstep(d);
    if (idictisrehashing(d)) {
        do {
            /* We are sure there are no elements in indexes from 0
             * to rehashidx-1 */
            h = d->rehashidx + (random() % (d->ht[0].size +
                                            d->ht[1].size -
                                            d->rehashidx));
            he = (h >= d->ht[0].size) ? d->ht[1].table[h - d->ht[0].size] :
            d->ht[0].table[h];
        } while(he == NULL);
    } else {
        do {
            h = random() & d->ht[0].sizemask;
            he = d->ht[0].table[h];
        } while(he == NULL);
    }
    
    /* Now we found a non empty bucket, but it is a linked
     * list and we need to get a random element from the list.
     * The only sane way to do so is counting the elements and
     * select a random index. */
    listlen = 0;
    orighe = he;
    while(he) {
        he = he->next;
        listlen++;
    }
    listele = random() % listlen;
    he = orighe;
    while(listele--) he = he->next;
    return he;
}

/* This function samples the dictionary to return a few keys from random
 * locations.
 *
 * It does not guarantee to return all the keys specified in 'count', nor
 * it does guarantee to return non-duplicated elements, however it will make
 * some effort to do both things.
 *
 * Returned pointers to hash table entries are stored into 'des' that
 * points to an array of dictEntry pointers. The array must have room for
 * at least 'count' elements, that is the argument we pass to the function
 * to tell how many random elements we need.
 *
 * The function returns the number of items stored into 'des', that may
 * be less than 'count' if the hash table has less than 'count' elements
 * inside, or if not enough elements were found in a reasonable amount of
 * steps.
 *
 * Note that this function is not suitable when you need a good distribution
 * of the returned items, but only when you need to "sample" a given number
 * of continuous elements to run some kind of algorithm or to produce
 * statistics. However the function is much faster than dictGetRandomKey()
 * at producing N elements. */
unsigned int idictgetsomekeys(idict *d, idictentry **des, unsigned int count) {
    unsigned long j; /* internal hash table id, 0 or 1. */
    unsigned long tables; /* 1 or 2 tables? */
    unsigned long stored = 0, maxsizemask;
    unsigned long maxsteps;
    unsigned long i;
    unsigned long emptylen;
    idictentry *he;
    
    if (idictsize(d) < count) count = idictsize(d);
    maxsteps = count*10;
    
    /* Try to do a rehashing work proportional to 'count'. */
    for (j = 0; j < count; j++) {
        if (idictisrehashing(d))
            _idictrehashstep(d);
        else
            break;
    }
    
    tables = idictisrehashing(d) ? 2 : 1;
    maxsizemask = d->ht[0].sizemask;
    if (tables > 1 && maxsizemask < d->ht[1].sizemask)
        maxsizemask = d->ht[1].sizemask;
    
    /* Pick a random point inside the larger table. */
    i = random() & maxsizemask;
    emptylen = 0; /* Continuous empty entries so far. */
    while(stored < count && maxsteps--) {
        for (j = 0; j < tables; j++) {
            /* Invariant of the dict.c rehashing: up to the indexes already
             * visited in ht[0] during the rehashing, there are no populated
             * buckets, so we can skip ht[0] for indexes between 0 and idx-1. */
            if (tables == 2 && j == 0 && i < (unsigned long) d->rehashidx) {
                /* Moreover, if we are currently out of range in the second
                 * table, there will be no elements in both tables up to
                 * the current rehashing index, so we jump if possible.
                 * (this happens when going from big to small table). */
                if (i >= d->ht[1].size) i = d->rehashidx;
                continue;
            }
            if (i >= d->ht[j].size) continue; /* Out of range for this table. */
            he = d->ht[j].table[i];
            
            /* Count contiguous empty buckets, and jump to other
             * locations if they reach 'count' (with a minimum of 5). */
            if (he == NULL) {
                emptylen++;
                if (emptylen >= 5 && emptylen > count) {
                    i = random() & maxsizemask;
                    emptylen = 0;
                }
            } else {
                emptylen = 0;
                while (he) {
                    /* Collect all the elements of the buckets found non
                     * empty while iterating. */
                    *des = he;
                    des++;
                    he = he->next;
                    stored++;
                    if (stored == count) return stored;
                }
            }
        }
        i = (i+1) & maxsizemask;
    }
    return stored;
}

/* Function to reverse bits. Algorithm from:
 * http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel */
static unsigned long _irev(unsigned long v) {
    unsigned long s = 8 * sizeof(v); /* bit size; must be power of 2 */
    unsigned long mask = ~0;
    while ((s >>= 1) > 0) {
        mask ^= (mask << s);
        v = ((v >> s) & mask) | ((v << s) & ~mask);
    }
    return v;
}

/* idictscan() is used to iterate over the elements of a dictionary.
 *
 * Iterating works the following way:
 *
 * 1) Initially you call the function using a cursor (v) value of 0.
 * 2) The function performs one step of the iteration, and returns the
 *    new cursor value you must use in the next call.
 * 3) When the returned cursor is 0, the iteration is complete.
 *
 * The function guarantees all elements present in the
 * dictionary get returned between the start and end of the iteration.
 * However it is possible some elements get returned multiple times.
 *
 * For every element returned, the callback argument 'fn' is
 * called with 'privdata' as first argument and the dictionary entry
 * 'de' as second argument.
 *
 * HOW IT WORKS.
 *
 * The iteration algorithm was designed by Pieter Noordhuis.
 * The main idea is to increment a cursor starting from the higher order
 * bits. That is, instead of incrementing the cursor normally, the bits
 * of the cursor are reversed, then the cursor is incremented, and finally
 * the bits are reversed again.
 *
 * This strategy is needed because the hash table may be resized between
 * iteration calls.
 *
 * dict.c hash tables are always power of two in size, and they
 * use chaining, so the position of an element in a given table is given
 * by computing the bitwise AND between Hash(key) and SIZE-1
 * (where SIZE-1 is always the mask that is equivalent to taking the rest
 *  of the division between the Hash of the key and SIZE).
 *
 * For example if the current hash table size is 16, the mask is
 * (in binary) 1111. The position of a key in the hash table will always be
 * the last four bits of the hash output, and so forth.
 *
 * WHAT HAPPENS IF THE TABLE CHANGES IN SIZE?
 *
 * If the hash table grows, elements can go anywhere in one multiple of
 * the old bucket: for example let's say we already iterated with
 * a 4 bit cursor 1100 (the mask is 1111 because hash table size = 16).
 *
 * If the hash table will be resized to 64 elements, then the new mask will
 * be 111111. The new buckets you obtain by substituting in ??1100
 * with either 0 or 1 can be targeted only by keys we already visited
 * when scanning the bucket 1100 in the smaller hash table.
 *
 * By iterating the higher bits first, because of the inverted counter, the
 * cursor does not need to restart if the table size gets bigger. It will
 * continue iterating using cursors without '1100' at the end, and also
 * without any other combination of the final 4 bits already explored.
 *
 * Similarly when the table size shrinks over time, for example going from
 * 16 to 8, if a combination of the lower three bits (the mask for size 8
 * is 111) were already completely explored, it would not be visited again
 * because we are sure we tried, for example, both 0111 and 1111 (all the
 * variations of the higher bit) so we don't need to test it again.
 *
 * WAIT... YOU HAVE *TWO* TABLES DURING REHASHING!
 *
 * Yes, this is true, but we always iterate the smaller table first, then
 * we test all the expansions of the current cursor into the larger
 * table. For example if the current cursor is 101 and we also have a
 * larger table of size 16, we also test (0)101 and (1)101 inside the larger
 * table. This reduces the problem back to having only one table, where
 * the larger one, if it exists, is just an expansion of the smaller one.
 *
 * LIMITATIONS
 *
 * This iterator is completely stateless, and this is a huge advantage,
 * including no additional memory used.
 *
 * The disadvantages resulting from this design are:
 *
 * 1) It is possible we return elements more than once. However this is usually
 *    easy to deal with in the application level.
 * 2) The iterator must return multiple elements per call, as it needs to always
 *    return all the keys chained in a given bucket, and all the expansions, so
 *    we are sure we don't miss keys moving during rehashing.
 * 3) The reverse cursor is somewhat hard to understand at first, but this
 *    comment is supposed to help.
 */
unsigned long idictscan(idict *d,
                       unsigned long v,
                       idictscanfunction fn,
                       void *privdata) {
    idicthashtable *t0, *t1;
    const idictentry *de, *next;
    unsigned long m0, m1;
    
    if (idictsize(d) == 0) return 0;
    
    if (!idictisrehashing(d)) {
        t0 = &(d->ht[0]);
        m0 = t0->sizemask;
        
        /* Emit entries at cursor */
        de = t0->table[v & m0];
        while (de) {
            next = de->next;
            fn(privdata, de);
            de = next;
        }
        
    } else {
        t0 = &d->ht[0];
        t1 = &d->ht[1];
        
        /* Make sure t0 is the smaller and t1 is the bigger table */
        if (t0->size > t1->size) {
            t0 = &d->ht[1];
            t1 = &d->ht[0];
        }
        
        m0 = t0->sizemask;
        m1 = t1->sizemask;
        
        /* Emit entries at cursor */
        de = t0->table[v & m0];
        while (de) {
            next = de->next;
            fn(privdata, de);
            de = next;
        }
        
        /* Iterate over indices in larger table that are the expansion
         * of the index pointed to by the cursor in the smaller table */
        do {
            /* Emit entries at cursor */
            de = t1->table[v & m1];
            while (de) {
                next = de->next;
                fn(privdata, de);
                de = next;
            }
            
            /* Increment bits not covered by the smaller mask */
            v = (((v | m0) + 1) & ~m0) | (v & m0);
            
            /* Continue while bits covered by mask difference is non-zero */
        } while (v & (m0 ^ m1));
    }
    
    /* Set unmasked bits so incrementing the reversed cursor
     * operates on the masked bits of the smaller table */
    v |= ~m0;
    
    /* Increment the reverse cursor */
    v = _irev(v);
    v++;
    v = _irev(v);
    
    return v;
}

/* ------------------------- private functions ------------------------------ */

/* Expand the hash table if needed */
static int _idictexpandifneeded(idict *d) {
    /* Incremental rehashing already in progress. Return. */
    if (idictisrehashing(d)) return DICT_OK;
    
    /* If the hash table is empty expand it to the initial size. */
    if (d->ht[0].size == 0) return _idictexpand(d, DICT_HT_INITIAL_SIZE);
    
    /* If we reached the 1:1 ratio, and we are allowed to resize the hash
     * table (global setting) or we should avoid it but the ratio between
     * elements/buckets is over the "safe" threshold, we resize doubling
     * the number of buckets. */
    if (d->ht[0].used >= d->ht[0].size &&
        (dict_can_resize ||
         d->ht[0].used/d->ht[0].size > dict_force_resize_ratio)) {
        return _idictexpand(d, d->ht[0].used*2);
    }
    return DICT_OK;
}

/* Our hash table capability is a power of two */
static unsigned long _idictnextpower(unsigned long size) {
    unsigned long i = DICT_HT_INITIAL_SIZE;
    
    if (size >= INT32_MAX) return INT32_MAX;
    while(1) {
        if (i >= size)
            return i;
        i *= 2;
    }
}

/* Returns the index of a free slot that can be populated with
 * a hash entry for the given 'key'.
 * If the key already exists, -1 is returned.
 *
 * Note that if we are in the process of rehashing the hash table, the
 * index is always returned in the context of the second (new) hash table. */
static int _idictkeyindex(idict *d, const void *key) {
    unsigned int h, idx, table;
    idictentry *he;
    
    /* Expand the hash table if needed */
    if (_idictexpandifneeded(d) == DICT_ERR)
        return -1;
    /* Compute the key hash value */
    h = idicthashkey(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        /* Search if this slot does not already contain the given key */
        he = d->ht[table].table[idx];
        while(he) {
            if (idictcomparekeys(d, key, he->key))
                return -1;
            he = he->next;
        }
        if (!idictisrehashing(d)) break;
    }
    return idx;
}

static void _idictempty(idict *d, void(callback)(void*)) {
    _idictclear(d,&d->ht[0],callback);
    _idictclear(d,&d->ht[1],callback);
    d->rehashidx = -1;
    d->iterators = 0;
}

void idictenableresize(void) {
    dict_can_resize = 1;
}

void idictdisableresize(void) {
    dict_can_resize = 0;
}

/* Clear & Release the hash table */
/* release the hash table and free dict self */
static void _idict_entry_free(iref *ref) {
    idict *d = icast(idict, ref);
    
    _idictclear(d,&d->ht[0],NULL);
    _idictclear(d,&d->ht[1],NULL);
    
    iobjfree(d);
}

/* make dict with type and privdata */
idict *idictmake(idicttype *type, void *privdata) {
    idict *d = iobjmalloc(idict);
    d->free = _idict_entry_free;
    _idictinit(d, type, privdata);
    return d;
}

/* get value by key */
void *idictget(idict *d, const void *key) {
    return _idictfetchvalue(d, key);
}

/* set value by key */
int idictset(idict *d, const void *key, void *value) {
    return _idictreplace(d, (void*)key, value);
}

/* Search and remove an element */
int idictremove(idict *d, const void *key) {
    return idictdelete(d, key);
}

/* get the dict size */
size_t idictsize(idict *d){
    return (size_t)(_idictsize(d));
}

/* clear all the key-values */
void idictclear(idict *d) {
    _idictempty(d, NULL);
}

/* if the dict has the key-value */
int idicthas(idict *d, const void *key) {
    idictentry *he;
    
    he = _idictfind(d,key);
    return he != NULL;
}

/*************************************************************/
/* ipolygon3d                                                */
/*************************************************************/

/* free resouces of polygon3d */
static void _ipolygon3d_entry_free(iref *ref) {
    ipolygon3d *poly = (ipolygon3d*)ref;
    irelease(poly->pos);
    poly->pos = NULL;
    
    iobjfree(poly);
}

/* create a polygon 3d*/
ipolygon3d *ipolygon3dmake(size_t capacity){
    ipolygon3d *poly = iobjmalloc(ipolygon3d);
    iarray* array = iarraymakeipos3(capacity);
    poly->pos = isliced(array, 0, 0);
    poly->free = _ipolygon3d_entry_free;
    
    /* the min pos */
    poly->min.x = 0x1.fffffep+127f;
    poly->min.y = 0x1.fffffep+127f;
    poly->min.z = 0x1.fffffep+127f;
    
    irelease(array);
    iretain(poly);
    return poly;
}

/* free a polygon 3d*/
void ipolygon3dfree(ipolygon3d *poly) {
    irelease(poly);
}

/* add ivec3 to polygon*/
void ipolygon3dadd(ipolygon3d *poly, const ipos3 *v, int nums) {
    int i;
    int j;
    ireal *values;
    ireal *max_values = (ireal*)&(poly->max);
    ireal *min_values = (ireal*)&(poly->min);
    ireal *accu_values = (ireal*)&(poly->accumulating);
    int slicelen = islicelen(poly->pos);
    icheck(v);
    icheck(poly);
    icheck(nums);
    
    /* update the min and max*/
    for (j=0; j<nums; ++j) {
        values = (ireal*)(&v[j]);
        for (i=0; i<3; ++i) {
            /* accumulating all pos */
            accu_values[i] += values[i];
            /* caculating the max and min */
            if (values[i] > max_values[i]) {
                /* for max */
                max_values[i] = values[i];
            } else if (values[i] < min_values[i]) {
                /* for min */
                min_values[i] = values[i];
            }
        }
    }
    
    /* add vec3 */
    poly->pos = isliceappendvalues(poly->pos, v, nums);
    
    /* set polygon plane */
    if (slicelen<3 && islicelen(poly->pos) >= 3) {
        /* set plane point */
        iplaneset(&poly->plane,
                  &isliceof(poly->pos, ipos3, 0),
                  &isliceof(poly->pos, ipos3, 1),
                  &isliceof(poly->pos, ipos3, 2));
    }
    
    /* auto polygon3d finish after add pos */
    ipolygon3dfinish(poly);
}

/* caculating the center of polygon3d  */
void ipolygon3dfinish(ipolygon3d *poly) {
    size_t n = islicelen(poly->pos);
    icheck(n > 1);
    poly->center.x = poly->accumulating.x / n;
    poly->center.y = poly->accumulating.y / n;
    poly->center.z = poly->accumulating.z / n;
}

/* take the polygon3d as a wrap buffer of pos */
const ipos3 *ipolygon3dpos3(ipolygon3d *polygon, int index) {
    size_t len = islicelen(polygon->pos);
    icheckret(len>0, &kipos3_zero);
    return (const ipos3 *) isliceat(polygon->pos, index%len);
}

/* take the polygon3d pos (x, z) as a wrap buffer of pos */
ipos ipolygon3dposxz(ipolygon3d *polygon, int index) {
    const ipos3* p3 = ipolygon3dpos3(polygon, index);
    ipos p = {p3->x, p3->z};
    return p;
}

/* the the edge (center-middle) point*/
ipos3 ipolygon3dedgecenter(ipolygon3d *polygon, int index) {
    const ipos3* p3_start = ipolygon3dpos3(polygon, index);
    const ipos3* p3_end = ipolygon3dpos3(polygon, index+1);
    ipos3 center = {
        (p3_start->x + p3_end->x)/2,
        (p3_start->y + p3_end->y)/2,
        (p3_start->z + p3_end->z)/2,
    };
    return center;
}

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
int ipolygon3dincollum(const ipolygon3d *poly, const ipos3 *v) {
    int inside = iino;
    int i, j, n;
    ipos3 *ui, *uj;
    
    icheckret(v, iiok);
    icheckret(poly, iino);
    
    /* beyond the bounding box*/
    if (v->x < poly->min.x ||
        v->x > poly->max.x ||
        v->z < poly->min.z ||
        v->z > poly->max.z) {
        return iino;
    }
    
    /* https://en.wikipedia.org/wiki/Point_in_polygon
     */
    n = islicelen(poly->pos);
    for (i = 0, j = n-1; i<n; j = i++) {
        ui = (ipos3*)isliceat(poly->pos, i);
        uj = (ipos3*)isliceat(poly->pos, j);
        if ((ui->z > v->z) != (uj->z > v->z) &&
            v->x < ((uj->x - ui->x)
                      * (v->z - ui->z)
                      / (uj->z - ui->z)
                      + ui->x) ) {
                inside = !inside;
            }
    }
    
    return inside;
}
    
/* give the projection rect in xz-plane */
void ipolygon3dtakerectxz(const ipolygon3d *poly, irect *r) {
    ipos min, max;
    ipos3takexz(&poly->min, &min);
    ipos3takexz(&poly->max, &max);
    r->pos = min;
    r->size.w = max.x - min.x;
    r->size.h = max.y - min.y;
}
    
/*************************************************************/
/* ipolygon2d                                                */
/*************************************************************/

/* free resouces of polygon3d */
static void _ipolygon2d_entry_free(iref *ref) {
    ipolygon2d *poly = (ipolygon2d*)ref;
    irelease(poly->slice);
    poly->slice = NULL;
    
    iobjfree(poly);
}

/* create a polygon 2d*/
ipolygon2d *ipolygon2dmake(size_t capacity) {
    ipolygon2d *poly = iobjmalloc(ipolygon2d);
    iarray* array = iarraymakeivec2(capacity);
    poly->slice = isliced(array, 0, 0);
    poly->free = _ipolygon2d_entry_free;
    
    irelease(array);
    iretain(poly);
    return poly;
}

/* free a polygon 2d*/
void ipolygon2dfree(ipolygon2d *poly) {
    irelease(poly);
}

/* add ivec2 to polygon*/
void ipolygon2dadd(ipolygon2d *poly, const ivec2 *v, int nums) {
    int i;
    int j;
    icheck(v);
    icheck(poly);
    
    /* update the min and max*/
    for (j=0; j<nums; ++j) {
        for (i=0; i<2; ++i) {
            if (v[j].values[i] > poly->max.values[i]) {
                /* for max */
                poly->max.values[i] = v[j].values[i];
            } else if (v[j].values[i] < poly->min.values[i]) {
                /* for min */
                poly->min.values[i] = v[j].values[i];
            }
        }
    }
    
    /* add vec2 */
    poly->slice = isliceappendvalues(poly->slice, v, nums);
}

/* if the point in polygon
 * http://stackoverflow.com/questions/217578/how-can-i-determine-whether-a-2d-point-is-within-a-polygon
 * https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
 **/
int ipolygon2dcontains(const ipolygon2d *poly, const ivec2 *v) {
    int inside = iino;
    int i, j, n;
    ivec2 *ui, *uj;
    
    icheckret(v, iiok);
    icheckret(poly, iino);
    
    /* beyond the bounding box*/
    if (v->v.x < poly->min.v.x ||
        v->v.x > poly->max.v.x ||
        v->v.y < poly->min.v.y ||
        v->v.y > poly->max.v.y) {
        return iino;
    }
    
    /* https://en.wikipedia.org/wiki/Point_in_polygon
     */
    n = islicelen(poly->slice);
    for (i = 0, j = n-1; i<n; j = i++) {
        ui = (ivec2*)isliceat(poly->slice, i);
        uj = (ivec2*)isliceat(poly->slice, j);
        if ((ui->v.y>v->v.y) != (uj->v.y > v->v.y) &&
            v->v.x < ((uj->v.x - ui->v.x)
                        * (v->v.y - ui->v.y)
                        / (uj->v.y - ui->v.y)
                        + ui->v.x) ) {
            inside = !inside;
        }
    }
    
    return inside;
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
        /* may release some resource hold by ref */
        if (cache->whenadd) {
            cache->whenadd(ref);
        }
        /* entry the cache */
		ireflistadd(cache->cache, ref);
	}else if (cache->envicted){
        /* the cache is full, may expand the capacity */
		cache->envicted(ref);
	}
}

/* 创造一个cache */
irefcache *irefcachemake(size_t capacity, icachenewentry newentry) {
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
size_t irefcachesize(irefcache *cache) {
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

	ineighborsclean(icast(irefneighbors, node));
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
    
/*坐标是否在节点里面*/
int inodecontains(const struct imap *map, const inode *node, const ipos *pos) {
    irect r = {node->code.pos, map->nodesizes[node->level]};
    return irectcontainspoint(&r, pos);
}
    
/* 从子节点的时间戳更新父节点 */
int inodeupdatparenttick(inode *node) {
    inode *parent = node->parent;
    icheckret(parent, iino);
    
	parent->tick = node->tick;
#if open_node_utick
	parent->utick = node->tick;
#endif
    return iiok;
}
    
/* 从单元身上获取更新时间戳*/
int inodeupdatetickfromunit(inode *node, iunit *unit) {
    node->tick = unit->tick;
#if open_node_utick
    node->utick = unit->tick;
#endif   
    return iiok;
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
    
	++map->state.unitcount;

	unit->node = node;
	unit->tick = igetnextmicro();

	node->unitcnt++;
    inodeupdatetickfromunit(node, unit);

#if open_log_unit
	ilog("[IMAP-Unit] Add Unit (%" PRId64 ", %s) To Node (%d, %s)\n",
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
    
    /* 移除单元统计 */
	--map->state.unitcount;

#if open_log_unit
	ilog("[IMAP-Unit] Remove Unit (%" PRId64 ", %s) From Node (%d, %s)\n",
			unit->id, unit->code.code, node->level, node->code.code);
#endif
    
    unit->node = NULL;
	unit->tick = igetnextmicro();

	node->unitcnt--;
    inodeupdatetickfromunit(node, unit);

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
		ilog("[IMAP-Unit-Add] Unit(%" PRId64 ", %s, x: %.3f, y: %.3f) To Node (%d, %s, %p) \n", \
				unit->id, unit->code.code, unit->code.pos.x, unit->code.pos.y, node->level, node->code.code, node); \
	}while(0)

/* 打印单元移除出节点的操作 */
#define _print_unit_remove(node, unit, idx) \
	do {\
		ilog("[IMAP-Unit-Remove] Unit(%" PRId64 ", %s, x: %.3f, y: %.3f) From Node (%d, %s, %p) \n", \
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
#if open_node_utick
	node->utick = 0;
#endif
	node->x = node->y = 0;
	node->state = 0;

	/* 清理 邻居 节点*/
	ineighborsclean(icast(irefneighbors, node));

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
	ilog("[IMAP-Code] (%" PRId64 ", %s, %p) Code %d, Idx: %d, Divide: %d\n", unit->id, unit->code.code, unit, code, idx, map->divide);
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
            inodeupdatparenttick(child);
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
            inodeupdatparenttick(child);
		}else {
			/* 已经在 justremoveunit 更新了时间戳 */
		}
		/* 回收可能的节点 */
        imaprecyclenodeaspossible(map, node, stop);
	}
	return ok;
}
    
/* 尽可能的回收节点 */
int imaprecyclenodeaspossible(imap *map, inode *node, inode *stop) {
	if (node != stop /* 不是停止节点 */
			&& !_state_is(node->state, EnumNodeStateStatic) /* 不是静态节点 */
			&& node->childcnt == 0 /* 孩子节点为0 */
			&& node->unitcnt == 0 /* 上面绑定的单元节点也为空 */
	   ) {
		_iremovenodefromparent(map, node);
        return iiok;
	}
    return iino;
}

    
/* 根据坐标生成code */
int imapgencode(const imap *map, const ipos *pos, icode *code) {
    return imapgencodewithlevel(map, pos, code, map->divide);
}

/* 根据坐标生成code */
int imapgencodewithlevel(const imap *map, const ipos *pos, icode *code, int level) {
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
	for( i=0; i<level; ++i) {
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
	code->code[i] = 0;

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
		ilog("%s Node: Count=%" PRId64 "\n", tag, map->state.nodecount);
		ilog("%s Node-Leaf: Count=%" PRId64 "\n", tag, map->state.leafcount);
		ilog("%s Node-Cache: Count=%lu\n", tag, irefcachesize(map->nodecache));
	}
	/* 单元信息 */
	if (require & EnumMapStateUnit) {
		ilog("%s Unit: Count=%" PRId64 "\n", tag, map->state.unitcount);
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

    /* 把单元加入地图， 到指定的分割层次 */
int imapaddunittolevel(imap *map, iunit *unit, int level) {
	int ok;
	int64_t micro;

	icheckret(unit, iino);
	icheckret(map, iino);

	/* move out side */
	imapgencodewithlevel(map, &unit->pos, &unit->code, level);

	/* log it */
#if open_log_unit
	ilog("[IMAP-Unit] Add Unit: %" PRId64 " - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
#endif
	micro = __Micros;
	ok = imapaddunitto(map, map->root, unit, 0);
	iplog(__Since(micro), "[IMAP-Unit] Add Unit: %" PRId64 " - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
	return ok;
}
    
/* 增加一个单元到地图上 */
int imapaddunit(imap *map, iunit *unit) {
    return imapaddunittolevel(map, unit, map->divide);
}

#define __ii_aoi_remove_direct (1)
/* 从地图上移除一个单元 */
int imapremoveunit(imap *map, iunit *unit) {
	int ok;
	int64_t micro;

	icheckret(unit, iino);
	icheckret(unit->node, iino);
	icheckret(map, iino);
    
    /* open the direct remove */
#if __ii_aoi_remove_direct
    if (imapremoveunitdirect(map, unit) == iiok) {
        return iiok;
    }
#endif

	/* log it */
#if open_log_unit
	ilog("[IMAP-Unit] Remove Unit: %" PRId64 " - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
#endif
	micro = __Micros;
	ok = imapremoveunitfrom(map, map->root, unit, 0, map->root);
	iplog(__Since(micro), "[IMAP-Unit] Remove Unit: "
			"%" PRId64 " - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);

	return ok;
}
    
/* direct remove the unit from the unit->node */
int imapremoveunitdirect(imap *map, iunit *unit) {
    inode *node = unit->node;
    inode *stop = map->root;
    inode *remove = NULL;
	int64_t micro;
    
    icheckret(unit, iino);
	icheckret(unit->node, iino);
	icheckret(map, iino);
    
	/* log it */
#if open_log_unit
	ilog("[IMAP-Unit] Remove Unit: %" PRId64 " - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);
#endif
	micro = __Micros;
    
    /*移除节点*/
    justremoveunit(map, node, unit);
    
    /*更新父亲时间戳，回收相关节点*/
    while (node->parent) {
        /*更新父节点时间戳*/
        inodeupdatparenttick(node);
        
        /* 记录可以尝试回收可能的节点 */
        remove = node;
        node = node->parent;
        /* 回收可能的节点 */
        if (stop && imaprecyclenodeaspossible(map, remove, stop)==iino ) {
            stop = NULL;
        }
    }
    iplog(__Since(micro), "[IMAP-Unit] Remove Unit: "
			"%" PRId64 " - (%.3f, %.3f) - %s\n",
			unit->id, unit->pos.x, unit->pos.y, unit->code.code);

    return iiok;
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
			level, code->code, (void*)node);

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
	iplog(__Since(micro), "[MAP-Unit] Update  Unit(%" PRId64 ") To (%s, %.3f, %.3f)\n",
			unit->id, code.code, code.pos.x, code.pos.y);

	return ok;
}

/* 更新一个单元的附加信息到地图数据上：现阶段就只更新了单元的半径信息 */
void imaprefreshunit(imap *map, const iunit *unit) {
	iunused(map);
	iunused(unit);

#if (iiradius)
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
    
/* 返回地图水平的分割层次：当前层次的分割单元大于给定的大小, 那么轴对齐的线段的断点最多在两个分割单元内部*/
int imapcontainslevelw(const imap *map, int level, ireal h) {
    while (level > 0 && map->nodesizes[level].h < h) {
        --level;
    }
    return level;
}

/* 返回地图垂直的分割层次：当前层次的分割单元大于给定的大小, 那么轴对齐的线段的断点最多在两个分割单元内部*/
int imapcontainslevelh(const imap *map, int level, ireal w) {
    while (level > 0 && map->nodesizes[level].w < w) {
        --level;
    }
    return level;
}

/* 返回地图的分割层次，当前分割层次可以把矩形包含进最多4个分割单元内*/
int imapcontainslevel(const imap *map, const irect* r) {
    int level = map->divide;
    level = imapcontainslevelw(map, level, r->size.w);
    level = imapcontainslevelh(map, level, r->size.h);
    return level;
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

#if (iiradius)
	ucircle.pos = unit->pos;
	ucircle.radius = unit->radius;
	/* 距离超出范围 */
	if (icircleintersect(&filter->s.u.circle, &ucircle) == iino) {
#if open_log_filter
		ilog("[MAP-Filter] NO : Unit: %" PRId64 " (%.3f, %.3f : %.3f) - (%.3f, %.3f: %.3f)\n",
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
		ilog("[MAP-Filter] NO : Unit: %" PRId64 " (%.3f, %.3f) - (%.3f, %.3f: %.3f)\n",
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

#if (iiradius)
	/* 距离超出范围 */
	c.pos = unit->pos;
	c.radius = unit->radius;
	if (irectintersect(&filter->s.u.rect, &c) == iino) {
#if open_log_filter
		ilog("[MAP-Filter] NO : Unit: %" PRId64 " (%.3f, %.3f: %.3f)"
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
		ilog("[MAP-Filter] NO : Unit: %" PRId64 " (%.3f, %.3f)"
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

/* 线段过滤 
 * 先求出线段上离圆心最近的点，然后算距离
 * http://doswa.com/2009/07/13/circle-segment-intersectioncollision.html
 */
static int _ientryfilter_line(imap *map,  const ifilter *filter, const iunit* unit) {
    const ipos *center = &unit->pos;
    const iline2d *line = &filter->s.u.line.line;
    ireal epsilon = filter->s.u.line.epsilon;
    ireal distanceradius = epsilon;
    ipos closest = iline2dclosestpoint(line, center, epsilon);
    ireal distance;
    
#if (iiradius)
    distanceradius += unit->radius * unit->radius;
#endif
    distance = idistancepow2(center, &closest);
    if (ireal_less(distance, distanceradius)) {
        return iiok;
    }
    return iino;
}

/* 线段过滤器的指纹信息 */
static int64_t _ientryfilterchecksum_line(imap *map, const ifilter *d) {
	int64_t hash = 0;

	/* line */
	_ihash(&hash, __realint(d->s.u.line.line.start.x));
	_ihash(&hash, __realint(d->s.u.line.line.start.y));
	_ihash(&hash, __realint(d->s.u.line.line.end.x));
	_ihash(&hash, __realint(d->s.u.line.line.end.y));
	_ihash(&hash, __realint(d->s.u.line.epsilon));

	return hash;
}

/*线段过滤器*/
ifilter *ifiltermake_line2d(const ipos *from, const ipos *to, ireal epsilon) {
    ifilter *filter = ifiltermake();
    filter->s.u.line.line.start = *from;
    filter->s.u.line.line.end = *to;
    filter->s.u.line.epsilon = epsilon;
    filter->entry = _ientryfilter_line;
    filter->entrychecksum =_ientryfilterchecksum_line;
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
        /* 需要跳过搜索 */
        if (_state_is(unit->flag, EnumUnitFlagSkipSearching)) {
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

/* collect the inode and not keep retain */
inode * _imapcollectnodefrompoint(imap *map, const ipos *point, int level, ireflist *collects) {
    icode code;
    inode *node;
    imapgencode(map, point, &code);
    node = imapgetnode(map, &code, level, EnumFindBehaviorAccurate);
    if (node && !_state_is(node->state, EnumNodeStateSearching)) {
        _state_add(node->state, EnumNodeStateSearching);
        ireflistadd(collects, irefcast(node));
        return node;
    }
    return NULL;
}

/* 搜索 */
void imapsearchfromnode(imap *map, const inode *node,
		isearchresult* result, const ireflist *innodes) {
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
			"(%d, %s) --> Result: %lu Units \n",
			node->level, node->code.code, ireflistlen(result->units));
}

/*to see if we clear the node searching tag*/
static void _imapsearchcollectnode_withclear(imap *map, const irect *rect, int clear, ireflist *collects) {
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
    level = imapcontainslevel(map, rect);
    
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
        _imapcollectnodefrompoint(map, &tpos, level, collects);
    }
    
    /* 清理标记 */
    if (clear == iiok) {
        imapcollectcleannodetag(map, collects);
    }
}

/* 收集包含指定矩形区域的节点(最多4个) */
void imapsearchcollectnode(imap *map, const irect *rect, ireflist *collects) {
    _imapsearchcollectnode_withclear(map, rect, iiok, collects);
}

/**/
static irect _irect_make_from(const ipos *p, const ipos *t) {
    irect r;
    
    r.pos.x = imin(p->x, t->x);
    r.pos.y = imin(p->y, t->y);
    r.size.w = imax(fabs(p->x - t->x), iepsilon);
    r.size.h = imax(fabs(p->y - t->y), iepsilon);
    return r;
}

/* expand the rect with radius */
static void _irect_expand_radius(irect *r, ireal radius) {
    r->pos.x -= radius;
    r->pos.y -= radius;
    r->size.w += 2 * radius;
    r->size.h += 2 * radius;
}

/* Collecting nodes that intersected with line with map radius */
void imapsearchcollectline(imap *map, const iline2d *line, ireflist *collects) {
    ivec2 line_direction = iline2ddirection(line);
    ireal line_len = iline2dlength(line);
    
    ireal radius = 0;
    ireal movedelta = 0;
    
    ipos start = line->start;
    ipos begin;
    ipos end;
    irect r;
    
    int movecnt;
    int i;
    int level = map->divide;
    
#if (iiradius)
    radius += map->maxradius;
    /* if the radius grater than zero,
     * we should move back the dir with radius*/
    if (ireal_greater_zero(radius)) {
        start = ivec2movepoint(&line_direction, -radius, &start);
        line_len += 2*radius;
    }
#endif
    
    /* every step will take max 4 node */
    /* calcuating the node level */
    level = imapcontainslevelw(map, level, line_len/4);
    level = imapcontainslevelh(map, level, line_len/4);
    
    /* calcuating the step counts and step delta*/
    movedelta = imin(map->nodesizes[level].h, map->nodesizes[level].w);
    movecnt = (int)((line_len + movedelta - iepsilon) / movedelta);
    
    /* move step by step*/
    /* collect all the relatived node */
    begin = start;
    for (i=0; i < movecnt; ++i) {
        /* move one step */
        end = ivec2movepoint(&line_direction, (i+1) * movedelta, &start);
        r = _irect_make_from(&begin, &end);
        
        /*expand the rect with radius*/
#if (iiradius)
        _irect_expand_radius(&r, map->maxradius);
#endif
        
        /* collect the inode*/
        _imapsearchcollectnode_withclear(map, &r, iino, collects);
        begin = end;
    }
    
    /* clean the searing tag in node */
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
    
    /* searching the collects */
    imapsearchfromcollectwithfilter(map, collects, result, filter);

	/* 释放所有候选节点 */
	ireflistfree(collects);

	/* 性能日志 */
	iplogwhen(__Since(micro), 10, "[MAP-Unit-Search] Search-Node-Range From: "
			"(%d, %s: (%.3f, %.3f)) In Rect (%.3f, %.3f , %.3f, %.3f) "
			"---> Result : %lu Units \n",
			node->level, node->code.code,
			node->code.pos.x, node->code.pos.y,
			rect->pos.x, rect->pos.y, rect->size.w, rect->size.h,
			ireflistlen(result->units));
}

/* Collecting units from nodes(collects) with the filter */
void imapsearchfromcollectwithfilter(imap *map, const ireflist* collects,
                                     isearchresult *result, ifilter *filter) {
    /* parent */
    inode *node;
    /* 没有任何潜在的节点 */
    if (ireflistlen(collects) == 0) {
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

/* 从地图上搜寻单元: 视野检测*/
void imaplineofsight(imap *map, const ipos *from,
                     const ipos *to, isearchresult *result) {
    iline2d line = {*from, *to};
    /* make node list*/
    ireflist *collects = ireflistmake();
    /* make line filter*/
    ifilter *filter = ifiltermake_line2d(from, to, iepsilon);
    
    /* collecting the node with line*/
    imapsearchcollectline(map, &line, collects);
    
    /* collect the unit with filter in collects*/
    imapsearchfromcollectwithfilter(map, collects, result, filter);
    
    /* free resouces*/
    ireflistfree(collects);
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
		ilog(" tick(%" PRId64 "", node->tick);
#if open_node_utick
		ilog(",%" PRId64 "", node->utick);
#endif
		ilog(")");
	}
	/* 打印节点单元 */
	if ((require & EnumNodePrintStateUnits) && node->units) {
		iunit *u = node->units;
		ilog(" units(");
		while (u) {
			ilog("%" PRId64 "%s", u->id, u->next ? ",":")");
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
