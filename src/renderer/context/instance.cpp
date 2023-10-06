#include "instance.h"

#include "GLFW/glfw3.h"
#include "present/swapchain.h"

#include <algorithm>
#include <cstring>
#include <gsl/gsl>
#include <iostream>
#include <stdexcept>
#include <tuple>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
  bool has_validation_layers_support();

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                 VkDebugUtilsMessageTypeFlagsEXT type,
                 VkDebugUtilsMessengerCallbackDataEXT const* data,
                 void* user_data);

  constexpr vk::DebugUtilsMessengerCreateInfoEXT messenger_info{
      .messageSeverity{vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                       | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError},
      .messageType{vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                   | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                   | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance},
      .pfnUserCallback{debug_callback}};
} // namespace

Instance::Instance()
{
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  if (!glfwInit())
    throw std::runtime_error{"failed to initialize glfw"};
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  vk::DynamicLoader dl;
  auto addr{
      dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(addr);

  if constexpr (validation_layers_enabled)
    if (!::has_validation_layers_support())
      throw std::runtime_error{
          "validation layers requested, but not available"};

  constexpr vk::ApplicationInfo app_info{
      .pApplicationName{""},
      .applicationVersion{VK_MAKE_API_VERSION(1, 0, 0, 0)},
      .pEngineName{""},
      .engineVersion{VK_MAKE_API_VERSION(1, 0, 0, 0)},
      .apiVersion{VK_API_VERSION_1_0}};

  Vector<char const*> extensions(
      std::begin(Swapchain::get_required_extensions()),
      std::end(Swapchain::get_required_extensions()));
  if constexpr (validation_layers_enabled)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#if defined(__APPLE__) && defined(__MACH__)
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

  vk::InstanceCreateInfo info{
      .pApplicationInfo{&app_info},
      .enabledExtensionCount{gsl::narrow<unsigned>(std::size(extensions))},
      .ppEnabledExtensionNames{extensions.data()}};
  if constexpr (validation_layers_enabled) {
    info.pNext = &::messenger_info;
    info.enabledLayerCount =
        gsl::narrow<unsigned>(std::size(enabled_validation_layers));
    info.ppEnabledLayerNames = enabled_validation_layers.data();
  }
#if defined(__APPLE__) && defined(__MACH__)
  info.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

  instance_ = vk::createInstance(info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);

  if constexpr (validation_layers_enabled)
    messenger_ = instance_.createDebugUtilsMessengerEXT(::messenger_info);

  auto count{0u};
  if (instance_.enumeratePhysicalDevices(&count, nullptr)
          != vk::Result::eSuccess
      || !count)
    throw std::runtime_error{"failed to find GPUs with Vulkan support"};
  gpus_.resize(count);
  if (instance_.enumeratePhysicalDevices(&count, gpus_.data())
      != vk::Result::eSuccess)
    throw std::runtime_error{"failed to enumerate physical devices"};
  std::ranges::partition(gpus_, [](vk::PhysicalDevice x) {
    return x.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
  });
}

Instance::Instance(Instance&& x) noexcept
    : instance_{x.instance_},
      messenger_{x.messenger_},
      gpus_{std::move(x.gpus_)}
{
  x.messenger_ = nullptr;
  x.instance_ = nullptr;
}

Instance& Instance::operator=(Instance&& x) noexcept
{
  gpus_ = std::move(x.gpus_);

  if constexpr (validation_layers_enabled)
    instance_.destroyDebugUtilsMessengerEXT(messenger_);
  messenger_ = x.messenger_;
  x.messenger_ = nullptr;

  if (instance_)
    instance_.destroy();
  instance_ = x.instance_;
  x.instance_ = nullptr;

  return *this;
}

Instance::~Instance()
{
  if constexpr (validation_layers_enabled)
    instance_.destroyDebugUtilsMessengerEXT(messenger_);
  if (instance_)
    instance_.destroy();
  glfwTerminate();
}

namespace {
  bool has_validation_layers_support()
  {
    auto count{0u};
    if (vk::enumerateInstanceLayerProperties(&count, nullptr)
        != vk::Result::eSuccess)
      throw std::runtime_error{"failed to enumerate instance layer properties"};
    Vector<vk::LayerProperties> p(count);
    if (vk::enumerateInstanceLayerProperties(&count, p.data())
        != vk::Result::eSuccess)
      throw std::runtime_error{"failed to enumerate instance layer properties"};

    for (auto&& i : Instance::enabled_validation_layers)
      if (std::ranges::none_of(p, [&](vk::LayerProperties x) noexcept {
            return std::strcmp(x.layerName, i) == 0;
          }))
        return false;
    return true;
  }

  VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                 VkDebugUtilsMessageTypeFlagsEXT type,
                 VkDebugUtilsMessengerCallbackDataEXT const* data,
                 void* user_data)
  {
    switch (severity) {
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      std::cerr << "verbose: ";
      break;
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      std::cerr << "info: ";
      break;
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::cerr << "warning: ";
      break;
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::cerr << "error: ";
      break;
    default:
      break;
    }
    std::cerr << data->pMessage << '\n';
    return VK_FALSE;
  }
} // namespace