# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)
set(CMAKE_MACOSX_RPATH 1)

# 项目信息
project (aoi)

# aoi 库文件
add_library(aoi STATIC aoi.c)

# aoi 单元测试
add_executable(aoi-test main.cpp aoi.c)

# aoi 性能工具
add_executable(aoi-prof prof.c aoi.c)

# lua aoi
set(CMAKE_BUILD_TYPE Release)
find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
add_library(laoi MODULE laoi.c aoi.c)
target_link_libraries(laoi ${LUA_LIBRARIES})
set_target_properties(laoi PROPERTIES PREFIX "" SUFFIX ".so")

# copy the test.lua to build
configure_file(test.lua test.lua)


