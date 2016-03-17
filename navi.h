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

typedef struct inavicell {
    /* 声明引用对象 */
    irefneighborsdeclare;

    /* the navi node polygon */
    ipolygon2d *polygon;
}inavicell;

/* navigation node in path*/
typedef struct inavinode {
    irefdeclare;
    
    /* cost */
    ireal cost;
    
    /* cell of this node */
    inavicell *cell;
    
    /* cell where we got from */
    inavicell *from;
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
    
    /*nodes list */
    ireflist *nodes;
}inavipath;

/*************************************************************/
/* inavimap                                                 */
/*************************************************************/

typedef struct inavimap {
    /* Declare the iref object */
    irefdeclare;

    /* All polygon cells */
    iarray *cells;
    
    /* NaviNode Cache */
    irefcache *nodecache;
    
    /* NaviCell Cache */
    irefcache *cellcache;
}inavimap;

/* Make navimap from the blocks */
inavimap* inavimapmake(size_t width, size_t height, char * blocks);

/* navi map find the cell */
const inavicell* inavimapfind(const inavimap *map, const ipos *pos);

/* navi map find the path */
int inavimapfindpath(const inavimap *map, const iunit *unit, const ipos *from, const ipos *to, inavipath *path);

/*************************************************************/
/* declare the new type for iimeta system                    */
/*************************************************************/

/* declare meta for inavicell */
iideclareregister(inavicell);

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


