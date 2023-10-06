#include "semaphore.h"

Semaphore::Semaphore() : handle_{g_context.get_device().createSemaphore({})}
{
}

Semaphore::Semaphore(Semaphore&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

Semaphore& Semaphore::operator=(Semaphore&& x) noexcept
{
  g_context.get_device().destroySemaphore(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

Semaphore::~Semaphore()
{
  g_context.get_device().destroySemaphore(handle_);
}
