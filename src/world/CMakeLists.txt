add_library(world "block.h"
                  "chunk.h"
                  "chunk.cpp"
                  "world.h"
                  "world.cpp"
                  "ray.h"
                  "ray.cpp"
                  "terrain.cpp"
)

target_include_directories(world PUBLIC ${CMAKE_SOURCE_DIR}/src)
                                 
target_link_libraries(world PUBLIC memory
                                   pool
                                   container
                                   control
                            PRIVATE FastNoiseLite
                                    math
)