#pragma once

#include "Core.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

[[nodiscard]]
inline u64 Win32_PerformanceFrequency()
{
  // Windows keeps it in memory shared with every process,
  // so reading it is a plain load, no system call
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  return frequency.QuadPart;
}

[[nodiscard]]
inline u64 Win32_NowNanoseconds()
{
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return MulDivU64(counter.QuadPart, Billion(1), Win32_PerformanceFrequency());
}

[[nodiscard]]
constexpr f64 SecondsFromNano(u64 nanoseconds)
{
  return static_cast<f64>(nanoseconds) / Billion(1);
}

[[nodiscard]]
constexpr f64 MillisecondsFromNano(u64 nanoseconds)
{
  return static_cast<f64>(nanoseconds) / Million(1);
}
