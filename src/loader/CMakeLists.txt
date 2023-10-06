add_library(loader "code.h"
                   "code.cpp"
                   "texture.h"
                   "texture.cpp"
)

find_package(Stb REQUIRED)

target_include_directories(loader PUBLIC ${CMAKE_SOURCE_DIR}/src
                                  PRIVATE ${Stb_INCLUDE_DIR}
)