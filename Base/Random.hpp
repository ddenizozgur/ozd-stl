#pragma once

#include "Core.hpp"
#include "Intrinsics.hpp"

// xoshiro256** (https://prng.di.unimi.it)
struct Random_State
{
  u64 s[4];
};

// splitmix64 never returns all-zero state
[[nodiscard]]
inline Random_State RandomInit(u64 seed)
{
  Random_State random = {};
  for (u64 &word : random.s)
  {
    seed += 0x9E3779B97F4A7C15;
    u64 z = seed;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    word = z ^ (z >> 31);
  }
  return random;
}

[[nodiscard]]
inline u64 RandomU64(Random_State *random)
{
  u64 *s = random->s;
  u64 res = RotateLeft64(s[1] * 5, 7) * 9;
  u64 t = s[1] << 17;
  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];
  s[2] ^= t;
  s[3] = RotateLeft64(s[3], 45);
  return res;
}

// [0, count)
[[nodiscard]]
inline u32 RandomBelowU32(Random_State *random, u32 count)
{
  assert(count != 0);
  u64 product = (RandomU64(random) >> 32) * count;
  auto low = static_cast<u32>(product);
  if (low < count)
  {
    u32 rejectBelow = (0u - count) % count;
    for (; low < rejectBelow;)
    {
      product = (RandomU64(random) >> 32) * count;
      low = static_cast<u32>(product);
    }
  }
  return static_cast<u32>(product >> 32);
}

// [0, 1)
[[nodiscard]]
inline f32 RandomF32(Random_State *random)
{
  auto bits = static_cast<f32>(RandomU64(random) >> 40);
  return bits * 0x1p-24f;
}
