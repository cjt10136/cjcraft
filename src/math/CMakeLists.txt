add_library(math INTERFACE "math.h"
                           
                           
)

find_package(GLM REQUIRED)

target_include_directories(math INTERFACE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(math INTERFACE glm::glm)
