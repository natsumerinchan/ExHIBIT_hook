cmake_minimum_required(VERSION 3.10)
project(ExHIBIT_hook)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置编译为DLL且输出指定名称
add_library(ExHIBIT_hook SHARED ExHIBIT_hook.cpp)

# 包含detours头文件目录
include_directories(detours)

# 链接detours库
target_link_libraries(ExHIBIT_hook ${CMAKE_SOURCE_DIR}/detours/detours.lib)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows特定设置
if(WIN32)
    target_compile_definitions(ExHIBIT_hook PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(ExHIBIT_hook gdi32)
endif()
