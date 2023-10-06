#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_SMART_HANDLE

#include "vulkan/vulkan.hpp"
#include "container/vector.h"

#include <array>
#include <span>
#include <vector>

class Instance {
public:
#ifndef NDEBUG
  static constexpr auto validation_layers_enabled{true};
  static constexpr std::array enabled_validation_layers{
      "VK_LAYER_KHRONOS_validation"};
#else
  static constexpr auto validation_layers_enabled{false};
  static constexpr std::array<char const*, 0> enabled_validation_layers{};
#endif

  Instance();
  Instance(Instance const&) = delete;
  Instance(Instance&&) noexcept;
  Instance& operator=(Instance const&) = delete;
  Instance& operator=(Instance&&) noexcept;
  ~Instance();

  vk::Instance get_instance() const noexcept { return instance_; }

  std::span<vk::PhysicalDevice const> get_gpus() const { return gpus_; }
private:
  vk::Instance instance_;
  vk::DebugUtilsMessengerEXT messenger_;
  Vector<vk::PhysicalDevice> gpus_;
};