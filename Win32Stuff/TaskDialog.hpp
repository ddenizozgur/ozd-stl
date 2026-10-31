#pragma once

#include "../Base/String.hpp"
#include <commctrl.h>
#pragma comment(lib, "user32")

using Win32_TaskDialog_Proc = HRESULT(WINAPI *)(HWND owner, HINSTANCE instance, PCWSTR windowTitle,
                                                PCWSTR mainInstruction, PCWSTR content,
                                                TASKDIALOG_COMMON_BUTTON_FLAGS buttons, PCWSTR icon,
                                                int *pressedButton);
inline HRESULT WINAPI Win32_TaskDialogStub(HWND, HINSTANCE, PCWSTR, PCWSTR, PCWSTR,
                                           TASKDIALOG_COMMON_BUTTON_FLAGS, PCWSTR, int *pressedButton)
{
  if (pressedButton != nullptr)
  {
    *pressedButton = 0;
  }
  return E_NOTIMPL;
}
inline Win32_TaskDialog_Proc win32TaskDialog = Win32_TaskDialogStub;

// shell32.dll embeds a manifest resource that asks for Common Controls v6
inline constexpr u16 WIN32_SHELL32_MANIFEST_RESOURCE_ID = 124;

// call it before creating any control or dialog.
inline bool Win32_EnableVisualStyles()
{
  wchar_t sysDir[MAX_PATH];
  u32 sysDirCount = GetSystemDirectoryW(sysDir, MAX_PATH);
  if (sysDirCount == 0 || sysDirCount >= MAX_PATH)
  {
    return false;
  }

  ACTCTXW context = {
      .cbSize = sizeof(context),
      .dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |
                 ACTCTX_FLAG_SET_PROCESS_DEFAULT,
      .lpSource = L"shell32.dll",
      .lpAssemblyDirectory = sysDir,
      .lpResourceName = MAKEINTRESOURCEW(WIN32_SHELL32_MANIFEST_RESOURCE_ID),
  };
  HANDLE handle = CreateActCtxW(&context);
  if (handle == INVALID_HANDLE_VALUE)
  {
    // turns out exe has its own embedded manifest, so the process default is already taken.
    // activates for this thread and the threads it creates from now on.
    context.dwFlags &= ~ACTCTX_FLAG_SET_PROCESS_DEFAULT;
    handle = CreateActCtxW(&context);
    if (handle == INVALID_HANDLE_VALUE)
    {
      return false;
    }

    ULONG_PTR cookie = 0;
    if (!ActivateActCtx(handle, &cookie))
    {
      return false;
    }
  }

  HMODULE comctl = LoadLibraryW(L"comctl32.dll");
  if (comctl != nullptr)
  {
    auto proc = reinterpret_cast<Win32_TaskDialog_Proc>(GetProcAddress(comctl, "TaskDialog"));
    if (proc != nullptr)
    {
      win32TaskDialog = proc;
    }
  }
  return true;
}

using Dialog_Icon = u32;
enum : Dialog_Icon
{
  Dialog_Icon_None,
  Dialog_Icon_Info,
  Dialog_Icon_Warning,
  Dialog_Icon_Error,
  Dialog_Icon_COUNT,
};

using Dialog_Buttons = u32;
enum : Dialog_Buttons
{
  Dialog_Buttons_Ok,
  Dialog_Buttons_OkCancel,
  Dialog_Buttons_YesNo,
  Dialog_Buttons_YesNoCancel,
  Dialog_Buttons_RetryCancel,
  Dialog_Buttons_COUNT,
};

using Dialog_Result = u32;
enum : Dialog_Result
{
  Dialog_Result_None, // the dialog couldn't be shown
  Dialog_Result_Ok,
  Dialog_Result_Cancel,
  Dialog_Result_Yes,
  Dialog_Result_No,
  Dialog_Result_Retry,
  Dialog_Result_COUNT,
};

inline const PCWSTR WIN32_TASK_DIALOG_ICONS[Dialog_Icon_COUNT] = {
    nullptr,
    TD_INFORMATION_ICON,
    TD_WARNING_ICON,
    TD_ERROR_ICON,
};

inline constexpr TASKDIALOG_COMMON_BUTTON_FLAGS WIN32_TASK_DIALOG_BUTTONS[Dialog_Buttons_COUNT] = {
    TDCBF_OK_BUTTON,
    TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON,
    TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
    TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON,
    TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON,
};

inline constexpr UINT WIN32_MESSAGE_BOX_ICONS[Dialog_Icon_COUNT] = {
    0,
    MB_ICONINFORMATION,
    MB_ICONWARNING,
    MB_ICONERROR,
};

inline constexpr UINT WIN32_MESSAGE_BOX_BUTTONS[Dialog_Buttons_COUNT] = {
    MB_OK,
    MB_OKCANCEL,
    MB_YESNO,
    MB_YESNOCANCEL,
    MB_RETRYCANCEL,
};

[[nodiscard]]
inline Dialog_Result Win32_DialogResultFrom(int pressedButton)
{
  switch (pressedButton)
  {
  case IDOK:
    return Dialog_Result_Ok;
  case IDCANCEL:
    return Dialog_Result_Cancel;
  case IDYES:
    return Dialog_Result_Yes;
  case IDNO:
    return Dialog_Result_No;
  case IDRETRY:
    return Dialog_Result_Retry;
  default:
    return Dialog_Result_None;
  }
}

// if owner is nullptr, the dialog disables every window in this thread until it closes.
inline Dialog_Result Win32_TaskDialog(HWND owner, String title, String instruction, String content,
                                      Dialog_Icon icon = Dialog_Icon_None,
                                      Dialog_Buttons buttons = Dialog_Buttons_Ok)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto title16 = Win32_String16From(scratch.arena, title);
  int pressedButton = 0;
  if (win32TaskDialog != Win32_TaskDialogStub)
  {
    auto instruction16 = Win32_String16From(scratch.arena, instruction);
    auto content16 = Win32_String16From(scratch.arena, content);

    auto hResult = win32TaskDialog(owner, nullptr, title16.ptr,
                                   instruction.count != 0 ? instruction16.ptr : nullptr,
                                   content.count != 0 ? content16.ptr : nullptr,
                                   WIN32_TASK_DIALOG_BUTTONS[buttons],
                                   WIN32_TASK_DIALOG_ICONS[icon],
                                   &pressedButton);
    if (FAILED(hResult))
    {
      pressedButton = 0;
    }
  }
  else
  {
    String message = StringFmt(scratch.arena, "%.*s%s%.*s", StringArgs(instruction),
                               instruction.count != 0 && content.count != 0 ? "\n\n" : "",
                               StringArgs(content));
    auto message16 = Win32_String16From(scratch.arena, message);
    UINT type = WIN32_MESSAGE_BOX_BUTTONS[buttons] | WIN32_MESSAGE_BOX_ICONS[icon];
    if (owner == nullptr)
    {
      type |= MB_TASKMODAL;
    }
    pressedButton = MessageBoxW(owner, message16.ptr, title16.ptr, type);
  }
  return Win32_DialogResultFrom(pressedButton);
}