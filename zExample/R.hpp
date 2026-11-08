#pragma once

#include "../Base/ArenaAllocator.hpp"
#include "../Base/Unicode.hpp"
#include "R_FontTer14.hpp"
#include "R_Fonts.hpp"
#include <float.h>

#pragma comment(lib, "gdi32")

union R_B8G8R8A8
{
  u8 data[4];
  struct
  {
    u8 b, g, r, a;
  };
};

inline constexpr R_B8G8R8A8 R_CGA_BLACK = {0x00, 0x00, 0x00, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_BLUE = {0xAA, 0x00, 0x00, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_GREEN = {0x00, 0xAA, 0x00, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_CYAN = {0xAA, 0xAA, 0x00, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_RED = {0x00, 0x00, 0xAA, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_MAGENTA = {0xAA, 0x00, 0xAA, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_BROWN = {0x00, 0x55, 0xAA, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_LIGHT_GRAY = {0xAA, 0xAA, 0xAA, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_DARK_GRAY = {0x55, 0x55, 0x55, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_LIGHT_BLUE = {0xFF, 0x55, 0x55, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_LIGHT_GREEN = {0x55, 0xFF, 0x55, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_LIGHT_CYAN = {0xFF, 0xFF, 0x55, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_LIGHT_RED = {0x55, 0x55, 0xFF, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_LIGHT_MAGENTA = {0xFF, 0x55, 0xFF, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_YELLOW = {0x55, 0xFF, 0xFF, 0xFF};
inline constexpr R_B8G8R8A8 R_CGA_WHITE = {0xFF, 0xFF, 0xFF, 0xFF};

inline constexpr R_B8G8R8A8 R_BLACK = {0, 0, 0, 255};
inline constexpr R_B8G8R8A8 R_WHITE = {255, 255, 255, 255};
inline constexpr R_B8G8R8A8 R_GRAY = {192, 192, 192, 255};
inline constexpr R_B8G8R8A8 R_DARK_GRAY = {128, 128, 128, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_GRAY = {64, 64, 64, 255};
inline constexpr R_B8G8R8A8 R_RED = {0, 0, 255, 255};
inline constexpr R_B8G8R8A8 R_DARK_RED = {0, 0, 128, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_RED = {0, 0, 64, 255};
inline constexpr R_B8G8R8A8 R_YELLOW = {0, 255, 255, 255};
inline constexpr R_B8G8R8A8 R_DARK_YELLOW = {0, 128, 128, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_YELLOW = {0, 64, 64, 255};
inline constexpr R_B8G8R8A8 R_GREEN = {0, 255, 0, 255};
inline constexpr R_B8G8R8A8 R_DARK_GREEN = {0, 128, 0, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_GREEN = {0, 64, 0, 255};
inline constexpr R_B8G8R8A8 R_CYAN = {255, 255, 0, 255};
inline constexpr R_B8G8R8A8 R_DARK_CYAN = {128, 128, 0, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_CYAN = {64, 64, 0, 255};
inline constexpr R_B8G8R8A8 R_BLUE = {255, 0, 0, 255};
inline constexpr R_B8G8R8A8 R_DARK_BLUE = {128, 0, 0, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_BLUE = {64, 0, 0, 255};
inline constexpr R_B8G8R8A8 R_MAGENTA = {255, 0, 255, 255};
inline constexpr R_B8G8R8A8 R_DARK_MAGENTA = {128, 0, 128, 255};
inline constexpr R_B8G8R8A8 R_VERY_DARK_MAGENTA = {64, 0, 64, 255};

inline constexpr R_B8G8R8A8 R_PALETTE_VINIK24[] = {
  {0x76, 0x67, 0x6f, 0xff},
  {0x97, 0x9a, 0x9a, 0xff},
  {0xb8, 0xcc, 0xc5, 0xff},
  {0x80, 0x55, 0x8b, 0xff},
  {0x90, 0x88, 0xc3, 0xff},
  {0xa5, 0x93, 0xa5, 0xff},
  {0x92, 0x60, 0x66, 0xff},
  {0x50, 0x4f, 0x9a, 0xff},
  {0x75, 0x8d, 0xc2, 0xff},
  {0xc0, 0xa1, 0x7c, 0xff},
  {0xa3, 0x6a, 0x41, 0xff},
  {0x68, 0x62, 0x8d, 0xff},
  {0x5c, 0x95, 0xbe, 0xff},
  {0xa9, 0xac, 0x68, 0xff},
  {0x80, 0x70, 0x38, 0xff},
  {0x62, 0x69, 0x6e, 0xff},
  {0x67, 0xa1, 0x93, 0xff},
  {0x78, 0xaa, 0x6e, 0xff},
  {0x64, 0x70, 0x55, 0xff},
  {0x7f, 0x9f, 0x9d, 0xff},
  {0x99, 0x9e, 0x7e, 0xff},
  {0x72, 0x68, 0x5d, 0xff},
  {0x55, 0x34, 0x43, 0xff},
  {0x00, 0x00, 0x00, 0xff},
};

struct R_FrameBuffer
{
  Slice<R_B8G8R8A8> colorData;
  Slice<f32> depthData;
  Vector2<i32> size;
};

[[nodiscard]]
inline R_FrameBuffer R_CreateFrameBuffer(Arena_Allocator *arena, Vector2<i32> size)
{
  auto count = static_cast<usize>(size.width) * size.height;
  auto colorData = ArenaPushSliceWithAlignment<R_B8G8R8A8>(arena, count, 16);
  auto depthData = ArenaPushSliceWithAlignment<f32>(arena, count, 16);
  if (colorData.ptr == nullptr || depthData.ptr == nullptr)
  {
    return {};
  }
  return {colorData, depthData, size};
}

[[nodiscard]]
inline Rect<i32> R_PresentViewport(Vector2<i32> clientSize, Vector2<i32> fbSize)
{
  i32 scale = Minimum(clientSize.width / fbSize.width,
                      clientSize.height / fbSize.height);
  scale = Maximum(scale, 1);
  auto size = fbSize * scale;
  return {.position = (clientSize - size) / 2, .size = size};
}

inline void R_PresentFrameBuffer(HWND hWnd, HDC hDC, R_FrameBuffer frameBuffer)
{
  RECT client;
  GetClientRect(hWnd, &client);
  i32 clientW = client.right - client.left;
  i32 clientH = client.bottom - client.top;

  auto viewport = R_PresentViewport({clientW, clientH}, frameBuffer.size);
  i32 dstW = viewport.width;
  i32 dstH = viewport.height;
  i32 dstX = viewport.x;
  i32 dstY = viewport.y;

  // black bars around the image
  PatBlt(hDC, 0, 0, clientW, dstY, BLACKNESS);
  PatBlt(hDC, 0, dstY + dstH, clientW, clientH - dstY - dstH, BLACKNESS);
  PatBlt(hDC, 0, dstY, dstX, dstH, BLACKNESS);
  PatBlt(hDC, dstX + dstW, dstY, clientW - dstX - dstW, dstH, BLACKNESS);

  BITMAPINFO info = {};
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  info.bmiHeader.biWidth = frameBuffer.size.width;
  info.bmiHeader.biHeight = -frameBuffer.size.height;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;

  SetStretchBltMode(hDC, COLORONCOLOR);
  StretchDIBits(hDC, dstX, dstY, dstW, dstH,
                0, 0, frameBuffer.size.width, frameBuffer.size.height,
                frameBuffer.colorData.ptr, &info, DIB_RGB_COLORS, SRCCOPY);
}

[[nodiscard]]
inline Vector2<i32> R_PointFrom2F32(Vector2<f32> point)
{
  i32 x = RoundHalfDownF32(point.x);
  i32 y = RoundHalfDownF32(point.y);
  return {x, y};
}

[[nodiscard]]
inline R_B8G8R8A8 *R_PixelPtrUnsafe(R_FrameBuffer *frameBuffer, Vector2<i32> position)
{
  auto index = static_cast<usize>(position.y) * frameBuffer->size.width + position.x;
  return &frameBuffer->colorData.ptr[index];
}

inline void R_BlendPixelUnsafe(R_FrameBuffer *frameBuffer, Vector2<i32> position, R_B8G8R8A8 color, f32 coverage)
{
  auto &pixel = *R_PixelPtrUnsafe(frameBuffer, position);
  for (auto i = 0; i < 4; i += 1)
  {
    f32 blended = LerpF32(pixel.data[i], color.data[i], coverage);
    pixel.data[i] = RoundHalfUpF32(blended);
  }
}

inline void R_Plot(R_FrameBuffer *frameBuffer, Vector2<i32> position, R_B8G8R8A8 color)
{
  if (RectContains(Rect<i32>{.position = {}, .size = frameBuffer->size}, position))
  {
    auto &pixel = *R_PixelPtrUnsafe(frameBuffer, position);
    pixel = color;
  }
}

[[nodiscard]]
inline R_B8G8R8A8 R_Peek(const R_FrameBuffer *frameBuffer, Vector2<i32> position)
{
  R_B8G8R8A8 color = {};
  if (RectContains(Rect<i32>{.position = {}, .size = frameBuffer->size}, position))
  {
    auto index = static_cast<usize>(position.y) * frameBuffer->size.width + position.x;
    color = frameBuffer->colorData.ptr[index];
  }
  return color;
}

inline void R_Clear(R_FrameBuffer *frameBuffer, R_B8G8R8A8 color, bool clearDepth = false)
{
  for (auto &it : frameBuffer->colorData)
  {
    it = color;
  }

  if (clearDepth)
  {
    for (auto &it : frameBuffer->depthData)
    {
      it = FLT_MAX;
    }
  }
}

inline void R_FillRect(R_FrameBuffer *frameBuffer, Rect<i32> rect, R_B8G8R8A8 color)
{
  auto bottomRight = rect.position + rect.size;

  auto left = Maximum(0, rect.x);
  auto right = Minimum(bottomRight.x, frameBuffer->size.width);
  auto top = Maximum(0, rect.y);
  auto bottom = Minimum(bottomRight.y, frameBuffer->size.height);

  for (auto y = top; y < bottom; y += 1)
  {
    for (auto x = left; x < right; x += 1)
    {
      auto &pixel = *R_PixelPtrUnsafe(frameBuffer, {x, y});
      pixel = color;
    }
  }
}

inline void R_FillRectAntialiased(R_FrameBuffer *frameBuffer, Rect<f32> rect, R_B8G8R8A8 color)
{
  auto bottomRight = rect.position + rect.size;
  i32 left = Maximum(0, static_cast<i32>(FloorF32(rect.x)));
  i32 top = Maximum(0, static_cast<i32>(FloorF32(rect.y)));
  i32 right = Minimum(static_cast<i32>(CeilF32(bottomRight.x)), frameBuffer->size.width);
  i32 bottom = Minimum(static_cast<i32>(CeilF32(bottomRight.y)), frameBuffer->size.height);

  for (auto y = top; y < bottom; y += 1)
  {
    f32 coverageY = Minimum(y + 1.0f, bottomRight.y) - Maximum(static_cast<f32>(y), rect.y);
    for (auto x = left; x < right; x += 1)
    {
      f32 coverageX = Minimum(x + 1.0f, bottomRight.x) - Maximum(static_cast<f32>(x), rect.x);
      f32 coverage = coverageX * coverageY;
      if (coverage >= 1.0f)
      {
        auto &pixel = *R_PixelPtrUnsafe(frameBuffer, {x, y});
        pixel = color;
      }
      else
      {
        R_BlendPixelUnsafe(frameBuffer, {x, y}, color, coverage);
      }
    }
  }
}

inline void R_RasterImage(R_FrameBuffer *frameBuffer, const Image *image,
                          Rect<i32> srcRect, Rect<i32> dstRect)
{
  assert(image->format == Image_Format_BGRA8 && "Image must be decoded as Image_Format_BGRA8");

  auto imageSize = Vector2Cast<i32>(image->size);
  if (srcRect.width <= 0 || srcRect.height <= 0)
  {
    srcRect.position = {};
    srcRect.size = imageSize;
  }

  auto bottomRight = dstRect.position + dstRect.size;
  i32 left = Maximum(0, dstRect.x);
  i32 right = Minimum(bottomRight.x, frameBuffer->size.width);
  i32 top = Maximum(0, dstRect.y);
  i32 bottom = Minimum(bottomRight.y, frameBuffer->size.height);
  if (left >= right)
  {
    return;
  }

  usize fbWidth = frameBuffer->size.width;
  i32 imagePitch = imageSize.width;

  auto srcPixels = reinterpret_cast<const R_B8G8R8A8 *>(image->pixels.ptr);
  srcPixels += srcRect.y * imagePitch + srcRect.x;

  f32 dstStartX = static_cast<f32>(dstRect.x);
  f32 dstEndX = static_cast<f32>(bottomRight.x);
  f32 dstStartY = static_cast<f32>(dstRect.y);
  f32 dstEndY = static_cast<f32>(bottomRight.y);
  auto dstPixels = frameBuffer->colorData.ptr;

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto srcColumns = ArenaPushSlice<i32>(scratch.arena, right - left);
  for (i32 x = left; x < right; x += 1)
  {
    f32 u = InverseLerpF32(dstStartX, dstEndX, x + 0.5f);
    srcColumns[x - left] = FloorF32(u * srcRect.width);
  }

  for (i32 y = top; y < bottom; y += 1)
  {
    auto row = dstPixels + y * fbWidth + left;
    f32 v = InverseLerpF32(dstStartY, dstEndY, y + 0.5f);
    i32 srcY = FloorF32(v * srcRect.height);
    auto srcRow = srcPixels + srcY * imagePitch;

    for (usize i = 0; i < srcColumns.count; i += 1)
    {
      row[i] = srcRow[srcColumns[i]];
    }
  }
}

inline void R_PutGlyph_(R_FrameBuffer *frameBuffer, const u8 *rows, Vector2<i32> glyphSize,
                        Vector2<i32> position, R_B8G8R8A8 color)
{
  auto topLeft = Vector2<i32>{Maximum(0, -position.x), Maximum(0, -position.y)};
  auto bottomRight = Vector2<i32>{Minimum(glyphSize.width, frameBuffer->size.width - position.x),
                                  Minimum(glyphSize.height, frameBuffer->size.height - position.y)};

  for (auto y = topLeft.y; y < bottomRight.y; y += 1)
  {
    u8 row = rows[y];
    for (auto x = topLeft.x; x < bottomRight.x; x += 1)
    {
      if (row & (0x80u >> x))
      {
        auto &pixel = *R_PixelPtrUnsafe(frameBuffer, position + Vector2<i32>{x, y});
        pixel = color;
      }
    }
  }
}

inline void R_PutChar(R_FrameBuffer *frameBuffer, R_BitmapFont font,
                      Vector2<i32> position, R_B8G8R8A8 color, u8 c)
{
  const u8 *rows = font.bitmap.ptr + c * font.glyphSize.height;
  R_PutGlyph_(frameBuffer, rows, font.glyphSize, position, color);
}

inline constexpr i32 R_TAB_WIDTH = 4;

inline void R_PutStringAscii(R_FrameBuffer *frameBuffer, R_BitmapFont font,
                             Vector2<i32> position, R_B8G8R8A8 color, String text)
{
  auto cursor = position;
  usize tabStopWidth = font.glyphSize.width * R_TAB_WIDTH;
  for (char c : text)
  {
    if (c == '\n')
    {
      cursor.x = position.x;
      cursor.y += font.glyphSize.height;
      if (cursor.y >= frameBuffer->size.height)
      {
        break;
      }
    }
    else if (c == '\t')
    { // moves at least one glyph
      usize offset = cursor.x - position.x + font.glyphSize.width;
      cursor.x = position.x + static_cast<i32>(AlignForward(offset, tabStopWidth));
    }
    else
    {
      R_PutChar(frameBuffer, font, cursor, color, c);
      cursor.x += font.glyphSize.width;
    }
  }
}

[[nodiscard]]
inline Vector2<i32> R_MeasureStringAscii(R_BitmapFont font, String text)
{
  if (text.count == 0)
  {
    return {};
  }

  usize tabStopWidth = font.glyphSize.width * R_TAB_WIDTH;
  i32 lineWidth = 0;
  i32 maxWidth = 0;
  i32 lineCount = 1;
  for (char c : text)
  {
    if (c == '\n')
    {
      lineWidth = 0;
      lineCount += 1;
    }
    else if (c == '\t')
    {
      usize offset = lineWidth + font.glyphSize.width;
      lineWidth = static_cast<i32>(AlignForward(offset, tabStopWidth));
    }
    else
    {
      lineWidth += font.glyphSize.width;
    }
    maxWidth = Maximum(maxWidth, lineWidth);
  }
  return {maxWidth, lineCount * font.glyphSize.height};
}

PrintfLike(5, 6) inline void R_PrintFmtAscii(R_FrameBuffer *frameBuffer, R_BitmapFont font,
                                             Vector2<i32> position, R_B8G8R8A8 color,
                                             const char *fmt, ...)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  va_list args;
  va_start(args, fmt);
  String text = StringFmtV(scratch.arena, fmt, args);
  va_end(args);

  R_PutStringAscii(frameBuffer, font, position, color, text);
}

[[nodiscard]]
inline const u8 *R_GlyphRows(R_UnicodeFont font, Rune rune)
{
  Pair<usize, bool> search = {};
  Rune candidates[3] = {rune, RUNE_ERROR, '?'};
  for (auto candidate : SliceFromArray(candidates))
  {
    if (candidate <= 0xFFFF)
    {
      search = SliceBinarySearch(font.sortedRunes, candidate);
      if (search.second)
      {
        break;
      }
    }
  }
  assert(search.second && "the font has no glyph for '?'");
  return font.bitmap.ptr + search.first * font.glyphSize.height;
}

inline void R_PutRune(R_FrameBuffer *frameBuffer, R_UnicodeFont font,
                      Vector2<i32> position, R_B8G8R8A8 color, Rune rune)
{
  R_PutGlyph_(frameBuffer, R_GlyphRows(font, rune), font.glyphSize, position, color);
}

inline void R_PutStringUtf8(R_FrameBuffer *frameBuffer, R_UnicodeFont font,
                            Vector2<i32> position, R_B8G8R8A8 color, String text)
{
  auto cursor = position;
  usize tabStopWidth = font.glyphSize.width * R_TAB_WIDTH;
  for (auto decoded : StringRunes(text))
  {
    if (decoded.rune == '\n')
    {
      cursor.x = position.x;
      cursor.y += font.glyphSize.height;
      if (cursor.y >= frameBuffer->size.height)
      {
        break;
      }
    }
    else if (decoded.rune == '\t')
    { // moves at least one glyph
      usize offset = cursor.x - position.x + font.glyphSize.width;
      cursor.x = position.x + static_cast<i32>(AlignForward(offset, tabStopWidth));
    }
    else
    {
      R_PutRune(frameBuffer, font, cursor, color, decoded.rune);
      cursor.x += font.glyphSize.width;
    }
  }
}

[[nodiscard]]
inline Vector2<i32> R_MeasureStringUtf8(R_UnicodeFont font, String text)
{
  if (text.count == 0)
  {
    return {};
  }

  usize tabStopWidth = font.glyphSize.width * R_TAB_WIDTH;
  i32 lineWidth = 0;
  i32 maxWidth = 0;
  i32 lineCount = 1;
  for (auto decoded : StringRunes(text))
  {
    if (decoded.rune == '\n')
    {
      lineWidth = 0;
      lineCount += 1;
    }
    else if (decoded.rune == '\t')
    {
      usize offset = lineWidth + font.glyphSize.width;
      lineWidth = static_cast<i32>(AlignForward(offset, tabStopWidth));
    }
    else
    {
      lineWidth += font.glyphSize.width;
    }
    maxWidth = Maximum(maxWidth, lineWidth);
  }
  return {maxWidth, lineCount * font.glyphSize.height};
}

inline void R_HexDump(R_FrameBuffer *frameBuffer, R_BitmapFont font, Vector2<i32> position,
                      const void *address, i32 bytesPerRow, i32 rowCount)
{
  auto bytes = static_cast<const u8 *>(address);
  for (auto row = 0; row < rowCount; row += 1, bytes += bytesPerRow)
  {
    auto cursor = position + Vector2<i32>{0, row * font.glyphSize.height};
    {
      auto scratch = ScratchBegin();
      Defer(ScratchEnd(scratch););

      auto addressString = StringFmt(scratch.arena, "%zx:", reinterpret_cast<usize>(bytes));
      R_PutStringAscii(frameBuffer, font, cursor, R_CGA_WHITE, addressString);
      cursor.x += static_cast<i32>((addressString.count + 1) * font.glyphSize.width);
    }

    for (auto i = 0; i < bytesPerRow; i += 1)
    {
      u8 value = bytes[i];
      R_PrintFmtAscii(frameBuffer, font, cursor, value == 0 ? R_CGA_LIGHT_GRAY : R_CGA_WHITE, "%02x", value);
      cursor.x += 3 * font.glyphSize.width;
    }
  }
}

[[nodiscard]]
constexpr f32 R_EdgeFunctionCCW(Vector2<f32> a, Vector2<f32> b, Vector2<f32> point)
{
  auto edge = b - a;
  auto toPoint = point - a;
  return edge.y * toPoint.x - edge.x * toPoint.y;
}

inline void R_RasterLine(R_FrameBuffer *frameBuffer, Vector2<f32> a, Vector2<f32> b, R_B8G8R8A8 color)
{
  auto fbSizeF32 = Vector2Cast<f32>(frameBuffer->size);

  auto delta = b - a;
  if (fabsf(delta.x) >= fabsf(delta.y))
  {
    auto start = a.x < b.x ? a : b;
    auto end = a.x < b.x ? b : a;
    f32 slope = (end.y - start.y) / (end.x - start.x);

    i32 first = RoundHalfDownF32(Clamp(start.x, 0.0f, fbSizeF32.x));
    i32 last = RoundHalfDownF32(Clamp(end.x, 0.0f, fbSizeF32.x));
    for (i32 x = first; x < last; x += 1)
    {
      i32 y = FloorF32(start.y + slope * (x + 0.5f - start.x));
      R_Plot(frameBuffer, {x, y}, color);
    }
  }
  else
  {
    auto start = a.y < b.y ? a : b;
    auto end = a.y < b.y ? b : a;
    f32 slope = (end.x - start.x) / (end.y - start.y);

    i32 first = RoundHalfDownF32(Clamp(start.y, 0.0f, fbSizeF32.y));
    i32 last = RoundHalfDownF32(Clamp(end.y, 0.0f, fbSizeF32.y));
    for (i32 y = first; y < last; y += 1)
    {
      i32 x = FloorF32(start.x + slope * (y + 0.5f - start.y));
      R_Plot(frameBuffer, {x, y}, color);
    }
  }
}

inline void R_RasterTriangleFlatCCW(R_FrameBuffer *frameBuffer,
                                    const Vector2<f32> (&positions)[3],
                                    R_B8G8R8A8 color)
{
  auto p0 = positions[0];
  auto p1 = positions[1];
  auto p2 = positions[2];

  auto areaTimes2 = R_EdgeFunctionCCW(p0, p1, p2);
  if (areaTimes2 <= 0)
  {
    return;
  }

  auto leftMost = Minimum3(p0.x, p1.x, p2.x);
  auto rightMost = Maximum3(p0.x, p1.x, p2.x);
  auto topMost = Minimum3(p0.y, p1.y, p2.y);
  auto bottomMost = Maximum3(p0.y, p1.y, p2.y);

  auto fbSizeF32 = Vector2Cast<f32>(frameBuffer->size);
  i32 left = RoundHalfDownF32(Clamp(leftMost, 0.0f, fbSizeF32.x));
  i32 right = RoundHalfDownF32(Clamp(rightMost, 0.0f, fbSizeF32.x));
  i32 top = RoundHalfDownF32(Clamp(topMost, 0.0f, fbSizeF32.y));
  i32 bottom = RoundHalfDownF32(Clamp(bottomMost, 0.0f, fbSizeF32.y));

  auto pixels = frameBuffer->colorData.ptr;
  usize fbWidth = frameBuffer->size.width;

  for (i32 y = top; y < bottom; y += 1)
  {
    auto row = pixels + y * fbWidth;
    for (i32 x = left; x < right; x += 1)
    {
      auto center = Vector2Cast<f32>(Vector2<i32>{x, y}) + Vector2<f32>{0.5, 0.5};
      bool isInside = R_EdgeFunctionCCW(p1, p2, center) >= 0 &&
                      R_EdgeFunctionCCW(p2, p0, center) >= 0 &&
                      R_EdgeFunctionCCW(p0, p1, center) >= 0;
      if (isInside)
      {
        row[x] = color;
      }
    }
  }
}

inline void R_RasterTriangleGouraudCCW(R_FrameBuffer *frameBuffer,
                                       const Vector2<f32> (&positions)[3],
                                       const R_B8G8R8A8 (&colors)[3])
{
  auto p0 = positions[0];
  auto p1 = positions[1];
  auto p2 = positions[2];

  auto c0 = colors[0];
  auto c1 = colors[1];
  auto c2 = colors[2];

  auto areaTimes2 = R_EdgeFunctionCCW(p0, p1, p2);
  if (areaTimes2 <= 0)
  {
    return;
  }

  auto leftMost = Minimum3(p0.x, p1.x, p2.x);
  auto rightMost = Maximum3(p0.x, p1.x, p2.x);
  auto topMost = Minimum3(p0.y, p1.y, p2.y);
  auto bottomMost = Maximum3(p0.y, p1.y, p2.y);

  auto fbSizeF32 = Vector2Cast<f32>(frameBuffer->size);
  i32 left = RoundHalfDownF32(Clamp(leftMost, 0.0f, fbSizeF32.x));
  i32 right = RoundHalfDownF32(Clamp(rightMost, 0.0f, fbSizeF32.x));
  i32 top = RoundHalfDownF32(Clamp(topMost, 0.0f, fbSizeF32.y));
  i32 bottom = RoundHalfDownF32(Clamp(bottomMost, 0.0f, fbSizeF32.y));

  auto pixels = frameBuffer->colorData.ptr;
  usize fbWidth = frameBuffer->size.width;
  auto oneOverAreaTimes2 = 1 / areaTimes2; // dont needed when -ffast-math

  for (i32 y = top; y < bottom; y += 1)
  {
    auto row = pixels + y * fbWidth;
    for (i32 x = left; x < right; x += 1)
    {
      auto center = Vector2Cast<f32>(Vector2<i32>{x, y}) + Vector2<f32>{0.5, 0.5};
      auto w0 = R_EdgeFunctionCCW(p1, p2, center) * oneOverAreaTimes2;
      auto w1 = R_EdgeFunctionCCW(p2, p0, center) * oneOverAreaTimes2;
      auto w2 = R_EdgeFunctionCCW(p0, p1, center) * oneOverAreaTimes2;

      if (bool isInside = w0 >= 0 && w1 >= 0 && w2 >= 0; isInside)
      {
        row[x].data[0] = RoundHalfUpF32(w0 * c0.data[0] + w1 * c1.data[0] + w2 * c2.data[0]);
        row[x].data[1] = RoundHalfUpF32(w0 * c0.data[1] + w1 * c1.data[1] + w2 * c2.data[1]);
        row[x].data[2] = RoundHalfUpF32(w0 * c0.data[2] + w1 * c1.data[2] + w2 * c2.data[2]);
        row[x].data[3] = RoundHalfUpF32(w0 * c0.data[3] + w1 * c1.data[3] + w2 * c2.data[3]);
      }
    }
  }
}
