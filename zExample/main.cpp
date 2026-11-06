#include "../Base/Time.hpp"
#include "../Win32Stuff/TaskDialog.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "R.hpp"

struct Example_Globals
{
  Win32_Window_State windowState;
  Win32_Input_State inputState;
};
inline Example_Globals g;

inline LRESULT CALLBACK ExampleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Win32_WindowStateOnMessage(&g.windowState, hWnd, message, wParam, lParam);
  Win32_InputStateOnMessage(&g.inputState, hWnd, message, wParam, lParam);
  return DefWindowProcW(hWnd, message, wParam, lParam);
}

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

int main()
{
  Vector2<i32> frameBufferSize = {1600, 900};
  auto [classId, classOk] = Win32_RegisterWindowClass(StringLit("ExampleClass"), ExampleWndProc);
  auto hWnd = Win32_CreateWindow(classId, StringLit("SomeApp"), frameBufferSize, Window_Kind_Normal);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic, true);
  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto arena = ArenaInit();
  auto frameBuffer = R_CreateFrameBuffer(&arena, frameBufferSize);

  auto timeNow = Win32_NowNanoseconds();
  auto timeLast = timeNow;

  for (; !g.windowState.destroyed;)
  {
    timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    Win32_PollMessages(&g.inputState);

    {
      auto hDC = GetDC(hWnd);
      Defer(ReleaseDC(hWnd, hDC););

      R_Clear(&frameBuffer, {0x44, 0x33, 0x22, 0xff});
      Defer(R_PresentFrameBuffer(hWnd, hDC, frameBuffer););

      R_HexDump(&frameBuffer, R_FONT_ATI_8X14, {}, (void *)main, 48, 48);
    }

    DwmFlush();
  }
}