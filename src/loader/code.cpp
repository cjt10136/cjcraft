#include "code.h"

#include <fstream>

Code::Code(std::string_view const path)
{
  std::ifstream file{path.data(), std::ios::binary};
  if (!file.is_open())
    throw std::runtime_error{"failed to open file"};

  std::noskipws(file);
  code_ = {std::istreambuf_iterator{file}, {}};
}