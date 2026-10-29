#pragma once

#include "Slice.hpp"
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct Base_Globals
{
  SYSTEM_INFO win32SysInfo;
  INIT_ONCE win32SysInfoOnce;
};
inline Base_Globals base = {};

inline BOOL CALLBACK Win32_SysInfoInit(PINIT_ONCE, PVOID, PVOID *)
{
  GetSystemInfo(&base.win32SysInfo);
  return TRUE;
}

[[nodiscard]]
inline const SYSTEM_INFO &Win32_SysInfo()
{
  InitOnceExecuteOnce(&base.win32SysInfoOnce, Win32_SysInfoInit, nullptr, nullptr);
  return base.win32SysInfo;
}

[[nodiscard]]
inline usize Win32_PageSize()
{
  return Win32_SysInfo().dwPageSize;
}

[[nodiscard]]
inline usize Win32_AllocationGranularity()
{
  return Win32_SysInfo().dwAllocationGranularity;
}

[[nodiscard]]
inline Slice<u8> Win32_VirtualReserve(usize size)
{
  void *ptr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
  if (ptr == nullptr)
  {
    Trap();
  }
  return {static_cast<u8 *>(ptr), size};
}

inline void Win32_VirtualCommit(Slice<u8> range)
{
  void *res = VirtualAlloc(range.ptr, range.count, MEM_COMMIT, PAGE_READWRITE);
  if (res == nullptr)
  {
    Trap();
  }
}

/*
inline bool Win32_VirtualDecommit(Slice<u8> range) {
  if (!VirtualFree(range.ptr, range.count, MEM_DECOMMIT)) {
    assert(false && "VirtualFree(): decommit failed");
    return false;
  }
  return true;
}
*/

inline void Win32_VirtualRelease(Slice<u8> range)
{
  if (!VirtualFree(range.ptr, 0, MEM_RELEASE))
  {
    Trap();
  }
}

// requires Windows 10 version 1803.
#pragma comment(lib, "onecore")

// ptr[0, size) maps the same memory as ptr[size, 2 * size).
[[nodiscard]]
inline Slice<u8> Win32_VirtualReserveRepeated(usize size)
{
  // wastes some address space, but the ring touches every page,
  // so rounding up to 64 KiB would cost real memory
  size = AlignForwardPow2(size, Win32_PageSize());

  // reserve 2 * size
  auto placeholder = static_cast<u8 *>(VirtualAlloc2(nullptr, nullptr, 2 * size,
                                                     MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                                                     PAGE_NOACCESS, nullptr, 0));
  if (placeholder == nullptr)
  {
    Trap();
  }

  // split into two placeholders
  if (!VirtualFree(placeholder, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
  {
    Trap();
  }

  // section backed by the page file
  HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                      static_cast<DWORD>(size >> 32), static_cast<DWORD>(size),
                                      nullptr);
  if (section == nullptr)
  {
    Trap();
  }

  // map the same section over both placeholders
  void *first = MapViewOfFile3(section, nullptr, placeholder, 0, size,
                               MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
  void *second = MapViewOfFile3(section, nullptr, placeholder + size, 0, size,
                                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
  if (first == nullptr || second == nullptr)
  {
    Trap();
  }

  // the views will keep the section alive
  CloseHandle(section);
  return {placeholder, size};
}

inline void Win32_VirtualReleaseRepeated(Slice<u8> repeated)
{
  if (!UnmapViewOfFile(repeated.ptr) || !UnmapViewOfFile(repeated.ptr + repeated.count))
  {
    Trap();
  }
}
