#include "device.h"

#include "present/surface.h"

#include <algorithm>
#include <cstring>
#include <gsl/gsl>
#include <stdexcept>

namespace {
  bool has_surface_support(vk::PhysicalDevice gpu,
                           vk::SurfaceKHR surface) noexcept;
  bool has_device_extensions_support(vk::PhysicalDevice gpu,
                                     vk::SurfaceKHR surface) noexcept;
  bool has_device_features_support(vk::PhysicalDevice gpu) noexcept;
} // namespace

Device::Device(Instance const& instance)
{
  select_gpu_and_queue_families(instance);

  constexpr auto queue_priority{1.f};
  Vector<vk::DeviceQueueCreateInfo> queue_infos;
  for (auto&& i : families_.unique)
    queue_infos.push_back({.queueFamilyIndex{i},
                           .queueCount{1},
                           .pQueuePriorities{&queue_priority}});

  vk::DeviceCreateInfo info{
      .queueCreateInfoCount{gsl::narrow<unsigned>(std::size(queue_infos))},
      .pQueueCreateInfos{queue_infos.data()},
      .enabledExtensionCount{
          gsl::narrow<unsigned>(std::ssize(enabled_device_extensions))},
      .ppEnabledExtensionNames{enabled_device_extensions.data()},
      .pEnabledFeatures{&enabled_device_features}};
  if constexpr (Instance::validation_layers_enabled) {
    info.enabledLayerCount =
        gsl::narrow<unsigned>(std::size(Instance::enabled_validation_layers));
    info.ppEnabledLayerNames = Instance::enabled_validation_layers.data();
  }
  else
    info.enabledLayerCount = 0u;
  device_ = gpu_.createDevice(info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);
}

Device::Device(Device&& x) noexcept
    : gpu_{x.gpu_}, device_{x.device_}, families_{x.families_}
{
  x.device_ = nullptr;
}

Device& Device::operator=(Device&& x) noexcept(false)
{
  families_ = x.families_;

  device_.waitIdle();
  if (device_)
    device_.destroy();
  device_ = x.device_;
  x.device_ = nullptr;

  gpu_ = x.gpu_;

  return *this;
}

Device::~Device() noexcept(false)
{
  device_.waitIdle();
  if (device_)
    device_.destroy();
}

void Device::select_gpu_and_queue_families(Instance const& instance)
{
  Surface surface{instance.get_instance()};
  for (auto&& i : instance.get_gpus()) {
    if (!::has_surface_support(i, surface.get_surface())
        || !::has_device_extensions_support(i, surface.get_surface())
        || !::has_device_features_support(i))
      continue;

    auto count{0u};
    i.getQueueFamilyProperties(&count, nullptr);
    Vector<vk::QueueFamilyProperties> families(count);
    i.getQueueFamilyProperties(&count, families.data());
    Vector<unsigned> graphics_candidates;
    for (gsl::index j{0}; j < std::size(families); ++j)
      if (families[j].queueFlags & vk::QueueFlagBits::eGraphics)
        graphics_candidates.push_back(gsl::narrow<unsigned>(j));
    if (!std::size(graphics_candidates))
      continue;

    for (auto&& j : graphics_candidates)
      if (i.getSurfaceSupportKHR(j, surface.get_surface())) {
        gpu_ = i;
        families_ = {j, j, {j}};
        return;
      }
    for (gsl::index j{0}; j < std::size(families); ++j)
      if (i.getSurfaceSupportKHR(j, surface.get_surface())) {
        gpu_ = i;
        families_ = {graphics_candidates[0],
                     gsl::narrow<unsigned>(j),
                     {graphics_candidates[0], gsl::narrow<unsigned>(j)}};
        return;
      }
  }
  throw std::runtime_error{"failed to find a suitable GPU"};
}

namespace {
  bool has_surface_support(vk::PhysicalDevice gpu,
                           vk::SurfaceKHR surface) noexcept
  {
    auto count{0u};
    if (gpu.getSurfaceFormatsKHR(surface, &count, nullptr)
            != vk::Result::eSuccess
        || !count)
      return false;

    if (gpu.getSurfacePresentModesKHR(surface, &count, nullptr)
            != vk::Result::eSuccess
        || !count)
      return false;

    return true;
  }

  bool has_device_extensions_support(vk::PhysicalDevice gpu,
                                     vk::SurfaceKHR surface) noexcept
  {
    auto count{0u};
    if (gpu.enumerateDeviceExtensionProperties(nullptr, &count, nullptr)
        != vk::Result::eSuccess)
      return false;
    Vector<vk::ExtensionProperties> extensions(count);
    if (gpu.enumerateDeviceExtensionProperties(
            nullptr, &count, extensions.data())
        != vk::Result::eSuccess)
      return false;

    for (auto&& i : Device::enabled_device_extensions)
      if (std::ranges::none_of(
              extensions, [&](vk::ExtensionProperties x) noexcept {
                return std::strncmp(
                    x.extensionName,
                    i,
                    sizeof(vk::ExtensionProperties::extensionName));
              }))
        return false;

    return true;
  }

  bool has_device_features_support(vk::PhysicalDevice gpu) noexcept
  {
    auto const features{gpu.getFeatures()};
    return features.samplerAnisotropy;
  }
} // namespace
