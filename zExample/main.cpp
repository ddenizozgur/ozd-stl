#include "../Base/Time.hpp"
#include "../Win32Stuff/DWrite.hpp"
#include "../Win32Stuff/TaskDialog.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "R.hpp"

struct Example_Globals
{
  bool shouldClose = false;
  u32 dpi = USER_DEFAULT_SCREEN_DPI;
};
inline Example_Globals g;

inline LRESULT CALLBACK ExampleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void UpdateTitle(HWND hWnd, f32 frameTime)
{
  static f32 elapsedTime = 0;
  static u32 frameCounter = 0;

  if (elapsedTime >= 1)
  {
    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch););

    auto title = StringFmt(scratch.arena, "SomeApp | %u FPS | %.4f ms",
                           frameCounter, frameTime * Thousand(1));
    Win32_SetWindowText(hWnd, title);

    elapsedTime -= 1;
    frameCounter = 0;
  }

  elapsedTime += frameTime;
  frameCounter += 1;
}

void FillRectAntialiased(R_FrameBuffer *frameBuffer, Vector2<f32> position, Vector2<f32> size, R_B8G8R8A8 color)
{
  auto bottomRight = position + size;
  i32 left = Maximum(0, static_cast<i32>(FloorF32(position.x)));
  i32 top = Maximum(0, static_cast<i32>(FloorF32(position.y)));
  i32 right = Minimum(static_cast<i32>(CeilF32(bottomRight.x)), frameBuffer->size.width);
  i32 bottom = Minimum(static_cast<i32>(CeilF32(bottomRight.y)), frameBuffer->size.height);

  for (auto y = top; y < bottom; y += 1)
  {
    f32 coverageY = Minimum(y + 1.0f, bottomRight.y) - Maximum(static_cast<f32>(y), position.y);
    for (auto x = left; x < right; x += 1)
    {
      f32 coverageX = Minimum(x + 1.0f, bottomRight.x) - Maximum(static_cast<f32>(x), position.x);
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

inline void SpitSomeGlypsUtf8(R_FrameBuffer *frameBuffer, R_UnicodeFont font, R_B8G8R8A8 color)
{
  i32 columns = frameBuffer->size.width / font.glyphSize.width;
  i32 rows = frameBuffer->size.height / font.glyphSize.height;
  usize glyph = 0;
  for (i32 y = 0; y < rows; y += 1)
  {
    for (i32 x = 0; x < columns; x += 1)
    {
      Rune rune = font.sortedRunes[glyph % font.sortedRunes.count];
      Vector2<i32> cell = {x * font.glyphSize.width, y * font.glyphSize.height};
      R_PutRune(frameBuffer, font, cell, color, rune);
      glyph += 1;
    }
  }
}

// blends the coverage as is
void PutGlyphBitmap(R_FrameBuffer *frameBuffer, Glyph_Grayscale glyph, Vector2<i32> pen, R_B8G8R8A8 color)
{
  auto topLeft = pen + glyph.offset;
  i32 width = glyph.size.width;
  i32 height = glyph.size.height;
  i32 left = Maximum(0, -topLeft.x);
  i32 top = Maximum(0, -topLeft.y);
  i32 right = Minimum(width, frameBuffer->size.width - topLeft.x);
  i32 bottom = Minimum(height, frameBuffer->size.height - topLeft.y);

  for (auto y = top; y < bottom; y += 1)
  {
    for (auto x = left; x < right; x += 1)
    {
      u8 coverage = glyph.coverage[static_cast<usize>(y) * width + x];
      if (coverage != 0)
      {
        R_BlendPixelUnsafe(frameBuffer, topLeft + Vector2<i32>{x, y}, color, coverage / 255.0f);
      }
    }
  }
}

Vector2<f32> PutStringUtf8(R_FrameBuffer *frameBuffer, Win32_DWrite dwrite,
                           Vector2<f32> pen, f32 fontSize, R_B8G8R8A8 color,
                           Win32_DWrite_Font font, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  for (auto shaped : Win32_DWriteShapeText(scratch.arena, dwrite, font, fontSize, text, codeLigatures))
  {
    auto [glyph, glyphOk] = Win32_DWriteRasterizeGlyph(scratch.arena, dwrite, font, fontSize, shaped.index);
    PutGlyphBitmap(frameBuffer, glyph, R_PositionFrom(pen + shaped.offset), color);
    pen.x += RoundHalfDownF32(shaped.advance);
  }
  return pen;
}

// blends with the user's gamma and contrast
void PutGlyphBitmapGammaCorrected(R_FrameBuffer *frameBuffer, Glyph_Grayscale glyph, Vector2<i32> pen,
                                  R_B8G8R8A8 color, const Win32_DWrite_Blend_Table_For_Cpu *blendTable)
{
  auto topLeft = pen + glyph.offset;
  i32 width = glyph.size.width;
  i32 height = glyph.size.height;
  i32 left = Maximum(0, -topLeft.x);
  i32 top = Maximum(0, -topLeft.y);
  i32 right = Minimum(width, frameBuffer->size.width - topLeft.x);
  i32 bottom = Minimum(height, frameBuffer->size.height - topLeft.y);

  for (auto y = top; y < bottom; y += 1)
  {
    for (auto x = left; x < right; x += 1)
    {
      u8 coverage = glyph.coverage[static_cast<usize>(y) * width + x];
      if (coverage != 0)
      {
        R_BlendPixelUnsafe(frameBuffer, topLeft + Vector2<i32>{x, y}, color, blendTable->alphas[coverage]);
      }
    }
  }
}

Vector2<f32> PutStringUtf8GammaCorrected(R_FrameBuffer *frameBuffer, Win32_DWrite dwrite,
                                         Win32_DWrite_Blend_Params blendParams,
                                         Vector2<f32> pen, f32 fontSize, R_B8G8R8A8 color,
                                         Win32_DWrite_Font font, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto blendTable = Win32_DWriteBlendTableForCpu(blendParams, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f});
  for (auto shaped : Win32_DWriteShapeText(scratch.arena, dwrite, font, fontSize, text, codeLigatures))
  {
    auto [glyph, glyphOk] = Win32_DWriteRasterizeGlyph(scratch.arena, dwrite, font, fontSize, shaped.index);
    PutGlyphBitmapGammaCorrected(frameBuffer, glyph, R_PositionFrom(pen + shaped.offset), color, &blendTable);
    pen.x += RoundHalfDownF32(shaped.advance);
  }
  return pen;
}

inline auto WIN32_NON_UNICODE_CURSOR_ARROW = MAKEINTRESOURCEA(32512);

int main()
{
  // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  // Win32_EnableVisualStyles();

  HINSTANCE hInstance = GetModuleHandleW(nullptr);

  WNDCLASSW wndClass = {};
  wndClass.hInstance = hInstance;
  wndClass.hCursor = LoadCursorA(nullptr, WIN32_NON_UNICODE_CURSOR_ARROW);
  wndClass.lpszClassName = L"ExampleClass";
  wndClass.lpfnWndProc = ExampleWndProc;
  RegisterClassW(&wndClass);

  DWORD dwStyle = WS_OVERLAPPEDWINDOW;
  DWORD dwExStyle = 0;

  Vector2<i32> frameBufferSize = {1600, 900};
  RECT wndRect = {0, 0, frameBufferSize.x, frameBufferSize.y};
  AdjustWindowRectExForDpi(&wndRect, dwStyle, false, dwExStyle, g.dpi);

  auto hWnd = CreateWindowExW(dwExStyle, wndClass.lpszClassName, L"SomeApp", dwStyle,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                              nullptr, nullptr, hInstance, nullptr);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic, true);
  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto arena = ArenaInit();
  auto frameBuffer = R_CreateFrameBuffer(&arena, frameBufferSize);

  auto dwrite = Win32_CreateDWrite();
  auto blendParams = Win32_DWriteBlendParams(dwrite);
  auto systemFonts = Win32_DWriteCollectionFromSystem(dwrite);
  auto [font, fontOk] = Win32_DWriteFontFromCollection(systemFonts, StringLit("Consolas"), 400);
  Defer(Win32_DWriteReleaseFont(&font);
        Win32_DWriteReleaseCollection(dwrite, &systemFonts);
        Win32_ReleaseDWrite(&dwrite););

  f32 pixelSize = 16;
  auto fontMetrics = Win32_DWriteFontMetrics(font, pixelSize);

  auto timeNow = Win32_NowNanoseconds();
  auto timeLast = timeNow;

  for (; !g.shouldClose;)
  {
    timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    for (MSG message; PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE);)
    {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }

    {
      auto hDC = GetDC(hWnd);
      Defer(ReleaseDC(hWnd, hDC););

      R_Clear(&frameBuffer, {0x44, 0x33, 0x22, 0xff});
      Defer(R_PresentFrameBuffer(hWnd, hDC, frameBuffer););

      PutStringUtf8GammaCorrected(&frameBuffer, dwrite, blendParams, {0, fontMetrics.ascent}, 16, R_WHITE,
                                  font, StringLit("DirectWrite: Hello, ğüşıöç ÆØÅ 12345 | Gamma Corrected"));
      PutStringUtf8(&frameBuffer, dwrite, {0, fontMetrics.ascent + fontMetrics.lineHeight}, 16, R_WHITE,
                    font, StringLit("DirectWrite: Hello, ğüşıöç ÆØÅ 12345"));
    }

    // DwmFlush();
  }
}

inline LRESULT CALLBACK ExampleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (message)
  {
  case WM_CREATE:
    g.dpi = GetDpiForWindow(hWnd);
    break;
  case WM_SIZE:
    break;
  case WM_ERASEBKGND:
    result = 1;
    break;
  case WM_PAINT:
  {
    PAINTSTRUCT paint;
    BeginPaint(hWnd, &paint);
    EndPaint(hWnd, &paint);
    break;
  }
  case WM_DPICHANGED:
  {
    auto suggested = reinterpret_cast<RECT *>(lParam);
    SetWindowPos(hWnd, nullptr,
                 suggested->left, suggested->top,
                 suggested->right - suggested->left,
                 suggested->bottom - suggested->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    g.dpi = HIWORD(wParam);
    break;
  }
  case WM_DESTROY:
    g.shouldClose = true;
    break;
  default:
    result = DefWindowProcW(hWnd, message, wParam, lParam);
    break;
  }
  return result;
}