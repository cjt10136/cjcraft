#pragma once

#include "container/hash_map.h"
#include "resource/buffer.h"

#include <array>
#include <gsl/gsl>
#include <utility>

enum class BlockType : unsigned char {
  air,
  leaves,
  glass,
  bedrock,
  water,
  sand,
  stone,
  dirt,
  grass,
  snow,
  log,
  wood,
  cobble
};

enum class FaceType : unsigned char { up, down, left, right, front, back };

struct Face {
  static constexpr auto nb_vertices{6};
  std::array<Vertex, nb_vertices> data{};
};

struct FaceKey {
  unsigned value;
};

constexpr bool operator==(FaceKey const& lhs, FaceKey const& rhs)
{
  return lhs.value == rhs.value;
}

struct FaceKeyHash {
  using is_avalanching = void;

  unsigned long long operator()(FaceKey const& k) const noexcept
  {
    return ankerl::unordered_dense::detail::wyhash::hash(k.value);
  }
};

constexpr Vertex pack_vertex(glm::ivec3 pos, glm::ivec2 uv) noexcept
{
  return {(gsl::narrow_cast<unsigned>(pos.y) << 24)
          | (gsl::narrow_cast<unsigned>(pos.x) << 16)
          | (gsl::narrow_cast<unsigned>(pos.z) << 8)
          | (gsl::narrow_cast<unsigned>(uv.y) << 4)
          | (gsl::narrow_cast<unsigned>(uv.x))};
}

constexpr Face
get_vertices(BlockType block, FaceType face, glm::ivec3 pos) noexcept
{
  glm::ivec2 uv_up;
  glm::ivec2 uv_down;
  glm::ivec2 uv_side;
  switch (block) {
  case BlockType::air:
    return {};
  case BlockType::glass:
    uv_up = uv_down = uv_side = {1, 3};
    break;
  case BlockType::bedrock:
    uv_up = uv_down = uv_side = {1, 1};
    break;
  case BlockType::water:
    uv_up = uv_down = uv_side = {14, 13};
    break;
  case BlockType::sand:
    uv_up = uv_down = uv_side = {2, 1};
    break;
  case BlockType::stone:
    uv_up = uv_down = uv_side = {1, 0};
    break;
  case BlockType::dirt:
    uv_up = uv_down = uv_side = {2, 0};
    break;
  case BlockType::grass:
    uv_up = {0, 0};
    uv_down = {2, 0};
    uv_side = {3, 0};
    break;
  case BlockType::snow:
    uv_up = {2, 4};
    uv_down = {2, 0};
    uv_side = {4, 4};
    break;
  case BlockType::log:
    uv_up = uv_down = {5, 1};
    uv_side = {4, 1};
    break;
  case BlockType::leaves:
    uv_up = uv_down = uv_side = {4, 3};
    break;
  case BlockType::wood:
    uv_up = uv_down = uv_side = {4, 0};
    break;
  case BlockType::cobble:
    uv_up = uv_down = uv_side = {0, 1};
    break;
  }

  constexpr std::array<glm::ivec2, 4> uv_offsets{
      {{0, 0}, {0, 1}, {1, 1}, {1, 0}}};
  switch (face) {
  case FaceType::up:
    return {pack_vertex(pos + glm::ivec3{0, 0, 1}, uv_up + uv_offsets[0]),
            pack_vertex(pos + glm::ivec3{0, 0, 0}, uv_up + uv_offsets[1]),
            pack_vertex(pos + glm::ivec3{1, 0, 0}, uv_up + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 0, 0}, uv_up + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 0, 1}, uv_up + uv_offsets[3]),
            pack_vertex(pos + glm::ivec3{0, 0, 1}, uv_up + uv_offsets[0])};
  case FaceType::down:
    return {pack_vertex(pos + glm::ivec3{0, 1, 0}, uv_down + uv_offsets[0]),
            pack_vertex(pos + glm::ivec3{0, 1, 1}, uv_down + uv_offsets[1]),
            pack_vertex(pos + glm::ivec3{1, 1, 1}, uv_down + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 1, 1}, uv_down + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 1, 0}, uv_down + uv_offsets[3]),
            pack_vertex(pos + glm::ivec3{0, 1, 0}, uv_down + uv_offsets[0])};
  case FaceType::front:
    return {pack_vertex(pos + glm::ivec3{0, 0, 0}, uv_side + uv_offsets[0]),
            pack_vertex(pos + glm::ivec3{0, 1, 0}, uv_side + uv_offsets[1]),
            pack_vertex(pos + glm::ivec3{1, 1, 0}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 1, 0}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 0, 0}, uv_side + uv_offsets[3]),
            pack_vertex(pos + glm::ivec3{0, 0, 0}, uv_side + uv_offsets[0])};
  case FaceType::back:
    return {pack_vertex(pos + glm::ivec3{1, 0, 1}, uv_side + uv_offsets[0]),
            pack_vertex(pos + glm::ivec3{1, 1, 1}, uv_side + uv_offsets[1]),
            pack_vertex(pos + glm::ivec3{0, 1, 1}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{0, 1, 1}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{0, 0, 1}, uv_side + uv_offsets[3]),
            pack_vertex(pos + glm::ivec3{1, 0, 1}, uv_side + uv_offsets[0])};
  case FaceType::left:
    return {pack_vertex(pos + glm::ivec3{0, 0, 1}, uv_side + uv_offsets[0]),
            pack_vertex(pos + glm::ivec3{0, 1, 1}, uv_side + uv_offsets[1]),
            pack_vertex(pos + glm::ivec3{0, 1, 0}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{0, 1, 0}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{0, 0, 0}, uv_side + uv_offsets[3]),
            pack_vertex(pos + glm::ivec3{0, 0, 1}, uv_side + uv_offsets[0])};
  case FaceType::right:
    return {pack_vertex(pos + glm::ivec3{1, 0, 0}, uv_side + uv_offsets[0]),
            pack_vertex(pos + glm::ivec3{1, 1, 0}, uv_side + uv_offsets[1]),
            pack_vertex(pos + glm::ivec3{1, 1, 1}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 1, 1}, uv_side + uv_offsets[2]),
            pack_vertex(pos + glm::ivec3{1, 0, 1}, uv_side + uv_offsets[3]),
            pack_vertex(pos + glm::ivec3{1, 0, 0}, uv_side + uv_offsets[0])};
  }
  return {};
}

constexpr unsigned pack_face_key(FaceType t, glm::ivec3 pos) noexcept
{
  return {(static_cast<unsigned>(t) << 24)
          | (gsl::narrow_cast<unsigned>(pos.x) << 16)
          | (gsl::narrow_cast<unsigned>(pos.y) << 8)
          | (gsl::narrow_cast<unsigned>(pos.z))};
}
