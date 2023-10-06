#include "chunk.h"

#include <algorithm>

namespace {
  constexpr std::pair<std::array<BlockType, 3>, int>
  get_types_and_height(float height) noexcept;

  void fill_side_faces(std::vector<Face>& faces,
                       std::array<BlockType, 3> types,
                       FaceType face,
                       int height_begin,
                       int height_end,
                       glm::ivec2 pos);

  void fill_hot_side_faces(std::vector<Face>& faces,
                           HashMap<unsigned, unsigned>& map,
                           std::array<BlockType, 3> types,
                           FaceType face,
                           int height_begin,
                           int height_end,
                           glm::ivec2 pos);
} // namespace

std::vector<Face> create_chunk(glm::ivec2 offset)
{
  auto const height_map{
      generate_height_map({chunk_width * offset.x, chunk_depth * offset.y})};
  std::vector<Face> faces;
  for (auto i{0}; i < chunk_width; ++i)
    for (auto j{0}; j < chunk_depth; ++j) {
      auto const [types, height]{get_types_and_height(height_map[i][j])};
      faces.push_back(get_vertices(types[0], FaceType::up, glm::ivec3{i, height, j}));

      if (auto const [back_types,
                      back_height]{get_types_and_height(height_map[i][j + 1])};
          height < back_height)
        fill_side_faces(
            faces, types, FaceType::back, height, back_height, {i, j});
      else
        fill_side_faces(faces,
                        back_types,
                        FaceType::front,
                        back_height,
                        height,
                        {i, j + 1});

      if (auto const [right_types,
                      right_height]{get_types_and_height(height_map[i + 1][j])};
          height < right_height)
        fill_side_faces(
            faces, types, FaceType::right, height, right_height, {i, j});
      else
        fill_side_faces(faces,
                        right_types,
                        FaceType::left,
                        right_height,
                        height,
                        {i + 1, j});
    }
  return faces;
}

std::tuple<std::vector<Face>, HashMap<unsigned, unsigned>, Terrain>
create_hot_chunk(glm::ivec2 offset)
{
  auto const height_map{
      generate_height_map({chunk_width * offset.x, chunk_depth * offset.y})};
  std::vector<Face> faces;
  HashMap<unsigned, unsigned> map;
  Terrain terrain(chunk_width + 1);
  for (auto i{0}; i < chunk_width + 1; ++i)
    for (auto j{0}; j < chunk_depth + 1; ++j) {
      auto const [types, height]{get_types_and_height(height_map[i][j])};
      std::fill_n(
          std::begin(terrain[i][j]) + height, first_layer_height, types[0]);
      std::fill_n(std::begin(terrain[i][j]) + height + first_layer_height,
                  second_layer_height,
                  types[1]);
      std::fill(std::begin(terrain[i][j]) + height + first_layer_height
                    + second_layer_height,
                std::end(terrain[i][j]) - bedrock_layer_height,
                types[2]);
      std::fill(std::end(terrain[i][j]) - bedrock_layer_height,
                std::end(terrain[i][j]),
                BlockType::bedrock);
      if (i == chunk_width || j == chunk_depth)
        continue;

      map[pack_face_key(FaceType::up, {i, height, j})] = std::size(faces);
      faces.push_back(
          get_vertices(types[0], FaceType::up, glm::ivec3{i, height, j}));

      if (auto const [back_types,
                      back_height]{get_types_and_height(height_map[i][j + 1])};
          height < back_height)
        fill_hot_side_faces(
            faces, map, types, FaceType::back, height, back_height, {i, j});
      else
        fill_hot_side_faces(faces,
                            map,
                            back_types,
                            FaceType::front,
                            back_height,
                            height,
                            {i, j + 1});

      if (auto const [right_types,
                      right_height]{get_types_and_height(height_map[i + 1][j])};
          height < right_height)
        fill_hot_side_faces(
            faces, map, types, FaceType::right, height, right_height, {i, j});
      else
        fill_hot_side_faces(faces,
                            map,
                            right_types,
                            FaceType::left,
                            right_height,
                            height,
                            {i + 1, j});
    }
  return {faces, map, terrain};
}

namespace {
  constexpr std::pair<std::array<BlockType, 3>, int>
  get_types_and_height(float height) noexcept
  {
    if (auto th{gsl::narrow_cast<int>(std::clamp(height, -0.7f, 0.1f) * 150)
                + 185};
        th >= 200)
      return {{BlockType::water, BlockType::water, BlockType::water}, th};
    else if (th >= 194)
      return {{BlockType::sand, BlockType::sand, BlockType::stone}, th};
    else if (th >= 144)
      return {{BlockType::grass, BlockType::dirt, BlockType::stone}, th};
    else
      return {{BlockType::snow, BlockType::dirt, BlockType::stone}, th};
  }

  void fill_side_faces(std::vector<Face>& faces,
                       std::array<BlockType, 3> types,
                       FaceType face,
                       int height_begin,
                       int height_end,
                       glm::ivec2 pos)
  {
    for (auto k{0}; height_begin < height_end && k < first_layer_height;
         ++height_begin, ++k)
      faces.push_back(
          get_vertices(types[0], face, glm::ivec3{pos.x, height_begin, pos.y}));
    for (auto k{0}; height_begin < height_end && k < second_layer_height;
         ++height_begin, ++k)
      faces.push_back(
          get_vertices(types[1], face, glm::ivec3{pos.x, height_begin, pos.y}));
    while (height_begin < height_end) {
      faces.push_back(
          get_vertices(types[2], face, glm::ivec3{pos.x, height_begin, pos.y}));
      ++height_begin;
    }
  }

  void fill_hot_side_faces(std::vector<Face>& faces,
                           HashMap<unsigned, unsigned>& map,
                           std::array<BlockType, 3> types,
                           FaceType face,
                           int height_begin,
                           int height_end,
                           glm::ivec2 pos)
  {
    for (auto k{0}; height_begin < height_end && k < first_layer_height;
         ++height_begin, ++k) {
      map[pack_face_key(face, glm::ivec3{pos.x, height_begin, pos.y})] =
          std::size(faces);
      faces.push_back(
          get_vertices(types[0], face, glm::ivec3{pos.x, height_begin, pos.y}));
    }
    for (auto k{0}; height_begin < height_end && k < second_layer_height;
         ++height_begin, ++k) {
      map[pack_face_key(face, glm::ivec3{pos.x, height_begin, pos.y})] =
          std::size(faces);
      faces.push_back(
          get_vertices(types[1], face, glm::ivec3{pos.x, height_begin, pos.y}));
    }
    while (height_begin < height_end) {
      map[pack_face_key(face, glm::ivec3{pos.x, height_begin, pos.y})] =
          std::size(faces);
      faces.push_back(
          get_vertices(types[2], face, glm::ivec3{pos.x, height_begin, pos.y}));
      ++height_begin;
    }
  }
} // namespace
