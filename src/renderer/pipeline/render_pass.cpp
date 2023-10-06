#include "render_pass.h"

#include "present/swapchain.h"

#include <gsl/gsl>

RenderPass::RenderPass()
{
  std::array<vk::AttachmentDescription, 2> const attachments{
      {{.format{Swapchain::format.format},
        .samples{vk::SampleCountFlagBits::e1},
        .loadOp{vk::AttachmentLoadOp::eClear},
        .storeOp{vk::AttachmentStoreOp::eStore},
        .stencilLoadOp{vk::AttachmentLoadOp::eDontCare},
        .stencilStoreOp{vk::AttachmentStoreOp::eDontCare},
        .initialLayout{vk::ImageLayout::eUndefined},
        .finalLayout{vk::ImageLayout::ePresentSrcKHR}},
       {.format{vk::Format::eD32Sfloat},
        .samples{vk::SampleCountFlagBits::e1},
        .loadOp{vk::AttachmentLoadOp::eClear},
        .storeOp{vk::AttachmentStoreOp::eDontCare},
        .stencilLoadOp{vk::AttachmentLoadOp::eDontCare},
        .stencilStoreOp{vk::AttachmentStoreOp::eDontCare},
        .initialLayout{vk::ImageLayout::eUndefined},
        .finalLayout{vk::ImageLayout::eDepthStencilAttachmentOptimal}}}};

  constexpr vk::AttachmentReference color_ref{
      .attachment{0}, .layout{vk::ImageLayout::eColorAttachmentOptimal}};
  constexpr vk::AttachmentReference depth_ref{
      .attachment{1}, .layout{vk::ImageLayout::eDepthStencilAttachmentOptimal}};
  vk::SubpassDescription const subpass{
      .pipelineBindPoint{vk::PipelineBindPoint::eGraphics},
      .colorAttachmentCount{1},
      .pColorAttachments{&color_ref},
      .pDepthStencilAttachment{&depth_ref}};

  constexpr vk::SubpassDependency dep{
      .srcSubpass{VK_SUBPASS_EXTERNAL},
      .dstSubpass{0},
      .srcStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput},
      .dstStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput},
      .srcAccessMask{vk::AccessFlagBits::eNone},
      .dstAccessMask{vk::AccessFlagBits::eColorAttachmentWrite}};

  handle_ = g_context.get_device().createRenderPass(
      {.attachmentCount{gsl::narrow<unsigned>(std::size(attachments))},
       .pAttachments{attachments.data()},
       .subpassCount{1},
       .pSubpasses{&subpass},
       .dependencyCount{1},
       .pDependencies{&dep}});
}

RenderPass::RenderPass(RenderPass&& x) noexcept : handle_{x.handle_}
{
  x.handle_ = nullptr;
}

RenderPass& RenderPass::operator=(RenderPass&& x) noexcept
{
  if (handle_)
    g_context.get_device().destroyRenderPass(handle_);
  handle_ = x.handle_;
  x.handle_ = nullptr;
  return *this;
}

RenderPass::~RenderPass()
{
  if (handle_)
    g_context.get_device().destroyRenderPass(handle_);
}
