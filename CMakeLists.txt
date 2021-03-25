
cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(Galvinise VERSION 0.2)


# This standard is a decade old. Let us make it a requirement
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra)

include(FindPkgConfig)
pkg_check_modules(TALLOC REQUIRED talloc)
pkg_check_modules(CHECK check)

pkg_check_modules(LUA lua5.4)
if(NOT LUA_FOUND)
  pkg_check_modules(LUA lua5.3)
endif()

include_directories(SYSTEM ${TALLOC_INCLUDEDIR} ${CHECK_INCLUDEDIR} ${LUA_INCLUDEDIR})

configure_file(config.h.in config.h)
add_subdirectory(src)

