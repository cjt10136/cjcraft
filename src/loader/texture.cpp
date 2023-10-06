#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#include <stdexcept>

Texture::Texture(std::string_view const path)
{
  auto channel{0};
  pixels_ = stbi_load(path.data(), &width_, &height_, &channel, STBI_rgb_alpha);
  if (!pixels_)
    throw std::runtime_error{"failed to load texture image"};
}

Texture::Texture(Texture&& x) noexcept
    : width_{x.width_}, height_{x.height_}, pixels_{x.pixels_}
{
  x.pixels_ = nullptr;
}

Texture& Texture::operator=(Texture&& x) noexcept
{
  width_ = x.width_;
  height_ = x.height_;
  pixels_ = x.pixels_;
  x.pixels_ = nullptr;
  return *this;
}

Texture::~Texture()
{
  if (pixels_)
    stbi_image_free(pixels_);
}
