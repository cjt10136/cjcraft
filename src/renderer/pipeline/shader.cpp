#include "shader.h"

Shader::Shader(Code const& code)
    : handle_{g_context.get_device().createShaderModule(
        {.codeSize{std::size(code)}, .pCode{code.data()}})}
{
}

Shader::Shader(Shader&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

Shader& Shader::operator=(Shader&& x) noexcept
{
  if (handle_)
    g_context.get_device().destroyShaderModule(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

Shader::~Shader()
{
  if (handle_)
    g_context.get_device().destroyShaderModule(handle_);
}