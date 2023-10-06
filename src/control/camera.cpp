#include "camera.h"

#include "key.h"

#include <gsl/gsl>

Camera::Camera(GLFWwindow* window, glm::vec3 pos, float yaw, float pitch)
    : window_{window}, pos_{pos}, yaw_{yaw}, pitch_{pitch}
{
  glfwGetCursorPos(window_, &last_x_, &last_y_);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

Camera::Camera(GLFWwindow* window) : Camera{window, {}, glm::radians(90.f), 0.f}
{
}

Camera::Camera(GLFWwindow* window, Camera const& c)
    : Camera{window, c.pos_, c.yaw_, c.pitch_}
{
}

void Camera::update(float delta_time, std::tuple<bool, bool, bool, bool> b4)
{
  process_keyboard_input(delta_time, b4);

  process_mouse_input(delta_time);

  front_ = glm::normalize(
      glm::vec3{cos(yaw_) * cos(pitch_), sin(pitch_), sin(yaw_) * cos(pitch_)});
  right_ = glm::normalize(glm::cross(front_, world_up));
  up_ = glm::normalize(glm::cross(right_, front_));
}

void Camera::process_keyboard_input(float delta_time,
                                    std::tuple<bool, bool, bool, bool> b4)
{
  auto const velocity{move_speed * delta_time};
  if (get_key(window_, Key::w))
    pos_ += glm::normalize(glm::cross(world_up, right_)) * velocity;
  if (get_key(window_, Key::s))
    pos_ -= glm::normalize(glm::cross(world_up, right_)) * velocity;
  if (get_key(window_, Key::a))
    pos_ -= right_ * velocity;
  if (get_key(window_, Key::d))
    pos_ += right_ * velocity;

  if (std::ceil(pos_.x) - pos_.x < 0.2 && !std::get<0>(b4))
    pos_.x = std::ceil(pos_.x) - 0.2;
  if (std::ceil(pos_.z) - pos_.z < 0.2 && !std::get<1>(b4))
    pos_.z = std::ceil(pos_.z) - 0.2;
  if (pos_.x - std::floor(pos_.x) < 0.2 && !std::get<2>(b4))
    pos_.x = 0.2 + std::floor(pos_.x);
  if (pos_.z - std::floor(pos_.z) < 0.2 && !std::get<3>(b4))
    pos_.z = 0.2 + std::floor(pos_.z);

  if (move_speed == god_speed && get_key(window_, Key::space))
    pos_ += world_up * velocity;
  // if (move_speed == god_speed && get_key(window_, Key::lctrl))
  if (move_speed == god_speed && get_key(window_, Key::lshift))
    pos_ -= world_up * velocity;
}

void Camera::process_mouse_input(float delta_time)
{
  auto const move_speed{rotate_speed * delta_time};

  auto x{0.};
  auto y{0.};
  glfwGetCursorPos(window_, &x, &y);

  yaw_ = glm::mod(yaw_ - move_speed * gsl::narrow<float>(x - last_x_),
                  glm::two_pi<float>());
  pitch_ = glm::clamp(
      pitch_ + move_speed * gsl::narrow<float>(y - last_y_), -1.5f, 1.5f);

  last_x_ = x;
  last_y_ = y;
}
