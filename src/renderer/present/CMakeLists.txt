find_package(glfw3 REQUIRED)

add_library(present "surface.h"
                    "surface.cpp"
                    "swapchain.h"
                    "swapchain.cpp"
)

target_link_libraries(present PUBLIC container
                                     glfw
                                     memory
)