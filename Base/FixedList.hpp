#pragma once

#include "Slice.hpp"

template <typename T>
struct Fixed_List
{
  static_assert(__is_trivially_copyable(T), "T must be trivially copyable");

  Slice<T> backingBuffer;
  usize count;

  [[nodiscard]]
  constexpr T &operator[](usize index)
  {
    assert(index < count && "list index out of bounds");
    return backingBuffer.ptr[index];
  }

  [[nodiscard]]
  constexpr const T &operator[](usize index) const
  {
    assert(index < count && "list index out of bounds");
    return backingBuffer.ptr[index];
  }

  [[nodiscard]] constexpr T *begin() { return backingBuffer.ptr; }
  [[nodiscard]] constexpr T *end() { return backingBuffer.ptr + count; }
  [[nodiscard]] constexpr const T *begin() const { return backingBuffer.ptr; }
  [[nodiscard]] constexpr const T *end() const { return backingBuffer.ptr + count; }
};

template <typename T>
[[nodiscard]]
constexpr Fixed_List<T> FixedListInit(Slice<T> backingBuffer)
{
  return {.backingBuffer = backingBuffer};
}

template <typename T>
[[nodiscard]]
constexpr Slice<T> SliceFromFixedList(Fixed_List<T> list)
{
  return {list.backingBuffer.ptr, list.count};
}

template <typename T>
inline T FixedListPop(Fixed_List<T> *list)
{
  assert(list->count != 0 && "popping an empty list");
  list->count -= 1;
  return list->backingBuffer[list->count];
}

template <typename T>
inline void FixedListUnorderedRemove(Fixed_List<T> *list, usize index)
{
  (*list)[index] = (*list)[list->count - 1];
  list->count -= 1;
}

// shifts every element after index down by one
template <typename T>
inline void FixedListOrderedRemove(Fixed_List<T> *list, usize index)
{
  assert(index < list->count && "list index out of bounds");
  T *items = list->backingBuffer.ptr;
  memmove(items + index, items + index + 1, (list->count - index - 1) * sizeof(T));
  list->count -= 1;
}

// shifts the elements from index up, returns how many were inserted
template <typename T>
inline usize FixedListInsert(Fixed_List<T> *list, usize index, Slice<Non_Deduced<T>> values)
{
  assert(index <= list->count && "list index out of bounds");
  usize count = Minimum(values.count, list->backingBuffer.count - list->count);
  if (count != 0)
  {
    T *items = list->backingBuffer.ptr;
    memmove(items + index + count, items + index, (list->count - index) * sizeof(T));
    memcpy(items + index, values.ptr, count * sizeof(T));
  }
  list->count += count;
  return count;
}

// returns how many were appended
template <typename T>
inline usize FixedListAppend(Fixed_List<T> *list, Slice<Non_Deduced<T>> values)
{
  return FixedListInsert(list, list->count, values);
}

template <typename T>
constexpr void FixedListClear(Fixed_List<T> *list)
{
  list->count = 0;
}