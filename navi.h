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

typedef struct inavinode {
    /* 声明引用对象 */
    irefdeclare;

    /* the navi node polygon */
    ipolygon2d *polygon;

    /*
     * 构成了一个有向图，可在联通上做单向通行
     * */
    /* 所有可以到达当前节点的邻居 other ===> this */
    ireflist *neighbors;
    /* 可走的列表 this ===> other */
    ireflist *neighbors_walkable;
}inavinode;

/*************************************************************/
/* inavimap                                                 */
/*************************************************************/

typedef struct inavimap {
    /* 声明引用对象 */
    irefdeclare;

    /* 所有polygons */
    ireflist *nodes;
}inavimap;

/* pathfind  */
ireflist* ipathfind(iunit *unit, inavinode *from, inavinode *to); 


/*************************************************************/
/* declare the new type for iimeta system                    */
/*************************************************************/

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


