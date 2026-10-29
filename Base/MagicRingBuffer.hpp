#pragma once

#include "Virtual.hpp"

// requires Windows 10 version 1803.

// the "magic ring buffer", by Fabian Giesen:
// https://fgiesen.wordpress.com/2012/07/21/the-magic-ring-buffer/
// also Casey Muratori's circular buffer:
// https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0121_circular_buffer_main.cpp

struct Magic_Ring_Buffer
{
  Slice<u8> data;
  u64 filledBegin;
  u64 filledEnd;
};

// the size is rounded up to the page size
[[nodiscard]]
inline Magic_Ring_Buffer MagicRingInit(usize reserveSize)
{
  Magic_Ring_Buffer res = {};
  res.data = Win32_VirtualReserveRepeated(reserveSize);
  return res;
}

inline void MagicRingFree(Magic_Ring_Buffer *ring)
{
  Win32_VirtualReleaseRepeated(ring->data);
  *ring = {};
}

[[nodiscard]]
constexpr usize MagicRingFilledCount(const Magic_Ring_Buffer *ring)
{
  return ring->filledEnd - ring->filledBegin;
}

[[nodiscard]]
constexpr usize MagicRingEmptyCount(const Magic_Ring_Buffer *ring)
{
  return ring->data.count - MagicRingFilledCount(ring);
}

// contiguous even after the end
[[nodiscard]]
inline Slice<u8> MagicRingEmptySlice(Magic_Ring_Buffer *ring)
{
  usize offset = ring->filledEnd % ring->data.count;
  return {ring->data.ptr + offset, MagicRingEmptyCount(ring)};
}

inline void MagicRingFill(Magic_Ring_Buffer *ring, usize count)
{
  assert(count <= MagicRingEmptyCount(ring) && "filling more than the empty region");
  ring->filledEnd += count;
}

// contiguous even after the end
[[nodiscard]]
inline Slice<u8> MagicRingFilledSlice(Magic_Ring_Buffer *ring)
{
  usize offset = ring->filledBegin % ring->data.count;
  return {ring->data.ptr + offset, MagicRingFilledCount(ring)};
}

inline void MagicRingDrain(Magic_Ring_Buffer *ring, usize count)
{
  assert(count <= MagicRingFilledCount(ring) && "draining more than the filled region");
  ring->filledBegin += count;
}
