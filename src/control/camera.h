#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <utility>

constexpr auto god_speed{5000.f};

struct Camera {
  static constexpr glm::vec3 world_up{0.f, -1.f, 0.f};

  float move_speed{10.f};
  float rotate_speed{1.f};

  Camera(GLFWwindow* window,
         glm::vec3 pos = glm::vec3{0.f, 0.f, 0.f},
         float yaw = 0.f,
         float pitch = 0.f);
  Camera(GLFWwindow* window, Camera const& c);
  explicit Camera(GLFWwindow* window);

  Camera(Camera const&) = default;
  Camera(Camera&&) = default;
  Camera& operator=(Camera const&) = default;
  Camera& operator=(Camera&&) = default;
  ~Camera() = default;

  glm::vec3 get_position() const noexcept { return pos_; }

  glm::vec3 get_front() const noexcept { return front_; }

  glm::mat4 get_view() const { return glm::lookAt(pos_, pos_ + front_, up_); }

  void update(float delta_time, std::tuple<bool, bool, bool, bool> b4);

  void process_keyboard_input(float delta_time,
                              std::tuple<bool, bool, bool, bool> b4);

  void process_mouse_input(float delta_time);

  GLFWwindow* window_{nullptr};
  glm::vec3 pos_{};
  float yaw_{};
  float pitch_{};
  glm::vec3 front_{glm::normalize(glm::vec3{cos(yaw_) * cos(pitch_),
                                            sin(pitch_),
                                            sin(yaw_) * cos(pitch_)})};
  glm::vec3 right_{glm::normalize(glm::cross(front_, world_up))};
  glm::vec3 up_{glm::normalize(glm::cross(right_, front_))};
  double last_x_{};
  double last_y_{};
};
