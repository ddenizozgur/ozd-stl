#pragma once

#include "../Base/Core.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "user32")

using Window_Backdrop = u32;
enum : Window_Backdrop
{
  Window_Backdrop_None,
  Window_Backdrop_Mica,
  Window_Backdrop_MicaAlt, // the darker, tinted Mica
  Window_Backdrop_Acrylic,
  Window_Backdrop_COUNT,
};

inline constexpr DWM_SYSTEMBACKDROP_TYPE WIN32_BACKDROP_TYPES[Window_Backdrop_COUNT] = {
  DWMSBT_NONE,
  DWMSBT_MAINWINDOW,
  DWMSBT_TABBEDWINDOW,
  DWMSBT_TRANSIENTWINDOW,
};

// Backdrops require Windows 11 22H2.
inline bool Win32_SetDarkTheme(HWND hWnd, Window_Backdrop backdrop)
{
  BOOL darkTitleBar = true;
  auto hResult = DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                       &darkTitleBar, sizeof(darkTitleBar));
  bool ok = SUCCEEDED(hResult);

  i32 margin = backdrop == Window_Backdrop_None ? 0 : -1;
  MARGINS margins = {margin, margin, margin, margin};
  hResult = DwmExtendFrameIntoClientArea(hWnd, &margins);
  ok &= SUCCEEDED(hResult);

  DWM_SYSTEMBACKDROP_TYPE type = WIN32_BACKDROP_TYPES[backdrop];
  hResult = DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
  ok &= SUCCEEDED(hResult);

  RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
  return ok;
}

inline bool Win32_SetLightTheme(HWND hWnd)
{
  BOOL darkTitleBar = false;
  auto hResult = DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                       &darkTitleBar, sizeof(darkTitleBar));
  bool ok = SUCCEEDED(hResult);

  MARGINS margins = {};
  hResult = DwmExtendFrameIntoClientArea(hWnd, &margins);
  ok &= SUCCEEDED(hResult);

  DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_NONE;
  hResult = DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
  ok &= SUCCEEDED(hResult);

  RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
  return ok;
}

inline bool Win32_SystemUsesDarkTheme()
{
  HIGHCONTRASTW highContrast = {.cbSize = sizeof(highContrast)};
  bool ok = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0);
  if (ok && (highContrast.dwFlags & HCF_HIGHCONTRASTON) != 0)
  {
    return false;
  }

  DWORD usesLightTheme = 1;
  DWORD size = sizeof(usesLightTheme);
  LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                                L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &usesLightTheme, &size);
  return status == ERROR_SUCCESS && usesLightTheme == 0;
}