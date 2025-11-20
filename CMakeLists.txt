cmake_minimum_required(VERSION 3.18)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ExternalProject)
include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckLinkerFlag)

project(tiny-ssh-honeypot VERSION 1.2.2 LANGUAGES C)

option(BUILD_STATIC_BINARY     "Build static binary"       OFF)
option(FORCE_EXTERNAL_LIBEV    "Do not use system libev"   OFF)
option(FORCE_EXTERNAL_LIBASSH  "Do not use system libassh" OFF)

add_executable(tiny-ssh-honeypot)
target_compile_features(tiny-ssh-honeypot PRIVATE c_std_11)
set_target_properties(tiny-ssh-honeypot PROPERTIES
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_LINK_WHAT_YOU_USE OFF)

function(add_compiler_flag_if_supported flag)
    check_c_compiler_flag(${flag} IS_SUPPORTED)
    target_compile_options(tiny-ssh-honeypot PRIVATE $<$<BOOL:${IS_SUPPORTED}>:${flag}>)
endfunction()

add_compiler_flag_if_supported(-Wall)
add_compiler_flag_if_supported(-Wextra)
add_compiler_flag_if_supported(-Wno-unused-parameter)
add_compiler_flag_if_supported(-fno-strict-aliasing)

configure_file(src/version.h.in version.h)

target_sources(
    tiny-ssh-honeypot PRIVATE
    src/cmdline.c
    src/globals.c
    src/log.c
    src/main.c
    src/server.c
    src/socketutils.c
    src/utils.c
)

if(BUILD_STATIC_BINARY)
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

find_program(CCACHE_EXE ccache)
if(CCACHE_EXE)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXE})
endif()

if(NOT FORCE_EXTERNAL_LIBEV)
    find_package(Libev)
endif()

if(NOT FORCE_EXTERNAL_LIBASSH)
    find_package(Libassh)
endif()

if(CMAKE_CROSSCOMPILING)
    if(DEFINED ENV{HOSTSPEC})
        set(CONFIGURE_HOST_FLAG "--host=$ENV{HOSTSPEC}")
    endif()
    set(HOST_CC "${CMAKE_C_COMPILER}")
    if(CMAKE_C_COMPILER_TARGET)
        set(HOST_CC "${HOST_CC} --target=${CMAKE_C_COMPILER_TARGET}")
    endif()
    set(HOST_CC_FLAG "CC=${HOST_CC}")
endif()

if(NOT LIBASSH_FOUND)
    ExternalProject_Add(
        libassh
        URL http://download.savannah.nongnu.org/releases/libassh/libassh-1.1.tar.gz
        URL_HASH MD5=94aae559e6b174b2b00e344186a69534
        DOWNLOAD_NO_PROGRESS ON
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        CONFIGURE_COMMAND <SOURCE_DIR>/configure ${CONFIGURE_HOST_FLAG} ${HOST_CC_FLAG}
            --disable-client --disable-examples --without-gcrypt --without-openssl --without-sodium --without-zlib --disable-dependency-tracking
            --prefix=${CMAKE_CURRENT_BINARY_DIR}/libassh
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
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        CONFIGURE_COMMAND <SOURCE_DIR>/configure ${CONFIGURE_HOST_FLAG} ${HOST_CC_FLAG}
            --disable-dependency-tracking --prefix=${CMAKE_CURRENT_BINARY_DIR}/libev
    )

    set(LIBEV_LIBRARIES    "${PROJECT_BINARY_DIR}/libev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ev${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(LIBEV_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/libev/include")
    add_dependencies(tiny-ssh-honeypot libev)
endif()

target_include_directories(tiny-ssh-honeypot PRIVATE ${LIBASSH_INCLUDE_DIRS} ${LIBEV_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})
if(UNIX)
    target_compile_definitions(tiny-ssh-honeypot PRIVATE _GNU_SOURCE)
endif()
target_compile_options(tiny-ssh-honeypot PRIVATE ${MY_CFLAGS} ${LTO_FLAGS})
target_link_libraries(tiny-ssh-honeypot PRIVATE ${LIBASSH_LIBRARIES} ${LIBEV_LIBRARIES} ${MATH_LIBS})
