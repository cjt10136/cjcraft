#include "control/camera.h"
#include "control/key.h"
#include "loader/texture.h"
#include "math/math.h"
#include "memory/buffer_manager.h"
#include "memory/image_manager.h"
#include "pipeline/framebuffer.h"
#include "pipeline/pipeline.h"
#include "pipeline/render_pass.h"
#include "pool/command.h"
#include "pool/descriptor.h"
#include "present/swapchain.h"
#include "resource/buffer.h"
#include "resource/image.h"
#include "resource/sampler.h"
#include "sync/fence.h"
#include "sync/semaphore.h"
#include "world/chunk.h"
#include "world/ray.h"
#include "world/world.h"

#include <array>
#include <chrono>
#include <iostream>

constexpr vk::Extent2D default_extent{1'920, 1'080};

BlockType item{BlockType::dirt};
constexpr std::array item_list{BlockType::glass,
                               BlockType::dirt,
                               BlockType::cobble,
                               BlockType::stone,
                               BlockType::log,
                               BlockType::wood,
                               BlockType::sand};

constexpr auto gravity_constant{3.5f};

void draw()
{
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  auto render_distance{24};
  std::cout << "Render distance (in chunk): ";
  std::cout.flush();
  std::cin >> render_distance;

  Swapchain swapchain{default_extent};
  Camera camera{
      swapchain.get_window(),
      {render_distance / 2 * chunk_width, 0, render_distance / 2 * chunk_depth},
      {0.f},
      {0.f}};
  camera.move_speed = god_speed;

  RenderPass render_pass{};
  Vector<Framebuffer> framebuffers;
  framebuffers.reserve(std::size(swapchain));
  for (auto i{0}; i < std::size(swapchain); ++i) {
    std::array attachments{swapchain.get_image_view(i),
                           swapchain.get_depth_view()};
    framebuffers.push_back(
        {render_pass.get(), attachments, swapchain.get_extent()});
  }

  DescriptorSetLayout descriptor_set_layout{};
  Pipeline pipeline{render_pass.get(),
                    descriptor_set_layout.get(),
                    Code{"shaders/vert.spv"},
                    Code{"shaders/frag.spv"}};

  CommandPool<QueueType::graphics> command_pool{};
  DescriptorPool descriptor_pool{};

  auto transfer_cmd{command_pool.create_command_buffers<1>()[0]};
  Fence transfer_fence;

  auto command{command_pool.begin_single_time_commands()};
  BufferManager staging_manager;
  auto const staging_buffer{
      staging_manager.create(vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible
                                 | vk::MemoryPropertyFlagBits::eHostCoherent,
                             2ll << 30,
                             2ll << 30)};
  World world{command, staging_buffer, render_distance};
  command_pool.end_single_time_commands(command);

  command = command_pool.begin_single_time_commands();

  BufferManager buffer_manager;
  std::array<Buffer, max_in_flight> uniform_buffers;
  for (auto&& i : uniform_buffers)
    i = buffer_manager.create(vk::BufferUsageFlagBits::eUniformBuffer,
                              vk::MemoryPropertyFlagBits::eHostVisible
                                  | vk::MemoryPropertyFlagBits::eHostCoherent,
                              sizeof(Uniform));

  Texture terrain{"../textures/blocks.png"};
  auto const staging_image{
      buffer_manager.create(vk::BufferUsageFlagBits::eTransferSrc,
                            vk::MemoryPropertyFlagBits::eHostVisible
                                | vk::MemoryPropertyFlagBits::eHostCoherent,
                            std::size(terrain))};
  std::memcpy(staging_image.data, terrain.data(), std::size(terrain));
  ImageManager image_manager;
  auto const color_attach{
      image_manager.create(vk::Format::eR8G8B8A8Srgb,
                           swapchain.get_extent(),
                           vk::ImageUsageFlagBits::eColorAttachment
                               | vk::ImageUsageFlagBits::eInputAttachment,
                           vk::MemoryPropertyFlagBits::eDeviceLocal)};
  ImageView color_attach_view{
      color_attach, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor};

  auto const image{image_manager.create(
      vk::Format::eR8G8B8A8Srgb,
      {gsl::narrow<unsigned>(terrain.get_width()),
       gsl::narrow<unsigned>(terrain.get_height())},
      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)};

  transition_image_layout<vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal>(command, image);
  command.copyBufferToImage(
      staging_image.handle,
      image,
      vk::ImageLayout::eTransferDstOptimal,
      {{.bufferOffset{gsl::narrow<unsigned long long>(staging_image.offset)},
        .bufferRowLength{0},
        .bufferImageHeight{0},
        .imageSubresource{.aspectMask{vk::ImageAspectFlagBits::eColor},
                          .mipLevel{0},
                          .baseArrayLayer{0},
                          .layerCount{1}},
        .imageOffset{0, 0, 0},
        .imageExtent{gsl::narrow<unsigned>(terrain.get_width()),
                     gsl::narrow<unsigned>(terrain.get_height()),
                     1}}});
  transition_image_layout<vk::ImageLayout::eTransferDstOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal>(command,
                                                                   image);
  command_pool.end_single_time_commands(command);

  Sampler sampler;
  ImageView image_view{
      image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor};

  auto commands{command_pool.create_command_buffers<max_in_flight>()};
  auto descriptor_sets{descriptor_pool.create_descriptor_sets<max_in_flight>(
      uniform_buffers,
      sampler.get(),
      image_view.get(),
      descriptor_set_layout.get())};

  std::array<Fence, max_in_flight> render_done_fences;
  std::array<Semaphore, max_in_flight> render_done_semaphores;
  std::array<Semaphore, max_in_flight> image_acquired_semaphores;
  Fence transfer_completed;

  gsl::index current_frame{0};

  auto prev_time{std::chrono::high_resolution_clock::now()};
  auto last_break{prev_time};
  auto last_jump{prev_time};
  auto v_down{0.f};
  bool jumping{false};
  auto move{true};
  while (!swapchain.should_close()) {
    glfwPollEvents();

    render_done_fences[current_frame].wait();

    auto image_index{0u};
    if (!swapchain.acquire_next_image(
            image_index, image_acquired_semaphores[current_frame].get())) {
      for (auto i{0}; i < std::size(swapchain); ++i) {
        framebuffers.clear();
        std::array attachments{swapchain.get_image_view(i),
                               swapchain.get_depth_view()};
        framebuffers.push_back(
            {render_pass.get(), attachments, swapchain.get_extent()});
      }
      camera = Camera{swapchain.get_window(), camera};
      continue;
    }

    auto const curr_time{std::chrono::high_resolution_clock::now()};
    std::cout << 1e6
                     / std::chrono::duration_cast<std::chrono::microseconds>(
                           curr_time - prev_time)
                           .count()
              << '\n';
    auto dt{std::chrono::duration<float>{curr_time - prev_time}.count()};
    prev_time = curr_time;
    if (!jumping && camera.move_speed != god_speed
        && get_key(swapchain.get_window(), Key::space)) {
      v_down = -0.8;
      jumping = true;
    }

    if (camera.move_speed != god_speed) {
      camera.update(dt, world.hit_wall(camera));
      auto pv{v_down};
      v_down += gravity_constant * dt;
      camera.pos_.y += (v_down * v_down - pv * pv) / 2 * gravity_constant;
      if (world.fix_camera(camera)) {
        v_down = 0;
        jumping = false;
      }
    }
    else
      camera.update(dt, {true, true, true, true});

    if (get_key(swapchain.get_window(), Key::zero))
      item = item_list[0];
    if (get_key(swapchain.get_window(), Key::one))
      item = item_list[1];
    if (get_key(swapchain.get_window(), Key::two))
      item = item_list[2];
    if (get_key(swapchain.get_window(), Key::three))
      item = item_list[3];
    if (get_key(swapchain.get_window(), Key::four))
      item = item_list[4];
    if (get_key(swapchain.get_window(), Key::five))
      item = item_list[5];
    if (get_key(swapchain.get_window(), Key::six))
      item = item_list[6];
    if (get_key(swapchain.get_window(), Key::g))
      camera.move_speed = god_speed;
    if (get_key(swapchain.get_window(), Key::n))
      camera.move_speed = 10;
    if (get_key(swapchain.get_window(), Key::f))
      move = false;

    if (transfer_fence.wait(0) && move) {
      transfer_cmd.reset();
      transfer_cmd.begin(vk::CommandBufferBeginInfo{});
      world.move(transfer_cmd,
                 staging_buffer,
                 {std::round(camera.get_position().x / chunk_width),
                  std::round(camera.get_position().z / chunk_depth)});
      transfer_cmd.end();

      transfer_fence.reset();
      g_context.get_queue<QueueType::graphics>().submit(
          {{.waitSemaphoreCount{0},
            .commandBufferCount{1},
            .pCommandBuffers{&transfer_cmd}}},
          transfer_fence.get());
    }

    Uniform const ubo{
        perspective(glm::radians(60.f), swapchain.get_aspect(), .1f, 1600.f)
        * camera.get_view()};
    std::memcpy(uniform_buffers[current_frame].data, &ubo, sizeof(ubo));

    if (get_button(swapchain.get_window(), Button::left)
        && (curr_time - last_break).count() > 2.5e8) {
      last_break = curr_time;
      auto const faces{cast_ray(camera.get_position(), camera.get_front())};
      for (auto&& [p, f] : faces)
        if (world.destroy_block(f, p) == true)
          break;
    }

    if (get_button(swapchain.get_window(), Button::right)
        && (curr_time - last_break).count() > 2.5e8) {
      last_break = curr_time;
      auto const faces{cast_ray(camera.get_position(), camera.get_front())};
      for (auto&& [p, f] : faces)
        if (world.place_block(item, f, p) == true)
          break;
    }

    commands[current_frame].reset();
    commands[current_frame].begin(vk::CommandBufferBeginInfo{});
    constexpr std::array clear_values{
        vk::ClearValue{
            .color{{{120.f / 255, 160.f / 255, 255.f / 255, 1.f}}},
            //.color{}
        },
        vk::ClearValue{.depthStencil{1.f, 0u}}};

    commands[current_frame].beginRenderPass(
        {.renderPass{render_pass.get()},
         .framebuffer{framebuffers[image_index].get()},
         .renderArea{.offset{0, 0}, .extent{swapchain.get_extent()}},
         .clearValueCount{gsl::narrow<unsigned>(std::size(clear_values))},
         .pClearValues{clear_values.data()}},
        vk::SubpassContents::eInline);

    commands[current_frame].bindPipeline(vk::PipelineBindPoint::eGraphics,
                                         pipeline.get_pipeline());
    commands[current_frame].setViewport(
        0,
        {{.x{0.f},
          .y{0.f},
          .width{gsl::narrow<float>(swapchain.get_extent().width)},
          .height{gsl::narrow<float>(swapchain.get_extent().height)},
          .minDepth{0.f},
          .maxDepth{1.f}}});
    commands[current_frame].setScissor(0, {{{0, 0}, swapchain.get_extent()}});

    commands[current_frame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                               pipeline.get_layout(),
                                               0,
                                               1,
                                               &descriptor_sets[current_frame],
                                               0,
                                               nullptr);
    world.draw(commands[current_frame],
               pipeline.get_layout(),
               glm::vec2{camera.get_position().x, camera.get_position().z},
               glm::vec2{camera.get_front().x, camera.get_front().z});

    commands[current_frame].endRenderPass();
    commands[current_frame].end();

    render_done_fences[current_frame].reset();

    std::array wait_semaphores{image_acquired_semaphores[current_frame].get()};
    std::array<vk::PipelineStageFlags, 1> wait_stages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::array signal_semaphores{render_done_semaphores[current_frame].get()};

    g_context.get_queue<QueueType::graphics>().submit(
        {{.waitSemaphoreCount{1},
          .pWaitSemaphores{wait_semaphores.data()},
          .pWaitDstStageMask{wait_stages.data()},
          .commandBufferCount{1},
          .pCommandBuffers{&commands[current_frame]},
          .signalSemaphoreCount{1},
          .pSignalSemaphores{signal_semaphores.data()}}},
        render_done_fences[current_frame].get());

    if (!swapchain.present(image_index, signal_semaphores)) {
      framebuffers.clear();
      for (auto i{0}; i < std::size(swapchain); ++i) {
        std::array attachments{swapchain.get_image_view(i),
                               swapchain.get_depth_view()};
        framebuffers.push_back(
            {render_pass.get(), attachments, swapchain.get_extent()});
      }
      camera = Camera{swapchain.get_window(), camera};
    }

    current_frame = (current_frame + 1) % max_in_flight;

    if (glfwGetKey(swapchain.get_window(), GLFW_KEY_ESCAPE)) {
      g_context.get_device().waitIdle();
      return;
    }
  }
  g_context.get_device().waitIdle();
}
