#pragma once

#include "instance.h"
#include "vulkan/vulkan_beta.h"
#include "container/vector.h"

#include <array>
#include <span>

enum class QueueType { graphics, present };

struct QueueFamilies {
  unsigned graphics{};
  unsigned present{};
  Vector<unsigned> unique{};
};

class Device {
public:
#if defined(__APPLE__) && defined(__MACH__)
  static constexpr std::array enabled_device_extensions{
      "VK_KHR_portability_subset",
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#else
  static constexpr std::array enabled_device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#endif

  static constexpr vk::PhysicalDeviceFeatures enabled_device_features{
      .samplerAnisotropy{VK_TRUE}};

  explicit Device(Instance const& instnace);
  Device(Device const&) = delete;
  Device(Device&&) noexcept;
  Device& operator=(Device const&) = delete;
  Device& operator=(Device&&) noexcept(false);
  ~Device() noexcept(false);

  vk::Device get_device() const noexcept { return device_; }

  vk::PhysicalDevice get_gpu() const noexcept { return gpu_; }

  template<QueueType Q>
  unsigned get_queue_family() const noexcept
  {
    if constexpr (Q == QueueType::graphics)
      return families_.graphics;
    else if constexpr (Q == QueueType::present)
      return families_.present;
    else
      static_assert(std::is_same_v<Q, void*>);
  }
private:
  void select_gpu_and_queue_families(Instance const& instance);

  vk::PhysicalDevice gpu_;
  vk::Device device_;
  QueueFamilies families_;
};