#pragma once

#include "../Base/Core.hpp"
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>
#pragma comment(lib, "ole32")

using Com_Apartment = u32;
enum : Com_Apartment
{
  Com_Apartment_SingleThreaded, // file dialogs, clipboard, drag-drop
  Com_Apartment_MultiThreaded,
};

struct Win32_Com_Thread
{
  bool needsUninitialize;
};

[[nodiscard]]
inline Win32_Com_Thread Win32_InitComOnThread(Com_Apartment apartment)
{
  DWORD flags = apartment == Com_Apartment_SingleThreaded ? COINIT_APARTMENTTHREADED : COINIT_MULTITHREADED;
  HRESULT result = CoInitializeEx(nullptr, flags | COINIT_DISABLE_OLE1DDE);
  if (result == RPC_E_CHANGED_MODE)
  {
    fprintf(stderr, "Win32_InitComOnThread: the thread is already in the other COM apartment, using that one\n");
  }
  // S_FALSE (already initialized in the same apartment) also needs an uninitialize
  return {.needsUninitialize = SUCCEEDED(result)};
}

inline void Win32_UninitComOnThread(Win32_Com_Thread *com)
{
  if (com->needsUninitialize)
  {
    CoUninitialize();
  }
  com->needsUninitialize = false;
}

template <typename T>
inline void Win32_ComSafeRelease(T **object)
{
  if (*object != nullptr)
  {
    (*object)->Release();
  }
  *object = nullptr;
}
