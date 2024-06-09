set(SOURCES global_object.cpp)
add_library(global_object STATIC ${SOURCES})


target_link_libraries(parser execution pthread)

target_link_libraries(global_object parser execution readline pthread planner analyze)