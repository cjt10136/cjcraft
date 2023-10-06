add_library(memory "buffer_manager.h"
                   "buffer_manager.cpp"
                   "image_manager.h"
                   "image_manager.cpp"
)

target_link_libraries(memory PUBLIC resource
                             PRIVATE math
)