#include "surface.h"

#include <stdexcept>

Surface::Surface(vk::Instance instance) : instance_{instance}
{
  glfwWindowHint(GLFW_VISIBLE, false);
  window_ =
      glfwCreateWindow(default_width, default_height, "", nullptr, nullptr);
  if (!window_)
    throw std::runtime_error{"failed to create window"};

  VkSurfaceKHR s;
  if (glfwCreateWindowSurface(instance_, window_, nullptr, &s) != VK_SUCCESS)
    throw std::runtime_error{"failed to create surface"};
  surface_ = s;
}

Surface::Surface(Surface&& x) noexcept
    : instance_{x.instance_}, window_{x.window_}, surface_{x.surface_}
{
  x.surface_ = nullptr;
  x.window_ = nullptr;
}

Surface& Surface::operator=(Surface&& x) noexcept
{
  if (surface_)
    instance_.destroySurfaceKHR(surface_);
  surface_ = x.surface_;
  x.surface_ = nullptr;

  if (window_)
    glfwDestroyWindow(window_);
  window_ = x.window_;
  x.window_ = nullptr;

  instance_ = x.instance_;

  return *this;
}

Surface::~Surface()
{
  if (surface_)
    instance_.destroySurfaceKHR(surface_);
  if (window_)
    glfwDestroyWindow(window_);
}