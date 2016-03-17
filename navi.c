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


/* navi map find the cell */
const inavicell* inavimapfind(const inavimap *map, const ipos *pos) {
    return NULL;
}

#define MAX_HEAP_DEPTH 64
/* A* */
static void _inavimapfindpath_cell(const inavimap *map,
                                         const iunit *unit,
                                         const inavicell *start,
                                         const inavicell *end,
                                         ireflist *nodes) {
    iarray * opens;
    
    icheck(start);
    icheck(end);
    
    opens = iarraymakeiref(MAX_HEAP_DEPTH);
    iheapbuild(opens);
    
    if (start == end) {
        
    }
}

/* navi map find the path */
int inavimapfindpath(const inavimap *map, const iunit *unit, const ipos *from, const ipos *to, inavipath *path) {
    const inavicell *start = inavimapfind(map, from);
    const inavicell *end = inavimapfind(map, to);
    _inavimapfindpath_cell(map, unit, start, end, path->nodes);
    return ireflistlen(path->nodes);
}

/*************************************************************/
/* implement the new type for iimeta system                  */
/*************************************************************/

/* implement meta for inavinode */
irealdeclareregister(inavinode);

/* implement meta for inavimap */
irealdeclareregister(inavimap);


/* NB!! should call first before call any navi funcs 
 * will registing all the navi types to meta system
 * */
int inavi_mm_init() {

    irealimplementregister(inavinode, 0);
    irealimplementregister(inavimap, 0);

    return iiok;
}
