# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)
set(CMAKE_MACOSX_RPATH 1)

#set(CMAKE_C_FLAGS "-O2 -Wall -Werror=declaration-after-statement -std=c89 -pedantic -Dinline='' -Wno-c99-extensions -Wno-variadic-macros -Wno-comment -Wno-unused-function")

# 项目信息
project (aoi)

# aoi 库文件
add_library(aoi STATIC aoi.c aoi.h)

# aoi 单元测试
include_directories(${CMAKE_SOURCE_DIR})
add_executable(aoi-test ${CMAKE_SOURCE_DIR}/test/main.cpp ${CMAKE_SOURCE_DIR}/test/simpletest.h ${CMAKE_SOURCE_DIR}/test/aoitest.h)
target_link_libraries(aoi-test aoi)

# aoi 性能工具
include_directories(${CMAKE_SOURCE_DIR})
add_executable(aoi-prof ${CMAKE_SOURCE_DIR}/prof/prof.c)
target_link_libraries(aoi-prof aoi)

# lua aoi
set(CMAKE_BUILD_TYPE Release)
find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
add_library(laoi MODULE ${CMAKE_SOURCE_DIR}/lua-bind/laoi.c)
target_link_libraries(laoi ${LUA_LIBRARIES} aoi)
set_target_properties(laoi PROPERTIES PREFIX "" SUFFIX ".so")

# copy the test.lua to build
configure_file(${CMAKE_SOURCE_DIR}/lua-bind/test.lua test.lua)


