#pragma once

#include "resource/buffer.h"

#include <vector>

class BufferManager {
public:
  static constexpr long long default_block_size{1 << 24};

  BufferManager() = default;
  BufferManager(BufferManager const&) = delete;
  BufferManager(BufferManager&&);
  BufferManager& operator=(BufferManager const&) = delete;
  BufferManager& operator=(BufferManager&&);
  ~BufferManager() noexcept(false);

  Buffer create(vk::BufferUsageFlags usage,
                vk::MemoryPropertyFlags property,
                long long size = default_block_size,
                long long block_size = default_block_size);
private:
  struct BufferInfo {
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags property;
    long long alignment;
    long long used_size;
    long long capacity;
    void* data;
  };

  Vector<BufferInfo> buffer_infos_;
  Vector<vk::Buffer> buffers_;
  Vector<vk::DeviceMemory> memories_;
};
