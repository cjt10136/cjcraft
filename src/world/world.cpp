#include "world.h"

#include <execution>
#include <iostream>

World::World(vk::CommandBuffer command,
             Buffer staging_buffer,
             int render_distance)
    : side_{render_distance}, buffers_(side_ * side_)
{
  std::span<Face> staging_view{
      static_cast<Face*>(staging_buffer.data),
      gsl::narrow_cast<unsigned long long>(staging_buffer.size)};
  auto it{std::begin(staging_view)};

  // create cold chunk
  for (auto i{0}; i < side_; ++i)
    for (auto j{0}; j < side_; ++j) {
      // skip if this chunk is around the player
      if ((i == (side_ - 1) / 2 || i == side_ / 2)
          && (j == (side_ - 1) / 2 || j == side_ / 2))
        continue;

      auto const faces{create_chunk({i, j})};
      std::ranges::copy(faces, it);

      buffers_[side_ * i + j] =
          buffer_manager_.create(vk::BufferUsageFlagBits::eTransferSrc
                                     | vk::BufferUsageFlagBits::eTransferDst
                                     | vk::BufferUsageFlagBits::eVertexBuffer,
                                 vk::MemoryPropertyFlagBits::eHostVisible
                                     | vk::MemoryPropertyFlagBits::eHostCoherent
                                     | vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 300'000,
                                 2e9);
      copy_buffer(command,
                  {staging_buffer.handle,
                   gsl::narrow_cast<long long>(staging_buffer.offset
                                               + (it - std::begin(staging_view))
                                                     * sizeof(Face)),
                   gsl::narrow_cast<long long>(std::size(faces) * sizeof(Face)),
                   nullptr},
                  buffers_[side_ * i + j]);
      buffers_[side_ * i + j].size = std::size(faces) * sizeof(Face);
      it += std::size(faces);
    }

  for (auto i{0}; i < 2; ++i)
    for (auto j{0}; j < 2; ++j) {
      auto [f, m, t]{create_hot_chunk({side_ / 2 - 1 + i, side_ / 2 - 1 + j})};

      buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j] =
          buffer_manager_.create(vk::BufferUsageFlagBits::eTransferSrc
                                     | vk::BufferUsageFlagBits::eTransferDst
                                     | vk::BufferUsageFlagBits::eVertexBuffer,
                                 vk::MemoryPropertyFlagBits::eHostVisible
                                     | vk::MemoryPropertyFlagBits::eHostCoherent
                                     | vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 300'000,
                                 2e9);
      buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j].size =
          std::size(f) * sizeof(Face);
      std::ranges::copy(
          f,
          static_cast<Face*>(
              buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j]
                  .data));
      terrains_[i][j] = std::move(t);
      maps_[i][j] = std::move(m);
    }
}

void World::draw(vk::CommandBuffer command,
                 vk::PipelineLayout layout,
                 glm::vec2 pos,
                 glm::vec2 front) const
{
  for (auto i{0}; i < side_; ++i)
    for (auto j{0}; j < side_; ++j) {
      PushConstant pd{
          {(offset_.x + i) * chunk_width, 0, (offset_.y + j) * chunk_depth}};
      auto diff{
          glm::normalize(glm::vec2{pd.offset.x + chunk_width / 2 - pos.x,
                                   pd.offset.z + chunk_depth / 2 - pos.y})};

      if (diff.x * front.x + diff.y * front.y < 0.2
          && (pd.offset.x - pos.x) * (pd.offset.x - pos.x)
                     + (pd.offset.z - pos.y) * (pd.offset.z - pos.y)
                 > 80'000)
        continue;

      command.pushConstants(layout,
                            vk::ShaderStageFlagBits::eVertex
                                | vk::ShaderStageFlagBits::eFragment,
                            0,
                            sizeof(PushConstant),
                            &pd);
      command.bindVertexBuffers(
          0u, buffers_[side_ * i + j].handle, buffers_[side_ * i + j].offset);
      command.draw(buffers_[side_ * i + j].size / sizeof(Vertex), 1u, 0u, 0u);
    }
}

bool World::place_block(BlockType block, FaceType face, glm::ivec3 pos)
{
  // decide the face is located at which one of the four hot chunk
  auto i{0};
  if (pos.x <= offset_.x * chunk_width + chunk_width * (side_ / 2 - 1) + 1
      || pos.x >= offset_.x * chunk_width + chunk_width * (side_ / 2 + 1) - 2
      || pos.y < 0 || pos.y >= chunk_height)
    return false;
  else if (pos.x >= offset_.x * chunk_width + chunk_width * (side_ / 2)) {
    i = 1;
    pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2);
  }
  else
    pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2 - 1);
  auto j{0};
  if (pos.z <= offset_.y * chunk_depth + chunk_depth * (side_ / 2 - 1)
      || pos.z >= offset_.y * chunk_depth + chunk_depth * (side_ / 2 + 1) - 1)
    return false;
  else if (pos.z >= offset_.y * chunk_depth + chunk_depth * (side_ / 2)) {
    j = 1;
    pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2);
  }
  else
    pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2 - 1);

  if (terrains_[i][j][pos.x][pos.z][pos.y] == BlockType::air)
    return false;

  // the actual position of the block
  switch (face) {
  case FaceType::up:
    --pos.y;
    if (pos.y < 0)
      return false;
    break;
  case FaceType::down:
    ++pos.y;
    if (pos.y < chunk_height)
      return false;
    break;
  case FaceType::left:
    --pos.x;
    if (pos.x < 0) {
      --i;
      pos.x = chunk_width - 1;
    }
    break;
  case FaceType::right:
    ++pos.x;
    if (pos.x == chunk_width) {
      ++i;
      pos.x = 0;
    }
    break;
  case FaceType::front:
    --pos.z;
    if (pos.z < 0) {
      --j;
      pos.z = chunk_depth - 1;
    }
    break;
  case FaceType::back:
    ++pos.z;
    if (pos.z == chunk_depth) {
      ++j;
      pos.z = 0;
    }
    break;
  }

  if (terrains_[i][j][pos.x][pos.z][pos.y] != BlockType::air)
    return false;

  // place block
  terrains_[i][j][pos.x][pos.z][pos.y] = block;
  block_mods_[static_cast<uint64_t>(offset_.x + (side_ - 1) / 2 + i) << 32
              | static_cast<uint64_t>(offset_.y + (side_ - 1) / 2 + j)]
      .push_back({block, static_cast<glm::u8vec3>(pos)});
  place_block_help(i, j, block, face, pos);

  return true;
}

bool World::destroy_block(FaceType face, glm::ivec3 pos)
{
  auto i{0};
  if (pos.x <= offset_.x * chunk_width + chunk_width * (side_ / 2 - 1)
      || pos.x >= offset_.x * chunk_width + chunk_width * (side_ / 2 + 1) - 1
      || pos.y < 0 || pos.y >= chunk_height)
    return false;
  else if (pos.x >= offset_.x * chunk_width + chunk_width * (side_ / 2)) {
    i = 1;
    pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2);
  }
  else
    pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2 - 1);
  auto j{0};
  if (pos.z <= offset_.y * chunk_depth + chunk_depth * (side_ / 2 - 1)
      || pos.z >= offset_.y * chunk_depth + chunk_depth * (side_ / 2 + 1) - 1)
    return false;
  else if (pos.z >= offset_.y * chunk_depth + chunk_depth * (side_ / 2)) {
    j = 1;
    pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2);
  }
  else
    pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2 - 1);

  if (terrains_[i][j][pos.x][pos.z][pos.y] == BlockType::air
      || terrains_[i][j][pos.x][pos.z][pos.y] == BlockType::bedrock)
    return false;

  terrains_[i][j][pos.x][pos.z][pos.y] = BlockType::air;
  block_mods_[static_cast<uint64_t>(offset_.x + (side_ - 1) / 2 + i) << 32
              | static_cast<uint64_t>(offset_.y + (side_ - 1) / 2 + j)]
      .push_back({BlockType::air, static_cast<glm::u8vec3>(pos)});
  destroy_block_help(i, j, face, pos);

  return true;
}

bool World::move(vk::CommandBuffer command,
                 Buffer staging_buffer,
                 glm::ivec2 position)
{
  std::span<Face> staging_view{
      static_cast<Face*>(staging_buffer.data),
      gsl::narrow_cast<unsigned long long>(staging_buffer.size)};
  auto staging_it{std::begin(staging_view)};

  if (position.x > offset_.x + side_ / 2) {
    for (auto i{0}; i < side_; ++i) {
      if (auto it{mods_.find(static_cast<uint64_t>(offset_.x + side_) << 32
                             | static_cast<uint64_t>(offset_.y + i))};
          it != std::end(mods_)) {
        auto [f, m, t]{create_hot_chunk({offset_.x + side_, offset_.y + i})};
        std::ranges::copy(f, staging_it);

        Buffer b{nullptr,
                 0,
                 static_cast<int64_t>(std::size(f) * sizeof(Face)),
                 static_cast<Face*>(staging_buffer.data)
                     + (staging_it - std::begin(staging_view))};
        Vector<unsigned> free;
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(b, m, free, block, face, pos);
            break;
          case Operation::destroy:
            destroy_face(b, m, free, face, pos);
            break;
          }

        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             b.size,
             staging_buffer.data},
            buffers_[i]);
        buffers_[i].size = b.size;
        staging_it += b.size / sizeof(Face);
      }
      else {
        auto const f{create_chunk({offset_.x + side_, offset_.y + i})};
        std::ranges::copy(f, staging_it);
        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             gsl::narrow_cast<long long>(std::size(f) * sizeof(Face)),
             staging_buffer.data},
            buffers_[i]);
        buffers_[i].size = std::size(f) * sizeof(Face);
        staging_it += std::size(f);
      }
    }

    for (auto i{0}; i < side_ - 1; ++i)
      for (auto j{0}; j < side_; ++j)
        std::swap(buffers_[i * side_ + j], buffers_[(i + 1) * side_ + j]);

    for (auto i{0}; i < 2; ++i) {
      auto [f, m, t]{create_hot_chunk(
          {offset_.x + side_ / 2 + 1, offset_.y + side_ / 2 - 1 + i})};
      std::ranges::copy(
          f,
          static_cast<Face*>(
              buffers_[side_ * ((side_ - 1) / 2 + 1) + (side_ - 1) / 2 + i]
                  .data));
      buffers_[side_ * ((side_ - 1) / 2 + 1) + (side_ - 1) / 2 + i].size =
          std::size(f) * sizeof(Face);

      Vector<unsigned> free;
      if (auto it{mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 + 1) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 - 1 + i))};
          it != std::end(mods_))
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(
                buffers_[side_ * ((side_ - 1) / 2 + 1) + (side_ - 1) / 2 + i],
                m,
                free,
                block,
                face,
                pos);
            break;
          case Operation::destroy:
            destroy_face(
                buffers_[side_ * ((side_ - 1) / 2 + 1) + (side_ - 1) / 2 + i],
                m,
                free,
                face,
                pos);
            break;
          }

      if (auto it{block_mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 + 1) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 - 1 + i))};
          it != std::end(block_mods_))
        for (auto&& [block, pos] : it->second)
          t[pos.x][pos.z][pos.y] = block;

      terrains_[0][i] = std::move(terrains_[1][i]);
      maps_[0][i] = std::move(maps_[1][i]);
      free_lists_[0][i] = std::move(free_lists_[1][i]);

      terrains_[1][i] = std::move(t);
      maps_[1][i] = std::move(m);
      free_lists_[i][0] = std::move(free);
    }

    ++offset_.x;
    return true;
  }
  else if (position.x < offset_.x + side_ / 2) {
    for (auto i{0}; i < side_; ++i) {
      if (auto it{mods_.find(static_cast<uint64_t>(offset_.x - 1) << 32
                             | static_cast<uint64_t>(offset_.y + i))};
          it != std::end(mods_)) {
        auto [f, m, t]{create_hot_chunk({offset_.x - 1, offset_.y + i})};
        std::ranges::copy(f, staging_it);

        Buffer b{nullptr,
                 0,
                 static_cast<int64_t>(std::size(f) * sizeof(Face)),
                 static_cast<Face*>(staging_buffer.data)
                     + (staging_it - std::begin(staging_view))};
        Vector<unsigned> free;
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(b, m, free, block, face, pos);
            break;
          case Operation::destroy:
            destroy_face(b, m, free, face, pos);
            break;
          }

        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             b.size,
             staging_buffer.data},
            buffers_[(side_ - 1) * side_ + i]);
        buffers_[(side_ - 1) * side_ + i].size = b.size;
        staging_it += b.size / sizeof(Face);
      }
      else {
        auto const f{create_chunk({offset_.x - 1, offset_.y + i})};
        std::ranges::copy(f, staging_it);
        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             gsl::narrow_cast<long long>(std::size(f) * sizeof(Face)),
             staging_buffer.data},
            buffers_[(side_ - 1) * side_ + i]);
        buffers_[(side_ - 1) * side_ + i].size = std::size(f) * sizeof(Face);
        staging_it += std::size(f);
      }
    }
    for (auto i{side_ - 1}; i > 0; --i)
      for (auto j{0}; j < side_; ++j)
        std::swap(buffers_[i * side_ + j], buffers_[(i - 1) * side_ + j]);

    for (auto i{0}; i < 2; ++i) {
      auto [f, m, t]{create_hot_chunk(
          {offset_.x + side_ / 2 - 2, offset_.y + side_ / 2 - 1 + i})};
      std::ranges::copy(
          f,
          static_cast<Face*>(
              buffers_[side_ * ((side_ - 1) / 2) + (side_ - 1) / 2 + i].data));
      buffers_[side_ * ((side_ - 1) / 2) + (side_ - 1) / 2 + i].size =
          std::size(f) * sizeof(Face);

      Vector<unsigned> free;
      if (auto it{mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 - 2) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 - 1 + i))};
          it != std::end(mods_))
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(
                buffers_[side_ * ((side_ - 1) / 2) + (side_ - 1) / 2 + i],
                m,
                free,
                block,
                face,
                pos);
            break;
          case Operation::destroy:
            destroy_face(
                buffers_[side_ * ((side_ - 1) / 2) + (side_ - 1) / 2 + i],
                m,
                free,
                face,
                pos);
            break;
          }

      if (auto it{block_mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 - 2) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 - 1 + i))};
          it != std::end(block_mods_))
        for (auto&& [block, pos] : it->second)
          t[pos.x][pos.z][pos.y] = block;

      terrains_[1][i] = std::move(terrains_[0][i]);
      maps_[1][i] = std::move(maps_[0][i]);
      free_lists_[1][i] = std::move(free_lists_[0][i]);

      terrains_[0][i] = std::move(t);
      maps_[0][i] = std::move(m);
      free_lists_[i][0] = std::move(free);
    }

    --offset_.x;
    return true;
  }
  else if (position.y > offset_.y + side_ / 2) {
    for (auto i{0}; i < side_; ++i) {
      if (auto it{mods_.find(static_cast<uint64_t>(offset_.x + i) << 32
                             | static_cast<uint64_t>(offset_.y + side_))};
          it != std::end(mods_)) {
        auto [f, m, t]{create_hot_chunk({offset_.x + i, offset_.y + side_})};
        std::ranges::copy(f, staging_it);

        Buffer b{nullptr,
                 0,
                 static_cast<int64_t>(std::size(f) * sizeof(Face)),
                 static_cast<Face*>(staging_buffer.data)
                     + (staging_it - std::begin(staging_view))};
        Vector<unsigned> free;
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(b, m, free, block, face, pos);
            break;
          case Operation::destroy:
            destroy_face(b, m, free, face, pos);
            break;
          }

        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             b.size,
             staging_buffer.data},
            buffers_[i * side_]);
        buffers_[i * side_].size = b.size;
        staging_it += b.size / sizeof(Face);
      }
      else {
        auto const f{create_chunk({offset_.x + i, offset_.y + side_})};
        std::ranges::copy(f, staging_it);
        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             gsl::narrow_cast<long long>(std::size(f) * sizeof(Face)),
             staging_buffer.data},
            buffers_[i * side_]);
        buffers_[i * side_].size = std::size(f) * sizeof(Face);
        staging_it += std::size(f);
      }
    }
    for (auto i{0}; i < side_; ++i)
      for (auto j{0}; j < side_ - 1; ++j)
        std::swap(buffers_[i * side_ + j], buffers_[i * side_ + j + 1]);

    for (auto i{0}; i < 2; ++i) {
      auto [f, m, t]{create_hot_chunk(
          {offset_.x + side_ / 2 - 1 + i, offset_.y + side_ / 2 + 1})};
      std::ranges::copy(
          f,
          static_cast<Face*>(
              buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + 1]
                  .data));
      buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + 1].size =
          std::size(f) * sizeof(Face);

      Vector<unsigned> free;
      if (auto it{mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 - 1 + i) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 + 1))};
          it != std::end(mods_))
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(
                buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + 1],
                m,
                free,
                block,
                face,
                pos);
            break;
          case Operation::destroy:
            destroy_face(
                buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + 1],
                m,
                free,
                face,
                pos);
            break;
          }

      if (auto it{block_mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 - 1 + i) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 + 1))};
          it != std::end(block_mods_))
        for (auto&& [block, pos] : it->second)
          t[pos.x][pos.z][pos.y] = block;

      terrains_[i][0] = std::move(terrains_[i][1]);
      maps_[i][0] = std::move(maps_[i][1]);
      free_lists_[i][0] = std::move(free_lists_[i][1]);

      terrains_[i][1] = std::move(t);
      maps_[i][1] = std::move(m);
      free_lists_[i][1] = std::move(free);
    }

    ++offset_.y;
    return true;
  }
  else if (position.y < offset_.y + side_ / 2) {
    for (auto i{0}; i < side_; ++i) {
      if (auto it{mods_.find(static_cast<uint64_t>(offset_.x + i) << 32
                             | static_cast<uint64_t>(offset_.y - 1))};
          it != std::end(mods_)) {
        auto [f, m, t]{create_hot_chunk({offset_.x + i, offset_.y - 1})};
        std::ranges::copy(f, staging_it);

        Buffer b{nullptr,
                 0,
                 static_cast<int64_t>(std::size(f) * sizeof(Face)),
                 static_cast<Face*>(staging_buffer.data)
                     + (staging_it - std::begin(staging_view))};
        Vector<unsigned> free;

        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(b, m, free, block, face, pos);
            break;
          case Operation::destroy:
            destroy_face(b, m, free, face, pos);
            break;
          }

        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             b.size,
             staging_buffer.data},
            buffers_[i * side_ + side_ - 1]);
        buffers_[i * side_ + side_ - 1].size = b.size;
        staging_it += b.size / sizeof(Face);
      }
      else {
        auto const f{create_chunk({offset_.x + i, offset_.y - 1})};
        std::ranges::copy(f, staging_it);
        copy_buffer(
            command,
            {staging_buffer.handle,
             gsl::narrow_cast<long long>(
                 staging_buffer.offset
                 + (staging_it - std::begin(staging_view)) * sizeof(Face)),
             gsl::narrow_cast<long long>(std::size(f) * sizeof(Face)),
             staging_buffer.data},
            buffers_[i * side_ + side_ - 1]);
        buffers_[i * side_ + side_ - 1].size = std::size(f) * sizeof(Face);
        staging_it += std::size(f);
      }
    }
    for (auto i{0}; i < side_; ++i)
      for (auto j{side_ - 1}; j > 0; --j)
        std::swap(buffers_[i * side_ + j], buffers_[i * side_ + j - 1]);

    for (auto i{0}; i < 2; ++i) {
      auto [f, m, t]{create_hot_chunk(
          {offset_.x + side_ / 2 - 1 + i, offset_.y + side_ / 2 - 2})};
      std::ranges::copy(
          f,
          static_cast<Face*>(
              buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2].data));
      buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2].size =
          std::size(f) * sizeof(Face);

      Vector<unsigned> free;
      if (auto it{mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 - 1 + i) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 - 2))};
          it != std::end(mods_))
        for (auto&& [op, block, face, pos] : it->second)
          switch (op) {
          case Operation::place:
            create_face(
                buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2],
                m,
                free,
                block,
                face,
                pos);
            break;
          case Operation::destroy:
            destroy_face(
                buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2],
                m,
                free,
                face,
                pos);
            break;
          }

      if (auto it{block_mods_.find(
              static_cast<uint64_t>(offset_.x + side_ / 2 - 1 + i) << 32
              | static_cast<uint64_t>(offset_.y + side_ / 2 - 2))};
          it != std::end(block_mods_))
        for (auto&& [block, pos] : it->second)
          t[pos.x][pos.z][pos.y] = block;

      terrains_[i][1] = std::move(terrains_[i][0]);
      maps_[i][1] = std::move(maps_[i][0]);
      free_lists_[i][1] = std::move(free_lists_[i][0]);

      terrains_[i][0] = std::move(t);
      maps_[i][0] = std::move(m);
      free_lists_[i][0] = std::move(free);
    }

    --offset_.y;
    return true;
  }

  return false;
}

void World::place_block_help(int i,
                             int j,
                             BlockType block,
                             FaceType face,
                             glm::ivec3 pos)
{
  /*
  if (block == BlockType::glass) {
    create_face(i, j, block, FaceType::up, {pos.x, pos.y, pos.z});
    create_face(i, j, block, FaceType::up, {pos.x, pos.y + 1, pos.z});

    create_face(i, j, block, FaceType::down, {pos.x, pos.y, pos.z});
    create_face(i, j, block, FaceType::down, {pos.x, pos.y - 1, pos.z});

    create_face(i, j, block, FaceType::left, {pos.x, pos.y, pos.z});
    create_face(i, j, block, FaceType::left, {pos.x + 1, pos.y, pos.z});

    create_face(i, j, block, FaceType::right, {pos.x, pos.y, pos.z});
    create_face(i, j, block, FaceType::right, {pos.x - 1, pos.y, pos.z});

    create_face(i, j, block, FaceType::front, {pos.x, pos.y, pos.z});
    create_face(i, j, block, FaceType::front, {pos.x, pos.y, pos.z + 1});

    create_face(i, j, block, FaceType::back, {pos.x, pos.y, pos.z});
    create_face(i, j, block, FaceType::back, {pos.x, pos.y, pos.z - 1});
    return;
  }
  */

  // up
  if (pos.y > 0)
    if (block != BlockType::glass
        && terrains_[i][j][pos.x][pos.z][pos.y - 1] != BlockType::air
        && terrains_[i][j][pos.x][pos.z][pos.y - 1] != BlockType::glass)
      destroy_face(i, j, FaceType::down, {pos.x, pos.y - 1, pos.z});
    else
      create_face(i, j, block, FaceType::up, {pos.x, pos.y, pos.z});

  // down
  if (block != BlockType::glass
      && terrains_[i][j][pos.x][pos.z][pos.y + 1] != BlockType::air
      && terrains_[i][j][pos.x][pos.z][pos.y - 1] != BlockType::glass)
    destroy_face(i, j, FaceType::up, {pos.x, pos.y + 1, pos.z});
  else
    create_face(i, j, block, FaceType::down, {pos.x, pos.y, pos.z});

  // right
  if (auto b{pos.x + 1 != chunk_width ? terrains_[i][j][pos.x + 1][pos.z][pos.y]
                                      : terrains_[i + 1][j][0][pos.z][pos.y]};
      block == BlockType::glass || b == BlockType::air || b == BlockType::glass)
    create_face(i, j, block, FaceType::right, {pos.x, pos.y, pos.z});
  else
    destroy_face(i, j, FaceType::left, {pos.x + 1, pos.y, pos.z});

  // back
  if (auto b{pos.z + 1 != chunk_depth ? terrains_[i][j][pos.x][pos.z + 1][pos.y]
                                      : terrains_[i][j + 1][pos.x][0][pos.y]};
      block == BlockType::glass || b == BlockType::air || b == BlockType::glass)
    create_face(i, j, block, FaceType::back, {pos.x, pos.y, pos.z});
  else
    destroy_face(i, j, FaceType::front, {pos.x, pos.y, pos.z + 1});

  // left
  if (pos.x != 0)
    if (block == BlockType::glass
        || terrains_[i][j][pos.x - 1][pos.z][pos.y] == BlockType::air
        || terrains_[i][j][pos.x - 1][pos.z][pos.y] == BlockType::glass)
      create_face(i, j, block, FaceType::left, {pos.x, pos.y, pos.z});
    else
      destroy_face(i, j, FaceType::right, {pos.x - 1, pos.y, pos.z});
  else if (block == BlockType::glass
           || terrains_[i - 1][j][chunk_width - 1][pos.z][pos.y]
                  == BlockType::air
           || terrains_[i - 1][j][chunk_width - 1][pos.z][pos.y]
                  == BlockType::glass)
    create_face(i - 1, j, block, FaceType::left, {chunk_width, pos.y, pos.z});
  else
    destroy_face(i - 1, j, FaceType::right, {chunk_width - 1, pos.y, pos.z});

  // front
  if (pos.z != 0)
    if (block == BlockType::glass
        || terrains_[i][j][pos.x][pos.z - 1][pos.y] == BlockType::air
        || terrains_[i][j][pos.x][pos.z - 1][pos.y] == BlockType::glass)
      create_face(i, j, block, FaceType::front, {pos.x, pos.y, pos.z});
    else
      destroy_face(i, j, FaceType::back, {pos.x, pos.y, pos.z - 1});
  else if (block == BlockType::glass
           || terrains_[i][j - 1][pos.x][chunk_depth - 1][pos.y]
                  == BlockType::air
           || terrains_[i][j - 1][pos.x][chunk_depth - 1][pos.y]
                  == BlockType::glass)
    create_face(i, j - 1, block, FaceType::front, {pos.x, pos.y, chunk_depth});
  else
    destroy_face(i, j - 1, FaceType::back, {pos.x, pos.y, chunk_depth - 1});
}

void World::destroy_block_help(int i, int j, FaceType face, glm::ivec3 pos)
{
  // destory up
  if (pos.y > 0)
    if (terrains_[i][j][pos.x][pos.z][pos.y - 1] == BlockType::air)
      destroy_face(i, j, FaceType::up, {pos.x, pos.y, pos.z});
    else
      create_face(i,
                  j,
                  terrains_[i][j][pos.x][pos.z][pos.y - 1],
                  FaceType::down,
                  {pos.x, pos.y - 1, pos.z});

  // destroy down
  if (terrains_[i][j][pos.x][pos.z][pos.y + 1] == BlockType::air)
    destroy_face(i, j, FaceType::down, {pos.x, pos.y, pos.z});
  else
    create_face(i,
                j,
                terrains_[i][j][pos.x][pos.z][pos.y + 1],
                FaceType::up,
                {pos.x, pos.y + 1, pos.z});

  // destroy right
  if (auto b{pos.x + 1 != chunk_width ? terrains_[i][j][pos.x + 1][pos.z][pos.y]
                                      : terrains_[i + 1][j][0][pos.z][pos.y]};
      b == BlockType::air)
    destroy_face(i, j, FaceType::right, {pos.x, pos.y, pos.z});
  else
    create_face(i, j, b, FaceType::left, {pos.x + 1, pos.y, pos.z});

  // destroy back
  if (auto b{pos.z + 1 != chunk_depth ? terrains_[i][j][pos.x][pos.z + 1][pos.y]
                                      : terrains_[i][j + 1][pos.x][0][pos.y]};
      b == BlockType::air)
    destroy_face(i, j, FaceType::back, {pos.x, pos.y, pos.z});
  else
    create_face(i, j, b, FaceType::front, {pos.x, pos.y, pos.z + 1});

  // destroy left
  if (pos.x != 0)
    if (terrains_[i][j][pos.x - 1][pos.z][pos.y] == BlockType::air)
      destroy_face(i, j, FaceType::left, {pos.x, pos.y, pos.z});
    else
      create_face(i,
                  j,
                  terrains_[i][j][pos.x - 1][pos.z][pos.y],
                  FaceType::right,
                  {pos.x - 1, pos.y, pos.z});
  else if (terrains_[i - 1][j][chunk_width - 1][pos.z][pos.y] == BlockType::air)
    destroy_face(i - 1, j, FaceType::left, {chunk_width, pos.y, pos.z});
  else
    create_face(i - 1,
                j,
                terrains_[i - 1][j][chunk_width - 1][pos.z][pos.y],
                FaceType::right,
                {chunk_width - 1, pos.y, pos.z});

  // destroy front
  if (pos.z != 0)
    if (terrains_[i][j][pos.x][pos.z - 1][pos.y] == BlockType::air)
      destroy_face(i, j, FaceType::front, {pos.x, pos.y, pos.z});
    else
      create_face(i,
                  j,
                  terrains_[i][j][pos.x][pos.z - 1][pos.y],
                  FaceType::back,
                  {pos.x, pos.y, pos.z - 1});
  else if (terrains_[i][j - 1][pos.x][chunk_depth - 1][pos.y] == BlockType::air)
    destroy_face(i, j - 1, FaceType::front, {pos.x, pos.y, chunk_depth});
  else
    create_face(i,
                j - 1,
                terrains_[i][j - 1][pos.x][chunk_depth - 1][pos.y],
                FaceType::back,
                {pos.x, pos.y, chunk_depth - 1});
}

void World::create_face(int i,
                        int j,
                        BlockType block,
                        FaceType face,
                        glm::ivec3 pos)
{
  mods_[static_cast<uint64_t>(offset_.x + (side_ - 1) / 2 + i) << 32
        | static_cast<uint64_t>(offset_.y + (side_ - 1) / 2 + j)]
      .push_back(
          {Operation::place, block, face, static_cast<glm::u8vec3>(pos)});

  auto ptr{static_cast<Face*>(
      buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j].data)};
  if (free_lists_[i][j].empty()) {
    ptr[buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j].size
        / sizeof(Face)] = get_vertices(block, face, pos);
    maps_[i][j][pack_face_key(face, pos)] =
        buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j].size
        / sizeof(Face);
    buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j].size +=
        sizeof(Face);
  }
  else {
    ptr[free_lists_[i][j].back()] = get_vertices(block, face, pos);
    maps_[i][j][pack_face_key(face, pos)] = free_lists_[i][j].back();
    free_lists_[i][j].pop_back();
  }
}

void World::create_face(Buffer& buffer,
                        HashMap<unsigned, unsigned>& map,
                        Vector<unsigned>& free,
                        BlockType block,
                        FaceType face,
                        glm::ivec3 pos)
{
  auto ptr{static_cast<Face*>(buffer.data)};
  if (free.empty()) {
    ptr[buffer.size / sizeof(Face)] = get_vertices(block, face, pos);
    map[pack_face_key(face, pos)] = buffer.size / sizeof(Face);
    buffer.size += sizeof(Face);
  }
  else {
    ptr[free.back()] = get_vertices(block, face, pos);
    map[pack_face_key(face, pos)] = free.back();
    free.pop_back();
  }
}

void World::destroy_face(int i, int j, FaceType face, glm::ivec3 pos)
{
  mods_[static_cast<uint64_t>(offset_.x + (side_ - 1) / 2 + i) << 32
        | static_cast<uint64_t>(offset_.y + (side_ - 1) / 2 + j)]
      .push_back({Operation::destroy, {}, face, static_cast<glm::u8vec3>(pos)});

  auto it{maps_[i][j].find(pack_face_key(face, pos))};
  auto ptr{static_cast<Face*>(
      buffers_[side_ * ((side_ - 1) / 2 + i) + (side_ - 1) / 2 + j].data)};
  ptr[it->second] = {};
  free_lists_[i][j].push_back(it->second);
  maps_[i][j].erase(it);
}

void World::destroy_face(Buffer& buffer,
                         HashMap<unsigned, unsigned>& map,
                         Vector<unsigned>& free,
                         FaceType face,
                         glm::ivec3 pos)
{
  if (auto it{map.find(pack_face_key(face, pos))}; it != std::end(map)) {
    auto ptr{static_cast<Face*>(buffer.data)};
    ptr[it->second] = {};
    free.push_back(it->second);
    map.erase(it);
  }
}
