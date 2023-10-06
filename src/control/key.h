#pragma once

#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"

#include <array>

enum class Key {
  space = GLFW_KEY_SPACE,

  comma = ',',
  minus,
  period,
  slash,
  zero,
  one,
  two,
  three,
  four,
  five,
  six,
  seven,
  eight,
  nine,

  a = 'A',
  b,
  c,
  d,
  e,
  f,
  g,
  h,
  i,
  j,
  k,
  l,
  m,
  n,
  o,
  p,
  q,
  r,
  s,
  t,
  u,
  v,
  w,
  x,
  y,
  z,

  esc = GLFW_KEY_ESCAPE,
  enter,
  tab,

  right = GLFW_KEY_RIGHT,
  left,
  down,
  up,

  lshift = GLFW_KEY_LEFT_SHIFT,
  lctrl,
  lalt
};

enum class Button {
  left,
  right,
  middle,
};

bool get_key(GLFWwindow* window, Key key) noexcept;
bool get_button(GLFWwindow* window, Button button) noexcept;