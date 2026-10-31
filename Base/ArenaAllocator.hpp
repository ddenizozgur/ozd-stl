#pragma once

#include "BumpAllocator.hpp"
#include "Virtual.hpp"

inline constexpr usize SCRATCH_DEFAULT_RESERVE_SIZE = Gigabytes(8);
inline constexpr usize ARENA_DEFAULT_RESERVE_SIZE = Gigabytes(1);
inline constexpr usize ARENA_DEFAULT_PER_COMMIT_SIZE = Kilobytes(16);

struct Arena_Allocator
{
  Bump_Allocator bump;
  usize committed;
  usize perCommitSize;
  bool isVirtual;
};

[[nodiscard]]
inline Arena_Allocator ArenaInit(usize reserveSize = ARENA_DEFAULT_RESERVE_SIZE,
                                 usize perCommitSize = ARENA_DEFAULT_PER_COMMIT_SIZE)
{
  reserveSize = AlignForwardPow2(reserveSize, Win32_AllocationGranularity());

  perCommitSize = Minimum(perCommitSize, reserveSize);
  perCommitSize = AlignForwardPow2(perCommitSize, Win32_PageSize());
  // reserved memory is already aligned for us
  Slice<u8> reserved = Win32_VirtualReserve(reserveSize);

  return {
      .bump = BumpInit(reserved),
      .perCommitSize = perCommitSize,
      .isVirtual = true,
  };
}

[[nodiscard]]
constexpr Arena_Allocator ArenaInitFromBuffer(Slice<u8> backingBuffer)
{
  return {
      .bump = BumpInit(backingBuffer),
      .committed = backingBuffer.count,
  };
}

[[nodiscard]]
constexpr usize ArenaPosition(const Arena_Allocator *arena)
{
  return arena->bump.position;
}

[[nodiscard]]
inline void *ArenaPushEx(Arena_Allocator *arena, usize size, usize alignment)
{
  // Windows always zeroes fresh commits.
  void *res = BumpPushEx(&arena->bump, size, alignment);
  if (res == nullptr)
  {
    return nullptr;
  }

  auto position = ArenaPosition(arena);
  if (arena->isVirtual && position > arena->committed)
  {
    auto backingBuffer = arena->bump.backingBuffer;
    auto baseAddress = reinterpret_cast<usize>(backingBuffer.ptr);
    auto commitEnd = AlignForward(baseAddress + position, arena->perCommitSize) - baseAddress;
    commitEnd = Minimum(commitEnd, backingBuffer.count);

    auto range = SliceFromRange(backingBuffer, arena->committed, commitEnd);
    Win32_VirtualCommit(range);
    arena->committed = commitEnd;
  }

  return res;
}

constexpr void ArenaPopTo(Arena_Allocator *arena, usize to)
{
  BumpPopTo(&arena->bump, to);
}

constexpr void ArenaPopBy(Arena_Allocator *arena, usize by)
{
  BumpPopBy(&arena->bump, by);
}

inline void ArenaFree(Arena_Allocator *arena)
{
  if (arena->isVirtual)
  {
    Win32_VirtualRelease(arena->bump.backingBuffer);
  }
  *arena = {};
}

template <typename T>
[[nodiscard]] inline T *ArenaPush(Arena_Allocator *arena, usize count = 1)
{
  void *ptr = ArenaPushEx(arena, sizeof(T) * count, alignof(T));
  return static_cast<T *>(ptr);
}

template <typename T>
[[nodiscard]]
inline Slice<T> ArenaPushSlice(Arena_Allocator *arena, usize count)
{
  T *ptr = ArenaPush<T>(arena, count);
  if (ptr == nullptr)
  {
    return {};
  }
  return {ptr, count};
}

template <typename T>
[[nodiscard]]
inline Slice<T> ArenaPushSliceEx(Arena_Allocator *arena, usize count, usize alignment)
{
  void *ptr = ArenaPushEx(arena, sizeof(T) * count, Maximum(alignof(T), alignment));
  if (ptr == nullptr)
  {
    return {};
  }
  return {static_cast<T *>(ptr), count};
}

template <typename T>
[[nodiscard]]
inline Slice<T> ArenaCloneSlice(Arena_Allocator *arena, Slice<T> source)
{
  static_assert(__is_trivially_copyable(T), "T must be trivially copyable");

  auto res = ArenaPushSlice<T>(arena, source.count);
  if (res.count != 0)
  {
    memmove(res.ptr, source.ptr, source.count * sizeof(T));
  }
  return res;
}

//
// Scratch
//
inline thread_local Arena_Allocator doNotUseThisScratch = {};

struct Scratch
{
  Arena_Allocator *arena;
  usize position;
};

[[nodiscard]]
inline bool IsScratch(const Arena_Allocator *arena)
{
  return (arena == &doNotUseThisScratch);
}

[[nodiscard]]
inline Arena_Allocator *ScratchGetArena()
{
  if (doNotUseThisScratch.bump.backingBuffer.ptr == nullptr)
  {
    doNotUseThisScratch = ArenaInit(SCRATCH_DEFAULT_RESERVE_SIZE,
                                    ARENA_DEFAULT_PER_COMMIT_SIZE);
  }
  return &doNotUseThisScratch;
}

[[nodiscard]]
inline Scratch ScratchBegin()
{
  auto arena = ScratchGetArena();
  return {arena, ArenaPosition(arena)};
}

inline void ScratchEnd(Scratch scratch, bool ignore = false)
{
  if (!ignore)
  {
    ArenaPopTo(scratch.arena, scratch.position);
  }
}

inline void ScratchFree()
{
  if (doNotUseThisScratch.bump.backingBuffer.ptr != nullptr)
  {
    ArenaFree(&doNotUseThisScratch);
  }
}
