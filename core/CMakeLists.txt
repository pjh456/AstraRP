cmake_minimum_required(VERSION 3.18)
project(AstraRP_core VERSION 0.1.0)

# 1. 设置 C++ 标准 (llama.cpp 要求至少 C++11，建议使用 17 或更高)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ================= 构建控制开关 =================
# 默认关闭 LTO，以大幅度提升日常编译和链接速度。发布正式版时可开启。
option(ASTRARP_ENABLE_LTO "Enable Link Time Optimization (LTO)" OFF)

# 默认关闭 examples 测试集构建，如果只是为了给 Node.js 编译 addon 根本不需要编这些
option(ASTRARP_BUILD_EXAMPLES "Build AstraRP examples" OFF)
option(ASTRARP_BUILD_TESTS "Build AstraRP tests" OFF)

# =================================================

# 2. CPU 指令集优化：开启高度优化并启用原生指令集 (AVX2/AVX512等)
if(MSVC)
    add_compile_options(
        /utf-8
        /MP # /MP 开启多核编译
        /Zc:char8_t-
    )

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Windows / MSVC 优化
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/arch:AVX2>

        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
    ) # 开启 O2 优化、全局优化和 AVX2

    # 只有开启 LTO 开关时，才应用导致链接巨慢的全局优化选项
    if(ASTRARP_ENABLE_LTO)
        add_compile_options(
            $<$<CONFIG:Release>:/GL>
            $<$<CONFIG:RelWithDebInfo>:/GL>
        )
        add_link_options(
            $<$<CONFIG:Release>:/LTCG>
            $<$<CONFIG:RelWithDebInfo>:/LTCG>
        )
    endif()
else()
    # Linux / macOS (GCC/Clang) 优化
    # 生成可用于共享库的静态库（Node addon 需要 -fPIC）
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_link_options(
        -static-libgcc
        -static-libstdc++
        -static)

    add_compile_options(-O2 -march=native -mtune=native)

    if(ASTRARP_ENABLE_LTO)
        add_compile_options(-flto=auto)
        add_link_options(-flto)
    endif()
endif()

# 3. 引入 llama.cpp 目录并开启内部优化开关
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama.cpp examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama.cpp server" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama.cpp tests" FORCE)
set(LLAMA_BUILD_COMMON OFF CACHE BOOL "Build common library" FORCE)

set(LLAMA_CURL OFF CACHE BOOL "Disable libcurl dependency" FORCE)
set(GGML_RPC OFF CACHE BOOL "Disable RPC support" FORCE)
set(LLAMA_STANDALONE OFF CACHE BOOL "Tell llama.cpp it is being included" FORCE)

set(GGML_NATIVE ON CACHE BOOL "Enable native optimization" FORCE)
set(GGML_OPENMP OFF CACHE BOOL "Enable OpenMP" FORCE)

set(LLAMA_HTTPLIB OFF CACHE BOOL "Disable llama.cpp httplib" FORCE)
set(LLAMA_OPENSSL OFF CACHE BOOL "Disable llama.cpp openssl" FORCE)

set(HAVE_STD_REGEX 1) # make benchmark happy

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static libraries" FORCE)
set(LLAMA_SHARED OFF CACHE BOOL "Disable llama shared lib" FORCE)
set(GGML_SHARED OFF CACHE BOOL "Disable ggml shared lib" FORCE)

set(LLAMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp)

# set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${LLAMA_DIR})

# 注入环境变量（在 CI 或无 .env 时允许缺失）
set(ENV_LIST)
if(EXISTS "${CMAKE_SOURCE_DIR}/.env")
    file(STRINGS "${CMAKE_SOURCE_DIR}/.env" ENV_LINES)

    foreach(line IN LISTS ENV_LINES)
        string(STRIP "${line}" line)

        if(line STREQUAL "" OR line MATCHES "^#")
            continue()
        endif()

        list(APPEND ENV_LIST "${line}")
    endforeach()
endif()

# 4. 添加可执行程序
file(GLOB_RECURSE ASTRARP_HEADERS
    include/*.h
    include/*.hpp
)

file(GLOB_RECURSE ASTRARP_SOURCES CONFIGURE_DEPENDS
    src/*.c
    src/*.cpp
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/pjh_json/include/pjh_json")

# 创建一个静态库，方便 examples 链接
add_library(astrarp_lib STATIC ${ASTRARP_SOURCES} ${ASTRARP_HEADERS})

target_include_directories(astrarp_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/pjh_json/include")

target_link_libraries(astrarp_lib PUBLIC llama pjh_json)

add_executable(astra src/main.cpp)
target_link_libraries(astra PRIVATE astrarp_lib)

add_custom_command(TARGET astra POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:llama>
    $<TARGET_FILE_DIR:astra>
)

# ================= 按需构建 Examples =================
if(ASTRARP_BUILD_EXAMPLES)
    include(CTest)
    enable_testing()

    # 查找 examples 目录下所有的 cpp 文件
    file(GLOB EXAMPLE_SOURCES "examples/*.cpp")

    foreach(SOURCE_FILE ${EXAMPLE_SOURCES})
        # 获取文件名（不带后缀）作为 Target 名称
        get_filename_component(EXAMPLE_NAME ${SOURCE_FILE} NAME_WE)

        # 定义 Target 名字
        set(TARGET_NAME "${EXAMPLE_NAME}")

        # 创建可执行程序
        # 使用 EXCLUDE_FROM_ALL 确保只有在手动构建它们或运行测试时才编译
        add_executable(${TARGET_NAME} ${SOURCE_FILE})

        # 链接到我们的核心库
        target_link_libraries(${TARGET_NAME} PRIVATE astrarp_lib)

        # 重点：仅在 Debug 模式下添加为测试用例
        # 注意：CMAKE_BUILD_TYPE 在多配置生成器（如 VS）中可能为空，
        # 这里使用逻辑：如果是 Debug 或者是多配置的 Debug 运行环境
        add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

        # 只在 Debug 模式下才编译这些 example
        # if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        # set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
        set_tests_properties(${TARGET_NAME} PROPERTIES ENVIRONMENT "${ENV_LIST}")

        # endif()
    endforeach()

endif()

if(ASTRARP_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
