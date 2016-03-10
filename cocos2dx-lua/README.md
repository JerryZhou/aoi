# 与quick-cocos2dx整合
将aoi.h、aoi.c、laoi.c三个文件加入cocos/quick_libs/src/extensions/aoi目录
在quick_extensions.c中为luax_exts数组的{NULL, NULL}之前加上一行{"laoi", luaopen_laoi}，编译工程。