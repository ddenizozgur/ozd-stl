#pragma once

#include "ArenaAllocator.hpp"
#include "Slice.hpp"
#include <stdarg.h>
#include <stdio.h>

using String = Slice<char>;
using String16 = Slice<wchar_t>;

// StringFmt(arena, "name: %.*s", StringArgs(name))
#define StringArgs(s) static_cast<i32>((s).count), (s).ptr

// the result is followed by a null terminator, not counted in count
[[nodiscard]]
PrintfLike(2, 0) inline String StringFmtV(Arena_Allocator *arena, const char *fmt, va_list args)
{
  va_list argsCopy;
  va_copy(argsCopy, args);
  i32 length = vsnprintf(nullptr, 0, fmt, argsCopy);
  va_end(argsCopy);
  if (length < 0)
  {
    return {};
  }

  auto res = ArenaPushSlice<char>(arena, static_cast<usize>(length) + 1);
  if (res.ptr == nullptr)
  {
    return {};
  }

  vsnprintf(res.ptr, res.count, fmt, args);
  return {res.ptr, static_cast<usize>(length)};
}

// the result is followed by a null terminator, not counted in count
[[nodiscard]]
PrintfLike(2, 3) inline String StringFmt(Arena_Allocator *arena, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String res = StringFmtV(arena, fmt, args);
  va_end(args);
  return res;
}

template <usize N>
[[nodiscard]]
constexpr String StringLit(const char (&literal)[N])
{
  return {const_cast<char *>(literal), N - 1};
}

#define DoNotUseThisStringify(S) #S
#define Stringify(S) StringLit(DoNotUseThisStringify(S))

template <usize N>
[[nodiscard]]
constexpr String16 String16Lit(const wchar_t (&literal)[N])
{
  return {const_cast<wchar_t *>(literal), N - 1};
}

[[nodiscard]]
inline String StringFromC(const char *source)
{
  return {const_cast<char *>(source), strlen(source)};
}

[[nodiscard]]
inline String StringFromBytes(Slice<u8> bytes)
{
  return {reinterpret_cast<char *>(bytes.ptr), bytes.count};
}

[[nodiscard]]
inline Slice<u8> BytesFromString(String source)
{
  return {reinterpret_cast<u8 *>(source.ptr), source.count};
}

[[nodiscard]]
inline String CloneString(Arena_Allocator *arena, String source)
{
  return ArenaCloneSlice(arena, source);
}

[[nodiscard]]
inline char *CStringFrom(Arena_Allocator *arena, String source)
{
  auto res = ArenaPushSlice<char>(arena, source.count + 1);
  if (res.ptr == nullptr)
  {
    return nullptr;
  }

  if (source.count != 0)
  {
    memmove(res.ptr, source.ptr, source.count);
  }
  res[source.count] = 0;
  return res.ptr;
}

// the result is followed by a null terminator, not counted in count
[[nodiscard]]
inline String Win32_StringFrom16(Arena_Allocator *arena, String16 source)
{
  i32 sourceCount = source.count;

  i32 count = 0;
  if (sourceCount != 0)
  {
    count = WideCharToMultiByte(CP_UTF8, 0, source.ptr, sourceCount, nullptr, 0, nullptr, nullptr);
  }

  auto res = ArenaPushSlice<char>(arena, static_cast<usize>(count) + 1);
  if (res.ptr == nullptr)
  {
    return {};
  }

  if (count != 0)
  {
    WideCharToMultiByte(CP_UTF8, 0, source.ptr, sourceCount, res.ptr, count, nullptr, nullptr);
  }
  res.ptr[count] = 0;
  return {res.ptr, static_cast<usize>(count)};
}

// the result is followed by a null terminator, not counted in count
[[nodiscard]]
inline String16 Win32_String16From(Arena_Allocator *arena, String source)
{
  i32 sourceCount = source.count;

  i32 count = 0;
  if (sourceCount != 0)
  {
    count = MultiByteToWideChar(CP_UTF8, 0, source.ptr, sourceCount, nullptr, 0);
  }

  auto res = ArenaPushSlice<wchar_t>(arena, static_cast<usize>(count) + 1);
  if (res.ptr == nullptr)
  {
    return {};
  }

  if (count != 0)
  {
    MultiByteToWideChar(CP_UTF8, 0, source.ptr, sourceCount, res.ptr, count);
  }
  res.ptr[count] = 0;
  return {res.ptr, static_cast<usize>(count)};
}
