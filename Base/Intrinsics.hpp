#pragma once

#include "Pre.hpp"
#include "Types.hpp"
#include <assert.h>

#if IS_COMPILER_MSVC
#include <intrin.h>
#endif

#if IS_COMPILER_MSVC
#define Trap() __debugbreak()
#elif IS_COMPILER_CLANG
#define Trap() __builtin_trap()
#endif

[[nodiscard]]
inline u32 CountLeadingZeros32(u32 x)
{
  assert(x != 0 && "leading zeros of 0 are undefined");
#if IS_COMPILER_MSVC
  unsigned long index;
  _BitScanReverse(&index, x);
  return 31 - index;
#else
  return __builtin_clz(x);
#endif
}

[[nodiscard]]
inline u32 CountLeadingZeros64(u64 x)
{
  assert(x != 0 && "leading zeros of 0 are undefined");
#if IS_COMPILER_MSVC
  unsigned long index;
  _BitScanReverse64(&index, x);
  return 63 - index;
#else
  return __builtin_clzll(x);
#endif
}

[[nodiscard]]
inline u32 CountTrailingZeros32(u32 x)
{
  assert(x != 0 && "trailing zeros of 0 are undefined");
#if IS_COMPILER_MSVC
  unsigned long index;
  _BitScanForward(&index, x);
  return index;
#else
  return __builtin_ctz(x);
#endif
}

[[nodiscard]]
inline u32 CountTrailingZeros64(u64 x)
{
  assert(x != 0 && "trailing zeros of 0 are undefined");
#if IS_COMPILER_MSVC
  unsigned long index;
  _BitScanForward64(&index, x);
  return index;
#else
  return __builtin_ctzll(x);
#endif
}

[[nodiscard]]
inline u32 CountSetBits32(u32 x)
{
#if IS_COMPILER_MSVC
  return __popcnt(x);
#else
  return __builtin_popcount(x);
#endif
}

[[nodiscard]]
inline u32 CountSetBits64(u64 x)
{
#if IS_COMPILER_MSVC
  return static_cast<u32>(__popcnt64(x));
#else
  return __builtin_popcountll(x);
#endif
}

// floor(log2(x))
[[nodiscard]]
inline u32 HighestSetBitIndex(u64 x)
{
  return 63 - CountLeadingZeros64(x);
}

[[nodiscard]]
inline u32 LowestSetBitIndex(u64 x)
{
  return CountTrailingZeros64(x);
}

[[nodiscard]]
inline u32 BitWidth(u64 x)
{
  u32 res = 0;
  if (x != 0)
  {
    res = HighestSetBitIndex(x) + 1;
  }
  return res;
}

[[nodiscard]]
inline u32 TimesDivisibleBy2(u64 x)
{
  return CountTrailingZeros64(x);
}

[[nodiscard]]
inline u64 LargestPow2Divisor(u64 x)
{
  return x & (0 - x);
}

[[nodiscard]]
inline u64 RoundUpPow2(u64 x)
{
  assert(x != 0 && x <= (u64{1} << 63) && "no power of two in u64 for it");
  return u64{1} << BitWidth(x - 1);
}
