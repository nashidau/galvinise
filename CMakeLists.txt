
cmake_minimum_required(VERSION 3.16)

# set the project name and version
project(Galvinise VERSION 0.2 LANGUAGES C)


# This standard is a decade old. Let us make it a requirement
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

include(FindPkgConfig)
pkg_check_modules(TALLOC REQUIRED talloc)
pkg_check_modules(CHECK check)

pkg_search_module(LUA lua5.4 lua5.3)

include_directories(${TALLOC_INCLUDEDIR} ${CHECK_INCLUDEDIR} ${LUA_INCLUDEDIR})

configure_file(config.h.in config.h)
add_subdirectory(src)

