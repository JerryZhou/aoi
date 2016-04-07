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

/* log some runtime */
#define __iopen_log_cell_add (1)
#define __iopen_log_cell_find (1)
#define __iopen_log_cell_del (1)

/* Max Path Finder Heap Depth */
#define KMAX_HEAP_DEPTH 64
/*************************************************************/
/* helper - coordinate system                                */
/*************************************************************/

/* change the pos3d to pos2d */
ipos _inavi_flat_pos(const ipos3 *p) {
    ipos pos;
    pos.x = p->x;
    pos.y = p->z;
    return pos;
}

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

/* Compare the heap node with cost*/
static int _ientry_heap_node_cmp(iarray *arr, int i, int j) {
    inavinode* lfs = iarrayof(arr, inavinode*, i);
    inavinode* rfs = iarrayof(arr, inavinode*, j);
    return lfs->cost < rfs->cost;
}

/* trace the cell index in heap */
static void _inode_trace_cell_heap_index(iarray *arr, iref *ref, int index) {
    inavinode *node;
    icheck(ref);
    node = icast(inavinode, ref);
    node->cell->heap_index = index;
}

/* def the entry for ref */
static const irefarrayentry _refarray_entry_inavinode = {
    _inode_trace_cell_heap_index,
};

/* Make a Navi Nodes Heap with Cost Order Desc */
iheap* inavinodeheapmake() {
    /*make heap with capacity: MAX_HEAP_DEPTH*/
    iheap *heap = iarraymakeirefwithentry(KMAX_HEAP_DEPTH, &_refarray_entry_inavinode);
    /*node cmp entry */
    heap->cmp = _ientry_heap_node_cmp;
    /* return heap*/
    return heap;
}

/*************************************************************/
/* inavimap                                                 */
/*************************************************************/

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
    
    /* release the cell link from */
    irelease(cell->link);
    /* release the cell connection from */
    irelease(cell->connection);
    
    /* release the cell units */
    inavicellunlinkaoi(cell);
    iarrayfree(cell->aoi_cellunits);
    
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
    
    /* the cell units */
    cell->aoi_cellunits = iarraymakeiref(4);
    
    iretain(cell);
    return cell;
}

/* add connection to cell */
void inavicelladdconnection(inavicell *cell, struct inavimap *map, int edge, int next, ireal cost) {
    inavicellconnection * connection;
    int append;
    icheck(edge>=0);
    
    /*make connection*/
    connection = inavicellconnectionmake();
    connection->index = edge;
    connection->cost = cost;
    connection->middle = ipolygon3dedgecenter(cell->polygon, edge);
    connection->start = *ipolygon3dpos3(cell->polygon, edge);
    connection->end = *ipolygon3dpos3(cell->polygon, edge+1);
    connection->next = next;
    connection->from = cell->cell_index;
    
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
    
    /*if not a new cell should disconnect first caller self */
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

/* Release the relation with aoi */
void inavicellunaoi(inavicell *cell, imap *aoimap) {
    iunit *u;
    size_t size = iarraylen(cell->aoi_cellunits);
#if __iopen_log_cell_del
    ilog("[INavi Cell-UnLink] ##Begin## cell:"__icell_format"\n", __icell_value(*cell));
#endif
    for (; size; --size) {
        u = iarrayof(cell->aoi_cellunits, iunit*, size-1);
#if __iopen_log_cell_del
    ilog("[INavi Cell-UnLink] Unmapping from node:"__inode_format"\n", __inode_value(*aoimap, *u->node));
#endif
        imapremoveunit(aoimap, u);
    }
    iarrayremoveall(cell->aoi_cellunits);
#if __iopen_log_cell_del
    ilog("[INavi Cell-UnLink] ##End## cell:"__icell_format"\n", __icell_value(*cell));
#endif
}

/* Just Single Release the relation with aoi */
void inavicellunlinkaoi(inavicell *cell) {
    iunit *u;
    size_t size = iarraylen(cell->aoi_cellunits);
    for (; size; --size) {
        u = iarrayof(cell->aoi_cellunits, iunit*, size-1);
        u->userdata.up1 = NULL;
        u->flag &= ~EnumNaviUnitFlag_Cell;
    }
}


/* classify the line relationship with cell */
int inavicellclassify(inavicell *cell, const iline2d *line,
                      ipos *intersection, int *connection) {
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
                if ( linerelation== EnumLineClass_Segments_Intersect) {
                    /* if segement is cross by line edge, 
                     should be some edge cross the edge interset with edge too */
                    /*|| linerelation == EnumLineClass_A_Bisects_B ){ */
                    relation = EnumNaviCellRelation_IntersetCell;
                    
                    /* Find Connections */
                    if (connection ) {
                        if (n < iarraylen(cell->connections)) {
                            *connection = iarrayof(cell->connections, int, n);
                        } else {
                            *connection = kindex_invalid;
                        }
                    }
                    break;
                }
            }
        }else {
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

/*************************************************************/
/* inavipathspeedup                                          */
/*************************************************************/

/* free the speedup */
static void _inavipathspeedup_entry_free(iref *ref) {
    inavipathspeedup *speed = icast(inavipathspeedup, ref);
    irelease(speed->path);
    iobjfree(speed);
}

/*make one*/
inavipathspeedup* inavipathspeedupmake() {
    inavipathspeedup *speed = iobjmalloc(inavipathspeedup);
    speed->free = _inavipathspeedup_entry_free;
    iretain(speed);
    return speed;
}
    
/*free it*/
void inavipathspeedupfree(inavipathspeedup *speed) {
    irelease(speed);
}

/* release the resource hold by desc */
void inavimapdescfreeresource(inavimapdesc *desc) {
    iarrayfree(desc->points); desc->points = NULL;
    iarrayfree(desc->polygons); desc->polygons = NULL;
    iarrayfree(desc->polygonsindex); desc->polygonsindex = NULL;
    iarrayfree(desc->polygonsconnection); desc->polygonsconnection = NULL;
    iarrayfree(desc->polygonscosts); desc->polygonscosts = NULL;
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
            n = fscanf(fp, "Map: width %lf height %lf points %ld polygons %ld polygonsize %ld\n",
                       &desc->header.size.w,
                       &desc->header.size.h,
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
                        n = fscanf(fp, "(%d,%d,%lf)\n", &j, &c, &cost);
                    } else {
                        n = fscanf(fp, "(%d,%d,%lf) ", &j, &c, &cost);
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

/* trace the cell index */
static void _irefarray_cell_index_change(iarray *arr, iref *ref, int index) {
    inavicell *cell = icast(inavicell, ref);
    icheck(cell);
    cell->cell_index = index;
}

/* cell array entry */
static const irefarrayentry _irefarray_entry_inavicell = {
    _irefarray_cell_index_change,
};

/* trace the connection location in array */
static void _irefarray_connection_index_change(iarray *arr, iref *ref, int index) {
    inavicellconnection *con = icast(inavicellconnection, ref);
    icheck(con);
    con->location = index;
}

/* cell connection entry */
static const irefarrayentry _irefarray_entry_inavicellconnection = {
    _irefarray_connection_index_change,
};

/* Make navimap from the blocks */
inavimap* inavimapmake(size_t capacity){
    inavimap *map = iobjmalloc(inavimap);
    map->free = _inavimap_entry_free;
    map->cells = iarraymakeirefwithentry(capacity, &_irefarray_entry_inavicell);
    map->polygons = iarraymakeiref(capacity);
    map->connections = iarraymakeirefwithentry(capacity*4, &_irefarray_entry_inavicellconnection);
   
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
    iarrayremoveall(map->connections);
    
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
    /*disconnect all the navicell*/
    irange(map->cells, inavicell*,
           inavicelldisconnectfrommap(__value);
           );
    
    /*free all the resources*/
    iarrayremoveall(map->cells);
    iarrayremoveall(map->polygons);
    iarrayremoveall(map->connections);
    
    /*release the free */
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
    ipos intersection = {0, 0};
    ipos start = {cell->polygon->center.x, cell->polygon->center.z};
    ipos end = {pos->x, pos->z};
    iline2d line = {start, end};
    ipos3 closest = {0, 0, 0};
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
            if (cell == NULL || dist < maxdistance) {
                closestcell = cell;
                maxdistance = dist;
            }
        }
        --len;
    }
    return closestcell;
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
    if (cell->heap_index != kindex_invalid) {
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
    
    ireflistremoveall(path->waypoints);
    path->current = NULL;
}

static void _inavipath_entry_free(iref *ref) {
    inavipath *path = icast(inavipath, ref);
    irelease(path->start);
    irelease(path->end);
    ireflistfree(path->waypoints);
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

/*find out the closest point in edge in 3d */
ipos3 _inavicellconnection_closest(const inavicellconnection *connection, const ipos3 *pos) {
    iline3d edge = {connection->start, connection->end};
    return iline3dclosestpoint(&edge, pos, iepsilon);
}

/*free the waypoint*/
static void _inaviwaypoint_entry_free(iref *ref) {
    inaviwaypoint *waypoint = icast(inaviwaypoint, ref);
    irelease(waypoint->cell);
    irelease(waypoint->connection);
    iobjfree(waypoint);
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make() {
    inaviwaypoint *waypoint = iobjmalloc(inaviwaypoint);
    waypoint->free = _inaviwaypoint_entry_free;
    return waypoint;
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make_by_cell(inavipath *path, inavicell *cell) {
    inaviwaypoint *waypoint = _inaviwaypoint_make();
    waypoint->type = EnumNaviWayPointType_Cell;
    iassign(waypoint->cell, cell);
    waypoint->waypoint = cell->polygon->center;
    return waypoint;
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make_by_connection(inavipath *path, inavicell *cell, inavicellconnection *connection) {
    inaviwaypoint *waypoint = _inaviwaypoint_make();
    inaviwaypoint *last;
    
    waypoint->type = EnumNaviWayPointType_Connection;
    iassign(waypoint->cell, cell);
    iassign(waypoint->connection, connection);
    if ( iiwaypoint_connection_cloest && ireflistlen(path->waypoints)) {
        last = icast(inaviwaypoint, ireflistfirst(path->waypoints)->value);
        waypoint->waypoint = _inavicellconnection_closest(connection, &last->waypoint);
    } else {
        waypoint->waypoint = connection->middle;
    }
    return waypoint;
}

/* NB!! no retain */
static inaviwaypoint *_inaviwaypoint_make_by_goal(inavipath *path, inavicell *cell, ipos3 *goal) {
    inaviwaypoint *waypoint = _inaviwaypoint_make();
    waypoint->type = EnumNaviWayPointType_Cell_Goal;
    iassign(waypoint->cell, cell);
    waypoint->waypoint = *goal;
    return waypoint;
}

void _inavipath_begin(inavipath *path, inavicell *end) {
    inaviwaypoint *waypoint;
    /* we found it */
    if (end == path->end) {
        waypoint = _inaviwaypoint_make_by_goal(path, end, &path->endpos);
        waypoint->flag |= EnumNaviWayPointFlag_End;
        ireflistadd(path->waypoints, irefcast(waypoint));
    } else {
        /*should insert a dynamic waypoint */
        waypoint = _inaviwaypoint_make_by_goal(path, path->end, &path->endpos);
        waypoint->flag |= EnumNaviWayPointFlag_Dynamic;
        ireflistadd(path->waypoints, irefcast(waypoint));
        
        /*the nearest end way point */
        waypoint = _inaviwaypoint_make_by_cell(path, end);
        ireflistadd(path->waypoints, irefcast(waypoint));
    }
}

void _inavipath_end(inavipath *path, inavicell* cell, inavicellconnection *connection) {
    inaviwaypoint *waypoint;
    /* should have connection */
    icheck(connection);
    
    if (cell == path->start) {
        /* connection */
        waypoint = _inaviwaypoint_make_by_connection(path, cell, connection);
        ireflistadd(path->waypoints, irefcast(waypoint));
    } else {
        /*may happend some ugly*/
    }
}

/* caculate the right cost */
static ireal _inavicell_arrivalcost(inavicell *cell, inavicontext *context,
                               inavinode *caller, inavicellconnection *connection) {
    ireal distmiddle = 0; ireal disttake = 0; ireal distnow = 0;
    ireal factor = 1.0;
    icheckret(caller && connection, 0);
    
    /* caculate the dist */
    if (caller->cell == context->path->start) {
        distmiddle = idistancepow3(&cell->polygon->center, &connection->middle);
        disttake = distmiddle + idistancepow3(&connection->middle, &caller->cell->polygon->center);
        distnow = distmiddle + idistancepow3(&connection->middle, &context->path->startpos);
    } else if (cell == context->path->end) {
        distmiddle = idistancepow3(&caller->cell->polygon->center, &connection->middle);
        disttake = distmiddle + idistancepow3(&connection->middle, &cell->polygon->center);
        distnow = distmiddle + idistancepow3(&connection->middle, &context->path->endpos);
    }
    
    /*there are some offset need to caculate the*/
    if (!ireal_equal(disttake, distnow) && !ireal_equal_zero(disttake)) {
        factor = distnow / disttake;
    }
    
    /* if we carefuly deal with cost , can be do some extermely moving stuffs*/
    /* if we do not want it just open the next line code */
    /* return caller->cell->costarrival + distnow; */
    return caller->cell->costarrival + connection->cost * factor;
}

static void _inavicell_process(inavicell *cell, inavicontext *context,
                               inavinode *caller, inavicellconnection *connection) {
    if (cell->sessionid != context->sessionid) {
        cell->sessionid = context->sessionid;
        cell->flag = EnumNaviCellFlag_Open;
        iwassign(cell->link, caller ? caller->cell:NULL);
        iwassign(cell->connection, connection);
        cell->heap_index = kindex_invalid;
       
        cell->costarrival = _inavicell_arrivalcost(cell, context, caller, connection);
        cell->costheuristic = _inavicontext_heuristic(context, cell);
        
        /* add cell to heap */
        _inavicontext_heap_cell(context, cell);
    } else if(cell->flag == EnumNaviCellFlag_Open) {
        iwassign(cell->link, caller->cell);
        iwassign(cell->connection, connection);
        cell->costarrival = _inavicell_arrivalcost(cell, context, caller, connection);
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
    context->sessionid = path->sessionid;
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
        /*get the top node */
        node = iheappeekof(context.heap, inavinode*);
        iretain(node);
        /*pop heap*/
        iheappop(context.heap);
        
        /* Arrived */
        if (node->cell == end) {
            found = true;
        } else {
            /* Process current neighbor */
            _inavinode_process(node, &context);
        }
        
        /* release node */
        irelease(node);
        
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
        connection = icast(inavicellconnection, iwrefunsafestrong(end->connection));
        cell = icast(inavicell, iwrefunsafestrong(end->link));
        while (cell && cell != path->start) {
            /* connection */
            waypoint = _inaviwaypoint_make_by_connection(path, cell, connection);
            ireflistadd(path->waypoints, irefcast(waypoint));
#if iiwaypoint_cell
            /* cell */
            waypoint = _inaviwaypoint_make_by_cell(path, cell);
            ireflistadd(path->waypoints, irefcast(waypoint));
#endif
            /* get next point */
            connection = icast(inavicellconnection, iwrefunsafestrong(cell->connection));
            cell = icast(inavicell, iwrefunsafestrong(cell->link));
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
    icheckret(start && end, 0);
    
    /*setup the path*/
    inavipathsetup(path, ++map->sessionid, start, from, end, to);
    
    /*find the path*/
    _inavimapfindpath_cell(map, unit, path, INT32_MAX);
    /*auto smooth the whole path*/
#if iiwaypoint_autosmooth
    inavimapsmoothpath(map, unit, path, INT32_MAX);
#endif
    /*return the path count*/
    return ireflistlen(path->waypoints);
}

/* The next cell in connection */
inavicell * inavimapnextcell(inavimap *map, int conn) {
    inavicellconnection *connection = NULL;
    icheckret(conn>=0 && conn<iarraylen(map->connections), NULL);
    
    connection = iarrayof(map->connections, inavicellconnection*, conn);
    icheckret(connection, NULL);
    icheckret(connection->next>=0 && connection->next<iarraylen(map->cells), NULL);
    
    return iarrayof(map->cells, inavicell*, connection->next);
}

/* used to smooth the waypoint */
typedef struct _inavi_smooth_point {
    inavicell *cell;
    ipos3 pos;
}_inavi_smooth_point;

/*check if we can see each other*/
int _inavi_cell_lineofsight_test(inavimap *map, _inavi_smooth_point *start, _inavi_smooth_point *end) {
    ipos pos;
    int conn;
    int result;
    inavicell *last = NULL;
    inavicell *next = start->cell;
    iline2d line = {_inavi_flat_pos(&start->pos), _inavi_flat_pos(&end->pos)};
    
    if (end->cell == NULL) {
        return iiok;
    }
    
    if (start->cell == end->cell) {
        return iiok;
    }
    
    while ((result=inavicellclassify(next, &line, &pos, &conn))==EnumNaviCellRelation_IntersetCell) {
        /* end the end cell */
        if (next == end->cell && ireal_equal(pos.x,end->pos.x) && ireal_equal(pos.y, end->pos.z)) {
            return iiok;
        }
        /* next util not the end ceil*/
        last = next;
        next = inavimapnextcell(map, conn);
        if (next == NULL) {
            return iino;
        }
        conn = kindex_invalid;
    }
    
    return result == EnumNaviCellRelation_InCell;
}

/* remove joint between (last, unitl )until the joint equal to until, and return the next */
irefjoint * _inavi_cell_remove_until(ireflist *list, irefjoint *last, irefjoint *until) {
    irefjoint *first = last ? last->next : ireflistfirst(list);
    icheckret(until, NULL);
    icheckret(first, NULL);
    
    while (first) {
        if (first != until) {
            first = ireflistremovejointandfree(list, first);
        }else {
            break;
        }
    }
    return until;
}

/* Make the path be smoothed */
int inavimapsmoothpath(inavimap *map, iunit *unit, inavipath *path, int steps) {
    irefjoint *joint;
    irefjoint *startjoint;
    irefjoint *lastvisiblejoint = NULL;
    
    inaviwaypoint * waypoint;
    _inavi_smooth_point start;
    _inavi_smooth_point end;
    /* will stop the loop in ugly forever */
    steps = imax(steps, 0);
    steps = imin(steps, ireflistlen(path->waypoints));
    
    icheckret(ireflistlen(path->waypoints) > 1, 0);
    
    /*start pos*/
    start.cell = path->start;
    start.pos = path->startpos;
    end.cell = NULL;
    
    /* the first way point */
    joint = ireflistfirst(path->waypoints);
    startjoint = NULL;
    lastvisiblejoint = joint;
    
    while (joint && steps) {
        /* way point */
        waypoint = icast(inaviwaypoint, joint->value);
        /* find next cell not equal to start */
        if (waypoint->cell && waypoint->cell != start.cell) {
            /* end cell */
            end.cell = waypoint->cell;
            end.pos = waypoint->waypoint;
        }
        /* judge if we can skip all waypoints in cell*/
        if (_inavi_cell_lineofsight_test(map, &start, &end) == iino) {
            /* remove all of visible (start, lastvisiblejoint)*/
            _inavi_cell_remove_until(path->waypoints, startjoint, lastvisiblejoint);
            /* line of sight invisible, should be try next */
            /* the waypoint in path can not see each other */
            waypoint = icast(inaviwaypoint, lastvisiblejoint->value);
            start.cell = waypoint->cell;
            start.pos = waypoint->waypoint;
            end.cell = NULL;
            
            startjoint = lastvisiblejoint;
            /* found max invisible step */
            --steps;
        } else if (joint->next == NULL) {
            /* remove all of visible (start, lastvisiblejoint)*/
            _inavi_cell_remove_until(path->waypoints, startjoint, joint);
            --steps;
        }
        
        lastvisiblejoint = joint;
        joint = joint->next;
    }
    
    return ireflistlen(path->waypoints);
}

/*************************************************************/
/* Make relation between the navimap and aoi map             */
/*************************************************************/

/* Add the cell to aoi map */
void inavimapcelladd(inavimap *map, inavicell *cell, imap *aoimap) {
    irect proj;
    ipos pos;
    int level;
    iunit *u_downleft;
    iunit *u;
    iunit *neighbor = NULL;
    
    /* the empty mapping should be */
    icheck(iarraylen(cell->aoi_cellunits) == 0);
    
#if __iopen_log_cell_add
    ilog("[INavi Cell-Add] ##Begin## "__icell_format"\n", __icell_value(*cell));
#endif
    
    /* get the polygon3d projection plane in xz */
    ipolygon3dtakerectxz(cell->polygon, &proj);
    /* caculating the contains level in aoi divide */
    level = imapcontainslevel(aoimap, &proj);
    
#if __iopen_log_cell_add
#define __inavi_cell_add_log \
    ilog("[INavi Cell-Add To Node] corner:"__ipos_format" node:"__inode_format"\n", \
           __ipos_value(pos), \
           __inode_value(*aoimap, *neighbor->node))
#else
#define __inavi_cell_add_log
#endif
    
    /* down-left */
    pos = irectdownleft(&proj);
    u_downleft = imakeunit(-1, proj.pos.x, proj.pos.y);
    u_downleft->userdata.up1 = cell;
    u_downleft->flag |= EnumNaviUnitFlag_Cell;
    imapaddunittolevel(aoimap, u_downleft, level);
    iarrayadd(cell->aoi_cellunits, &u_downleft);
    iassign(neighbor, u_downleft);
    __inavi_cell_add_log;

/* add a macro to speed it */
#define __inavi_cell_justaddit \
    u = imakeunit(-1, pos.x, pos.y); \
    u->flag |= EnumNaviUnitFlag_Cell; \
    imapaddunittolevel(aoimap, u, level); \
    iarrayadd(cell->aoi_cellunits, &u); \
    iassign(neighbor, u); \
    irelease(u);\
    __inavi_cell_add_log
    
    /* down-right */
    pos = irectdownright(&proj);
    if (!inodecontains(aoimap, neighbor->node, &pos)) {
        __inavi_cell_justaddit;
    }
    
    /* up-left */
    pos = irectupleft(&proj);
    if (!inodecontains(aoimap, u_downleft->node, &pos)) {
        __inavi_cell_justaddit;
    }
    
    /* up-right */
    pos = irectupright(&proj);
    if (!inodecontains(aoimap, u_downleft->node, &pos)
        && (neighbor == NULL || !inodecontains(aoimap, neighbor->node, &pos))) {
        __inavi_cell_justaddit;
    }
    
#if __iopen_log_cell_add
    ilog("[INavi Cell-Add] ##End## "__icell_format"\n", __icell_value(*cell));
#endif
    
    irelease(u_downleft);
    irelease(neighbor);
}
    
/* Del the cell to aoi map */
void inavimapcelldel(inavimap *map, inavicell *cell, imap *aoimap) {
    inavicellunaoi(cell, aoimap);
}
    
/* Find the cells in aoi map */
iarray *inavimapcellfind(inavimap *map, imap *aoimap, const ipos3 *pos) {
    iarray* arr = iarraymakeiref(4);
    ipos pos2 = {pos->x, pos->z};
    icode code;
    inode *node;
    iunit *u;
    inavicell *cell;
    
#if __iopen_log_cell_find
    ilog("[INavi Cell-Find] #Begin# pos:"__ipos_format"\n", __ipos_value(*pos));
#endif
    
    /* gen node code */
    imapgencode(aoimap, &pos2, &code);
    /*Fuzzy Find the top node */
    node = imapgetnode(aoimap, &code, aoimap->divide, EnumFindBehaviorFuzzy);
    while (node) {
#if __iopen_log_cell_find
        ilog("[INavi Cell-Find] Node:"__inode_format"\n", __inode_value(*aoimap, *node));
#endif
        u = node->units;
        /* for-each unit in node */
        if (u) do {
            if (u->flag & EnumNaviUnitFlag_Cell) {
                cell = icast(inavicell, u->userdata.up1);
#if __iopen_log_cell_find
                ilog("[INavi Cell-Find] cell:"__icell_format"\n", __icell_value(*cell));
#endif
                iarrayadd(arr, &cell);
            }
            u = u->next;
        }while (u);
        /*to parent*/
        node = node->parent;
    }
    
#if __iopen_log_cell_find
    ilog("[INavi Cell-Find] #End# pos:"__ipos_format"\n", __ipos_value(*pos));
#endif
    
    return arr;
}

/*************************************************************/
/* Map Convex-Polygon-Builder                                */
/*************************************************************/
/* load from height-map */
int inavimapdescreadfromheightmap(inavimapdesc *desc, iheightmap *height) {
    return iiok;
}


/*************************************************************/
/* Map Convex-Hull-Algorithm                                 */
/*************************************************************/
/*[QuickHull](http://www.cnblogs.com/Booble/archive/2011/03/10/1980089.html)*/
/*[](http://xuewen.cnki.net/CJFD-BJHK901.019.html) */
/*[](https://en.wikipedia.org/wiki/Convex_hull)*/
/*[QuickHull](http://www.cs.princeton.edu/courses/archive/spr10/cos226/demo/ah/QuickHull.html)*/
/*[ConvexHull](http://www.csie.ntnu.edu.tw/~u91029/ConvexHull.html)*/

/* 
 * @return: [] *ipolygon3d 
 * @param pos: [] *ipos3 */
iarray* inavimapgenconvex3d(iarray* pos) {
    /*todo:*/
    return NULL;
}

/*
 * @return: [] *iarray([]*ipos3)
 * @param pos: [] *ipos3 */
iarray* inavimapgenplanes(iarray *pos) {
    /*todo:*/
    return NULL;
}

/* 
 * @return: [] *ipolygon
 * @param pos: [] *ipos */
iarray* inavimapgenconvex(iarray* pos){
    /*todo:*/
    return NULL;
}


/*************************************************************/
/* implement the new type for iimeta system                  */
/*************************************************************/

#undef __inavi_typeof
#define __inavi_typeof(type, cachesize) irealdeclareregister(type);
/* implement meta for inavicell */
__inavi_types

/* NB!! should call first before call any navi funcs 
 * will registing all the navi types to meta system
 * */
int inavi_mm_init() {
#undef __inavi_typeof
#define __inavi_typeof(type, cachesize) irealimplementregister(type, cachesize);
__inavi_types

    return iiok;
}
