# aoi
area of interest(AOI)

# How to Build

### on Linux/Mac OSX

###### to make libaoi.a for c:

```
make
```

###### make laoi.so and laoi.a for lua

```
make ENABLE_LUALIB=true
```

### on Windows

##### see msvc_build.bat

```
msvc_build.bat
```

# 视野服务
游戏服务器的AOI（area of interest)部分，位置有关的游戏实体一般都有一个视野或关心的范围


# 效率案例
地图 512x512 , 10 级分割
在地图上同时移动 1000 个对象
实时搜索100个对象周边的视野对象
总共平均耗时 1-2 毫秒
单次搜索平均耗时 8-15 纳秒
机器：i7(2.4), 8G

详情见wiki

# 简单实用
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


# lua Test

build first and then:

```
lua test.lua
```

