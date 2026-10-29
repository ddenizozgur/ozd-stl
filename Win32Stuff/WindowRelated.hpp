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
