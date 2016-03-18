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
/* Max Count Of NaviPath Waypoint Cache */
#define KMAX_NAVIPATH_WAYPOINT_CACHE_COUNT 5000


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
        icast(inavinode, arrs[i])->cell->heap_index = i;
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
        
        icast(inavinode, arrs[i])->cell->heap_index = i;
        icast(inavinode, arrs[j])->cell->heap_index = j;
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
inavicell* inavimapfind(const inavimap *map, const ipos3 *pos) {
    return NULL;
}

/* Navigation Context*/
typedef struct inavicontext {
    int64_t sessionid;
    
    iheap *heap;
    inavipath *path;
    iunit *unit;
    inavimap *map;
}inavicontext;

/* make a heuristic */
static ireal _inavicontext_heuristic(inavicontext *context, inavicell *cell) {
    return idistancepow3(&cell->polygon->center, &context->path->endpos);
}

/* Make a node by cell, NB!! not retain */
static inavinode * _inavicontext_makenode(inavicontext *context, inavicell *cell) {
    inavinode *node = iobjmalloc(inavinode);
    iassign(node->cell, cell);
    node->cost = cell->costarrival + cell->costheuristic;
    return node;
}

/* Add cell to heap or update the node by cell in heap*/
static void _inavicontext_heap_cell(inavicontext *context, inavicell *cell) {
    inavinode *node;
    if (cell->heap_index != arr_invalid) {
        /* update the cost of node */
        node = iarrayof(context->heap, inavinode*, cell->heap_index);
        node->cost = cell->costarrival + cell->costheuristic;
        iheapadjust(context->heap, cell->heap_index);
    } else {
        node = _inavicontext_makenode(context, cell);
        iheapadd(context->heap, &node);
    }
}

/* setup the path */
void inavipathsetup(inavipath *path, int64_t sessionid,
                    inavicell *start, const ipos3 *startpos,
                    inavicell *end, const ipos3 *endpos) {
    path->sessionid = sessionid;
    
    iassign(path->start, start);
    path->startpos = *startpos;
    
    iassign(path->end, end);
    path->endpos = *endpos;
}


ipos3 _ipolygon_point(ipolygon3d *polygon, int index) {
    size_t len = islicelen(polygon->slice);
    icheckret(len>0, kipos3_zero);
    return isliceof(polygon->slice, ipos3, index%len);
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make_by_cell(inavicell *cell) {
    inaviwaypoint *waypoint = iobjmalloc(inaviwaypoint);
    waypoint->type = EnumNaviWayPointType_Cell;
    iassign(waypoint->cell, cell);
    waypoint->waypoint = cell->polygon->center;
    return waypoint;
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make_by_connection(inavicell *cell, inavicellconnection *connection) {
    inaviwaypoint *waypoint = iobjmalloc(inaviwaypoint);
    waypoint->type = EnumNaviWayPointType_Connection;
    iassign(waypoint->cell, cell);
    iassign(waypoint->connection, connection);
    waypoint->waypoint = connection->middle;
    return waypoint;
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make_by_goal(inavicell *cell, ipos3 *goal) {
    inaviwaypoint *waypoint = iobjmalloc(inaviwaypoint);
    waypoint->type = EnumNaviWayPointType_Cell_Goal;
    iassign(waypoint->cell, cell);
    waypoint->waypoint = *goal;
    return waypoint;
}

void _inavipath_begin(inavipath *path, inavicell *end) {
    inaviwaypoint *waypoint;
    /* we found it */
    if (end == path->end) {
        waypoint = _inaviwaypoint_make_by_goal(end, &path->endpos);
        waypoint->flag |= EnumNaviWayPointFlag_End;
        ireflistadd(path->waypoints, irefcast(waypoint));
    } else {
        /*should insert a dynamic waypoint */
        waypoint = _inaviwaypoint_make_by_goal(path->end, &path->endpos);
        waypoint->flag |= EnumNaviWayPointFlag_Dynamic;
        ireflistadd(path->waypoints, irefcast(waypoint));
        
        /*the nearest end way point */
        waypoint = _inaviwaypoint_make_by_cell(end);
        ireflistadd(path->waypoints, irefcast(waypoint));
    }
}

void _inavipath_end(inavipath *path, inavicell* cell, inavicellconnection *connection) {
    inaviwaypoint *waypoint;
    /* should have connection */
    icheck(connection);
    
    if (cell == path->start) {
        /* connection */
        waypoint = _inaviwaypoint_make_by_connection(cell, connection);
        ireflistadd(path->waypoints, irefcast(waypoint));
    } else {
        /*may happend some ugly*/
    }
}

static void _inavicell_process(inavicell *cell, inavicontext *context,
                               inavinode *caller, inavicellconnection *connection) {
    if (cell->sessionid != context->sessionid) {
        cell->sessionid = context->sessionid;
        cell->flag = EnumNaviCellFlag_Open;
        cell->link = caller->cell;
        cell->connection = connection;
        cell->heap_index = arr_invalid;
        
        if (caller && connection) {
            cell->costarrival = caller->cell->costarrival + connection->cost;
        } else {
            cell->costarrival = 0;
        }
        cell->costheuristic = _inavicontext_heuristic(context, cell);
    } else if(cell->flag == EnumNaviCellFlag_Open) {
        cell->connection = connection;
        cell->link = caller->cell;
        cell->costarrival = caller->cell->costarrival + connection->cost;
        cell->costheuristic = _inavicontext_heuristic(context, cell);
    }
}

/* Process the node with neighbors */
static void _inavinode_process(inavinode *node, inavicontext *context) {
    
    irefjoint *joint;
    inavicell *cell;
    inavicellconnection *connection;
    
    /* mark close */
    node->cell->flag = EnumNaviCellFlag_Close;
    
    /* all of children */
    joint = ireflistfirst(node->cell->neighbors_to);
    while (joint) {
        cell = icast(inavicell, joint->value);
        connection = icast(inavicellconnection, joint->res);
        
        /* process cell */
        _inavicell_process(cell, context, node, connection);
        /* next */
        joint = joint->next;
    }
}

static void _inavicontext_setup(inavicontext *context,
                                inavimap *map, iunit *unit,
                                inavipath *path) {
    context->map = map;
    context->sessionid = ++map->sessionid;
    context->unit = unit;
    context->path = path;
    context->heap = inavinodeheapmake();
    
    /* add start cell to heap */
    _inavicell_process(path->start, context, NULL, NULL);
}

static void _inavicontext_free(inavicontext *context) {
    irelease(context->heap);
}

/* A* */
static void _inavimapfindpath_cell(inavimap *map,
                                         iunit *unit,
                                         inavipath *path,
                                         int maxstep) {
    inavinode * node = NULL;
    inavicell * cell = NULL;
    inavicellconnection * connection = NULL;
    inaviwaypoint * waypoint = NULL;
    inavicell * end = path->end;
    inavicontext context = {0};
    
    int found = iino;
    int steps = maxstep;
    
    _inavicontext_setup(&context, map, unit, path);
    
    /* found the path */
    while (iheapsize(context.heap) && found == iino && steps) {
        node = iheappeekof(context.heap, inavinode*);
        iheappop(context.heap);
        
        /* Arrived */
        if (node->cell == end) {
            found = true;
        } else {
            /* Process current neighbor */
            _inavinode_process(node, &context);
        }
        
        --steps;
    }
    
    /* found a nearest reaching path */
    if (steps == 0 && found == iino) {
        end = node->cell;
        found = iiok;
    }
    
    /* found path */
    if (found == iiok) {
        /* path begin */
        _inavipath_begin(path, end);
        /* reverse insert the waypoint to list */
        cell = end->link;
        connection = end->connection;
        while (cell && cell != path->start) {
            /* connection */
            waypoint = _inaviwaypoint_make_by_connection(cell, connection);
            ireflistadd(path->waypoints, irefcast(waypoint));
            /* cell */
            waypoint = _inaviwaypoint_make_by_cell(cell);
            ireflistadd(path->waypoints, irefcast(waypoint));
            /* get next point */
            cell = cell->link;
            connection = cell->connection;
        }
        
        /* end of path */
        _inavipath_end(path, cell, connection);
    }
    
    /* free the navigation context */
    _inavicontext_free(&context);
}

/* navi map find the path */
int inavimapfindpath(inavimap *map, iunit *unit, const ipos3 *from, const ipos3 *to, inavipath *path) {
    inavicell *start = inavimapfind(map, from);
    inavicell *end = inavimapfind(map, to);
    inavipathsetup(path, ++map->sessionid, start, from, end, to);
    
    _inavimapfindpath_cell(map, unit, path, INT32_MAX);
    return ireflistlen(path->waypoints);
}

/*************************************************************/
/* implement the new type for iimeta system                  */
/*************************************************************/

/* implement meta for inavicell */
irealdeclareregister(inavicell);

irealdeclareregister(inaviwaypoint);

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
    irealimplementregister(inaviwaypoint, KMAX_NAVIPATH_WAYPOINT_CACHE_COUNT);
    irealimplementregister(inavimap, 0);

    return iiok;
}
