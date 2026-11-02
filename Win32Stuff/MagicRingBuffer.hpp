#pragma once

// the "magic ring buffer", by Fabian Giesen:
// https://fgiesen.wordpress.com/2012/07/21/the-magic-ring-buffer/
// also Casey Muratori's circular buffer:
// https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0121_circular_buffer_main.cpp

#include "../Base/Virtual.hpp"
#pragma comment(lib, "onecore")

// requires Windows 10 version 1803.
// ptr[0, size) maps the same memory as ptr[size, 2 * size).
[[nodiscard]]
inline Slice<u8> Win32_VirtualReserveRepeated(usize size)
{
  // wastes some address space, but the ring touches every page,
  // so rounding up to 64 KiB would cost real memory
  size = AlignForwardPow2(size, Win32_PageSize());

  auto placeholder = static_cast<u8 *>(VirtualAlloc2(nullptr, nullptr, 2 * size,
                                                     MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                     PAGE_NOACCESS, nullptr, 0));
  if (placeholder == nullptr)
  {
    Trap();
  }

  // split into two
  if (!VirtualFree(placeholder, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
  {
    Trap();
  }

  // section backed by the page file
  HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                      static_cast<DWORD>(size >> 32), static_cast<DWORD>(size),
                                      nullptr);
  if (section == nullptr)
  {
    Trap();
  }

  // map the same section over both placeholders
  void *first = MapViewOfFile3(section, nullptr, placeholder, 0, size,
                               MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
  void *second = MapViewOfFile3(section, nullptr, placeholder + size, 0, size,
                                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
  if (first == nullptr || second == nullptr)
  {
    Trap();
  }

  // the views will keep the section alive
  CloseHandle(section);
  return {placeholder, size};
}

inline void Win32_VirtualReleaseRepeated(Slice<u8> repeated)
{
  if (!UnmapViewOfFile(repeated.ptr) || !UnmapViewOfFile(repeated.ptr + repeated.count))
  {
    Trap();
  }
}

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
  return {.data = Win32_VirtualReserveRepeated(reserveSize)};
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
