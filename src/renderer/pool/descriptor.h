#pragma once

#include "resource/buffer.h"

#include <gsl/gsl>
#include <span>

class DescriptorSetLayout {
public:
  DescriptorSetLayout();
  DescriptorSetLayout(DescriptorSetLayout const&) = delete;
  DescriptorSetLayout(DescriptorSetLayout&&) noexcept;
  DescriptorSetLayout& operator=(DescriptorSetLayout const&) = delete;
  DescriptorSetLayout& operator=(DescriptorSetLayout&&) noexcept;
  ~DescriptorSetLayout();

  vk::DescriptorSetLayout get() const noexcept { return handle_; }
private:
  vk::DescriptorSetLayout handle_;
};

class DescriptorPool {
public:
  DescriptorPool();
  DescriptorPool(DescriptorPool const&) = delete;
  DescriptorPool(DescriptorPool&&) noexcept;
  DescriptorPool& operator=(DescriptorPool const&) = delete;
  DescriptorPool& operator=(DescriptorPool&&) noexcept;
  ~DescriptorPool();

  vk::DescriptorPool get() const noexcept { return handle_; }

  template<gsl::index N>
  std::array<vk::DescriptorSet, N>
  create_descriptor_sets(std::span<Buffer const, N> buffers,
                         vk::Sampler sampler,
                         vk::ImageView image_view,
                         vk::DescriptorSetLayout layout) const;
private:
  vk::DescriptorPool handle_;
};

template<gsl::index N>
std::array<vk::DescriptorSet, N>
DescriptorPool::create_descriptor_sets(std::span<Buffer const, N> buffers,
                                       vk::Sampler sampler,
                                       vk::ImageView image_view,
                                       vk::DescriptorSetLayout layout) const
{
  std::array<vk::DescriptorSetLayout, N> layouts;
  layouts.fill(layout);
  vk::DescriptorSetAllocateInfo const info{
      .descriptorPool{handle_},
      .descriptorSetCount{gsl::narrow<unsigned>(N)},
      .pSetLayouts{layouts.data()}};

  std::array<vk::DescriptorSet, N> sets;
  if (g_context.get_device().allocateDescriptorSets(&info, sets.data())
      != vk::Result::eSuccess)
    throw std::runtime_error{"failed to allocate descriptor sets"};

  for (gsl::index i{0}; i < N; ++i) {
    vk::DescriptorBufferInfo const buffer_info{
        .buffer{buffers[i].handle},
        .offset{gsl::narrow<unsigned long long>(buffers[i].offset)},
        .range{sizeof(Uniform)}};
    vk::DescriptorImageInfo const image_info{
        .sampler{sampler},
        .imageView{image_view},
        .imageLayout{vk::ImageLayout::eShaderReadOnlyOptimal}};

    g_context.get_device().updateDescriptorSets(
        {
            {.dstSet{sets[i]},
             .dstBinding{0},
             .dstArrayElement{0},
             .descriptorCount{1},
             .descriptorType{vk::DescriptorType::eUniformBuffer},
             .pBufferInfo{&buffer_info}},
            {.dstSet{sets[i]},
             .dstBinding{1},
             .dstArrayElement{0},
             .descriptorCount{1},
             .descriptorType{vk::DescriptorType::eCombinedImageSampler},
             .pImageInfo{&image_info}  }
    },
        {});
  }

  return sets;
}