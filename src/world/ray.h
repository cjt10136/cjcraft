#pragma once

#include "block.h"
#include "container/vector.h"

#include <utility>
#include <vector>

inline constexpr auto reach_distance{5};
inline constexpr auto max_faces_can_see{3};

Vector<std::pair<glm::ivec3, FaceType>> cast_ray(glm::vec3 position,
                                                 glm::vec3 direction) noexcept;
