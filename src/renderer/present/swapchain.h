#pragma once

#include "container/vector.h"
#include "core.h"
#include "GLFW/glfw3.h"
#include "memory/image_manager.h"

#include <array>
#include <expected>
#include <gsl/gsl>
#include <span>

class Swapchain {
public:
  static constexpr vk::SurfaceFormatKHR format{vk::Format::eB8G8R8A8Srgb};

  static std::span<char const*> get_required_extensions();

  Swapchain(vk::Extent2D expected_extent);
  Swapchain(Swapchain const&) = delete;
  Swapchain(Swapchain&&) noexcept;
  Swapchain& operator=(Swapchain const&) = delete;
  Swapchain& operator=(Swapchain&&) noexcept;
  ~Swapchain();

  GLFWwindow* get_window() const noexcept { return window_; }

  vk::Extent2D get_extent() const noexcept { return extent_; }

  gsl::index size() const noexcept { return std::size(images_); }

  float get_aspect() const noexcept
  {
    return gsl::narrow_cast<float>(extent_.width) / extent_.height;
  }

  vk::ImageView get_image_view(gsl::index i) const noexcept
  {
    return image_views_[i].get();
  }

  vk::ImageView get_depth_view() const noexcept
  {
    return depth_view_.get();
  }

  bool should_close() const noexcept { return glfwWindowShouldClose(window_); }

  bool acquire_next_image(unsigned& next_index, vk::Semaphore to_signal);

  bool present(unsigned index, std::span<vk::Semaphore> to_wait);
private:
  void recreate();

  gsl::owner<GLFWwindow*> window_;
  vk::SurfaceKHR surface_;
  vk::Extent2D extent_;
  vk::SwapchainKHR swapchain_;
  Vector<vk::Image> images_;
  Vector<ImageView> image_views_;
  ImageManager image_manager_;
  vk::Image depth_;
  ImageView depth_view_;
};