#include "fence.h"

#include <gsl/gsl>

Fence::Fence()
    : handle_{g_context.get_device().createFence(
        {.flags{vk::FenceCreateFlagBits::eSignaled}})}
{
}

Fence::Fence(Fence&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

Fence& Fence::operator=(Fence&& x) noexcept
{
  g_context.get_device().destroyFence(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

Fence::~Fence()
{
  g_context.get_device().destroyFence(handle_);
}

bool Fence::wait(long long timeout) const
{
  if (auto result{g_context.get_device().waitForFences(
          1, &handle_, VK_TRUE, gsl::narrow<unsigned long long>(timeout))};
      result != vk::Result::eSuccess)
    return false;
  return true;
}

void Fence::reset() const
{
  g_context.get_device().resetFences({handle_});
}