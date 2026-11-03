#include "../Base/Time.hpp"
#include "../Win32Stuff/DWrite.hpp"
#include "../Win32Stuff/GlyphAtlas.hpp"
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

// blends the coverage as is
void PutGlyphBitmap(R_FrameBuffer *frameBuffer, const Glyph_Atlas *atlas, Glyph_Atlas_Slot glyph,
                    Vector2<i32> pen, R_B8G8R8A8 color)
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
      auto atlasIndex = (static_cast<usize>(glyph.position.y) + y) * atlas->size.width + glyph.position.x + x;
      if (u8 coverage = atlas->coverage[atlasIndex]; coverage != 0)
      {
        R_BlendPixelUnsafe(frameBuffer, topLeft + Vector2<i32>{x, y}, color, coverage / 255.0f);
      }
    }
  }
}

Vector2<f32> PutStringDWriteUtf8(R_FrameBuffer *frameBuffer, Win32_DWrite dwrite,
                                 Glyph_Atlas *atlas, Vector2<f32> pen, R_B8G8R8A8 color,
                                 Win32_DWrite_Font font, f32 fontSize, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  for (auto shaped : Win32_DWriteShapeText(scratch.arena, dwrite, font, fontSize, text, codeLigatures))
  {
    auto [glyph, glyphOk] = Win32_DWriteGlyphFromAtlas(atlas, dwrite, font, fontSize, shaped.index);
    PutGlyphBitmap(frameBuffer, atlas, glyph, R_PositionFrom(pen + shaped.offset), color);
    pen.x += RoundHalfDownF32(shaped.advance);
  }
  return pen;
}

// blends with the user's gamma and contrast
void PutGlyphBitmapGammaCorrected(R_FrameBuffer *frameBuffer, const Glyph_Atlas *atlas, Glyph_Atlas_Slot glyph,
                                  Vector2<i32> pen, R_B8G8R8A8 color, const Win32_DWrite_Blend_Table_For_Cpu *blendTable)
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
      auto atlasIndex = (static_cast<usize>(glyph.position.y) + y) * atlas->size.width + glyph.position.x + x;
      if (u8 coverage = atlas->coverage[atlasIndex]; coverage != 0)
      {
        R_BlendPixelUnsafe(frameBuffer, topLeft + Vector2<i32>{x, y}, color, blendTable->alphas[coverage]);
      }
    }
  }
}

Vector2<f32> PutStringDWriteUtf8GammaCorrected(R_FrameBuffer *frameBuffer, Win32_DWrite dwrite,
                                               Win32_DWrite_Blend_Params blendParams,
                                               Glyph_Atlas *atlas, Vector2<f32> pen, R_B8G8R8A8 color,
                                               Win32_DWrite_Font font, f32 fontSize, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto blendTable = Win32_DWriteBlendTableForCpu(blendParams, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f});
  for (auto shaped : Win32_DWriteShapeText(scratch.arena, dwrite, font, fontSize, text, codeLigatures))
  {
    auto [glyph, glyphOk] = Win32_DWriteGlyphFromAtlas(atlas, dwrite, font, fontSize, shaped.index);
    PutGlyphBitmapGammaCorrected(frameBuffer, atlas, glyph, R_PositionFrom(pen + shaped.offset), color, &blendTable);
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

  {
    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch););

    for (auto name : Win32_DWriteCollectionFamilyNames(scratch.arena, systemFonts))
    {
      printf("systemFont: %.*s\n", StringArgs(name));
    }
  }

  auto [font, fontOk] = Win32_DWriteFontFromCollection(systemFonts, StringLit("Consolas"), 400);
  Defer(Win32_DWriteReleaseFont(&font);
        Win32_DWriteReleaseCollection(dwrite, &systemFonts);
        Win32_ReleaseDWrite(&dwrite););

  auto [atlas, atlasOk] = GlyphAtlasInit(&arena, {1024, 1024});
  Defer(GlyphAtlasRelease(&atlas););

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

      for (u32 y = 1; y < 30; y += 1)
      {
        auto scratch = ScratchBegin();
        Defer(ScratchEnd(scratch););

        f32 fontSize = y + 8;
        String text = StringFmt(scratch.arena, "DirectWrite: Hello, ğüşıöç ÆØÅ 12345 | %.2f", fontSize);
        PutStringDWriteUtf8GammaCorrected(&frameBuffer, dwrite, blendParams, &atlas, {0, fontSize * y * 2}, R_WHITE, font, fontSize, text);
        PutStringDWriteUtf8(&frameBuffer, dwrite, &atlas, {0, fontSize * (y * 2 + 1)}, R_WHITE, font, fontSize, text);

        PutStringDWriteUtf8GammaCorrected(&frameBuffer, dwrite, blendParams, &atlas, {frameBuffer.size.x / 2.0f, fontSize * y * 2}, R_CGA_YELLOW, font, fontSize, text);
        PutStringDWriteUtf8(&frameBuffer, dwrite, &atlas, {frameBuffer.size.x / 2.0f, fontSize * (y * 2 + 1)}, R_CGA_YELLOW, font, fontSize, text);
      }
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