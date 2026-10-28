#include "Win32Stuff.hpp"
#include "R.hpp"
#include "../Base/TaskDialog.hpp"
#include "../Base/Theme.hpp"
#include "../Base/Time.hpp"

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

[[nodiscard]] constexpr f32 LerpF32(f32 start, f32 end, f32 t)
{
  return (1 - t) * start + t * end;
}

[[nodiscard]] constexpr Vector2<f32> Lerp2F32(Vector2<f32> start, Vector2<f32> end, f32 t)
{
  return {LerpF32(start.x, end.x, t), LerpF32(start.y, end.y, t)};
}

// coverage 0 keeps the pixel, 1 replaces it with color
void BlendPixelUnsafe(R_FrameBuffer *frameBuffer, Vector2<i32> position, R_B8G8R8A8 color, f32 coverage)
{
  auto &pixel = frameBuffer->data.ptr[static_cast<usize>(position.y) * frameBuffer->size.width + position.x];
  for (auto i = 0; i < 4; i += 1)
  {
    f32 blended = pixel.data[i] + (color.data[i] - pixel.data[i]) * coverage;
    pixel.data[i] = static_cast<u8>(blended + 0.5f);
  }
}

// pixel (x, y) covers the area from (x, y) to (x + 1, y + 1).
// edge pixels the rect only partly covers get the color mixed in by the covered fraction.
void FillRectAntialiased(R_FrameBuffer *frameBuffer, Vector2<f32> position, Vector2<f32> size, R_B8G8R8A8 color)
{
  auto bottomRight = position + size;

  auto left = Maximum(0, static_cast<i32>(floorf(position.x)));
  auto top = Maximum(0, static_cast<i32>(floorf(position.y)));
  auto right = Minimum(static_cast<i32>(ceilf(bottomRight.x)), frameBuffer->size.width);
  auto bottom = Minimum(static_cast<i32>(ceilf(bottomRight.y)), frameBuffer->size.height);

  for (auto y = top; y < bottom; y += 1)
  {
    f32 coverageY = Minimum(y + 1.0f, bottomRight.y) - Maximum(static_cast<f32>(y), position.y);
    for (auto x = left; x < right; x += 1)
    {
      f32 coverageX = Minimum(x + 1.0f, bottomRight.x) - Maximum(static_cast<f32>(x), position.x);
      f32 coverage = coverageX * coverageY;
      if (coverage >= 1.0f)
      {
        R_PlotUnsafe(frameBuffer, {x, y}, color);
      }
      else
      {
        BlendPixelUnsafe(frameBuffer, {x, y}, color, coverage);
      }
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
  DWORD dwExStyle = 0; // DX11 flip model: WS_EX_NOREDIRECTIONBITMAP

  Vector2<i32> frameBufferSize = {720, 480};
  RECT wndRect = {0, 0, frameBufferSize.x * 2, frameBufferSize.y * 2};
  AdjustWindowRectExForDpi(&wndRect, dwStyle, false, dwExStyle, g.dpi);

  auto hWnd = CreateWindowExW(dwExStyle, wndClass.lpszClassName, L"SomeApp", dwStyle,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                              nullptr, nullptr, hInstance, nullptr);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic);
  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto [arena, arenaOk] = ArenaInit();
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

      position += {20 * frameTime, 16 * frameTime};
      // R_FillRect(&frameBuffer, R_PixelFrom(position), {32 * 2, 24 * 2}, R_CGA_LIGHT_RED);
      // FillRectAntialiased(&frameBuffer, position + Vector2<f32>{0, 40 * 2}, {32 * 2, 24 * 2}, R_CGA_LIGHT_RED);

      R_HexDump(&frameBuffer, R_FONT_COURIER, {}, (void *)main, 16, 48);
    }

    DwmFlush();
  }
}