#pragma once

#include "../Base/ArenaAllocator.hpp"
#include "../Base/String.hpp"
#include "R_Fonts.hpp"
#include <math.h>

#pragma comment(lib, "gdi32")

union R_B8G8R8A8
{
  u8 data[4];
  struct
  {
    u8 b, g, r, a;
  };

  // these only exist for -Wmissing-braces
  R_B8G8R8A8() = default;
  constexpr R_B8G8R8A8(u8 b, u8 g, u8 r, u8 a) : data{b, g, r, a} {}
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

struct R_FrameBuffer
{
  Slice<R_B8G8R8A8> data;
  Vector2<i32> size;
};

[[nodiscard]]
inline R_FrameBuffer R_CreateFrameBuffer(Arena_Allocator *arena, Vector2<i32> size)
{
  auto count = static_cast<usize>(size.width) * size.height;
  auto data = ArenaPushSliceEx<R_B8G8R8A8>(arena, count, 16);
  if (data.ptr == nullptr)
  {
    return {};
  }
  return {data, size};
}

inline void R_PresentFrameBuffer(HWND hWnd, HDC hDC, R_FrameBuffer frameBuffer)
{
  RECT client;
  GetClientRect(hWnd, &client);
  i32 clientW = client.right - client.left;
  i32 clientH = client.bottom - client.top;

  i32 scale = Minimum(clientW / frameBuffer.size.width,
                      clientH / frameBuffer.size.height);
  scale = Maximum(scale, 1);
  i32 dstW = frameBuffer.size.width * scale;
  i32 dstH = frameBuffer.size.height * scale;
  i32 dstX = (clientW - dstW) / 2;
  i32 dstY = (clientH - dstH) / 2;

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
                frameBuffer.data.ptr, &info, DIB_RGB_COLORS, SRCCOPY);
}

[[nodiscard]]
inline Vector2<i32> R_PositionFrom(Vector2<f32> point)
{
  i32 x = RoundHalfDownF32(point.x);
  i32 y = RoundHalfDownF32(point.y);
  return {x, y};
}

[[nodiscard]]
inline bool R_PointInRect(Vector2<i32> rectPosition, Vector2<i32> rectSize, Vector2<i32> position)
{
  auto rectbottomRight = rectPosition + rectSize;
  bool insideX = position.x >= rectPosition.x && position.x < rectbottomRight.x;
  bool insideY = position.y >= rectPosition.y && position.y < rectbottomRight.y;
  return insideX && insideY;
}

[[nodiscard]]
inline R_B8G8R8A8 *R_PixelPtrUnsafe(R_FrameBuffer *frameBuffer, Vector2<i32> position)
{
  auto index = static_cast<usize>(position.y) * frameBuffer->size.width + position.x;
  return &frameBuffer->data.ptr[index];
}

inline void R_Plot(R_FrameBuffer *frameBuffer, Vector2<i32> position, R_B8G8R8A8 color)
{
  if (R_PointInRect({}, frameBuffer->size, position))
  {
    auto &pixel = *R_PixelPtrUnsafe(frameBuffer, position);
    pixel = color;
  }
}

[[nodiscard]]
inline R_B8G8R8A8 R_Peek(const R_FrameBuffer *frameBuffer, Vector2<i32> position)
{
  R_B8G8R8A8 color = {};
  if (R_PointInRect({}, frameBuffer->size, position))
  {
    auto index = static_cast<usize>(position.y) * frameBuffer->size.width + position.x;
    color = frameBuffer->data.ptr[index];
  }
  return color;
}

inline void R_Clear(R_FrameBuffer *frameBuffer, R_B8G8R8A8 color)
{
  for (auto &it : frameBuffer->data)
  {
    it = color;
  }
}

inline void R_FillRect(R_FrameBuffer *frameBuffer,
                       Vector2<i32> position, Vector2<i32> size,
                       R_B8G8R8A8 color)
{
  auto bottomRight = position + size;

  auto left = Maximum(0, position.x);
  auto right = Minimum(bottomRight.x, frameBuffer->size.width);
  auto top = Maximum(0, position.y);
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

inline void R_PutChar(R_FrameBuffer *frameBuffer, R_BitmapFont font,
                      Vector2<i32> position, R_B8G8R8A8 color, u8 c)
{
  auto topLeft = Vector2<i32>{Maximum(0, -position.x), Maximum(0, -position.y)};
  auto bottomRight = Vector2<i32>{Minimum(font.glyphSize.width, frameBuffer->size.width - position.x),
                                  Minimum(font.glyphSize.height, frameBuffer->size.height - position.y)};

  const u8 *rows = font.bitmap.ptr + c * font.glyphSize.height;
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
    {
      // moves at least one glyph, so a tab right at a stop goes to the next one
      usize offset = cursor.x - position.x + font.glyphSize.width;
      cursor.x = position.x + AlignForward(offset, tabStopWidth);
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
      lineWidth = AlignForward(offset, tabStopWidth);
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
      cursor.x += (addressString.count + 1) * font.glyphSize.width;
    }

    for (auto i = 0; i < bytesPerRow; i += 1)
    {
      u8 value = bytes[i];
      R_PrintFmtAscii(frameBuffer, font, cursor, value == 0 ? R_CGA_LIGHT_GRAY : R_CGA_WHITE, "%02x", value);
      // 2 digits and a space
      cursor.x += 3 * font.glyphSize.width;
    }
  }
}