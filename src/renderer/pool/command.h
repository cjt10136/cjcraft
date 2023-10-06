#pragma once

#include "resource/buffer.h"

#include <array>
#include <gsl/gsl>
#include <stdexcept>
#include <vector>

template<QueueType T>
class CommandPool {
public:
  CommandPool();
  CommandPool(CommandPool const&) = delete;
  CommandPool(CommandPool&&) noexcept;
  CommandPool& operator=(CommandPool const&) = delete;
  CommandPool& operator=(CommandPool&&) noexcept;
  ~CommandPool();

  vk::CommandPool get() const noexcept { return handle_; }

  template<gsl::index N>
  std::array<vk::CommandBuffer, N> create_command_buffers() const;

  vk::CommandBuffer begin_single_time_commands() const;

  void end_single_time_commands(vk::CommandBuffer buffer) const;
private:
  vk::CommandPool handle_;
};

template<QueueType T>
CommandPool<T>::CommandPool()
    : handle_{g_context.get_device().createCommandPool(
        {.flags{vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
         .queueFamilyIndex{g_context.get_queue_family<T>()}})}
{
}

template<QueueType T>
CommandPool<T>::CommandPool(CommandPool&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

template<QueueType T>
CommandPool<T>& CommandPool<T>::operator=(CommandPool&& x) noexcept
{
  if (handle_)
    g_context.get_device().destroyCommandPool(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

template<QueueType T>
CommandPool<T>::~CommandPool()
{
  if (handle_)
    g_context.get_device().destroyCommandPool(handle_);
}

template<QueueType T>
vk::CommandBuffer CommandPool<T>::begin_single_time_commands() const
{
  vk::CommandBufferAllocateInfo const info{
      .commandPool{handle_},
      .level{vk::CommandBufferLevel::ePrimary},
      .commandBufferCount{1}};
  vk::CommandBuffer buffer;
  if (g_context.get_device().allocateCommandBuffers(&info, &buffer)
      != vk::Result::eSuccess)
    throw std::runtime_error{"fail to allocate a single time command buffer"};
  buffer.begin(vk::CommandBufferBeginInfo{
      .flags{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}});
  return buffer;
}

template<QueueType T>
void CommandPool<T>::end_single_time_commands(vk::CommandBuffer buffer) const
{
  buffer.end();
  g_context.get_queue<T>().submit({
      {.commandBufferCount{1}, .pCommandBuffers{&buffer}}
  });
  g_context.get_queue<T>().waitIdle();
  g_context.get_device().freeCommandBuffers(handle_, 1, &buffer);
}

template<QueueType T>
template<gsl::index N>
std::array<vk::CommandBuffer, N> CommandPool<T>::create_command_buffers() const
{
  std::array<vk::CommandBuffer, N> buffers;
  vk::CommandBufferAllocateInfo const info{
      .commandPool{handle_},
      .level{vk::CommandBufferLevel::ePrimary},
      .commandBufferCount{gsl::narrow<unsigned>(std::size(buffers))}};
  if (g_context.get_device().allocateCommandBuffers(&info, buffers.data())
      != vk::Result::eSuccess)
    throw std::runtime_error{"failed to allocate command buffers"};
  return buffers;
}

template<vk::ImageLayout OldLayout, vk::ImageLayout NewLayout>
void transition_image_layout(vk::CommandBuffer command,
                             vk::Image image) noexcept
{
  vk::ImageMemoryBarrier barrier{
      .oldLayout{OldLayout},
      .newLayout{NewLayout},
      .srcQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED},
      .dstQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED},
      .image{image},
      .subresourceRange{.aspectMask{vk::ImageAspectFlagBits::eColor},
                 .baseMipLevel{0},
                 .levelCount{1},
                 .baseArrayLayer{0},
                 .layerCount{1}}
  };

  vk::PipelineStageFlags src_stage{};
  vk::PipelineStageFlags dst_stage{};
  if constexpr (OldLayout == vk::ImageLayout::eUndefined
                && NewLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    dst_stage = vk::PipelineStageFlagBits::eTransfer;
  }
  else if constexpr (OldLayout == vk::ImageLayout::eTransferDstOptimal
                     && NewLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    src_stage = vk::PipelineStageFlagBits::eTransfer;
    dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
  }
  else
    static_assert(std::is_same_v<OldLayout, void*>);

  command.pipelineBarrier(src_stage, dst_stage, {}, {}, {}, {barrier});
}

void copy_buffer(vk::CommandBuffer cb, Buffer src, Buffer dst) noexcept;
