#pragma once

#include "core.h"

#include <gsl/gsl>
#include <span>

class Framebuffer {
public:
  Framebuffer(vk::RenderPass render_pass,
              std::span<vk::ImageView const> attachments,
              vk::Extent2D extent);
  Framebuffer(Framebuffer const&) = delete;
  Framebuffer(Framebuffer&&) noexcept;
  Framebuffer& operator=(Framebuffer const&) = delete;
  Framebuffer& operator=(Framebuffer&&) noexcept;
  ~Framebuffer();

  vk::Framebuffer get() const noexcept { return handle_; }
private:
  vk::Framebuffer handle_;
};