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
  RECT wndRect = {0, 0, 640 * 2, 480 * 2};
  AdjustWindowRectExForDpi(&wndRect, dwStyle, false, dwExStyle, g.dpi);

  auto hWnd = CreateWindowExW(dwExStyle, wndClass.lpszClassName, L"SomeApp", dwStyle,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                              nullptr, nullptr, hInstance, nullptr);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic);
  ShowWindow(hWnd, SW_SHOW);

  auto [arena, arenaOk] = ArenaInit();
  auto frameBuffer = R_CreateFrameBuffer(&arena, {640, 480});

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

      R_Fill(&frameBuffer, {0x44, 0x33, 0x22, 0xff});

      Vector2<i32> cursor = {};
#define X(Font, Rows)                                            \
  R_HexDump(&frameBuffer, Font, cursor, (void *)main, 16, Rows); \
  cursor.y += (Rows + 1) * Font.glyphSize.height;
      X(R_FONT_3DFX, 4)
      X(R_FONT_DEBUG, 4)
      X(R_FONT_MINECRAFT, 4)
#undef X

      for (auto j = cursor.y + 8; j < frameBuffer.size.height; j += 1)
      {
        for (auto i = 0; i < frameBuffer.size.width; i += 1)
        {
          R_PlotUnsafe(&frameBuffer, {i, j}, {u8(i), u8(j), u8(i + j), 0xff});
        }
      }

      R_PresentFrameBuffer(hWnd, hDC, frameBuffer);
    }

    // DwmFlush();
  }
}