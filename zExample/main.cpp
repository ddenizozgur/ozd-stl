#include "../Base/Time.hpp"
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

  Vector2<i32> frameBufferSize = {720, 480};
  RECT wndRect = {0, 0, frameBufferSize.x * 2, frameBufferSize.y * 2};
  AdjustWindowRectExForDpi(&wndRect, dwStyle, false, dwExStyle, g.dpi);

  auto hWnd = CreateWindowExW(dwExStyle, wndClass.lpszClassName, L"SomeApp", dwStyle,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                              nullptr, nullptr, hInstance, nullptr);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic, true);
  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto arena = ArenaInit();
  auto frameBuffer = R_CreateFrameBuffer(&arena, frameBufferSize);

  auto timeNow = Win32_NowNanoseconds();
  auto timeLast = timeNow;

  Vector2<f32> position{};

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

      SpitSomeGlypsUtf8(&frameBuffer, R_FONT_TER_U14N, R_GRAY);

      position += {20 * frameTime, 16 * frameTime};
      R_FillRect(&frameBuffer, R_PositionFrom(position), {32 * 2, 24 * 2}, R_CGA_LIGHT_RED);
      FillRectAntialiased(&frameBuffer, position + Vector2<f32>{0, 40 * 2}, {32 * 2, 24 * 2}, R_CGA_LIGHT_RED);
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