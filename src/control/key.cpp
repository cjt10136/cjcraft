#include "key.h"

bool get_key(GLFWwindow* window, Key key) noexcept
{
  return glfwGetKey(window, static_cast<int>(key));
}

bool get_button(GLFWwindow* window, Button button) noexcept
{
  return glfwGetMouseButton(window, static_cast<int>(button));
}
