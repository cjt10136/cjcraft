#include "chunk.h"

#include "FastNoiseLite/FastNoiseLite.h"

#include <gsl/gsl>
#include <random>

HeightMap generate_height_map(glm::ivec2 offset)
{
  static auto const noise{[] {
    FastNoiseLite noise{1};
    noise.SetSeed(gsl::narrow_cast<int>(std::random_device{}()));
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    return noise;
  }()};

  constexpr auto frequency{0.5f};
  HeightMap map(chunk_width + 1);
  for (auto i{0}; i < chunk_width + 1; ++i)
    for (auto j{0}; j < chunk_depth + 1; ++j)
      map[i][j] = noise.GetNoise(frequency * (i + offset.x),
                                 frequency * (j + offset.y));
  return map;
}
