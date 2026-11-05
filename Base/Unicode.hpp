#pragma once

// the original implementation is from Odin's core library:
// https://github.com/odin-lang/Odin/blob/master/core/unicode/utf8/utf8.odin

#include "FixedList.hpp"
#include "String.hpp"

using Rune = u32;

inline constexpr Rune RUNE_ERROR = 0xFFFD;
inline constexpr Rune RUNE_MAX = 0x10FFFF;

inline constexpr Rune SURROGATE_HIGH_MIN = 0xD800;
inline constexpr Rune SURROGATE_HIGH_MAX = 0xDBFF;
inline constexpr Rune SURROGATE_LOW_MIN = 0xDC00;
inline constexpr Rune SURROGATE_LOW_MAX = 0xDFFF;

//
// UTF-8
//
struct Utf8_Decoded
{
  Rune rune;
  usize offset;
  u32 size;
};

// 2nd, 3rd or 4th bytes all look like 10xxxxxx
[[nodiscard]]
constexpr bool IsUtf8Continuation(u8 byte)
{
  return (byte & 0xC0) == 0x80;
}

[[nodiscard]]
inline Utf8_Decoded Utf8DecodeRuneAt(String s, usize offset)
{
  Utf8_Decoded res = {.rune = RUNE_ERROR, .offset = offset, .size = 1};
  if (offset == s.count)
  {
    res.size = 0;
    return res;
  }

  u8 lead = s.ptr[offset];
  if (lead < 0x80)
  {
    res.rune = lead;
    return res;
  }

  u32 size = 0;
  u8 secondMin = 0x80;
  u8 secondMax = 0xBF;
  if (lead < 0xC2)
  {
    return res;
  }
  else if (lead <= 0xDF)
  {
    size = 2;
  }
  else if (lead <= 0xEF)
  {
    size = 3;
    if (lead == 0xE0)
    {
      secondMin = 0xA0;
    }
    if (lead == 0xED)
    {
      secondMax = 0x9F;
    }
  }
  else if (lead <= 0xF4)
  {
    size = 4;
    if (lead == 0xF0)
    {
      secondMin = 0x90;
    }
    if (lead == 0xF4)
    {
      secondMax = 0x8F;
    }
  }
  else
  {
    return res;
  }

  if (s.count - offset < size)
  {
    return res;
  }

  u8 second = s.ptr[offset + 1];
  if (second < secondMin || second > secondMax)
  {
    return res;
  }

  Rune rune = ((lead & (0x7F >> size)) << 6) | (second & 0x3F);
  for (u32 i = 2; i < size; i += 1)
  {
    u8 next = s.ptr[offset + i];
    if (!IsUtf8Continuation(next))
    {
      return res;
    }
    rune = (rune << 6) | (next & 0x3F);
  }

  res.rune = rune;
  res.size = size;
  return res;
}

[[nodiscard]]
inline Utf8_Decoded Utf8DecodeRuneBefore(String s, usize end)
{
  Utf8_Decoded res = {.rune = RUNE_ERROR, .offset = end, .size = 0};
  if (end == 0)
  {
    return res;
  }

  usize start = end - 1;
  usize limit = end >= 4 ? end - 4 : 0;
  for (; start > limit && IsUtf8Continuation(s.ptr[start]);)
  {
    start -= 1;
  }

  res = Utf8DecodeRuneAt({s.ptr, end}, start);
  if (res.offset + res.size != end)
  {
    res = {.rune = RUNE_ERROR, .offset = end - 1, .size = 1};
  }
  return res;
}

struct Utf8_Encoded
{
  char bytes[4];
  u32 size;
};

// surrogates and runes above RUNE_MAX encode as RUNE_ERROR
[[nodiscard]]
inline Utf8_Encoded Utf8EncodeRune(Rune rune)
{
  if ((rune >= SURROGATE_HIGH_MIN && rune <= SURROGATE_LOW_MAX) || rune > RUNE_MAX)
  {
    rune = RUNE_ERROR;
  }

  Utf8_Encoded res = {};
  if (rune < 0x80)
  {
    res.bytes[0] = rune;
    res.size = 1;
  }
  else if (rune < 0x800)
  {
    res.bytes[0] = 0xC0 | (rune >> 6);
    res.bytes[1] = 0x80 | (rune & 0x3F);
    res.size = 2;
  }
  else if (rune < 0x10000)
  {
    res.bytes[0] = 0xE0 | (rune >> 12);
    res.bytes[1] = 0x80 | ((rune >> 6) & 0x3F);
    res.bytes[2] = 0x80 | (rune & 0x3F);
    res.size = 3;
  }
  else
  {
    res.bytes[0] = 0xF0 | (rune >> 18);
    res.bytes[1] = 0x80 | ((rune >> 12) & 0x3F);
    res.bytes[2] = 0x80 | ((rune >> 6) & 0x3F);
    res.bytes[3] = 0x80 | (rune & 0x3F);
    res.size = 4;
  }
  return res;
}

inline void Utf8AppendWholeRune(Fixed_List<char> *text, Rune rune)
{
  auto encoded = Utf8EncodeRune(rune);
  if (text->count + encoded.size <= text->backingBuffer.count)
  {
    FixedListAppend(text, {encoded.bytes, encoded.size});
  }
}

//
// Iteration
//
struct Utf8_Iterator_End
{
};

struct Utf8_Forward_Iterator
{
  String s;
  Utf8_Decoded current;

  [[nodiscard]] Utf8_Decoded operator*() const { return current; }
  [[nodiscard]] bool operator!=(Utf8_Iterator_End) const { return current.size != 0; }
  void operator++() { current = Utf8DecodeRuneAt(s, current.offset + current.size); }
};

struct Utf8_Reverse_Iterator
{
  String s;
  Utf8_Decoded current;

  [[nodiscard]] Utf8_Decoded operator*() const { return current; }
  [[nodiscard]] bool operator!=(Utf8_Iterator_End) const { return current.size != 0; }
  void operator++() { current = Utf8DecodeRuneBefore(s, current.offset); }
};

struct Utf8_Runes
{
  String s;

  [[nodiscard]] Utf8_Forward_Iterator begin() const { return {s, Utf8DecodeRuneAt(s, 0)}; }
  [[nodiscard]] Utf8_Iterator_End end() const { return {}; }
};

struct Utf8_Runes_Reverse
{
  String s;

  [[nodiscard]] Utf8_Reverse_Iterator begin() const { return {s, Utf8DecodeRuneBefore(s, s.count)}; }
  [[nodiscard]] Utf8_Iterator_End end() const { return {}; }
};

// for (auto decoded : StringRunes(s))
[[nodiscard]]
constexpr Utf8_Runes StringRunes(String s)
{
  return {s};
}

// last rune first
[[nodiscard]]
constexpr Utf8_Runes_Reverse StringRunesReverse(String s)
{
  return {s};
}

//
// Search
//
[[nodiscard]]
inline bool StringContainsRune(String s, Rune rune)
{
  for (auto decoded : StringRunes(s))
  {
    if (decoded.rune == rune)
    {
      return true;
    }
  }
  return false;
}

// the last rune of s that is one of runes, size 0 when there is none
[[nodiscard]]
inline Utf8_Decoded StringFindLast(String s, String runes)
{
  for (auto decoded : StringRunesReverse(s))
  {
    if (StringContainsRune(runes, decoded.rune))
    {
      return decoded;
    }
  }
  return {.rune = RUNE_ERROR, .offset = s.count, .size = 0};
}

// If none of runes is in s, Before returns all of s and After returns an empty string.
[[nodiscard]]
inline String StringBeforeLast(String s, String runes)
{
  String res = s;
  if (auto found = StringFindLast(s, runes); found.size != 0)
  {
    res = {s.ptr, found.offset};
  }
  return res;
}

[[nodiscard]]
inline String StringBeforeLastInclusive(String s, String runes)
{
  String res = s;
  if (auto found = StringFindLast(s, runes); found.size != 0)
  {
    res = {s.ptr, found.offset + found.size};
  }
  return res;
}

[[nodiscard]]
inline String StringAfterLast(String s, String runes)
{
  String res = {};
  if (auto found = StringFindLast(s, runes); found.size != 0)
  {
    usize start = found.offset + found.size;
    res = {s.ptr + start, s.count - start};
  }
  return res;
}

[[nodiscard]]
inline String StringAfterLastInclusive(String s, String runes)
{
  String res = {};
  if (auto found = StringFindLast(s, runes); found.size != 0)
  {
    res = {s.ptr + found.offset, s.count - found.offset};
  }
  return res;
}

//
// UTF-16
//
[[nodiscard]]
constexpr bool IsHighSurrogate(u16 unit)
{
  return unit >= SURROGATE_HIGH_MIN && unit <= SURROGATE_HIGH_MAX;
}

[[nodiscard]]
constexpr bool IsLowSurrogate(u16 unit)
{
  return unit >= SURROGATE_LOW_MIN && unit <= SURROGATE_LOW_MAX;
}

[[nodiscard]]
constexpr Rune RuneFromSurrogates(u16 high, u16 low)
{
  assert(IsHighSurrogate(high) && IsLowSurrogate(low) && "not a surrogate pair");
  return 0x10000 + (((high - SURROGATE_HIGH_MIN) << 10) | (low - SURROGATE_LOW_MIN));
}

struct Utf16_Decoder
{
  u16 pendingHigh; // 0 when there is none
};

struct Utf16_Decoded
{
  Rune runes[2];
  u32 count;
};

// a surrogate without its other half comes out as RUNE_ERROR
[[nodiscard]]
inline Utf16_Decoded Utf16DecoderPush(Utf16_Decoder *decoder, u16 unit)
{
  Utf16_Decoded res = {};
  if (decoder->pendingHigh != 0 && IsLowSurrogate(unit))
  {
    res.runes[0] = RuneFromSurrogates(decoder->pendingHigh, unit);
    res.count = 1;
    decoder->pendingHigh = 0;
    return res;
  }

  if (decoder->pendingHigh != 0)
  {
    res.runes[res.count] = RUNE_ERROR;
    res.count += 1;
    decoder->pendingHigh = 0;
  }

  if (IsHighSurrogate(unit))
  {
    decoder->pendingHigh = unit;
  }
  else if (IsLowSurrogate(unit))
  {
    res.runes[res.count] = RUNE_ERROR;
    res.count += 1;
  }
  else
  {
    res.runes[res.count] = unit;
    res.count += 1;
  }
  return res;
}
