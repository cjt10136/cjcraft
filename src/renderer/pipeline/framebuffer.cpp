#include "framebuffer.h"

Framebuffer::Framebuffer(vk::RenderPass render_pass,
                         std::span<vk::ImageView const> attachments,
                         vk::Extent2D extent)
    : handle_{g_context.get_device().createFramebuffer(
        {.renderPass{render_pass},
         .attachmentCount{gsl::narrow<unsigned>(std::size(attachments))},
         .pAttachments{attachments.data()},
         .width{extent.width},
         .height{extent.height},
         .layers{1}})}
{
}

Framebuffer::Framebuffer(Framebuffer&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& x) noexcept
{
  if (handle_)
     g_context.get_device().destroyFramebuffer(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

Framebuffer::~Framebuffer()
{
  if (handle_)
     g_context.get_device().destroyFramebuffer(handle_);
}