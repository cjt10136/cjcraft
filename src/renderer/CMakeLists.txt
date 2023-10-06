add_subdirectory(context)
add_subdirectory(memory)
add_subdirectory(pipeline)
add_subdirectory(pool)
add_subdirectory(present)
add_subdirectory(resource)
add_subdirectory(sync)

add_library(core INTERFACE core.h)
target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(core INTERFACE context)

add_library(renderer renderer.h renderer.cpp)
target_link_libraries(renderer PUBLIC math context memory pipeline pool present resource sync control loader world)