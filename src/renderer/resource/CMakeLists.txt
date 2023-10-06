add_library(resource "buffer.h"
                     "buffer.cpp"
                     "image.h"
                     "image.cpp"
                     "sampler.h"
                     "sampler.cpp"
)

find_package(glm REQUIRED)

target_link_libraries(resource PUBLIC core
                                      glm::glm
)