# - Try to find libassh
# Once done this will define
#  LIBASSH_FOUND        - System has libassh
#  LIBASSH_INCLUDE_DIRS - The libassh include directories
#  LIBASSH_LIBRARIES    - The libraries needed to use libassh

find_path(
    LIBASSH_INCLUDE_DIR
    NAMES assh/assh.h
)

find_library(
    LIBASSH_LIBRARY
    NAMES assh
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Libassh
    REQUIRED_VARS LIBASSH_LIBRARY LIBASSH_INCLUDE_DIR
)

if(LIBASSH_FOUND)
    set(LIBASSH_LIBRARIES     ${LIBASSH_LIBRARY})
    set(LIBASSH_INCLUDE_DIRS  ${LIBASSH_INCLUDE_DIR})
endif()

mark_as_advanced(LIBASSH_INCLUDE_DIR LIBASSH_LIBRARY)
