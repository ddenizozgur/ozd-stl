#pragma once

#include "../Base/Core.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "user32")

// keyboard, mouse or touch in current session, across apps.
// game controllers don't count.
[[nodiscard]]
inline u32 Win32_MillisecondsSinceLastInput()
{
  LASTINPUTINFO info = {};
  info.cbSize = sizeof(info);
  GetLastInputInfo(&info);
  return GetTickCount() - info.dwTime;
}
