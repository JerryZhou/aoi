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

/*neighbors connection resouce */
typedef struct inavicellconnection {
    /*pologon point begin index */
    int index;
    /*const: cell a to b*/
    ireal cost;
    /*cell*/
    struct inavicell *next;
}inavicellconnection;

typedef struct inavicell {
    /* 声明引用对象 */
    irefneighborsdeclare;

    /* the navi node polygon */
    ipolygon3d *polygon;
    
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
    
/*Make navi node with cell*/
inavinode *inavinodemake(const inavicell *from, const inavicell *to);

/*Release navi node*/
void inavinodefree(inavinode *node);

/* Make a Navi Nodes Heap with Cost Order Desc */
iheap* inavinodeheapmake();

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
    
    /*inavinode array */
    iarray *waypoints;
}inavipath;

/* setup the path */
void inavipathsetup(inavipath *path, int64_t sessionid,
                    inavicell *start, const ipos3 *startpos,
                    inavicell *end, const ipos3 *endpos);

/*************************************************************/
/* inavimap                                                 */
/*************************************************************/

typedef struct inavimap {
    /* Declare the iref object */
    irefdeclare;

    /* All polygon cells */
    iarray *cells;
    
    /* Global session Id */
    int64_t sessionid;
}inavimap;

/* Make navimap from the blocks */
inavimap* inavimapmake(size_t width, size_t height, char * blocks);

/* navi map find the cell */
inavicell* inavimapfind(const inavimap *map, const ipos3 *pos);

/* navi map find the path */
int inavimapfindpath(inavimap *map, iunit *unit, const ipos3 *from, const ipos3 *to, inavipath *path);

/*************************************************************/
/* declare the new type for iimeta system                    */
/*************************************************************/

/* declare meta for inavicell */
iideclareregister(inavicell);
    
/* declare meta for inavinode */
iideclareregister(inavinode);

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


