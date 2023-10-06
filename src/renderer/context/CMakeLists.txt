find_package(glfw3 REQUIRED)
find_package(Microsoft.GSL REQUIRED)
find_package(Vulkan REQUIRED)

add_library(context "context.h"
                    "context.cpp"
                    "device.h"
                    "device.cpp"
                    "instance.h"
                    "instance.cpp"
)

target_link_libraries(context PUBLIC ${Vulkan_LIBRARIES}
                                     Microsoft.GSL::GSL
                                     container
                              PRIVATE glfw
                                      present
)
