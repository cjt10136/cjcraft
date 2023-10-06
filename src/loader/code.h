#pragma once

#include <string_view>
#include <vector>

class Code {
public:
  explicit Code(std::string_view const path);

  unsigned const* data() const noexcept
  {
    return reinterpret_cast<unsigned const*>(code_.data());
  }

  size_t size() const noexcept { return std::size(code_); };
private:
  std::vector<char> code_;
};