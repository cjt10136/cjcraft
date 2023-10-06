#include "image.h"

ImageView::ImageView(vk::Image image,
                     vk::Format format,
                     vk::ImageAspectFlags aspect)
    : handle_{
        g_context.get_device().createImageView(
            {.image{image},
                                           .viewType{vk::ImageViewType::e2D},
                                           .format{format},
                                           .components{.r{vk::ComponentSwizzle::eIdentity},
                                           .g{vk::ComponentSwizzle::eIdentity},
                                           .b{vk::ComponentSwizzle::eIdentity},
                                           .a{vk::ComponentSwizzle::eIdentity}},
                                           .subresourceRange{.aspectMask{aspect},
                                           .levelCount{1},
                                           .layerCount{1}}}
            )
}
{
}

ImageView::ImageView(ImageView&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

ImageView& ImageView::operator=(ImageView&& x) noexcept
{
  g_context.get_device().destroyImageView(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

ImageView::~ImageView()
{
  g_context.get_device().destroyImageView(handle_);
}