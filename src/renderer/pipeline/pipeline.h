#pragma once

#include "core.h"

#include "loader/code.h"
#include "shader.h"

#include <vector>

class Pipeline {
public:
  Pipeline(vk::RenderPass render_pass,
           vk::DescriptorSetLayout layout,
           Code const& vert_code,
           Code const& frag_code);
  Pipeline(Pipeline const&) = delete;
  Pipeline(Pipeline&&) noexcept;
  Pipeline& operator=(Pipeline const&) = delete;
  Pipeline& operator=(Pipeline&&) noexcept;
  ~Pipeline();

  vk::PipelineLayout get_layout() const noexcept { return layout_; }

  vk::Pipeline get_pipeline() const noexcept { return pipeline_; }
private:
  vk::PipelineLayout layout_;
  vk::Pipeline pipeline_;
};