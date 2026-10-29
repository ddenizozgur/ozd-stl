#pragma once

#include "String.hpp"

#include <shellapi.h>
#pragma comment(lib, "shell32")

[[nodiscard]]
inline Slice<String> Win32_CommandLineArgs(Arena_Allocator *arena)
{
  i32 count = 0;
  LPWSTR *args16 = CommandLineToArgvW(GetCommandLineW(), &count);
  if (args16 == nullptr)
  {
    return {};
  }
  Defer(LocalFree(args16););

  auto res = ArenaPushSlice<String>(arena, count);
  for (usize i = 0; i < res.count; i += 1)
  {
    res[i] = Win32_StringFrom16(arena, {args16[i], wcslen(args16[i])});
  }
  return res;
}
