#pragma once

#include "device.h"

class Context {
public:
  Context() = default;
  Context(Context const&) = delete;
  Context(Context&& x) = default;
  Context& operator=(Context const&) = default;
  Context& operator=(Context&&) = default;
  ~Context() = default;

  vk::Instance get_instance() const noexcept
  {
    return instance_.get_instance();
  }

  vk::Device get_device() const noexcept { return device_.get_device(); }

  vk::PhysicalDevice get_gpu() const noexcept { return device_.get_gpu(); }

  template<QueueType Q>
  unsigned get_queue_family() const noexcept
  {
    return device_.get_queue_family<Q>();
  }

  template<QueueType Q>
  vk::Queue get_queue() const noexcept
  {
    if constexpr (Q == QueueType::graphics)
      return graphics_queue_;
    else if constexpr (Q == QueueType::present)
      return present_queue_;
    else
      static_assert(std::is_same_v<Q, void*>);
  }

  vk::PhysicalDeviceProperties const& get_gpu_properties() const noexcept
  {
    return properties_;
  }

  vk::PhysicalDeviceMemoryProperties const&
  get_gpu_memory_properties() const noexcept
  {
    return memory_properties;
  }
private:
  Instance instance_{};
  Device device_{instance_};
  vk::Queue graphics_queue_{device_.get_device().getQueue(
      device_.get_queue_family<QueueType::graphics>(),
      0)};
  vk::Queue present_queue_{device_.get_device().getQueue(
      device_.get_queue_family<QueueType::present>(),
      0)};
  vk::PhysicalDeviceProperties properties_{device_.get_gpu().getProperties()};
  vk::PhysicalDeviceMemoryProperties memory_properties{
      device_.get_gpu().getMemoryProperties()};
};

extern Context const g_context;

constexpr auto max_in_flight{2};
