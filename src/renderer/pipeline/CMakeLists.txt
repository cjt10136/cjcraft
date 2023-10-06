add_library(pipeline "framebuffer.h"
                     "framebuffer.cpp"
                     "pipeline.h"
                     "pipeline.cpp"
                     "render_pass.h"
                     "render_pass.cpp"
                     "shader.h"
                     "shader.cpp"
)

target_link_libraries(pipeline PUBLIC core
                                      loader
)