# 与quick-cocos2dx整合
* 将aoi.h、aoi.c、laoi.c三个文件加入cocos/quick_libs/src/extensions/aoi目录
* 在quick_extensions.c中为luax_exts数组的{NULL, NULL}之前加上一行{"laoi", luaopen_laoi}，编译工程。
* 在代码中运行TestAoiScene场景，运行结果如下：


![](https://raw.githubusercontent.com/donnki/aoi/master/cocos2dx-lua/DC9CDF27-63A7-40F5-94DF-9CB73A42575A.png)

（实心圆表示单位本身半径，外围红圈表示单位视野半径）
