#include "sampler.h"

Sampler::Sampler()
    : handle_{g_context.get_device().createSampler(
        {.magFilter{vk::Filter::eNearest},
         .minFilter{vk::Filter::eNearest},
         .addressModeU{vk::SamplerAddressMode::eClampToBorder},
         .addressModeV{vk::SamplerAddressMode::eClampToBorder},
         .addressModeW{vk::SamplerAddressMode::eClampToBorder},
         .anisotropyEnable{VK_TRUE},
         .maxAnisotropy{
             g_context.get_gpu_properties().limits.maxSamplerAnisotropy},
         .borderColor{vk::BorderColor::eIntOpaqueBlack}})}
{
}

Sampler::Sampler(Sampler&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

Sampler& Sampler::operator=(Sampler&& x) noexcept
{
  g_context.get_device().destroySampler(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

Sampler::~Sampler()
{
  g_context.get_device().destroySampler(handle_);
}
