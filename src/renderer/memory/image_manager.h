#pragma once

#include "resource/image.h"

#include <vector>

class ImageManager {
public:
  static constexpr long long block_size{1 << 28};

  ImageManager() = default;
  ImageManager(ImageManager const&) = delete;
  ImageManager(ImageManager&&) noexcept;
  ImageManager& operator=(ImageManager const&) = delete;
  ImageManager& operator=(ImageManager&&) noexcept;
  ~ImageManager();

  vk::Image create(vk::Format format,
                   vk::Extent2D extent,
                   vk::ImageUsageFlags usage,
                   vk::MemoryPropertyFlags property =
                       vk::MemoryPropertyFlagBits::eDeviceLocal);
private:
  struct MemoryInfo {
    unsigned type_index;
    long long used_size;
  };

  Vector<MemoryInfo> memory_infos_;
  Vector<vk::Image> images_;
  Vector<vk::DeviceMemory> memories_;
};
