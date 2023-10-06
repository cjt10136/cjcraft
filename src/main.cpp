#include "renderer/renderer.cpp"

#include <iostream>
#include <stdexcept>

int main()
{
  try {
    draw();
  }
  catch (std::runtime_error const& e) {
    std::cout << e.what() << '\n';
  }
}
