#pragma once

#include "chunk.h"
#include "control/camera.h"
#include "memory/buffer_manager.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

enum struct Operation : unsigned char {
  place,
  destroy
};

struct alignas(8) FaceMod {
  Operation op;
  BlockType block;
  FaceType face;
  glm::u8vec3 pos;
};

struct BlockMod {
  BlockType block;
  glm::u8vec3 pos;
};

class World {
public:
  World(vk::CommandBuffer command, Buffer staging_buffer, int render_distance);

  std::tuple<bool, bool, bool, bool> hit_wall(Camera& camera)
  {
    auto pos{glm::ivec3{gsl::narrow_cast<int>(camera.pos_.x),
                        gsl::narrow_cast<int>(camera.pos_.y),
                        gsl::narrow_cast<int>(camera.pos_.z)}};
    auto hot_chunk_i{0};
    if (pos.x >= offset_.x * chunk_width + chunk_width * (side_ / 2)) {
      hot_chunk_i = 1;
      pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2);
    }
    else
      pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2 - 1);
    auto hot_chunk_j{0};
    if (pos.z >= offset_.y * chunk_depth + chunk_depth * (side_ / 2)) {
      hot_chunk_j = 1;
      pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2);
    }
    else
      pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2 - 1);

    auto b1{false};
    auto b2{false};
    auto b3{false};
    auto b4{false};
    if (terrains_[hot_chunk_i][hot_chunk_j][pos.x + 1][pos.z][pos.y]
            == BlockType::air
        && terrains_[hot_chunk_i][hot_chunk_j][pos.x + 1][pos.z][pos.y + 1]
               == BlockType::air)
      b1 = true;
    if (terrains_[hot_chunk_i][hot_chunk_j][pos.x][pos.z + 1][pos.y]
            == BlockType::air
        && terrains_[hot_chunk_i][hot_chunk_j][pos.x][pos.z + 1][pos.y + 1]
               == BlockType::air)
      b2 = true;
    if (pos.x != 0) {
      if (terrains_[hot_chunk_i][hot_chunk_j][pos.x - 1][pos.z][pos.y]
              == BlockType::air
          && terrains_[hot_chunk_i][hot_chunk_j][pos.x - 1][pos.z][pos.y + 1]
                 == BlockType::air)
        b3 = true;
    }
    else if (terrains_[hot_chunk_i - 1][hot_chunk_j][chunk_width - 1][pos.z]
                      [pos.y]
                 == BlockType::air
             && terrains_[hot_chunk_i - 1][hot_chunk_j][chunk_width - 1][pos.z]
                         [pos.y + 1]
                    == BlockType::air)
      b3 = true;
    if (pos.z != 0) {
      if (terrains_[hot_chunk_i][hot_chunk_j][pos.x][pos.z - 1][pos.y]
              == BlockType::air
          && terrains_[hot_chunk_i][hot_chunk_j][pos.x][pos.z - 1][pos.y + 1]
                 == BlockType::air)
        b4 = true;
    }
    else if (terrains_[hot_chunk_i][hot_chunk_j - 1][pos.x][chunk_depth - 1]
                      [pos.y]
                 == BlockType::air
             && terrains_[hot_chunk_i][hot_chunk_j - 1][pos.x][chunk_depth - 1]
                         [pos.y + 1]
                    == BlockType::air)
      b4 = true;
    return {b1, b2, b3, b4};
  }

  bool fix_camera(Camera& camera)
  {
    auto pos{glm::ivec3{gsl::narrow_cast<int>(camera.pos_.x),
                        gsl::narrow_cast<int>(camera.pos_.y + 1.75),
                        gsl::narrow_cast<int>(camera.pos_.z)}};
    auto hot_chunk_i{0};
    if (pos.x >= offset_.x * chunk_width + chunk_width * (side_ / 2)) {
      hot_chunk_i = 1;
      pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2);
    }
    else
      pos.x -= offset_.x * chunk_width + chunk_width * (side_ / 2 - 1);
    auto hot_chunk_j{0};
    if (pos.z >= offset_.y * chunk_depth + chunk_depth * (side_ / 2)) {
      hot_chunk_j = 1;
      pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2);
    }
    else
      pos.z -= offset_.y * chunk_depth + chunk_depth * (side_ / 2 - 1);

    if (terrains_[hot_chunk_i][hot_chunk_j][pos.x][pos.z][pos.y]
        == BlockType::air)
      return false;
    while (terrains_[hot_chunk_i][hot_chunk_j][pos.x][pos.z][pos.y]
           != BlockType::air) {
      --pos.y;
    }
    camera.pos_.y = gsl::narrow_cast<float>(pos.y) - 0.75;
    return true;
  }

  bool place_block(BlockType block, FaceType face, glm::ivec3 pos);

  bool destroy_block(FaceType face, glm::ivec3 pos);

  // update
  bool
  move(vk::CommandBuffer command, Buffer staging_buffer, glm::ivec2 position);

  void draw(vk::CommandBuffer command,
            vk::PipelineLayout layout,
            glm::vec2 pos,
            glm::vec2 front) const;
private:
  void place_block_help(int i,
                        int j,
                        BlockType block,
                        FaceType face,
                        glm::ivec3 pos);
  void destroy_block_help(int i, int j, FaceType face, glm::ivec3 pos);

  void
  create_face(int i, int j, BlockType block, FaceType face, glm::ivec3 pos);
  void create_face(Buffer& buffer,
                   HashMap<unsigned, unsigned>& map,
                   Vector<unsigned>& free,
                   BlockType block,
                   FaceType face,
                   glm::ivec3 pos);

  void destroy_face(int i, int j, FaceType face, glm::ivec3 pos);
  void destroy_face(Buffer& buffer,
                    HashMap<unsigned, unsigned>& map,
                    Vector<unsigned>& free,
                    FaceType face,
                    glm::ivec3 pos);

  int side_{};
  glm::ivec2 offset_{};

  BufferManager buffer_manager_{};

  std::vector<Buffer> buffers_{};
  // std::array<Buffer, 2>* hot_buffers_{};

  std::array<std::array<Terrain, 2>, 2> terrains_{};
  std::array<std::array<HashMap<unsigned, unsigned>, 2>, 2> maps_{};
  std::array<std::array<Vector<unsigned>, 2>, 2> free_lists_;

  HashMap<uint64_t, Vector<FaceMod>> mods_;
  HashMap<uint64_t, Vector<BlockMod>> block_mods_;
};
