# CMakeList.txt: test 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD, 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -W -no-pie -g")
message(${CMAKE_CXX_FLAGS})
SET(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS asm)  # 设置NASM的文件扩展名为asm
ENABLE_LANGUAGE(ASM_NASM)  # 让CMacke启用NASM的汇编
SET(CMAKE_ASM_NASM_FLAGS "-g") # 让NASM生成调试信息




if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
	message("--------------64 bit-------------")
	add_definitions(-DPLATFORM_64)
	set(ASM_CONTEXT context_64.asm)
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bit
	message("--------------32 bit-------------")
	add_definitions(-DPLATFORM_32)
	set(ASM_CONTEXT context.asm)
endif()
message(${ASM_CONTEXT})

# 将源代码添加到此项目的可执行文件。
add_executable (coroutine "coroutine.cpp" "coroutine.h" ${ASM_CONTEXT} "test.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET coroutine PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
