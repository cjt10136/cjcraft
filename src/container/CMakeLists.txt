add_library(container INTERFACE "vector.h" "hash_map.h")

target_include_directories(container INTERFACE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(container INTERFACE small_vector unordered_dense)
