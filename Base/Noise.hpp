#pragma once

#include "Math.hpp"

// OpenSimplex2 2D noise (https://github.com/KdotJPG/OpenSimplex2), ported from Odin's core:math/noise

inline constexpr Vector2<f32> NOISE_GRADIENTS_2D[] = {
  {38.20591f, 92.23722f},
  {92.23722f, 38.20591f},
  {92.23722f, -38.20591f},
  {38.20591f, -92.23722f},
  {-38.20591f, -92.23722f},
  {-92.23722f, -38.20591f},
  {-92.23722f, 38.20591f},
  {-38.20591f, 92.23722f},
  {13.031323f, 98.98274f},
  {60.776825f, 79.2059f},
  {79.2059f, 60.776825f},
  {98.98274f, 13.031323f},
  {98.98274f, -13.031323f},
  {79.2059f, -60.776825f},
  {60.776825f, -79.2059f},
  {13.031323f, -98.98274f},
  {-13.031323f, -98.98274f},
  {-60.776825f, -79.2059f},
  {-79.2059f, -60.776825f},
  {-98.98274f, -13.031323f},
  {-98.98274f, 13.031323f},
  {-79.2059f, 60.776825f},
  {-60.776825f, 79.2059f},
  {-13.031323f, 98.98274f},
};

inline constexpr u64 NOISE_PRIME_X = 0x5205402B9270C86F;
inline constexpr u64 NOISE_PRIME_Y = 0x598CD327003817B5;

[[nodiscard]]
inline f32 NoiseCorner2D(u64 seed, u64 cornerX, u64 cornerY, Vector2<f32> delta)
{
  f32 falloff = 0.5f - LengthSquared(delta);
  if (falloff <= 0)
  {
    return 0;
  }
  u64 hash = (seed ^ cornerX ^ cornerY) * 0x53A3F72DEEC546F5;
  hash ^= static_cast<u64>(static_cast<i64>(hash) >> 58);
  Vector2<f32> gradient = NOISE_GRADIENTS_2D[((hash >> 1) & 127) % ArrayCount(NOISE_GRADIENTS_2D)];
  return (falloff * falloff) * (falloff * falloff) * DotProduct(gradient, delta);
}

// about [-1, 1], the same seed and position always give the same value
[[nodiscard]]
inline f32 Noise2D(u64 seed, Vector2<f32> position)
{
  constexpr f32 SKEW = 0.366025403784439f;
  constexpr f32 UNSKEW = -0.21132486540518713f;

  Vector2<f32> skewed = position + SKEW * (position.x + position.y);
  Vector2<f32> base = {FloorF32(skewed.x), FloorF32(skewed.y)};
  auto cornerX = static_cast<u64>(static_cast<i64>(base.x)) * NOISE_PRIME_X;
  auto cornerY = static_cast<u64>(static_cast<i64>(base.y)) * NOISE_PRIME_Y;

  Vector2<f32> inside = skewed - base;
  Vector2<f32> delta = inside + UNSKEW * (inside.x + inside.y);
  f32 value = NoiseCorner2D(seed, cornerX, cornerY, delta);
  value += NoiseCorner2D(seed, cornerX + NOISE_PRIME_X, cornerY + NOISE_PRIME_Y, delta - (1 + 2 * UNSKEW));
  if (delta.y > delta.x)
  {
    value += NoiseCorner2D(seed, cornerX, cornerY + NOISE_PRIME_Y, delta - Vector2<f32>{UNSKEW, UNSKEW + 1});
  }
  else
  {
    value += NoiseCorner2D(seed, cornerX + NOISE_PRIME_X, cornerY, delta - Vector2<f32>{UNSKEW + 1, UNSKEW});
  }
  return value;
}
