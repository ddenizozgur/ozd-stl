#pragma once

#include "../Base/String.hpp"
#pragma comment(lib, "ntdll")
#pragma comment(lib, "advapi32")

// GetVersionEx and VerifyVersionInfo reports Windows 8 without a manifest
extern "C" __declspec(dllimport) LONG NTAPI RtlGetVersion(RTL_OSVERSIONINFOW *info);

struct Win32_Os_Version
{
  u32 major, minor, build; // Windows 11 still says 10.0
  u32 revision;
  String release; // "25H2", or "1909" on older Windows 10, empty if unknown
  bool server;
};

struct Win32_Os_Version_Globals
{
  Win32_Os_Version version;
  char release[16];
  INIT_ONCE once;
};
inline Win32_Os_Version_Globals win32OsVersion = {};

inline BOOL CALLBACK Win32_OsVersionInit(PINIT_ONCE, PVOID, PVOID *)
{
  auto version = &win32OsVersion.version;

  OSVERSIONINFOEXW info = {};
  info.dwOSVersionInfoSize = sizeof(info);
  RtlGetVersion(reinterpret_cast<RTL_OSVERSIONINFOW *>(&info));
  version->major = info.dwMajorVersion;
  version->minor = info.dwMinorVersion;
  version->build = info.dwBuildNumber;
  version->server = info.wProductType != VER_NT_WORKSTATION;

  const wchar_t *key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
  DWORD revision = 0;
  DWORD size = sizeof(revision);
  if (RegGetValueW(HKEY_LOCAL_MACHINE, key, L"UBR", RRF_RT_REG_DWORD, nullptr, &revision, &size) == ERROR_SUCCESS)
  {
    version->revision = revision;
  }

  // DisplayVersion since 20H2, ReleaseId before it
  wchar_t release[16] = {};
  size = sizeof(release);
  if (RegGetValueW(HKEY_LOCAL_MACHINE, key, L"DisplayVersion", RRF_RT_REG_SZ, nullptr, release, &size) != ERROR_SUCCESS)
  {
    size = sizeof(release);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, key, L"ReleaseId", RRF_RT_REG_SZ, nullptr, release, &size) != ERROR_SUCCESS)
    {
      return TRUE;
    }
  }

  {
    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch););

    String converted = Win32_StringFrom16(scratch.arena, {release, wcsnlen(release, ARRAYSIZE(release))});
    usize count = Minimum(converted.count, sizeof(win32OsVersion.release));
    memcpy(win32OsVersion.release, converted.ptr, count);
    version->release = {win32OsVersion.release, count};
  }

  return TRUE;
}

[[nodiscard]]
inline const Win32_Os_Version &Win32_OsVersion()
{
  InitOnceExecuteOnce(&win32OsVersion.once, Win32_OsVersionInit, nullptr, nullptr);
  return win32OsVersion.version;
}

// Windows 11 still says 10.0
[[nodiscard]]
inline bool Win32_IsWindows11OrGreater()
{
  return Win32_OsVersion().build >= 22000;
}
