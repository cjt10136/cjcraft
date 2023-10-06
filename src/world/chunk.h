#pragma once

#include "block.h"
#include "container/hash_map.h"
#include "glm/glm.hpp"

#include <array>
#include <tuple>
#include <vector>

inline constexpr auto chunk_height{255};
inline constexpr auto chunk_depth{63};
inline constexpr auto chunk_width{63};

inline constexpr auto first_layer_height{1};
inline constexpr auto second_layer_height{4};
inline constexpr auto bedrock_layer_height{4};

using Terrain = std::vector<
    std::array<std::array<BlockType, chunk_height>, chunk_depth + 1>>;
using HeightMap = std::vector<std::array<float, chunk_depth + 1>>;

std::vector<Face> create_chunk(glm::ivec2 offset);
std::tuple<std::vector<Face>, HashMap<unsigned, unsigned>, Terrain>
create_hot_chunk(glm::ivec2 offset);

HeightMap generate_height_map(glm::ivec2 offset);
