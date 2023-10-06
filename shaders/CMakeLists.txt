set(source_shaders vert.vert frag.frag)

find_program(shader_compiler NAMES glslc)

foreach(s ${source_shaders})
  string(REGEX REPLACE "[.]vert|[.]frag" ".spv" out_name ${s})
  set(out_path ${PROJECT_BINARY_DIR}/shaders/${out_name})
  set(s_path ${CMAKE_CURRENT_SOURCE_DIR}/${s})
  add_custom_command(OUTPUT  ${out_path}
                     COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/shaders
                     COMMAND ${shader_compiler} ${s_path} -o ${out_path}
                     DEPENDS ${s})
  list(APPEND compiled_shaders ${out_path})
endforeach()

add_custom_target(shaders DEPENDS ${compiled_shaders})