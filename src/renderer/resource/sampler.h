#pragma once

#include "core.h"

class Sampler {
public:
  Sampler();
  Sampler(Sampler const&) = delete;
  Sampler(Sampler&&) noexcept;
  Sampler& operator=(Sampler const&) = delete;
  Sampler& operator=(Sampler&&) noexcept;
  ~Sampler();

  vk::Sampler get() const noexcept { return handle_; }
private:
  vk::Sampler handle_;
};