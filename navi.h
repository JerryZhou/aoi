#ifndef __NAVI_H_
#define __NAVI_H_

/*iref iarray islice ivec2 ivec3*/
#include "aoi.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif /*end of __cplusplus*/

/*
 * 1. Nodal-pathfinding:
 * http://www.jgallant.com/nodal-pathfinding-in-unity-2d-with-a-in-non-grid-based-games/
 *
 * 2. A* algorithm:
 * http://www.policyalmanac.org/games/aStarTutorial.htm
 * https://en.wikipedia.org/wiki/A*_search_algorithm
 * https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
 * http://gamedevelopment.tutsplus.com/tutorials/how-to-speed-up-a-pathfinding-with-the-jump-point-search-algorithm--gamedev-5818
 *
 * 3. Unit-Movement
 * http://www.gamasutra.com/view/feature/131720/coordinated_unit_movement.php
 *
 * 4. Navigation-Mesh
 * https://en.wikipedia.org/wiki/Navigation_mesh
 * http://www.gameenginegems.net/gemsdb/article.php?id=92
 *      [Simplified 3D Movement and Pathfinding Using Navigation Meshes]
 * http://accu.org/index.php/journals/1838
 *      [Automatic Navigation Mesh Generation in Configuration Space] 
 * http://www.staff.science.uu.nl/~gerae101/pdf/CAVW_Dynamic_ECM.pdf
 *      [A navigation mesh for dynamic environments]
 *
 * 5. AI
 * http://www.kbs.twi.tudelft.nl/docs/MSc/2001/Waveren_Jean-Paul_van/thesis.pdf 
 *      [Quake III Bot System]
 * */
    
/* Left Hand System
 * y     z
 * ^     ^
 * |    /
 * |   /
 * |  /
 * | /
 * |---------> x
 * */

/*************************************************************/
/* inavinode                                                 */
/*************************************************************/

/* cell flag */
enum EnumNaviCellFlag {
    EnumNaviCellFlag_Open = 1<<1,
    EnumNaviCellFlag_Close = 1<<2,
};

/* cell */
struct inavicell;
/* map */
struct inavimap;
    
/*neighbors connection resouce */
typedef struct inavicellconnection {
    irefdeclare;
    
    /*pologon point begin index */
    int index;
    /* const: cell a to b*/
    ireal cost;
    /* the middle point in connecting edge*/
    ipos3 middle;
    
    /* where we came from (cell index in map )*/
    int from;
    /* where we going for (cell index in map) */
    int next;
    
    /* the localtion of connection (connection index in map )*/
    int location;
}inavicellconnection;

/* Make connection */
inavicellconnection * inavicellconnectionmake();
/* Release connection */
void inavicellconnectionfree(inavicellconnection* con);

typedef struct inavicell {
    /* 声明引用对象 */
    irefneighborsdeclare;

    /* the navi node polygon */
    ipolygon3d *polygon;
    
    /* all connections with others, according to edge count */
    /* [] int */
    /* con = map->connections[cell->connetions[i]] */
    iarray* connections;
    
    /* trace the cell index in map */
    /* map->cells[cell->cell_index] == cell*/
    int cell_index;
    
    /* session id that the cell last deal */
    int64_t sessionid;
    /* session flag */
    int flag;
    /* trace the the index in navi heap */
    int heap_index;
    /* the navigation link */
    struct inavicell *link;
    /* the navigation connection */
    inavicellconnection *connection;
    /* cost */
    ireal costarrival;
    ireal costheuristic;
}inavicell;
    
/* Make a cell with poly and connections */
inavicell *inavicellmake(struct inavimap* map, ipolygon3d *poly, islice* connections, islice *costs);
    
/* add connection to cell */
void inavicelladdconnection(inavicell *cell, struct inavimap *map, int edge, int next, ireal cost);

/* Connect the cell to map */
void inavicellconnecttomap(inavicell *cell, struct inavimap* map);

/* Disconnect the cell to map */
void inavicelldisconnectfrommap(inavicell *cell);
    
/* Release the cell */
void inavicellfree(inavicell *cell);
    
/* Fetch the height from cell to pos */
int inavicellmapheight(inavicell *cell, ipos3 *pos);
    
/* Cell relation with line end */
typedef enum EnumNaviCellRelation {
    EnumNaviCellRelation_OutCell,
    EnumNaviCellRelation_InCell,
    EnumNaviCellRelation_IntersetCell,
}EnumNaviCellRelation;
    
/* classify the line relationship with cell */
int inavicellclassify(inavicell *cell, const iline2d *line,
                      ipos *intersection, inavicellconnection **connection);

/* Waypoint Type */
typedef enum EnumNaviWayPointType {
    /*the connection is a cell */
    EnumNaviWayPointType_Cell = 1,
    /*the connection is a connection */
    EnumNaviWayPointType_Connection = 2,
    /*the connection is a unit*/
    EnumNaviWayPointType_Unit = 3,
    /*the connection is a cell goal */
    EnumNaviWayPointType_Cell_Goal = 4,
}EnumNaviWayPointType;

/* Waypoint Flag */
typedef enum EnumNaviWayPointFlag {
    /*the way point*/
    EnumNaviWayPointFlag_Nothing = 0,
    
    /* mark the way point is the path start */
    EnumNaviWayPointFlag_Start = 1<<1,
    /* mark the way point is the path end */
    EnumNaviWayPointFlag_End = 1<<2,
    /* mark the way point is the dynamic point*/
    EnumNaviWayPointFlag_Dynamic = 1<<3,
}EnumNaviWayPointFlag;

/**/
typedef struct inaviwaypoint {
    irefdeclare;
    /* way point type */
    EnumNaviWayPointType type;
    /* way point flag */
    EnumNaviWayPointFlag flag;
    /* cell of this node */
    inavicell *cell;
    /* cell of connection to next */
    inavicellconnection *connection;
    
    /*real goal of waypoint */
    ipos3 waypoint;
}inaviwaypoint;

/* Navigation path */
typedef struct inavipath {
    irefdeclare;
    
    /* session id*/
    int64_t sessionid;
    
    /* cell */
    inavicell *start;
    inavicell *end;
    
    /* pos */
    ipos3 startpos;
    ipos3 endpos;
    
    /* inaviwaypoint */
    ireflist *waypoints;
    /* current node */
    irefjoint *current;
}inavipath;
    
/* make a navi path */
inavipath *inavipathmake();
    
/* navigation path free */
void inavipathfree(inavipath *path);

/*************************************************************/
/* inavimap                                                 */
/*************************************************************/

typedef struct inavimap {
    /* Declare the iref object */
    irefdeclare;
 
    /* all polygons */
    /* [] *polygons */
    iarray *polygons;
    
    /* All polygon cells */
    /* [] *inavicell */
    iarray *cells;
    
    /* all connections */
    /* [] inavicellconnections */
    iarray *connections;
    
    /* Global session Id */
    int64_t sessionid;
}inavimap;

/* header */
typedef struct inavimapheader {
    size_t width;               /* map size width*/
    size_t height;              /* map size height*/
    size_t points;              /* number of point */
    size_t polygons;            /* number of polygons */
    size_t polygonsize;         /* number of int*/
}inavimapheader;
    
/* inavimap desc can be read from file or write to file */
typedef struct inavimapdesc {
    /* navi map header */
    inavimapheader header;
    
    /* all ipos3: []ipos */
    iarray *points;
    
    /* all polygons: []int 
     * poly0-len, poly1-len, poly2-len ...
     */
    iarray *polygons;
    
    /* all polygonsindex: []int
     * poly0-idx0, poly0-idx1, poly0-idx2, poly1-idx0, poly1-idx1, poly1-idx2 ...
     */
    iarray *polygonsindex;
    
    /* all connections: []int, kindex_invalid is invalid connection
     * poly0-idx0-connection, poly0-idx1-connection, poly0-idx2-connection, 
     * poly0-idx0-connection, poly0-idx1-connection, poly0-idx2-connection ...
     */
    iarray *polygonsconnection;
    
    /* all costs: []ireal
     * poly0-idx-cost, poly0-idx1-cost ...
     */
    iarray *polygonscosts;
}inavimapdesc;
    
/* release the resource hold by desc */
void inavimapdescfreeresource(inavimapdesc *desc);

/* read the navimap from textfile */
/*
 Map: width 512 height 512 points 8 polygons 4 polygonsize 14
 
 (0,0,0)(1,0,1)(1,0,0)(3,0,0)(3,0,1)(4,0,1)
 (3,0,5)(4,0,5)
 
 3:0-0-1 1-0-1 2-0-1
 4:2-0-1 1-0-1 4-0-1 3-0-1
 3:3-0-1 4-0-1 5-0-1
 4:4-0-1 6-0-1 7-0-1 5-0-1
 */
int inavimapdescreadfromtextfile(inavimapdesc *desc, const char* file);
    
/* write the navimap to textfile */
void inavimapdescwritetotextfile(inavimapdesc *desc, const char* file);

/* Make navimap  */
inavimap* inavimapmake(size_t capacity);
    
/* load the navimap from heightmap */
void inavimapload(inavimap *map, size_t width, size_t height, ireal *heightmap, ireal block);
    
/* load the navimap from desc */
void inavimaploadfromdesc(inavimap *map, const inavimapdesc *desc);
    
/* Free the navi map */
void inavimapfree(inavimap *map);

/* navi map find the cell */
inavicell* inavimapfind(const inavimap *map, const ipos3 *pos);
    
/* navi map find the cell */
inavicell* inavimapfindclosestcell(const inavimap *map, const islice* cells, const ipos3 *pos);

/* navi map find the path */
int inavimapfindpath(inavimap *map, iunit *unit, const ipos3 *from, const ipos3 *to, inavipath *path);

/*************************************************************/
/* declare the new type for iimeta system                    */
/*************************************************************/

/* declare meta for inavicell */
iideclareregister(inavicell);
    
/* declare meta for inavicellconnection */
iideclareregister(inavicellconnection);
    
/* declare meta for inaviwaypoint */
iideclareregister(inaviwaypoint);
    
/* declare meta for inavipath */
iideclareregister(inavipath);

/* declare meta for inavimap */
iideclareregister(inavimap);

/* NB!! should call first before call any navi funcs 
 * will registing all the navi types to meta system
 * */
int inavi_mm_init();

#ifdef __cplusplus
}
#endif /*end of __cplusplus*/

#endif /*end of __NAVI_H_*/


