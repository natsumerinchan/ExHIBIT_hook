cmake_minimum_required(VERSION 3.10)
project(soranana_chs)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置编译为DLL且输出指定名称
add_library(soranana_chs SHARED soranana_chs.cpp)

# 包含detours头文件目录
include_directories(detours)

# 链接detours库
target_link_libraries(soranana_chs ${CMAKE_SOURCE_DIR}/detours/detours.lib)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows特定设置
if(WIN32)
    target_compile_definitions(soranana_chs PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(soranana_chs gdi32)
endif()
