/*!
  Navigation Mesh In Game Developing
  Copyright Jerryzhou@outlook.com
Licence: Apache 2.0

Project: https://github.com/JerryZhou/aoi.git
Purpose: Resolve the Navigation Problem In Game Developing
with high run fps
with minimal memory cost,

Please see examples for more details.
*/

#include "navi.h"

/* Max Path Finder Heap Depth */
#define KMAX_HEAP_DEPTH 64
/* Max Count Of Navimap Cell Cache */
#define KMAX_NAVIMAP_CELL_CACHE_COUNT 50000
/* Max Count Of Navimap Node Cache */
#define KMAX_NAVIMAP_NODE_CACHE_COUNT 2000


/*************************************************************/
/* iheap - inavinode                                         */
/*************************************************************/

/* Assign value to place */
static void _ientry_heap_node_assign(struct iarray *arr,
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
        icast(inavinode, arrs[i])->heap_index = i;
        ++j;
        ++i;
    }
}

/* Swap two node */
static void _ientry_heap_node_swap(struct iarray *arr,
                                    int i, int j) {
    iref* tmp;
    iref* *arrs = (iref* *)arr->buffer;
    if (j == arr_invalid) {
        /* arr_int[i] = 0;
         * may call assign */
        _ientry_heap_node_assign(arr, i, 0, 1);
    } else if (i == arr_invalid) {
        /* arr_int[j] = 0;
         * may call assign */
        _ientry_heap_node_assign(arr, j, 0, 1);
    } else {
        tmp = arrs[i];
        arrs[i] = arrs[j];
        arrs[j] = tmp;
        
        icast(inavinode, arrs[i])->heap_index = i;
        icast(inavinode, arrs[j])->heap_index = j;
    }
}

/* Compare the heap node with cost*/
static int _ientry_heap_node_cmp(iarray *arr, int i, int j) {
    inavinode* lfs = iarrayof(arr, inavinode*, i);
    inavinode* rfs = iarrayof(arr, inavinode*, j);
    return lfs->cost > rfs->cost;
}

/* Definition of inavinode array */
static const iarrayentry _arr_entry_inavinode = {
    EnumArrayFlagKeepOrder |
    EnumArrayFlagMemsetZero,
    sizeof(inavinode*),
    _ientry_heap_node_swap,
    _ientry_heap_node_assign,
    _ientry_heap_node_cmp,
};

/* Make a Navi Nodes Heap with Cost Order Desc */
iheap* inavinodeheapmake() {
    /*make heap with capacity: MAX_HEAP_DEPTH*/
    iheap *heap = iarraymake(KMAX_HEAP_DEPTH, &_arr_entry_inavinode);
    /* return heap*/
    return heap;
}

/*************************************************************/
/* inavimap                                                 */
/*************************************************************/

/* Navimap node cache object-make entry*/
static iref* _icachenewentry_node() {
    inavinode *n = (inavinode*)iobjmalloc(inavinode);
    iretain(n);
    return irefcast(n);
}

/* Navimap node cache object-make entry*/
static iref* _icachenewentry_cell() {
    inavicell *n = (inavicell*)iobjmalloc(inavicell);
    iretain(n);
    return irefcast(n);
}

/* Build all navi cells from blocks */
static void _inavimap_build_cells(inavimap *map, size_t width, size_t height, char * blocks) {
    
}

/* Release all the resouces hold by navi map */
static void _inavimap_entry_free(iref *ref) {
    inavimap *map = icast(inavimap, ref);
    iarrayfree(map->cells);
    iobjfree(map);
}

/* Make navimap from the blocks */
inavimap* inavimapmake(size_t width, size_t height, char * blocks) {
    inavimap *map = iobjmalloc(inavimap);
    map->free = _inavimap_entry_free;
    
    /*Build the Cells */
    _inavimap_build_cells(map, width, height, blocks);
    
    iretain(map);
    return map;
}

/* Navi map find the cell 
 * Should be carefully deal with profile */
inavicell* inavimapfind(const inavimap *map, const ipos *pos) {
    return NULL;
}

/* inavi cost */
static ireal _inavinode_cost(const inavimap *map, const iunit *unit, inavicell *from, inavicell *to) {
    return 1;
}

/* Make a node by cell, NB!! not retain */
static inavinode *_inavinode_make_by_cell(const inavimap *map, const iunit *unit, inavicell* from, inavicell *cell) {
    inavinode *n = iobjmalloc(inavinode);
    iassign(n->from, from);
    iassign(n->cell, cell);
    n->cost = _inavinode_cost(map, unit, from, cell);
    return n;
}

/* A* */
static void _inavimapfindpath_cell(inavimap *map,
                                         const iunit *unit,
                                         inavicell *start,
                                         inavicell *end,
                                         ireflist *nodes,
                                         int maxstep) {
    iheap * opens = NULL;
    inavinode * node = NULL;
    int found = iino;
    int steps = maxstep;
    
    icheck(start);
    icheck(end);
    
    ++map->sessionid;
    opens = iarraymakeiref(KMAX_HEAP_DEPTH);
    iheapbuild(opens);
    
    node = _inavinode_make_by_cell(map, unit, NULL, start);
    iheapadd(opens, &node);
    
    while (iheapsize(opens) && found == iino && steps) {
        node = iheappeekof(opens, inavinode*);
        if (node->cell == end) {
            found = true;
        }
        
        --steps;
    }
}

/* navi map find the path */
int inavimapfindpath(inavimap *map, const iunit *unit, const ipos *from, const ipos *to, inavipath *path) {
    inavicell *start = inavimapfind(map, from);
    inavicell *end = inavimapfind(map, to);
    _inavimapfindpath_cell(map, unit, start, end, path->nodes, INT32_MAX);
    return ireflistlen(path->nodes);
}

/*************************************************************/
/* implement the new type for iimeta system                  */
/*************************************************************/

/* implement meta for inavicell */
irealdeclareregister(inavicell);

/* implement meta for inavinode */
irealdeclareregister(inavinode);

/* implement meta for inavimap */
irealdeclareregister(inavimap);


/* NB!! should call first before call any navi funcs 
 * will registing all the navi types to meta system
 * */
int inavi_mm_init() {

    irealimplementregister(inavicell, KMAX_NAVIMAP_CELL_CACHE_COUNT);
    irealimplementregister(inavinode, KMAX_NAVIMAP_NODE_CACHE_COUNT);
    irealimplementregister(inavimap, 0);

    return iiok;
}
