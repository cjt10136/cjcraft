#pragma once

#include "core.h"

class Semaphore {
public:
  Semaphore();
  Semaphore(Semaphore const&) = delete;
  Semaphore(Semaphore&&) noexcept;
  Semaphore& operator=(Semaphore const&) = delete;
  Semaphore& operator=(Semaphore&&) noexcept;
  ~Semaphore();

  vk::Semaphore get() const noexcept { return handle_; }
private:
  vk::Semaphore handle_;
};
