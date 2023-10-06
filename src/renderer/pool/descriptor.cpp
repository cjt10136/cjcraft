#include "descriptor.h"

#include <array>
#include <stdexcept>

DescriptorSetLayout::DescriptorSetLayout()
{
  constexpr std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
      {{.binding{0},
.descriptorType{vk::DescriptorType::eUniformBuffer},
.descriptorCount{1},
.stageFlags{vk::ShaderStageFlagBits::eVertex}},
       {
       .binding{1},
       .descriptorType{vk::DescriptorType::eCombinedImageSampler},
       .descriptorCount{1},
       .stageFlags{vk::ShaderStageFlagBits::eFragment},
       }}
  };

  handle_ = g_context.get_device().createDescriptorSetLayout(
      {.bindingCount{gsl::narrow<unsigned>(std::size(bindings))},
       .pBindings{bindings.data()}});
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& x) noexcept
    : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

DescriptorSetLayout&
DescriptorSetLayout::operator=(DescriptorSetLayout&& x) noexcept
{
  if (handle_)
    g_context.get_device().destroyDescriptorSetLayout(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

DescriptorSetLayout::~DescriptorSetLayout()
{
  if (handle_)
    g_context.get_device().destroyDescriptorSetLayout(handle_);
}

DescriptorPool::DescriptorPool()
{
  constexpr std::array<vk::DescriptorPoolSize, 2> sizes{
      {{.type{vk::DescriptorType::eUniformBuffer},
.descriptorCount{gsl::narrow<unsigned>(max_in_flight)}},
       {.type{vk::DescriptorType::eCombinedImageSampler},
       .descriptorCount{gsl::narrow<unsigned>(max_in_flight)}}}
  };
  handle_ = g_context.get_device().createDescriptorPool(
      {.maxSets{gsl::narrow<unsigned>(max_in_flight)},
       .poolSizeCount{gsl::narrow<unsigned>(std::size(sizes))},
       .pPoolSizes{sizes.data()}});
}

DescriptorPool::DescriptorPool(DescriptorPool&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& x) noexcept
{
  if (handle_)
    g_context.get_device().destroyDescriptorPool(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

DescriptorPool::~DescriptorPool()
{
  if (handle_)
    g_context.get_device().destroyDescriptorPool(handle_);
}