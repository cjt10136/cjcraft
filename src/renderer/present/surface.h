#pragma once

#include "core.h"
#include "GLFW/glfw3.h"

#include <gsl/gsl>

class Surface {
public:
  static constexpr auto default_width{1920};
  static constexpr auto default_height{1080};

  explicit Surface(vk::Instance instance);
  Surface(Surface const&) = delete;
  Surface(Surface&&) noexcept;
  Surface& operator=(Surface const&) = delete;
  Surface& operator=(Surface&&) noexcept;
  ~Surface();

  GLFWwindow* get_window() const noexcept { return window_; }

  vk::SurfaceKHR get_surface() const noexcept { return surface_; }
private:
  vk::Instance instance_;
  gsl::owner<GLFWwindow*> window_;
  vk::SurfaceKHR surface_;
};
