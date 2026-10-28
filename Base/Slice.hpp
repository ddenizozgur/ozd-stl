#pragma once

#include "Core.hpp"
#include <string.h>

// non-owning view over contiguous memory.
template <typename T>
struct Slice
{
  T *ptr;
  usize count;

  [[nodiscard]]
  constexpr T &operator[](usize index)
  {
    assert(index < count && "slice index out of bounds");
    return ptr[index];
  }

  [[nodiscard]]
  constexpr const T &operator[](usize index) const
  {
    assert(index < count && "slice index out of bounds");
    return ptr[index];
  }

  // for range-based for loops
  [[nodiscard]] constexpr T *begin() { return ptr; }
  [[nodiscard]] constexpr T *end() { return ptr + count; }
  [[nodiscard]] constexpr const T *begin() const { return ptr; }
  [[nodiscard]] constexpr const T *end() const { return ptr + count; }
};

template <typename T>
[[nodiscard]]
constexpr Slice<T> SliceMake(T *data, usize count)
{
  return {data, count};
}

template <typename T, usize N>
[[nodiscard]]
constexpr Slice<T> SliceFromArray(T (&array)[N])
{
  return {array, N};
}

// [first, onePastLast)
template <typename T>
[[nodiscard]]
constexpr Slice<T> SliceFromRange(Slice<T> slice, usize first, usize onePastLast)
{
  assert(first <= onePastLast && "slice range is reversed");
  assert(onePastLast <= slice.count && "slice range out of bounds");
  return {slice.ptr + first, onePastLast - first};
}

// the slices may overlap.
template <typename T>
inline usize SliceCopyInto(Slice<T> destination, Slice<T> source)
{
  static_assert(__is_trivially_copyable(T), "T must be trivially copyable");

  usize count = Minimum(destination.count, source.count);
  if (count != 0)
  {
    memmove(destination.ptr, source.ptr, count * sizeof(T));
  }
  return count;
}