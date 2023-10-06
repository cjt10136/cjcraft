#include "pipeline.h"

#include "resource/buffer.h"

#include <array>
#include <gsl/gsl>

Pipeline::Pipeline(vk::RenderPass render_pass,
                   vk::DescriptorSetLayout layout,
                   Code const& vert_code,
                   Code const& frag_code)
{
  constexpr vk::PushConstantRange push_constant{
      .stageFlags{vk::ShaderStageFlagBits::eVertex
                  | vk::ShaderStageFlagBits::eFragment},
      .size{sizeof(PushConstant)}};
  layout_ = g_context.get_device().createPipelineLayout(
      {.setLayoutCount{1},
       .pSetLayouts{&layout},
       .pushConstantRangeCount{1},
       .pPushConstantRanges{&push_constant}});

  Shader const vert_shader{vert_code};
  vk::PipelineShaderStageCreateInfo const vert_stage{
      .stage{vk::ShaderStageFlagBits::eVertex},
      .module{vert_shader.get()},
      .pName{"main"}};
  Shader const frag_shader{frag_code};
  vk::PipelineShaderStageCreateInfo const frag_stage{
      .stage{vk::ShaderStageFlagBits::eFragment},
      .module{frag_shader.get()},
      .pName{"main"}};
  std::array const stages{vert_stage, frag_stage};

  constexpr auto binding_description{Vertex::get_binding_description()};
  constexpr auto attribute_descriptions{Vertex::get_attribute_description()};
  vk::PipelineVertexInputStateCreateInfo const vert_input{
      .vertexBindingDescriptionCount{1},
      .pVertexBindingDescriptions{&binding_description},
      .vertexAttributeDescriptionCount{std::size(attribute_descriptions)},
      .pVertexAttributeDescriptions{attribute_descriptions.data()}};

  constexpr vk::PipelineInputAssemblyStateCreateInfo input_assembly{
      .topology{vk::PrimitiveTopology::eTriangleList}};
  constexpr vk::PipelineViewportStateCreateInfo viewport{.viewportCount{1},
                                                         .scissorCount{1}};
  constexpr vk::PipelineRasterizationStateCreateInfo rasterization{
      .polygonMode{vk::PolygonMode::eFill},
      .cullMode{vk::CullModeFlagBits::eBack},
      .frontFace{vk::FrontFace::eCounterClockwise},
      .lineWidth{1.0f}};
  constexpr vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples{vk::SampleCountFlagBits::e1}};
  constexpr vk::PipelineDepthStencilStateCreateInfo depth{
      .depthTestEnable{VK_TRUE},
      .depthWriteEnable{VK_TRUE},
      .depthCompareOp{vk::CompareOp::eLess}};
  constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment{
      .colorWriteMask{
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
          | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA}};
  vk::PipelineColorBlendStateCreateInfo const color_blending{
      .attachmentCount{1}, .pAttachments{&color_blend_attachment}};

  std::array const states{vk::DynamicState::eViewport,
                          vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo const dynamics{
      .dynamicStateCount{std::size(states)}, .pDynamicStates{states.data()}};

  pipeline_ = g_context.get_device()
                  .createGraphicsPipeline(
                      nullptr,
                      {.stageCount{gsl::narrow<unsigned>(std::size(stages))},
                       .pStages{stages.data()},
                       .pVertexInputState{&vert_input},
                       .pInputAssemblyState{&input_assembly},
                       .pViewportState{&viewport},
                       .pRasterizationState{&rasterization},
                       .pMultisampleState{&multisampling},
                       .pDepthStencilState{&depth},
                       .pColorBlendState{&color_blending},
                       .pDynamicState{&dynamics},
                       .layout{layout_},
                       .renderPass{render_pass}})
                  .value;
}

Pipeline::Pipeline(Pipeline&& x) noexcept
    : layout_{x.layout_}, pipeline_{x.pipeline_}
{
  x.layout_ = nullptr;
  x.pipeline_ = nullptr;
}

Pipeline& Pipeline::operator=(Pipeline&& x) noexcept
{
  VkPipeline p{pipeline_};
  g_context.get_device().destroyPipeline(p);
  if (pipeline_)
    g_context.get_device().destroyPipeline(pipeline_);
  pipeline_ = x.pipeline_;
  x.pipeline_ = nullptr;

  if (layout_)
    g_context.get_device().destroyPipelineLayout(layout_);
  layout_ = x.layout_;
  x.layout_ = nullptr;

  return *this;
}

Pipeline::~Pipeline()
{
  if (pipeline_)
    g_context.get_device().destroyPipeline(pipeline_);
  if (layout_)
    g_context.get_device().destroyPipelineLayout(layout_);
}
