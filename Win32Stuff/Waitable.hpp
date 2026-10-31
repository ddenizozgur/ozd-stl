#pragma once

#include "../Base/Slice.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "user32")

struct Win32_Waitable
{
  HANDLE handle;
};

using Wait_Flags = u32;
enum : Wait_Flags
{
  Wait_Flag_None = 0,
  Wait_Flag_RunQueuedCallbacks = 1 << 0, // APCs, e.g. ReadFileEx completions, run inside the wait
};

// index only means something for Signaled and Abandoned
using Wait_Result = u32;
enum : Wait_Result
{
  Wait_Result_Signaled,
  Wait_Result_Abandoned, // a mutex whose owner thread exited without releasing it
  Wait_Result_Timeout,
  Wait_Result_RanQueuedCallbacks,
  Wait_Result_WindowMessages,
  Wait_Result_Failed,
};

[[nodiscard]]
inline Pair<u32, Wait_Result> Win32_WaitResultFrom_(DWORD result, u32 count)
{
  if (result - WAIT_OBJECT_0 < count)
  {
    return {result - WAIT_OBJECT_0, Wait_Result_Signaled};
  }
  if (result - WAIT_ABANDONED_0 < count)
  {
    return {result - WAIT_ABANDONED_0, Wait_Result_Abandoned};
  }
  if (result == WAIT_TIMEOUT)
  {
    return {0, Wait_Result_Timeout};
  }
  if (result == WAIT_IO_COMPLETION)
  {
    return {0, Wait_Result_RanQueuedCallbacks};
  }
  return {0, Wait_Result_Failed};
}

[[nodiscard]]
inline Wait_Result Win32_Wait(Win32_Waitable waitable, u32 timeoutMs = INFINITE, Wait_Flags flags = Wait_Flag_None)
{
  bool alertable = flags & Wait_Flag_RunQueuedCallbacks;
  DWORD result = WaitForSingleObjectEx(waitable.handle, timeoutMs, alertable);
  return Win32_WaitResultFrom_(result, 1).second;
}

// wakes for the first one that's signaled
[[nodiscard]]
inline Pair<u32, Wait_Result>
Win32_WaitAny(Slice<Win32_Waitable> waitables, u32 timeoutMs = INFINITE, Wait_Flags flags = Wait_Flag_None)
{
  assert(waitables.count <= MAXIMUM_WAIT_OBJECTS && "Windows waits on at most 64 handles");

  auto count = static_cast<DWORD>(waitables.count);
  auto handles = reinterpret_cast<const HANDLE *>(waitables.ptr);
  bool alertable = flags & Wait_Flag_RunQueuedCallbacks;
  DWORD result = WaitForMultipleObjectsEx(count, handles, false, timeoutMs, alertable);
  return Win32_WaitResultFrom_(result, count);
}

// Also wakes when the thread has window messages
[[nodiscard]]
inline Pair<u32, Wait_Result>
Win32_WaitAnyOrWindowMessages(Slice<Win32_Waitable> waitables, u32 timeoutMs = INFINITE,
                              Wait_Flags flags = Wait_Flag_None)
{
  assert(waitables.count < MAXIMUM_WAIT_OBJECTS && "Windows waits on at most 63 handles plus window messages");

  auto count = static_cast<DWORD>(waitables.count);
  auto handles = reinterpret_cast<const HANDLE *>(waitables.ptr);
  DWORD waitFlags = MWMO_INPUTAVAILABLE;
  if (flags & Wait_Flag_RunQueuedCallbacks)
  {
    waitFlags |= MWMO_ALERTABLE;
  }
  DWORD result = MsgWaitForMultipleObjectsEx(count, handles, timeoutMs, QS_ALLINPUT, waitFlags);
  if (result == WAIT_OBJECT_0 + count)
  {
    return {0, Wait_Result_WindowMessages};
  }
  return Win32_WaitResultFrom_(result, count);
}

inline void Win32_CloseWaitable(Win32_Waitable *waitable)
{
  CloseHandle(waitable->handle);
  *waitable = {};
}
