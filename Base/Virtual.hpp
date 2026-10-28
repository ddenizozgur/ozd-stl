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
    assert(false && "Reserve failed");
    return {};
  }
  return {static_cast<u8 *>(ptr), size};
}

[[nodiscard]]
inline bool Win32_VirtualCommit(Slice<u8> range)
{
  void *res = VirtualAlloc(range.ptr, range.count, MEM_COMMIT, PAGE_READWRITE);
  if (res == nullptr)
  {
    assert(false && "Commit failed");
    return false;
  }
  return true;
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

inline bool Win32_VirtualRelease(Slice<u8> range)
{
  if (!VirtualFree(range.ptr, 0, MEM_RELEASE))
  {
    assert(false && "Release failed");
    return false;
  }
  return true;
}
