add_library(sync "fence.h"
                 "fence.cpp"
                 "semaphore.cpp"
                 "semaphore.cpp"
)

target_link_libraries(sync PUBLIC core)