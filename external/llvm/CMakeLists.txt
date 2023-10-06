add_library(small_vector INTERFACE "SmallVector.h")

target_include_directories(small_vector INTERFACE ${CMAKE_SOURCE_DIR}/external)
