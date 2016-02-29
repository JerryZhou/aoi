#include "aoi.h"
#include "draw.h"

#include "aoitest.h"

// Draw App Event Loop With Window
void drawapp(int argc, char **argv) {
    // initialize OpenSteerDemo application
    Draw::App::initialize ();
    
    // initialize graphics
    Draw::initializeGraphics (argc, argv);
    
    // run the main event processing loop
    Draw::runGraphics ();
}

void __whenirefalloc(iref* ref) {
    
}

#define irefalloc(type, name) type* name = NULL; name = iobjmalloc(type); irefretain((iref*)name); __whenirefalloc((iref*)name)

int main(int argc, char ** argv){
    
    irefalloc(inode, node);
    
    irelease(node);
    
    runAllTest();
    
    drawapp(argc, argv);
    
    return 0;
}
