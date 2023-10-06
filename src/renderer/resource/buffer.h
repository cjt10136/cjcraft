#pragma once

#include "core.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <gsl/gsl>

struct Buffer {
  vk::Buffer handle;
  long long offset;
  long long size;
  void* data;
};

struct Uniform {
  alignas(16) glm::mat4 view_proj;
};

struct PushConstant {
  glm::vec3 offset;
};

struct Vertex {
  unsigned data;

  static constexpr vk::VertexInputBindingDescription
  get_binding_description() noexcept
  {
    return {0u, sizeof(Vertex), vk::VertexInputRate::eVertex};
  }

  static constexpr std::array<vk::VertexInputAttributeDescription, 1>
  get_attribute_description() noexcept
  {
    return {
        {0u, 0u, vk::Format::eR32Uint, offsetof(Vertex, data)}
    };
  }
};

void copy_buffer(vk::CommandBuffer cb, Buffer src, Buffer dst) noexcept;
