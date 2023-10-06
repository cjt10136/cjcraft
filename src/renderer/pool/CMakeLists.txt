add_library(pool "command.h"
                 "descriptor.h"
                 "descriptor.cpp"
                 "command.cpp")

target_link_libraries(pool PUBLIC resource)