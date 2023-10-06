#pragma once

#include "core.h"

struct Image {
  vk::Image handle;
  long long offset;
  long long size;
  void* data;
};

class ImageView {
public:
  ImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect);
  ImageView() = default;
  ImageView(ImageView const&) = delete;
  ImageView(ImageView&&) noexcept;
  ImageView& operator=(ImageView const&) = delete;
  ImageView& operator=(ImageView&&) noexcept;
  ~ImageView();

  vk::ImageView get() const noexcept { return handle_; }
private:
  vk::ImageView handle_;
};
