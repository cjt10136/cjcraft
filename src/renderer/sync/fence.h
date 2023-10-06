#pragma once

#include "core.h"

#include <numeric>

class Fence {
public:
  Fence();
  Fence(Fence const&) = delete;
  Fence(Fence&& x) noexcept;
  Fence& operator=(Fence const&) = delete;
  Fence& operator=(Fence&&) noexcept;
  ~Fence();

  vk::Fence get() const noexcept { return handle_; }

  bool wait(long long timeout = std::numeric_limits<long long>::max()) const;

  void reset() const;
private:
  vk::Fence handle_;
};
