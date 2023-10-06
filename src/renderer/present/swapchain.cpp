#include "swapchain.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace {
  bool is_format_supported(vk::SurfaceKHR surface,
                           vk::SurfaceFormatKHR format) noexcept;

  vk::Extent2D select_extent(vk::SurfaceCapabilitiesKHR const& cap,
                             GLFWwindow* window);

  vk::PresentModeKHR select_present_mode(vk::SurfaceKHR surface);
} // namespace

std::span<char const*> Swapchain::get_required_extensions()
{
  static auto extensions{[] {
    auto count{0u};
    auto extensions{glfwGetRequiredInstanceExtensions(&count)};
    return Vector<char const*>{extensions, extensions + count};
  }()};
  return extensions;
}

Swapchain::Swapchain(vk::Extent2D expected_extent)
{
  glfwWindowHint(GLFW_VISIBLE, true);
  glfwWindowHint(GLFW_DECORATED, false);
  window_ = glfwCreateWindow(gsl::narrow<int>(expected_extent.width),
                             gsl::narrow<int>(expected_extent.height),
                             "",
                             glfwGetPrimaryMonitor(),
                             //nullptr,
                             nullptr);

  if (!window_)
    throw std::runtime_error{"failed to create window"};

  VkSurfaceKHR s;
  if (glfwCreateWindowSurface(g_context.get_instance(), window_, nullptr, &s)
      != VK_SUCCESS)
    throw std::runtime_error{"failed to create surface"};
  surface_ = s;
  if (!::is_format_supported(surface_, format))
    throw std::runtime_error{"GPU not support RGB format"};

  auto const cap{g_context.get_gpu().getSurfaceCapabilitiesKHR(surface_)};
  extent_ = ::select_extent(cap, window_);
  auto count{cap.minImageCount + 1};
  if (cap.maxImageCount)
    count = std::min(count, cap.maxImageCount);

  vk::SwapchainCreateInfoKHR info{
      .surface{surface_},
      .minImageCount{count},
      .imageFormat{format.format},
      .imageColorSpace{format.colorSpace},
      .imageExtent{extent_},
      .imageArrayLayers{1},
      .imageUsage{vk::ImageUsageFlagBits::eColorAttachment},
      .imageSharingMode{vk::SharingMode::eExclusive},
      .preTransform{cap.currentTransform},
      .compositeAlpha{vk::CompositeAlphaFlagBitsKHR::eOpaque},
      .presentMode{select_present_mode(surface_)},
      .clipped{VK_TRUE}};
  if (std::array const indexes{
          g_context.get_queue_family<QueueType::graphics>(),
          g_context.get_queue_family<QueueType::present>()};
      indexes[0] != indexes[1]) {
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = indexes.data();
  }
  swapchain_ = g_context.get_device().createSwapchainKHR(info);

  if (g_context.get_device().getSwapchainImagesKHR(swapchain_, &count, nullptr)
      != vk::Result::eSuccess)
    throw std::runtime_error{"failed to get swapchain images"};
  images_.resize(count);
  if (g_context.get_device().getSwapchainImagesKHR(
          swapchain_, &count, images_.data())
      != vk::Result::eSuccess)
    throw std::runtime_error{"failed to get swapchain images"};

  image_views_.reserve(count);
  for (auto&& i : images_)
    image_views_.emplace_back(
        i, format.format, vk::ImageAspectFlagBits::eColor);

  depth_ =
      image_manager_.create(vk::Format::eD32Sfloat,
                            extent_,
                            vk::ImageUsageFlagBits::eDepthStencilAttachment);
  depth_view_ = {
      depth_, vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth};
}

Swapchain::Swapchain(Swapchain&& x) noexcept
    : window_{x.window_},
      surface_{x.surface_},
      extent_{x.extent_},
      swapchain_{x.swapchain_},
      images_{std::move(x.images_)},
      image_views_{std::move(x.image_views_)},
      image_manager_{std::move(x.image_manager_)},
      depth_{x.depth_},
      depth_view_{std::move(x.depth_view_)}
{
  x.swapchain_ = nullptr;
  x.surface_ = nullptr;
  x.window_ = nullptr;
}

Swapchain& Swapchain::operator=(Swapchain&& x) noexcept
{
  depth_view_ = std::move(x.depth_view_);
  depth_ = x.depth_;
  image_manager_ = std::move(x.image_manager_);

  image_views_ = std::move(x.image_views_);
  images_ = std::move(x.images_);

  if (swapchain_)
    g_context.get_device().destroySwapchainKHR(swapchain_);
  swapchain_ = x.swapchain_;
  x.swapchain_ = nullptr;

  extent_ = x.extent_;

  if (surface_)
    g_context.get_instance().destroySurfaceKHR(surface_);
  surface_ = x.surface_;
  x.surface_ = nullptr;

  if (window_)
    glfwDestroyWindow(window_);
  window_ = x.window_;
  x.window_ = nullptr;

  return *this;
}

Swapchain::~Swapchain()
{
  if (swapchain_)
    g_context.get_device().destroySwapchainKHR(swapchain_);
  swapchain_ = nullptr;
  if (surface_)
    g_context.get_instance().destroySurfaceKHR(surface_);
  surface_ = nullptr;
  if (window_)
    glfwDestroyWindow(window_);
  window_ = nullptr;
}

bool Swapchain::acquire_next_image(unsigned& next_index,
                                   vk::Semaphore to_signal)
{
  if (auto const result{g_context.get_device().acquireNextImageKHR(
          swapchain_,
          std::numeric_limits<unsigned long long>::max(),
          to_signal,
          nullptr,
          &next_index)};
      result == vk::Result::eErrorOutOfDateKHR) {
    recreate();
    return false;
  }
  else if (result != vk::Result::eSuccess
           && result != vk::Result::eSuboptimalKHR)
    throw std::runtime_error{"failed to acquire swap chain image"};
  return true;
}

bool Swapchain::present(unsigned index, std::span<vk::Semaphore> to_wait)
{
  vk::PresentInfoKHR const info{
      .waitSemaphoreCount{gsl::narrow<unsigned>(std::size(to_wait))},
      .pWaitSemaphores{to_wait.data()},
      .swapchainCount{1},
      .pSwapchains{&swapchain_},
      .pImageIndices{&index}};
  if (auto const result{
          g_context.get_queue<QueueType::present>().presentKHR(&info)};
      result == vk::Result::eErrorOutOfDateKHR
      || result == vk::Result::eSuboptimalKHR) {
    recreate();
    return false;
  }
  else if (result != vk::Result::eSuccess)
    throw std::runtime_error{"failed to present swapchain image"};
  return true;
}

void Swapchain::recreate()
{
  auto width{0};
  auto height{0};
  glfwGetFramebufferSize(window_, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window_, &width, &height);
    glfwWaitEvents();
  }
  g_context.get_device().waitIdle();
  this->~Swapchain();
  *this =
      Swapchain{{gsl::narrow<unsigned>(width), gsl::narrow<unsigned>(height)}};
}

namespace {
  bool is_format_supported(vk::SurfaceKHR surface,
                           vk::SurfaceFormatKHR format) noexcept
  {
    auto count{0u};
    if (g_context.get_gpu().getSurfaceFormatsKHR(surface, &count, nullptr)
        != vk::Result::eSuccess)
      return false;
    Vector<vk::SurfaceFormatKHR> formats(count);

    if (g_context.get_gpu().getSurfaceFormatsKHR(
            surface, &count, formats.data())
            != vk::Result::eSuccess
        || std::ranges::find(formats, format) == std::end(formats))
      return false;
    return true;
  }

  vk::Extent2D select_extent(vk::SurfaceCapabilitiesKHR const& cap,
                             GLFWwindow* window)
  {
    if (cap.currentExtent.width != std::numeric_limits<unsigned>::max())
      return cap.currentExtent;

    auto width{0};
    auto height{0};
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }
    return {std::clamp(gsl::narrow<unsigned>(width),
                       cap.minImageExtent.width,
                       cap.maxImageExtent.width),
            std::clamp(gsl::narrow<unsigned>(height),
                       cap.minImageExtent.height,
                       cap.maxImageExtent.height)};
  }

  vk::PresentModeKHR select_present_mode(vk::SurfaceKHR surface)
  {
    //return vk::PresentModeKHR::eImmediate;
    auto count{0u};
    if (g_context.get_gpu().getSurfacePresentModesKHR(surface, &count, nullptr)
        != vk::Result::eSuccess)
      throw std::runtime_error{"failed to get surface present modes"};
    Vector<vk::PresentModeKHR> modes(count);
    if (g_context.get_gpu().getSurfacePresentModesKHR(
            surface, &count, modes.data())
        != vk::Result::eSuccess)
      throw std::runtime_error{"failed to get surface present modes"};

    if (std::ranges::find(modes, vk::PresentModeKHR::eMailbox)
        != std::end(modes))
      return vk::PresentModeKHR::eMailbox;
    return vk::PresentModeKHR::eFifo;
  }
} // namespace
