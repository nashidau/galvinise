

add_library(galvinise galv.c diskslurp.c blam_buf.c blam_direct.c blam_writev.c modules/css/colours.c)
list(APPEND EXTRA_LIBS galvinise)



add_executable(galv main.c)
target_link_libraries(galv PUBLIC galvinise ${TALLOC_LDFLAGS} ${CHECK_LDFLAGS} ${LUA_LDFLAGS})


target_link_libraries(galvinise ${TALLOC_LIBRARIES} ${CHECK_LIBRARIES} ${LUA_LIBRARIES})
target_include_directories(galvinise PUBLIC ${TALLOC_INCLUDE_DIRS} ${CHECK_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS})
target_compile_options(galvinise PUBLIC ${TALLOC_CFLAGS_OTHERS} ${CHECK_CFLAGS_OTHERS} ${LUA_CFLAGS_OTHERS})
