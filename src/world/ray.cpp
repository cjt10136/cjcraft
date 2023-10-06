#include "ray.h"

#include <algorithm>
#include <cmath>
#include <gsl/gsl>

Vector<std::pair<glm::ivec3, FaceType>> cast_ray(glm::vec3 position,
                                                 glm::vec3 direction) noexcept
{
  Vector<std::pair<glm::vec3, FaceType>> contacts;

  if (direction.x != 0) {
    auto const [d, round, face]{[&] {
      if (direction.x < 0)
        return std::tuple{
            -glm::vec3{1, direction.y / direction.x, direction.z / direction.x},
            std::floor(position.x),
            FaceType::right};
      else
        return std::tuple{
            glm::vec3{1, direction.y / direction.x, direction.z / direction.x},
            std::ceil(position.x),
            FaceType::left};
    }()};
    auto n{(direction.x < 0 ? -1 : 1) * (round - position.x) * d};
    for (auto i{0}; i < reach_distance + 1; ++i) {
      contacts.push_back({n, face});
      n += d;
    }
  }

  if (direction.y != 0) {
    auto const [d, round, face]{[&] {
      if (direction.y < 0)
        return std::tuple{
            -glm::vec3{direction.x / direction.y, 1, direction.z / direction.y},
            std::floor(position.y),
            FaceType::down};
      else
        return std::tuple{
            glm::vec3{direction.x / direction.y, 1, direction.z / direction.y},
            std::ceil(position.y),
            FaceType::up};
    }()};
    auto n{(direction.y < 0 ? -1 : 1) * (round - position.y) * d};
    for (auto i{0}; i < reach_distance; ++i) {
      contacts.push_back({n, face});
      n += d;
    }
  }

  if (direction.z != 0) {
    auto const [d, round, face]{[&] {
      if (direction.z < 0)
        return std::tuple{
            -glm::vec3{direction.x / direction.z, direction.y / direction.z, 1},
            std::floor(position.z),
            FaceType::back};
      else
        return std::tuple{
            glm::vec3{direction.x / direction.z, direction.y / direction.z, 1},
            std::ceil(position.z),
            FaceType::front};
    }()};
    auto n{(direction.z < 0 ? -1 : 1) * (round - position.z) * d};
    for (auto i{0}; i < reach_distance; ++i) {
      contacts.push_back({n, face});
      n += d;
    }
  }

  contacts.erase(std::remove_if(std::begin(contacts),
                                std::end(contacts),
                                [](std::pair<glm::vec3, FaceType> const& i) {
                                  return (i.first.x * i.first.x)
                                             + (i.first.y, i.first.y)
                                             + (i.first.z, i.first.z)
                                         > reach_distance * reach_distance;
                                }),
                 std::end(contacts));

  std::ranges::sort(contacts,
                    [](std::pair<glm::vec3, FaceType> const& lhs,
                       std::pair<glm::vec3, FaceType> const& rhs) {
                      return (lhs.first.x * lhs.first.x)
                                 + (lhs.first.y * lhs.first.y)
                                 + (lhs.first.z * lhs.first.z)
                             < (rhs.first.x * rhs.first.x)
                                   + (rhs.first.y * rhs.first.y)
                                   + (rhs.first.z * rhs.first.z);
                    });
  for (auto&& [v, _] : contacts)
    v += position;

  Vector<std::pair<glm::ivec3, FaceType>> faces;
  faces.reserve(std::size(contacts));
  for (auto&& [p, f] : contacts)
    faces.push_back({{gsl::narrow_cast<int>(p.x) - (f == FaceType::right),
                      gsl::narrow_cast<int>(p.y) - (f == FaceType::down),
                      gsl::narrow_cast<int>(p.z) - (f == FaceType::back)},
                     f});
  return faces;
}
