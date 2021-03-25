

add_library(galvinise galv.c diskslurp.c blam_buf.c blam_direct.c blam_writev.c modules/css/colours.c)
list(APPEND EXTRA_LIBS galvinise)


add_executable(galv main.c)
target_link_libraries(galv PUBLIC galvinise ${TALLOC_LDFLAGS} ${CHECK_LDFLAGS} ${LUA_LDFLAGS})
