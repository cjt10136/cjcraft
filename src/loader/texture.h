#pragma once

#include <string_view>

class Texture {
public:
  static constexpr auto channel{4};

  explicit Texture(std::string_view const path);
  Texture(Texture const&) = delete;
  Texture(Texture&&) noexcept;
  Texture& operator=(Texture const&) = delete;
  Texture& operator=(Texture&&) noexcept;
  ~Texture();

  int get_width() const noexcept { return width_; }

  int get_height() const noexcept { return height_; }

  int size() const noexcept { return width_ * height_ * channel; };

  unsigned char const* data() const noexcept { return pixels_; }
private:
  int width_;
  int height_;
  unsigned char* pixels_;
};