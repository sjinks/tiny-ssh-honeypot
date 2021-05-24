cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ExternalProject)
include(CheckLibraryExists)
include(CheckCCompilerFlag)

project(tiny-ssh-honeypot VERSION 1.0.0 LANGUAGES C)

set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_LINK_WHAT_YOU_USE OFF)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_EXTENSIONS OFF)

configure_file(version.h.in version.h)

add_executable(
    tiny-ssh-honeypot
    cmdline.c
    globals.c
    log.c
    main.c
    server.c
    socketutils.c
    utils.c
)

option(BUILD_STATIC_BINARY     "Build static binary"       OFF)
option(FORCE_EXTERNAL_LIBEV    "Do not use system libev"   OFF)
option(FORCE_EXTERNAL_LIBASSH  "Do not use system libassh" OFF)

if(BUILD_STATIC_BINARY)
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

find_program(MAKE_EXE NAMES gmake nmake make)

if(NOT FORCE_EXTERNAL_LIBEV)
    find_package(Libev)
endif()

if(NOT FORCE_EXTERNAL_LIBASSH)
    find_package(Libassh)
endif()

if(NOT LIBASSH_FOUND)
    ExternalProject_Add(
        libassh
        URL http://download.savannah.nongnu.org/releases/libassh/libassh-1.0.tar.gz
        URL_HASH MD5=1705b4a78e249a55fcd762720aacbc5c
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --disable-client --disable-examples --without-gcrypt --without-openssl --without-sodium --without-zlib --disable-dependency-tracking --prefix=${CMAKE_CURRENT_BINARY_DIR}/libassh
        BUILD_COMMAND ${MAKE_EXE}
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
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --disable-dependency-tracking --prefix=${CMAKE_CURRENT_BINARY_DIR}/libev
        BUILD_COMMAND ${MAKE_EXE}
    )

    set(LIBEV_LIBRARIES    "${PROJECT_BINARY_DIR}/libev/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ev${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(LIBEV_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/libev/include")
    add_dependencies(tiny-ssh-honeypot libev)
endif()

check_c_compiler_flag(-Wall HAS_CF_WALL)
if(HAS_CF_WALL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

check_c_compiler_flag(-Wextra HAS_CF_WEXTRA)
if(HAS_CF_WEXTRA)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()

check_c_compiler_flag(-Wno-unused-parameter HAS_CF_WNOUNUSEDPARAMETER)
if(HAS_CF_WNOUNUSEDPARAMETER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
endif()

check_c_compiler_flag(-fno-strict-aliasing HAS_CF_FNOSTRICTALIASING)
if(HAS_CF_FNOSTRICTALIASING)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
endif()

target_include_directories(tiny-ssh-honeypot PUBLIC ${LIBASSH_INCLUDE_DIRS} ${LIBEV_INCLUDE_DIRS})
target_include_directories(tiny-ssh-honeypot PUBLIC "${PROJECT_BINARY_DIR}")
if(UNIX)
    target_compile_definitions(tiny-ssh-honeypot PUBLIC _GNU_SOURCE)
endif()
target_link_libraries(tiny-ssh-honeypot ${LIBASSH_LIBRARIES} ${LIBEV_LIBRARIES} "${MATH_LIBS}")
