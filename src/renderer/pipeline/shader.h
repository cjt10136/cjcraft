#pragma once

#include "core.h"

#include "loader/code.h"

class Shader {
public:
  Shader(Code const& code);
  Shader(Shader const&) = delete;
  Shader(Shader&&) noexcept;
  Shader& operator=(Shader const&) = delete;
  Shader& operator=(Shader&&) noexcept;
  ~Shader();

  vk::ShaderModule get() const noexcept { return handle_; }
private:
  vk::ShaderModule handle_;
};