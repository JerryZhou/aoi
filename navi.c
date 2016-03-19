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

#ifndef imax
#define imax(a, b) ((a) > (b) ? (a) : (b))
#endif  /* end: ifndef imax */

#ifndef imin
#define imin(a, b) ((a) < (b) ? (a) : (b))
#endif /* end: ifndef imin */

/* Max Path Finder Heap Depth */
#define KMAX_HEAP_DEPTH 64
/* Max Count Of Navimap Cell Cache */
#define KMAX_NAVIMAP_CELL_CACHE_COUNT 50000
/* Max Count Of Navimap Cell Connection Cache */
#define KMAX_NAVIMAP_CONNECTION_CACHE_COUNT 50000
/* Max Count Of Navimap Node Cache */
#define KMAX_NAVIMAP_NODE_CACHE_COUNT 2000
/* Max Count Of NaviPath Waypoint Cache */
#define KMAX_NAVIPATH_WAYPOINT_CACHE_COUNT 5000

/*invalid index */
const int kindex_invalid = -1;

/*************************************************************/
/* iheap - inavinode                                         */
/*************************************************************/
/* declare meta for inavinode */
iideclareregister(inavinode);

/* navigation node in path*/
typedef struct inavinode {
    irefdeclare;
    
    /* cost */
    ireal cost;
    
    /* cell of this node */
    inavicell *cell;
    
    /* cell of connection to next */
    inavicellconnection *connection;
} inavinode;
    
/* Make a Navi Nodes Heap with Cost Order Desc */
iheap* inavinodeheapmake();

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
static void _inavimap_build_cells(inavimap *map, size_t width, size_t height, ireal * heightmap, ireal block) {
    
}

/* Release all the resouces hold by navi map */
static void _inavimap_entry_free(iref *ref) {
    inavimap *map = icast(inavimap, ref);
    iarrayfree(map->cells);
    iarrayfree(map->polygons);
    iarrayfree(map->connections);
    
    iobjfree(map);
}

static void _ipolygon_edge(ipolygon3d *polygon, iline2d *edge, int index) {
    edge->start = ipolygon3dposxz(polygon, index);
    edge->end = ipolygon3dposxz(polygon, index+1);
}

/* Make connection */
inavicellconnection * inavicellconnectionmake() {
    inavicellconnection *con = iobjmalloc(inavicellconnection);
    /* no need descrtuor*/
    iretain(con);
    return con;
}
/* Release connection */
void inavicellconnectionfree(inavicellconnection* con) {
    irelease(con);
}

/* release all the resource hold by cell */
static void _inavicell_entry_free(iref *ref) {
    inavicell *cell = icast(inavicell, ref);
    /* clear the neighbors */
    ineighborsclean(icast(irefneighbors, cell));
    /* release the polygons */
    ipolygon3dfree(cell->polygon);
    /* releaset the connections */
    iarrayfree(cell->connections);
    
    iobjfree(cell);
}

/* Make a cell with poly and connections */
inavicell *inavicellmake(struct inavimap* map, ipolygon3d *poly, islice* connections, islice *costs) {
    inavicell * cell = iobjmalloc(inavicell);
    size_t len = imin(islicelen(connections), islicelen(costs));
    size_t n = 0;
    int next;
    
    /* descructor */
    cell->free = _inavicell_entry_free;
    
    /*add poly and cell to map*/
    cell->cell_index = iarraylen(map->cells);
    iarrayadd(map->polygons, &poly);
    iarrayadd(map->cells, &cell);
    
    /* poly */
    iassign(cell->polygon, poly);
    /* connections */
    cell->connections = iarraymakeint(islicelen(connections));
    for (n=0; n < len; ++n) {
        next = isliceof(connections, int, n);
        
        /*if invalid connection */
        if (next == kindex_invalid) {
            continue;
        }
        
        /* add connection */
        inavicelladdconnection(cell, map, n, next, isliceof(costs, ireal, n));
    }
    
    iretain(cell);
    return cell;
}

/* add connection to cell */
void inavicelladdconnection(inavicell *cell, struct inavimap *map, int edge, int next, ireal cost) {
    inavicellconnection * connection;
    int append;
    icheck(edge>0);
    
    /*make connection*/
    connection = inavicellconnectionmake();
    connection->index = edge;
    connection->cost = cost;
    connection->middle = ipolygon3dedgecenter(cell->polygon, edge);
    connection->next = next;
    connection->from = cell->cell_index;
    connection->location = iarraylen(map->connections);
    
    /* add to map */
    iarrayadd(map->connections, &connection);
    
    if (edge<iarraylen(cell->connections)) {
        /* set connection */
        iarrayset(cell->connections, edge, &connection->location);
    } else if (edge >= iarraylen(cell->connections)) {
        /*fill the invalid*/
        for (append = iarraylen(cell->connections); append<edge; ++append) {
            iarrayadd(cell->connections, &kindex_invalid);
        }
        /* add to cell */
        iarrayadd(cell->connections, &connection->location);
    } else {
        
    }
    
    
    inavicellconnectionfree(connection);
}

/* Connect the cell to map */
void inavicellconnecttomap(inavicell *cell, struct inavimap* map) {
    size_t len = iarraylen(cell->connections);
    int conindex=0;
    inavicellconnection *con=NULL;
    inavicell *neighbor=NULL;
    const void* convalue;
    
    /*disconnect first */
    inavicelldisconnectfrommap(cell);
    while (len) {
        /* get the cell connection */
        conindex = iarrayof(cell->connections, int, len-1);
        convalue = iarrayat(map->connections, conindex);
        
        /* connected as neighbor */
        if (convalue && (con = ((inavicellconnection**)(convalue))[0], con)) {
            neighbor = iarrayof(map->cells, inavicell*, con->next);
            /* add neighbor and append con as link resouce */
            ineighborsaddvalue(icast(irefneighbors,cell),
                          icast(irefneighbors,neighbor), con, con);
        }
        --len;
    }
}

/* Disconnect the cell to map */
void inavicelldisconnectfrommap(inavicell *cell) {
    ineighborsclean(icast(irefneighbors, cell));
}
    
/* Release the cell */
void inavicellfree(inavicell *cell) {
    irelease(cell);
}

/* Fetch the height from cell to pos */
int inavicellmapheight(inavicell *cell, ipos3 *pos) {
    pos->y = iplanesolvefory(&cell->polygon->plane, pos->x, pos->z);
    return iiok;
}

/* classify the line relationship with cell */
int inavicellclassify(inavicell *cell, const iline2d *line,
                      ipos *intersection, inavicellconnection **connection) {
    int interiorcount = 0;
    int relation = EnumNaviCellRelation_OutCell;
    iline2d edge;
    int edgecount = islicelen(cell->polygon->pos);
    int n = 0;
    int linerelation;
    
    while (n < edgecount) {
        _ipolygon_edge(cell->polygon, &edge, n);
        
        if (iline2dclassifypoint(&edge, &line->end, iepsilon) != EnumPointClass_Right) {
            if (iline2dclassifypoint(&edge, &line->start, iepsilon) != EnumPointClass_Left) {
                linerelation = iline2dintersection(&edge, line, intersection);
                if ( linerelation== EnumLineClass_Segments_Intersect ||
                    linerelation == EnumLineClass_A_Bisects_B ){
                    relation = EnumNaviCellRelation_IntersetCell;
                    
                    /* Find Connections */
                    if (connection) {
                        *connection = iarrayof(cell->connections, inavicellconnection*, n);
                    }
                    break;
                }
            }
        } else  {
            interiorcount++;
        }
        ++n;
    }
    /* all right */
    if (interiorcount == edgecount) {
        relation = EnumNaviCellRelation_InCell;
    }
    return relation;
}

/* release the resource hold by desc */
void inavimapdescfreeresource(inavimapdesc *desc) {
    iarrayfree(desc->points); desc->points = NULL;
    iarrayfree(desc->polygons); desc->polygons = NULL;
    iarrayfree(desc->polygonsindex); desc->polygonsindex = NULL;
    iarrayfree(desc->polygonsconnection); desc->polygonsconnection = NULL;
}

char * _file_read(const char *file) {
    FILE *fp = NULL;
    char *fcontent = NULL;
    long size;
    fp = fopen(file, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        fcontent = (char*)malloc(size+1);
        fread(fcontent, 1, size, fp);
        fclose(fp);
        fcontent[size] = 0;
    }
    return fcontent;
}

typedef enum EnumErrCode {
    EnumErrCode_WrongHeaderFormat,
    EnumErrCode_WrongPointFormat,
    EnumErrCode_WrongPolygonFormat,
}EnumErrCode;

/* read the navimap from textfile */
int inavimapdescreadfromtextfile(inavimapdesc *desc, const char* file) {
    
    FILE *fp = NULL;
    int n = 0;
    int m, k, i, j, c = 0;
    ireal cost;
    int err = 0;
    ipos3 p;
    fp = fopen(file, "r");
    
    inavimapdescfreeresource(desc);
    if (fp) {
        while (true) {
            /*read header */
            n = fscanf(fp, "Map: width %ld height %ld points %ld polygons %ld polygonsize %ld\n",
                       &desc->header.width,
                       &desc->header.height,
                       &desc->header.points,
                       &desc->header.polygons,
                       &desc->header.polygonsize);
            if (n != 5) {
                err = EnumErrCode_WrongHeaderFormat; /* header format wrong */
                break;
            }
            
            /*read the points */
            desc->points = iarraymakeipos3(desc->header.points);
            for (m=0; m <desc->header.points; ++m) {
                if (m%6==0) {
                    fscanf(fp, "\n");
                }
                n = fscanf(fp, "(%lf,%lf,%lf)", &p.x, &p.y, &p.z);
                if (n !=3) {
                    err = EnumErrCode_WrongPointFormat;
                    break;
                } else {
                    iarrayadd(desc->points, &p);
                }
            }
            if (err != 0) {
                break;
            }
            fscanf(fp, "\n");
            
            /* read the polygons */
            desc->polygons = iarraymakeint(desc->header.polygons);
            desc->polygonsindex = iarraymakeint(desc->header.polygonsize);
            desc->polygonsconnection = iarraymakeint(desc->header.polygonsize);
            desc->polygonscosts = iarraymakeireal(desc->header.polygonsize);
            for (m=0; m<desc->header.polygons; ++m) {
                n = fscanf(fp, "%d:", &k);
                if (n != 1) {
                    err = EnumErrCode_WrongPolygonFormat;
                    break;
                }
                iarrayadd(desc->polygons, &k);
                
                for (i=0; i<k; ++i) {
                    if (i == k-1) {
                        n = fscanf(fp, "%d-%d-%lf\n", &j, &c, &cost);
                    } else {
                        n = fscanf(fp, "%d-%d-%lf ", &j, &c, &cost);
                    }
                    if (n != 3) {
                        err = EnumErrCode_WrongPolygonFormat;
                        break;
                    } else {
                        iarrayadd(desc->polygonsindex, &j);
                        iarrayadd(desc->polygonsconnection, &c);
                        iarrayadd(desc->polygonscosts, &cost);
                    }
                }
                
                if (err != 0) {
                    break;
                }
            }
            
            break;
        }
    }
    return err;
}
    
/* write the navimap to textfile */
void inavimapdescwritetotextfile(inavimapdesc *desc, const char* file) {
    
}

/* Make navimap from the blocks */
inavimap* inavimapmake(size_t capacity){
    inavimap *map = iobjmalloc(inavimap);
    map->free = _inavimap_entry_free;
    map->cells = iarraymakeiref(capacity);
    map->polygons = iarraymakeiref(capacity);
    map->connections = iarraymakeiref(capacity*4);
   
    iretain(map);
    return map;
}

/* load the navimap from heightmap */
void inavimapload(inavimap *map, size_t width, size_t height, ireal *heightmap, ireal block) {
    /*Build the Cells */
    _inavimap_build_cells(map, width, height, heightmap, block);
}

/* load the navimap from desc */
void inavimaploadfromdesc(inavimap *map, const inavimapdesc *desc) {
    int i = 0;
    int j = 0;
    size_t len = 0;
    size_t lentotal = 0;
    islice *sliceindex = NULL;
    islice *sliceconnection = NULL;
    islice *slicecosts = NULL;
    inavicell *cell;
    ipolygon3d *polygon;
    
    /* remove all old cells */
    iarrayremoveall(map->polygons);
    iarrayremoveall(map->cells);
    /* make sure the capacity of polygons */
    iarrayexpandcapacity(map->polygons, desc->header.polygons);
    iarrayexpandcapacity(map->cells, desc->header.polygons);
    /* make all the cells */
    for (i=0, lentotal=0; i<desc->header.polygons; ++i) {
        len = iarrayof(desc->polygons, int, i);
        sliceindex = isliced(desc->polygonsindex, lentotal, lentotal+len);
        sliceconnection = isliced(desc->polygonsconnection, lentotal, lentotal+len);
        slicecosts = isliced(desc->polygonscosts, lentotal, lentotal+len);
        
        /* make polygon */
        polygon = ipolygon3dmake(len);
        for (j=0; j<len; ++j) {
            ipolygon3dadd(polygon , &iarrayof(desc->points, ipos3,
                                              isliceof(sliceindex, int, j)),
                          1);
        }
        
        /* make cell */
        cell = inavicellmake(map, polygon, sliceconnection, slicecosts);
        
        /* release the middle values */
        islicefree(sliceindex);
        islicefree(sliceconnection);
        islicefree(slicecosts);
        ipolygon3dfree(polygon);
        inavicellfree(cell);
        
        /* next cell */
        lentotal += len;
    }
    
    /* connected all the cells: then make all connections in map */
    for (i=0; i<iarraylen(map->cells); ++i) {
        inavicellconnecttomap(iarrayof(map->cells, inavicell*, i), map);
    }
}

/* Free the navi map */
void inavimapfree(inavimap *map) {
    irelease(map);
}

/* Navi map find the cell 
 * Should be carefully deal with profile */
inavicell* inavimapfind(const inavimap *map, const ipos3 *pos) {
    islice *cells = islicemakearg(map->cells, ":");
    inavicell * cell = inavimapfindclosestcell(map, cells, pos);
    
    islicefree(cells);
    return cell;
}

ireal _inavicell_pos_dist(inavicell *cell, const ipos3 *pos ) {
    ipos intersection;
    ipos start = {cell->polygon->center.x, cell->polygon->center.z};
    ipos end = {pos->x, pos->z};
    iline2d line = {start, end};
    ipos3 closest;
    ireal dist = INT32_MAX;
    
    if (inavicellclassify(cell, &line, &intersection, NULL) == EnumNaviCellRelation_IntersetCell) {
        closest.x = intersection.x;
        closest.z = intersection.y;
        inavicellmapheight(cell, &closest);
        dist = sqrtf(idistancepow3(pos, &closest));
    }
    return dist;
}

/* navi map find the cell */
inavicell* inavimapfindclosestcell(const inavimap *map, const islice* cells, const ipos3 *pos) {
    inavicell *cell = NULL;
    ireal maxdistance = INT32_MAX;
    inavicell *closestcell = NULL;
    size_t len = islicelen(cells);
    int found = iino;
    ipos3 newpos;
    ireal dist;
    
    while (len) {
        cell = isliceof(cells, inavicell*, len-1);
        if (ipolygon3dincollum(cell->polygon, pos) == iiok) {
            /*found it, try closest, may be cell overlap in y value for 3d space*/
            newpos = *pos;
            inavicellmapheight(cell, &newpos);
            dist = fabs(newpos.y - pos->y);
            
            found = iiok;
            if (dist < maxdistance) {
                closestcell = cell;
                maxdistance = dist;
            }
        } else if(found == iino) {
            /*no found , find a closest cell in dist*/
            dist = _inavicell_pos_dist(cell, pos);
            if (dist < maxdistance) {
                closestcell = cell;
                maxdistance = dist;
            }
        }
        --len;
    }
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

static void _inavinode_entry_free(iref *ref) {
    inavinode *node = icast(inavinode, ref);
    irelease(node->cell);
    iobjfree(node);
}

/* Make a node by cell, NB!! not retain */
static inavinode * _inavicontext_makenode(inavicontext *context, inavicell *cell) {
    inavinode *node = iobjmalloc(inavinode);
    node->free = _inavinode_entry_free;
    
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

static void _inavipath_entry_free(iref *ref) {
    inavipath *path = icast(inavipath, ref);
    irelease(path->start);
    irelease(path->end);
    irelease(path->waypoints);
    iobjfree(path);
}

inavipath *inavipathmake() {
    inavipath *path = iobjmalloc(inavipath);
    path->free = _inavipath_entry_free;
    path->waypoints = ireflistmake();
    
    iretain(path);
    return path;
}

/* navigation path free */
void inavipathfree(inavipath *path) {
    irelease(path);
}

ipos3 _ipolygon_point(ipolygon3d *polygon, int index) {
    size_t len = islicelen(polygon->pos);
    icheckret(len>0, kipos3_zero);
    return isliceof(polygon->pos, ipos3, index%len);
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
        
        /* add cell to heap */
        _inavicontext_heap_cell(context, cell);
    } else if(cell->flag == EnumNaviCellFlag_Open) {
        cell->connection = connection;
        cell->link = caller->cell;
        cell->costarrival = caller->cell->costarrival + connection->cost;
        cell->costheuristic = _inavicontext_heuristic(context, cell);
        
        /* adjust cell in heap */
        _inavicontext_heap_cell(context, cell);
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

/* implement meta for inavicellconnection */
irealdeclareregister(inavicellconnection);

/* implement meta for inaviwaypoint */
irealdeclareregister(inaviwaypoint);

/* implement meta for inavipath */
irealdeclareregister(inavipath);

/* implement meta for inavinode */
irealdeclareregister(inavinode);

/* implement meta for inavimap */
irealdeclareregister(inavimap);

/* NB!! should call first before call any navi funcs 
 * will registing all the navi types to meta system
 * */
int inavi_mm_init() {

    irealimplementregister(inavicell, KMAX_NAVIMAP_CELL_CACHE_COUNT);
    irealimplementregister(inavicellconnection, KMAX_NAVIMAP_CONNECTION_CACHE_COUNT);
    irealimplementregister(inavinode, KMAX_NAVIMAP_NODE_CACHE_COUNT);
    irealimplementregister(inaviwaypoint, KMAX_NAVIPATH_WAYPOINT_CACHE_COUNT);
    irealimplementregister(inavipath, 0);
    irealimplementregister(inavimap, 0);

    return iiok;
}
