
#include "aoi.h"
#include "aoitest.h"

// because the sdl have define the main be _sdl_main
#ifdef main
#undef main
#endif

// 入口函数
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    irect r = {{0,0}};
    (void)r;

    runAllTest();

    return 0;
}
