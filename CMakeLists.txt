cmake_minimum_required(VERSION 3.18)

cmake_policy(SET CMP0135 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ExternalProject)
include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckLinkerFlag)

project(tiny-ssh-honeypot VERSION 1.1.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_LINK_WHAT_YOU_USE OFF)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_EXTENSIONS OFF)

set(MY_CFLAGS "")
check_c_compiler_flag(-Wall HAS_CF_WALL)
if(HAS_CF_WALL)
    list(APPEND MY_CFLAGS -Wall)
endif()

check_c_compiler_flag(-Wextra HAS_CF_WEXTRA)
if(HAS_CF_WEXTRA)
    list(APPEND MY_CFLAGS -Wextra)
endif()

check_c_compiler_flag(-Wno-unused-parameter HAS_CF_WNOUNUSEDPARAMETER)
if(HAS_CF_WNOUNUSEDPARAMETER)
    list(APPEND MY_CFLAGS -Wno-unused-parameter)
endif()

check_c_compiler_flag(-fno-strict-aliasing HAS_CF_FNOSTRICTALIASING)
if(HAS_CF_FNOSTRICTALIASING)
    list(APPEND MY_CFLAGS -fno-strict-aliasing)
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

set(LTO_FLAGS "")
check_c_compiler_flag(-flto HAS_CF_FLTO)
if(HAS_CF_FLTO)
    list(APPEND LTO_FLAGS -flto)

    check_c_compiler_flag(-ffat-lto-objects HAS_CF_FFATLTOOBJECTS)
    if(HAS_CF_FFATLTOOBJECTS)
        list(APPEND LTO_FLAGS -ffat-lto-objects)
    endif()

    check_linker_flag(C -fuse-linker-plugin HAS_CF_FUSELINKERPLUGIN)
    if(HAS_CF_FUSELINKERPLUGIN)
        list(APPEND LTO_LDFLAGS -fuse-linker-plugin)
    endif()
endif()

list(APPEND EXT_CFLAGS ${CMAKE_C_FLAGS_${BUILD_TYPE}} ${LTO_FLAGS})
list(JOIN EXT_CFLAGS " " EXT_CFLAGS)
if(HAS_CF_FLTO)
    list(APPEND EXT_LDFLAGS ${CMAKE_C_FLAGS_${BUILD_TYPE}} ${LTO_FLAGS})
    list(JOIN EXT_LDFLAGS " " EXT_LDFLAGS)
    list(APPEND LTO_LDFLAGS ${CMAKE_C_FLAGS_${BUILD_TYPE}} ${LTO_FLAGS})
    separate_arguments(LTO_LDFLAGS)
endif()

configure_file(src/version.h.in version.h)

add_executable(
    tiny-ssh-honeypot
    src/cmdline.c
    src/globals.c
    src/log.c
    src/main.c
    src/server.c
    src/socketutils.c
    src/utils.c
)

option(BUILD_STATIC_BINARY     "Build static binary"       OFF)
option(FORCE_EXTERNAL_LIBEV    "Do not use system libev"   OFF)
option(FORCE_EXTERNAL_LIBASSH  "Do not use system libassh" OFF)

if(BUILD_STATIC_BINARY)
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

find_program(CCACHE_EXE ccache)
if(CCACHE_EXE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_EXE})
    set(C_COMPILER "${CCACHE_EXE} ${CMAKE_C_COMPILER}")
else()
    set(C_COMPILER ${CMAKE_C_COMPILER})
endif()

if(NOT FORCE_EXTERNAL_LIBEV)
    find_package(Libev)
endif()

if(NOT FORCE_EXTERNAL_LIBASSH)
    find_package(Libassh)
endif()

if(DEFINED ENV{HOSTSPEC})
    set(CONFIGURE_HOST_FLAG "--host=$ENV{HOSTSPEC}")
endif()

if(NOT LIBASSH_FOUND)
    ExternalProject_Add(
        libassh
        URL http://download.savannah.nongnu.org/releases/libassh/libassh-1.1.tar.gz
        URL_HASH MD5=94aae559e6b174b2b00e344186a69534
        DOWNLOAD_NO_PROGRESS ON
        CONFIGURE_COMMAND <SOURCE_DIR>/configure ${CONFIGURE_HOST_FLAG} --quiet --enable-silent-rules --disable-client --disable-examples --without-gcrypt --without-openssl --without-sodium --without-zlib --disable-dependency-tracking --prefix=${CMAKE_CURRENT_BINARY_DIR}/libassh CC=${C_COMPILER} CFLAGS=${EXT_CFLAGS} LDFLAGS=${EXT_LDFLAGS} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB}
    )

    set(LIBASSH_LIBRARIES    "${PROJECT_BINARY_DIR}/libassh/lib/${CMAKE_STATIC_LIBRARY_PREFIX}assh${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(LIBASSH_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/libassh/include")
    add_dependencies(tiny-ssh-honeypot libassh)
endif()

set(MATH_LIBS "")
check_library_exists(m floor "" LIBM)
if(LIBM)
    list(APPEND MATH_LIBS "m")
endif()

if(NOT LIBEV_FOUND)
    ExternalProject_Add(
        libev
        URL http://dist.schmorp.de/libev/Attic/libev-4.33.tar.gz
        URL_HASH MD5=a3433f23583167081bf4acdd5b01b34f
        DOWNLOAD_NO_PROGRESS ON
        CONFIGURE_COMMAND <SOURCE_DIR>/configure ${CONFIGURE_HOST_FLAG} --quiet --enable-silent-rules --disable-dependency-tracking --prefix=${CMAKE_CURRENT_BINARY_DIR}/libev CC=${C_COMPILER} CFLAGS=${EXT_CFLAGS} LDFLAGS=${EXT_LDFLAGS} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB}
    )

    set(LIBEV_LIBRARIES    "${PROJECT_BINARY_DIR}/libev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ev${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(LIBEV_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/libev/include")
    add_dependencies(tiny-ssh-honeypot libev)
endif()

target_include_directories(tiny-ssh-honeypot PUBLIC ${LIBASSH_INCLUDE_DIRS} ${LIBEV_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})
if(UNIX)
    target_compile_definitions(tiny-ssh-honeypot PUBLIC _GNU_SOURCE)
endif()
target_compile_options(tiny-ssh-honeypot PUBLIC ${MY_CFLAGS} ${LTO_FLAGS})
target_link_options(tiny-ssh-honeypot PUBLIC ${LTO_LDFLAGS})
target_link_libraries(tiny-ssh-honeypot ${LIBASSH_LIBRARIES} ${LIBEV_LIBRARIES} ${MATH_LIBS})
