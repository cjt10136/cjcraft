add_library(control "camera.h"
                    "camera.cpp"
                    "key.h" "key.cpp")

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Microsoft.GSL REQUIRED)

target_include_directories(control PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(control PUBLIC glfw
                                     glm::glm
                              PRIVATE Microsoft.GSL::GSL
)