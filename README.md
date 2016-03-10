# 视野服务 area of interest(AOI)
游戏服务器的AOI（area of interest)部分，位置有关的游戏实体一般都有一个视野或关心的范围
 
* 适用于大批量的对象管理和查询
* 采用四叉树来进行管理，可以通过少量改动支持3D
* 动态节点回收管理，减少内存, 节点的数量不会随着四叉树的层次变得不可控，与对象数量成线性关系
* 缓存搜索上下文，减少无效搜索，对于零变更的区域，搜索会快速返回
* 整个搜索过程，支持自定义过滤器进行单元过滤
* AOI 支持对象的内存缓冲区管理
* 线程不安全
* 支持单位半径，不同单位可以定义不同的半径(开启半径感知，会损失一部分性能)
* 只提供了搜索结构，不同单位的视野区域可以定义不一样

*提高内存访问，建议启用 iimeta (1)*
*如果不需要单位半径感知 建议关闭 iiradius (0)*

# How to Build (CMake Tools)

    ```
    mkdir build
    cd build
    cmake ../
    make
    ```

# 性能测试
通过提供的prof 工具测试获取自己需要的参数
    ```
    aoi-prof c [divide] [max-unit] [min-search-range] [max-rand-search-range] [benchtimes]
    ```
![测试样例](http://dwgaga-image.qiniudn.com/more_img_1__Default__bash_.png)

# 单元测试
    ```
    aoi-test
    ```

# lua Test
build first and then:

    ```
    lua test.lua
    ```


# 效率案例
地图 512x512 , 10 级分割
在地图上同时移动 1000 个对象
实时搜索100个对象周边的视野对象
总共平均耗时 1-2 毫秒
单次搜索平均耗时 8-15 纳秒
机器：i7(2.4), 8G

详情见wiki

# 简单实用
    ```
    // make game world
    imap *map = imapmake(pos, size, divide)
 
    // make a unit
    iunit *unit = iunitmake(id, x, y);
 
    // add unit to game world
    imapaddunit(map, unit);
 
    // update the unit position
    unit->pos.x = 1;
    unit->pos.y = 1;
 
    // refresh the unit position in game world
    imapupdateunit(map, unit):
 
    // remove the unit from game world
    imapremoveunit(map, unit);
 
    // make a aoi list
    isearchresult *result = isearchresultmake();
 
    // search the unit in range of unita
    imapsearchfromunit(map, unita, result, range)
 
    // free the aoi list
    isearchresultfree(result)
    ```


# wiki
主要是维护实体区域，并维护对象的AOI列表

![8个对象，1个视野对象维护AOI](http://dwgaga-image.qiniudn.com/App_0_8_23.png)


![1.2M内存消耗，750 个对象同时移动，100个对象同时计算周围的AOI，总共消耗2毫秒的样子](http://dwgaga-image.qiniudn.com/Banners_and_Alerts_App_0_8_2.png)


