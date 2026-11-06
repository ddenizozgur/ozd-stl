#pragma once

#include "../Base/Math.hpp"
#include "../Base/String.hpp"
#include "../Base/Unicode.hpp"

#define WIN32_LEAN_AND_MEAN
#include <ShellScalingApi.h>
#include <Windows.h>
#include <windowsx.h>
#pragma comment(lib, "user32")
#pragma comment(lib, "shcore")

struct Win32_Monitor
{
  HMONITOR handle;
  String name;           // GDI device name, \\.\DISPLAY1
  Vector2<i32> position; // the primary monitor's top-left is 0,0
  Vector2<i32> size;
  Vector2<i32> workPosition; // without the taskbar
  Vector2<i32> workSize;
  u32 dpi; // USER_DEFAULT_SCREEN_DPI at 100% scaling
  bool primary;
};

inline BOOL CALLBACK Win32_CollectMonitor_(HMONITOR monitor, HDC, LPRECT, LPARAM handles)
{
  FixedListAppend(reinterpret_cast<Fixed_List<HMONITOR> *>(handles), {&monitor, 1});
  return true;
}

// the primary monitor isn't necessarily first
[[nodiscard]]
inline Slice<Win32_Monitor> Win32_GetMonitors(Arena_Allocator *arena)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  auto monitorCount = static_cast<usize>(GetSystemMetrics(SM_CMONITORS));
  auto handles = FixedListInit(ArenaPushSlice<HMONITOR>(scratch.arena, monitorCount));
  EnumDisplayMonitors(nullptr, nullptr, Win32_CollectMonitor_, reinterpret_cast<LPARAM>(&handles));

  auto monitors = ArenaPushSlice<Win32_Monitor>(arena, handles.count);
  if (monitors.ptr == nullptr)
  {
    return {};
  }
  for (usize i = 0; i < handles.count; i += 1)
  {
    MONITORINFOEXW info = {};
    info.cbSize = sizeof(info);
    GetMonitorInfoW(handles[i], &info);

    UINT dpiX = 0;
    UINT dpiY = 0;
    GetDpiForMonitor(handles[i], MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    RECT full = info.rcMonitor;
    RECT work = info.rcWork;
    monitors[i] = {
      .handle = handles[i],
      .name = Win32_StringFrom16(arena, {info.szDevice, wcslen(info.szDevice)}),
      .position = {full.left, full.top},
      .size = {full.right - full.left, full.bottom - full.top},
      .workPosition = {work.left, work.top},
      .workSize = {work.right - work.left, work.bottom - work.top},
      .dpi = dpiX,
      .primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0,
    };
  }
  return monitors;
}

struct Win32_Window_Class_Id
{
  ATOM atom;
  HINSTANCE hInstance;
};

// icon is used for both sizes
// false when a class with this name is already registered
[[nodiscard]]
inline Pair<Win32_Window_Class_Id, bool>
Win32_RegisterWindowClass(String name, WNDPROC wndProc, UINT classStyle = 0, HICON icon = nullptr)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto name16 = Win32_String16From(scratch.arena, name);
  HINSTANCE hInstance = GetModuleHandleW(nullptr);
  WNDCLASSEXW wndClass = {
    .cbSize = sizeof(wndClass),
    .style = classStyle,
    .lpfnWndProc = wndProc,
    .hInstance = hInstance,
    .hIcon = icon,
    .hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)), // IDC_ARROW
    .lpszClassName = name16.ptr,
    .hIconSm = icon,
  };
  ATOM atom = RegisterClassExW(&wndClass);
  if (atom == 0)
  {
    return {};
  }
  return {{atom, hInstance}, true};
}

using Window_Kind = u32;
enum : Window_Kind
{
  Window_Kind_Normal,
  Window_Kind_Borderless,
};

[[nodiscard]]
inline Vector2<i32> Win32_WindowSizeForClient_(Vector2<i32> clientSize, u32 dpi, DWORD style, DWORD exStyle)
{
  auto dpiI32 = static_cast<i32>(dpi);
  RECT rect = {0, 0, clientSize.width * dpiI32 / USER_DEFAULT_SCREEN_DPI, clientSize.height * dpiI32 / USER_DEFAULT_SCREEN_DPI};
  AdjustWindowRectExForDpi(&rect, style, false, exStyle, dpi);
  return {rect.right - rect.left, rect.bottom - rect.top};
}

[[nodiscard]]
inline Vector2<i32> Win32_CenterInWorkArea_(Vector2<i32> workPosition, Vector2<i32> workSize, Vector2<i32> windowSize)
{
  Vector2<i32> centered = workPosition + (workSize - windowSize) / 2;
  return {Maximum(centered.x, workPosition.x), Maximum(centered.y, workPosition.y)};
}

// window starts hidden.
[[nodiscard]]
inline HWND Win32_CreateWindow(Win32_Window_Class_Id classId, String title, Vector2<i32> clientSize,
                               Window_Kind kind, DWORD exStyle = 0, const Win32_Monitor *monitor = nullptr)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto title16 = Win32_String16From(scratch.arena, title);
  // not a real pointer: CreateWindowExW sees the high bits are zero and reads it as an atom
  auto className = reinterpret_cast<const wchar_t *>(static_cast<ULONG_PTR>(classId.atom));
  DWORD style = kind == Window_Kind_Borderless ? WS_POPUP : WS_OVERLAPPEDWINDOW;
  if (monitor != nullptr)
  {
    auto windowSize = Win32_WindowSizeForClient_(clientSize, monitor->dpi, style, exStyle);
    auto position = Win32_CenterInWorkArea_(monitor->workPosition, monitor->workSize, windowSize);
    return CreateWindowExW(exStyle, className, title16.ptr, style,
                           position.x, position.y, windowSize.width, windowSize.height,
                           nullptr, nullptr, classId.hInstance, nullptr);
  }

  HWND hWnd = CreateWindowExW(exStyle, className, title16.ptr, style,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              nullptr, nullptr, classId.hInstance, nullptr);
  if (hWnd == nullptr)
  {
    return nullptr;
  }

  auto windowSize = Win32_WindowSizeForClient_(clientSize, GetDpiForWindow(hWnd), style, exStyle);
  Vector2<i32> position = {};
  UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
  if (kind == Window_Kind_Borderless)
  {
    MONITORINFO info = {.cbSize = sizeof(info)};
    GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &info); // NOTE: maybe MONITOR_DEFAULTTOPRIMARY
    RECT work = info.rcWork;
    position = Win32_CenterInWorkArea_({work.left, work.top}, {work.right - work.left, work.bottom - work.top}, windowSize);
  }
  else
  {
    flags |= SWP_NOMOVE;
  }
  SetWindowPos(hWnd, nullptr, position.x, position.y, windowSize.width, windowSize.height, flags);
  return hWnd;
}

struct Win32_Windowed_Placement
{
  WINDOWPLACEMENT placement;
  DWORD style;
};

[[nodiscard]]
inline Win32_Windowed_Placement Win32_EnterFullscreen(HWND hWnd)
{
  Win32_Windowed_Placement windowed = {.placement = {.length = sizeof(WINDOWPLACEMENT)}};
  GetWindowPlacement(hWnd, &windowed.placement);
  windowed.style = static_cast<DWORD>(GetWindowLongPtrW(hWnd, GWL_STYLE));

  MONITORINFO info = {.cbSize = sizeof(info)};
  GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &info); // NOTE: maybe MONITOR_DEFAULTTOPRIMARY
  RECT full = info.rcMonitor;
  SetWindowLongPtrW(hWnd, GWL_STYLE, windowed.style & ~WS_OVERLAPPEDWINDOW);
  SetWindowPos(hWnd, HWND_TOP, full.left, full.top, full.right - full.left, full.bottom - full.top,
               SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  return windowed;
}

inline void Win32_ExitFullscreen(HWND hWnd, Win32_Windowed_Placement windowed)
{
  SetWindowLongPtrW(hWnd, GWL_STYLE, windowed.style);
  SetWindowPlacement(hWnd, &windowed.placement);
  u32 flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED;
  SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, flags);
}

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
  usize copied = GetWindowTextW(hWnd, text.ptr, static_cast<int>(text.count));
  return Win32_StringFrom16(arena, {text.ptr, copied});
}

inline void Win32_SetWindowText(HWND hWnd, String title)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto title16 = Win32_String16From(scratch.arena, title);
  SetWindowTextW(hWnd, title16.ptr);
}

//
// Input State
//

// Call the OnMessage functions from your own WndProc.
// They only watch messages, your WndProc still decides what to return.
// Assumes a Unicode window class (RegisterClassW), so WM_CHAR carries UTF-16.

struct Key_Bits
{
  u64 words[4]; // one bit per virtual-key code
};

[[nodiscard]]
inline bool KeyBitsGet(const Key_Bits *bits, u8 virtualKey)
{
  return (bits->words[virtualKey >> 6] >> (virtualKey & 63)) & 1;
}

inline void KeyBitsSet(Key_Bits *bits, u8 virtualKey, bool set)
{
  if (set)
  {
    bits->words[virtualKey >> 6] |= u64{1} << (virtualKey & 63);
  }
  else
  {
    bits->words[virtualKey >> 6] &= ~(u64{1} << (virtualKey & 63));
  }
}

using Mouse_Button = u32;
enum : Mouse_Button
{
  Mouse_Button_Left,
  Mouse_Button_Right,
  Mouse_Button_Middle,
  Mouse_Button_X1,
  Mouse_Button_X2,
};

struct Win32_Input_State
{
  Key_Bits keysDown;
  Key_Bits keysPressed;
  Key_Bits keysReleased;
  u8 buttonsDown;
  u8 buttonsPressed;
  u8 buttonsReleased;
  bool mouseInsideClient;
  Vector2<f32> mousePosition; // client pixels, top-left origin
  Vector2<f32> wheelDelta;
  char textStorage[256];
  Fixed_List<char> textWithCtrlChars; // UTF-8, ctrl chars are \b \t \n.
  Utf16_Decoder decoder16;
};

inline void Win32_InputStateClearFrame(Win32_Input_State *inputState)
{
  inputState->keysPressed = {};
  inputState->keysReleased = {};
  inputState->buttonsPressed = 0;
  inputState->buttonsReleased = 0;
  inputState->wheelDelta = {};
  inputState->textWithCtrlChars = FixedListInit(SliceFromArray(inputState->textStorage));
}

inline void Win32_PollMessages(Win32_Input_State *inputState)
{
  Win32_InputStateClearFrame(inputState);
  for (MSG message; PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE);)
  {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
}

[[nodiscard]] inline bool
Win32_KeyDown(const Win32_Input_State *inputState, u8 virtualKey)
{
  return KeyBitsGet(&inputState->keysDown, virtualKey);
}

[[nodiscard]] inline bool
Win32_KeyPressed(const Win32_Input_State *inputState, u8 virtualKey)
{
  return KeyBitsGet(&inputState->keysPressed, virtualKey);
}

[[nodiscard]] inline bool
Win32_KeyReleased(const Win32_Input_State *inputState, u8 virtualKey)
{
  return KeyBitsGet(&inputState->keysReleased, virtualKey);
}

[[nodiscard]] inline bool
Win32_ButtonDown(const Win32_Input_State *inputState, Mouse_Button button)
{
  return (inputState->buttonsDown >> button) & 1;
}

[[nodiscard]] inline bool
Win32_ButtonPressed(const Win32_Input_State *inputState, Mouse_Button button)
{
  return (inputState->buttonsPressed >> button) & 1;
}

[[nodiscard]] inline bool
Win32_ButtonReleased(const Win32_Input_State *inputState, Mouse_Button button)
{
  return (inputState->buttonsReleased >> button) & 1;
}

[[nodiscard]]
inline u8 Win32_MouseButtonBit(UINT message, WPARAM wParam)
{
  Mouse_Button button = Mouse_Button_Left;
  switch (message)
  {
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
  case WM_RBUTTONUP:
    button = Mouse_Button_Right;
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
  case WM_MBUTTONUP:
    button = Mouse_Button_Middle;
    break;
  case WM_XBUTTONDOWN:
  case WM_XBUTTONDBLCLK:
  case WM_XBUTTONUP:
    button = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Mouse_Button_X1 : Mouse_Button_X2;
    break;
  }
  return static_cast<u8>(1 << button);
}

inline void Win32_InputStateOnMessage(Win32_Input_State *inputState, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  {
    auto key = static_cast<u8>(wParam);
    if (!KeyBitsGet(&inputState->keysDown, key))
    {
      KeyBitsSet(&inputState->keysPressed, key, true);
    }
    KeyBitsSet(&inputState->keysDown, key, true);
    break;
  }
  case WM_KEYUP:
  case WM_SYSKEYUP:
  {
    auto key = static_cast<u8>(wParam);
    // Print Screen only sends key up
    if (!KeyBitsGet(&inputState->keysDown, key))
    {
      KeyBitsSet(&inputState->keysPressed, key, true);
    }
    KeyBitsSet(&inputState->keysDown, key, false);
    KeyBitsSet(&inputState->keysReleased, key, true);
    break;
  }
  case WM_KILLFOCUS:
    // key ups after this go to newly focused window, so release every held key now.
    for (u32 i = 0; i < ArrayCount(inputState->keysDown.words); i += 1)
    {
      inputState->keysReleased.words[i] |= inputState->keysDown.words[i];
    }
    inputState->keysDown = {};
    inputState->decoder16 = {};
    break;

  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
  case WM_XBUTTONDOWN:
  case WM_XBUTTONDBLCLK:
  {
    if (inputState->buttonsDown == 0)
    {
      // mouse messages keep coming while dragging outside the window
      SetCapture(hWnd);
    }
    u8 bit = Win32_MouseButtonBit(message, wParam);
    inputState->buttonsDown |= bit;
    inputState->buttonsPressed |= bit;
    break;
  }
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
  case WM_XBUTTONUP:
  {
    u8 bit = Win32_MouseButtonBit(message, wParam);
    inputState->buttonsDown &= ~bit;
    inputState->buttonsReleased |= bit;
    if (inputState->buttonsDown == 0)
    {
      ReleaseCapture();
    }
    break;
  }
  case WM_CAPTURECHANGED:
    // for captures taken away mid-drag (alt-tab, a popup)
    inputState->buttonsReleased |= inputState->buttonsDown;
    inputState->buttonsDown = 0;
    break;

  case WM_MOUSEMOVE:
    inputState->mousePosition = {static_cast<f32>(GET_X_LPARAM(lParam)), static_cast<f32>(GET_Y_LPARAM(lParam))};
    if (!inputState->mouseInsideClient)
    {
      TRACKMOUSEEVENT track = {.cbSize = sizeof(track), .dwFlags = TME_LEAVE, .hwndTrack = hWnd};
      TrackMouseEvent(&track);
      inputState->mouseInsideClient = true;
    }
    break;
  case WM_MOUSELEAVE:
    inputState->mouseInsideClient = false;
    break;
  case WM_MOUSEWHEEL:
  {
    auto delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam));
    inputState->wheelDelta.y += delta / WHEEL_DELTA;
    break;
  }
  case WM_MOUSEHWHEEL:
  {
    auto delta = static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam));
    inputState->wheelDelta.x += delta / WHEEL_DELTA;
    break;
  }

  case WM_CHAR:
  {
    auto unit = static_cast<u16>(wParam);
    auto decoded = Utf16DecoderPush(&inputState->decoder16, unit);
    for (u32 i = 0; i < decoded.count; i += 1)
    {
      Rune rune = decoded.runes[i];
      bool isText = rune >= 0x20 && rune != 0x7F;
      if (rune == '\r')
      {
        Utf8AppendWholeRune(&inputState->textWithCtrlChars, '\n');
      }
      else if (isText || rune == '\b' || rune == '\t')
      {
        Utf8AppendWholeRune(&inputState->textWithCtrlChars, rune);
      }
    }
    break;
  }
  }
}

//
// Window State
//
struct Win32_Window_State
{
  Vector2<i32> clientSize;
  u32 dpi;
  bool focused;
  bool destroyed;
};

inline void Win32_WindowStateOnMessage(Win32_Window_State *windowState,
                                       HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CREATE:
    windowState->dpi = GetDpiForWindow(hWnd);
    break;
  case WM_SIZE:
    windowState->clientSize = {LOWORD(lParam), HIWORD(lParam)};
    break;
  case WM_SETFOCUS:
    windowState->focused = true;
    break;
  case WM_KILLFOCUS:
    windowState->focused = false;
    break;
  case WM_DPICHANGED:
  {
    windowState->dpi = HIWORD(wParam);
    auto suggested = reinterpret_cast<RECT *>(lParam);
    SetWindowPos(hWnd, nullptr, suggested->left, suggested->top,
                 suggested->right - suggested->left, suggested->bottom - suggested->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    break;
  }
  case WM_DESTROY:
    windowState->destroyed = true;
    break;
  }
}
