#pragma once

#include "Slice.hpp"

struct Bump_Allocator
{
  Slice<u8> backingBuffer;
  usize position;
};

[[nodiscard]]
constexpr Bump_Allocator BumpInit(Slice<u8> backingBuffer)
{
  Bump_Allocator res = {};
  res.backingBuffer = backingBuffer;
  return res;
}

[[nodiscard]]
constexpr usize BumpPosition(const Bump_Allocator *bump)
{
  return bump->position;
}

[[nodiscard]]
inline void *BumpPushEx(Bump_Allocator *bump, usize size, usize alignment)
{
  auto baseAddress = reinterpret_cast<usize>(bump->backingBuffer.ptr);
  auto lastPosition = AlignForwardPow2(baseAddress + bump->position, alignment) - baseAddress;

  auto count = bump->backingBuffer.count;
  if (lastPosition > count || size > count - lastPosition)
  {
    assert(false && "allocator size exceeded");
    return nullptr;
  }

  void *res = bump->backingBuffer.ptr + lastPosition;
  bump->position = lastPosition + size;
  return res;
}

constexpr void BumpPopTo(Bump_Allocator *bump, usize to)
{
  assert(bump->position >= to && "trying to pop forward");
  bump->position = to;
}

constexpr void BumpPopBy(Bump_Allocator *bump, usize by)
{
  BumpPopTo(bump, bump->position - by);
}

template <typename T>
[[nodiscard]] inline T *BumpPush(Bump_Allocator *bump, usize count = 1)
{
  void *ptr = BumpPushEx(bump, sizeof(T) * count, alignof(T));
  return static_cast<T *>(ptr);
}