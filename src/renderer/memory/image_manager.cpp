#include "image_manager.h"

#include "math/math.h"

#include <gsl/gsl>

ImageManager::ImageManager(ImageManager&& x) noexcept
    : memory_infos_{std::move(x.memory_infos_)},
      images_{std::move(x.images_)},
      memories_{std::move(x.memories_)}
{
  x.images_.clear();
  x.memories_.clear();
}

ImageManager& ImageManager::operator=(ImageManager&& x) noexcept
{
  for (auto&& i : images_)
    g_context.get_device().destroyImage(i);
  images_ = std::move(x.images_);
  x.images_.clear();

  for (auto&& i : memories_)
    g_context.get_device().freeMemory(i);
  memories_ = std::move(x.memories_);
  x.memories_.clear();

  memory_infos_ = std::move(x.memory_infos_);

  return *this;
}

ImageManager::~ImageManager()
{
  for (auto&& i : images_)
    g_context.get_device().destroyImage(i);
  for (auto&& i : memories_)
    g_context.get_device().freeMemory(i);
}

vk::Image ImageManager::create(vk::Format format,
                               vk::Extent2D extent,
                               vk::ImageUsageFlags usage,
                               vk::MemoryPropertyFlags property)
{
  images_.push_back(g_context.get_device().createImage({
      .imageType{vk::ImageType::e2D},
      .format{format},
      .extent{extent.width, extent.height, 1},
      .mipLevels{1},
      .arrayLayers{1},
      .samples{vk::SampleCountFlagBits::e1},
      .tiling{vk::ImageTiling::eOptimal},
      .usage{usage},
      .sharingMode{vk::SharingMode::eExclusive},
      .initialLayout{vk::ImageLayout::eUndefined}
  }));
  auto const requirement{
      g_context.get_device().getImageMemoryRequirements(images_.back())};
  for (auto i{0u}; i < g_context.get_gpu_memory_properties().memoryTypeCount;
       ++i)
    if (requirement.memoryTypeBits & 1 << i
        && (g_context.get_gpu_memory_properties().memoryTypes[i].propertyFlags
            & property)
               == property) {
      for (gsl::index j{0}; j < std::size(memory_infos_); ++j) {
        auto&& [t, s]{memory_infos_[j]};
        if (auto const r{round_to(s, requirement.alignment)};
            i == t && r + requirement.size <= block_size) {
          s = r + requirement.size;
          g_context.get_device().bindImageMemory(
              images_.back(), memories_[j], r);
          return images_.back();
        }
      }
      memories_.push_back(g_context.get_device().allocateMemory(
          {.allocationSize{block_size}, .memoryTypeIndex{i}}));
      g_context.get_device().bindImageMemory(
          images_.back(), memories_.back(), 0);
      memory_infos_.push_back({i, gsl::narrow<long long>(requirement.size)});
      return images_.back();
    }
  throw std::runtime_error{"failed to find a suitable memory type"};
}