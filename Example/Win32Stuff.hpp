#pragma once

#include "../Base/String.hpp"

#pragma comment(lib, "user32")

[[nodiscard]]
inline String Win32_GetWindowText(Arena_Allocator *arena, HWND hWnd)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  usize length = GetWindowTextLengthW(hWnd);
  auto text = ArenaPushSlice<wchar_t>(scratch.arena, length + 1);
  if (text.ptr == nullptr)
  {
    return {};
  }
  usize copied = GetWindowTextW(hWnd, text.ptr, text.count);
  return Win32_StringFrom16(arena, {text.ptr, copied});
}

inline void Win32_SetWindowText(HWND hWnd, String title)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto title16 = Win32_String16From(scratch.arena, title);
  SetWindowTextW(hWnd, title16.ptr);
}

// ----------------------------------------------------------------

struct Example_Globals
{
  bool shouldClose = false;
  u32 dpi = USER_DEFAULT_SCREEN_DPI;
};
inline Example_Globals g;

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
    // the window moved to a monitor with a different scale, or the user changed the scale.
    // lParam is the size and position Windows suggests at the new dpi.
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