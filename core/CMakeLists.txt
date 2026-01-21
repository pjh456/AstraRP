cmake_minimum_required(VERSION 3.18)
project(AstraRP_core VERSION 0.1.0)

# 1. 设置 C++ 标准 (llama.cpp 要求至少 C++11，建议使用 17 或更高)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. CPU 指令集优化：开启高度优化并启用原生指令集 (AVX2/AVX512等)
if(MSVC)
    # Windows / MSVC 优化
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/arch:AVX2>

        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/GL>
        $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
    ) # 开启 O2 优化、全局优化和 AVX2
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:RelWithDebInfo>:/LTCG>
    ) # 开启链接时代码生成
else()
    # Linux / macOS (GCC/Clang) 优化
    # -O3: 最高级优化
    # -march=native: 让编译器针对你当前的 CPU 指令集进行全速优化
    # -flto: 开启链接时优化，减小体积并提升跨文件调用性能
    add_compile_options(-O3 -march=native -mtune=native -flto=auto)
    add_link_options(-flto)
endif()

# 3. 引入 llama.cpp 目录并开启内部优化开关
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama.cpp examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama.cpp server" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama.cpp tests" FORCE)
set(GGML_NATIVE ON CACHE BOOL "Enable native optimization" FORCE)
set(LLAMA_BUILD_COMMON ON CACHE BOOL "Build common library" FORCE)
set(LLAMA_DIR ${CMAKE_SOURCE_DIR}/llama.cpp)
add_subdirectory(${LLAMA_DIR})

# 4. 添加可执行程序
file(GLOB_RECURSE ASTRARP_HEADERS
    RELATIVE ${CMAKE_SOURCE_DIR}
    include/*.h
    include/*.hpp
)

file(GLOB ASTRARP_SOURCES
    RELATIVE ${CMAKE_SOURCE_DIR}
    src/*.c
    src/*.cpp
)

function(target_config TARGET SRC)
    add_executable(${TARGET} ${SRC})
    target_link_libraries(${TARGET} PRIVATE
        llama
        common
    )
    target_include_directories(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/llama.cpp/include
        ${CMAKE_SOURCE_DIR}/llama.cpp/common
    )
endfunction()

target_config(astra ${ASTRARP_SOURCES})