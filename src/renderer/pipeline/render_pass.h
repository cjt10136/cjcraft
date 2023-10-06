#pragma once

#include "core.h"

class RenderPass {
public:
  RenderPass();
  RenderPass(RenderPass const&) = delete;
  RenderPass(RenderPass&&) noexcept;
  RenderPass& operator=(RenderPass const&) = delete;
  RenderPass& operator=(RenderPass&&) noexcept;
  ~RenderPass();

  vk::RenderPass get() const noexcept { return handle_; }
private:
  vk::RenderPass handle_;
};