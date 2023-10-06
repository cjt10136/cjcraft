#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

template<typename T, typename U>
constexpr T round_to(T x, U a) noexcept
{
  if (auto const r{x % a}; r)
    return x + a - r;
  return x;
}

template<typename T>
constexpr glm::mat<4, 4, T, glm::qualifier::defaultp>
perspective(T fovy, T aspect, T near, T far)
{
  auto proj{glm::perspective(fovy, aspect, near, far)};
  proj[1][1] *= -1;
  return proj;
}
