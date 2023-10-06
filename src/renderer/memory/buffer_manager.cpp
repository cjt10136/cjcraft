#include "buffer_manager.h"

#include "math/math.h"

#include <chrono>
#include <gsl/gsl>
#include <iostream>

BufferManager::BufferManager(BufferManager&& x)
    : buffer_infos_{std::move(x.buffer_infos_)},
      buffers_{std::move(x.buffers_)},
      memories_{std::move(x.memories_)}
{
  x.buffers_.clear();
  x.memories_.clear();
}

BufferManager& BufferManager::operator=(BufferManager&& x)
{
  for (auto&& i : buffers_)
    g_context.get_device().destroyBuffer(i);
  buffers_ = std::move(x.buffers_);
  x.buffers_.clear();

  for (auto&& i : memories_)
    g_context.get_device().freeMemory(i);
  memories_ = std::move(x.memories_);
  x.memories_.clear();

  buffer_infos_ = std::move(x.buffer_infos_);

  return *this;
}

BufferManager::~BufferManager() noexcept(false)
{
  for (auto&& i : buffers_)
    g_context.get_device().destroyBuffer(i);
  for (auto&& i : memories_)
    g_context.get_device().freeMemory(i);
}

Buffer BufferManager::create(vk::BufferUsageFlags usage,
                             vk::MemoryPropertyFlags property,
                             long long size,
                             long long block_size)
{
  for (gsl::index i{0}; i < std::size(buffer_infos_); ++i) {
    auto&& [u, p, a, s, c, d]{buffer_infos_[i]};
    if (auto r{round_to(size, a)}; usage == u && property == p && s + r <= c) {
      s += r;
      if (property & vk::MemoryPropertyFlagBits::eHostVisible) {
        d = static_cast<char*>(d) + r;
        return {buffers_[i], s - r, r, static_cast<char*>(d) - r};
      }
      else
        return {buffers_[i], s - r, r, nullptr};
    }
  }

  buffers_.push_back(g_context.get_device().createBuffer(
      {.size{gsl::narrow<unsigned long long>(block_size)},
       .usage{usage},
       .sharingMode{vk::SharingMode::eExclusive}}));

  auto const requirement{
      g_context.get_device().getBufferMemoryRequirements(buffers_.back())};
  memories_.push_back(g_context.get_device().allocateMemory(
      {.allocationSize{requirement.size},
       .memoryTypeIndex{[&]() -> unsigned {
         for (auto i{0u};
              i < g_context.get_gpu_memory_properties().memoryTypeCount;
              ++i)
           if (requirement.memoryTypeBits & 1 << i
               && (g_context.get_gpu_memory_properties()
                       .memoryTypes[i]
                       .propertyFlags
                   & property)
                      == property)
             return i;
         throw std::runtime_error{"failed to find a suitable memory type"};
       }()}}));

  g_context.get_device().bindBufferMemory(
      buffers_.back(), memories_.back(), 0);

  if (auto aligned_size{round_to(size, requirement.alignment)};
      property & vk::MemoryPropertyFlagBits::eHostVisible) {
    auto data{g_context.get_device().mapMemory(
        memories_.back(), 0ull, requirement.size)};
    buffer_infos_.push_back({usage,
                             property,
                             gsl::narrow<long long>(requirement.alignment),
                             aligned_size,
                             block_size,
                             static_cast<char*>(data) + aligned_size});
    return {buffers_.back(), 0ll, size, data};
  }
  else {
    buffer_infos_.push_back({usage,
                             property,
                             gsl::narrow<long long>(requirement.alignment),
                             aligned_size,
                             block_size,
                             nullptr});
    return {buffers_.back(), 0ll, size, nullptr};
  }
}
