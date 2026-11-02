#pragma once

#include "Math.hpp"
#include "Slice.hpp"

// rows top to bottom with no padding
using Image_Format = u32;
enum : Image_Format
{
  Image_Format_RGBA8,
  Image_Format_RGBA8Premultiplied,
  Image_Format_Gray8,
  Image_Format_RGBA16,
  Image_Format_Gray16,
  Image_Format_RGBA16Float,
  Image_Format_RGBA32Float,
};

[[nodiscard]]
constexpr u32 ImageFormatBytesPerPixel(Image_Format format)
{
  switch (format)
  {
  case Image_Format_Gray8:
    return 1;
  case Image_Format_Gray16:
    return 2;
  case Image_Format_RGBA16:
  case Image_Format_RGBA16Float:
    return 8;
  case Image_Format_RGBA32Float:
    return 16;
  default:
    return 4;
  }
}

struct Image
{
  Slice<u8> pixels;
  Vector2<u32> size;
  Image_Format format;
};