#pragma once

#include "String.hpp"

inline constexpr usize WIN32_FILE_IO_CHUNK_SIZE = Megabytes(32);
inline constexpr usize WIN32_FILE_READ_DEFAULT_ALIGNMENT = 8;

struct Win32_File
{
  HANDLE handle;
};

using File_Mode_Flags = u32;
enum : File_Mode_Flags
{
  File_Mode_Flag_Read = 1 << 0,
  File_Mode_Flag_Write = 1 << 1, // opens or creates, keeps the content
};

using File_Share_Flags = u32;
enum : File_Share_Flags
{
  File_Share_Flag_None = 0,
  File_Share_Flag_Read = 1 << 0,
  File_Share_Flag_Write = 1 << 1,
  File_Share_Flag_DeleteOrRename = 1 << 2,
};

[[nodiscard]]
inline Pair<Win32_File, bool> Win32_FileOpen(
    String path,
    File_Mode_Flags mode = File_Mode_Flag_Read,
    File_Share_Flags share = File_Share_Flag_Read | File_Share_Flag_Write)
{
  DWORD access = 0;
  DWORD disposition = OPEN_EXISTING;
  if (mode & File_Mode_Flag_Read)
  {
    access |= GENERIC_READ;
  }
  if (mode & File_Mode_Flag_Write)
  {
    access |= GENERIC_WRITE;
    disposition = OPEN_ALWAYS;
  }

  DWORD win32Share = 0;
  if (share & File_Share_Flag_Read)
  {
    win32Share |= FILE_SHARE_READ;
  }
  if (share & File_Share_Flag_Write)
  {
    win32Share |= FILE_SHARE_WRITE;
  }
  if (share & File_Share_Flag_DeleteOrRename)
  {
    win32Share |= FILE_SHARE_DELETE;
  }

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  String16 path16 = Win32_String16From(scratch.arena, path);
  HANDLE handle = CreateFileW(path16.ptr, access, win32Share, nullptr,
                              disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
  {
    return {};
  }
  return {{handle}, true};
}

// false can mean the written data never reached the disk (e.g. a network drive)
inline bool Win32_FileClose(Win32_File *file)
{
  assert(file->handle != nullptr && "closing a file that isn't open");
  bool ok = CloseHandle(file->handle);
  *file = {};
  return ok;
}

[[nodiscard]]
inline usize Win32_FileSize(Win32_File file)
{
  assert(file.handle != nullptr && "file isn't open");
  LARGE_INTEGER size = {};
  GetFileSizeEx(file.handle, &size);
  return static_cast<usize>(size.QuadPart);
}

[[nodiscard]]
inline Pair<Slice<u8>, bool> Win32_FileRead(
    Arena_Allocator *arena, Win32_File file, usize offset, usize size,
    usize alignment = WIN32_FILE_READ_DEFAULT_ALIGNMENT)
{
  assert(file.handle != nullptr && "file isn't open");

  auto ptr = static_cast<u8 *>(ArenaPushEx(arena, size, alignment));
  if (ptr == nullptr)
  {
    return {};
  }

  usize total = 0;
  for (; total < size;)
  {
    usize position = offset + total;
    OVERLAPPED overlapped = {};
    overlapped.Offset = static_cast<DWORD>(position);
    overlapped.OffsetHigh = static_cast<DWORD>(position >> 32);

    DWORD chunk = Minimum(size - total, WIN32_FILE_IO_CHUNK_SIZE);
    DWORD read = 0;
    if (!ReadFile(file.handle, ptr + total, chunk, &read, &overlapped) || read == 0)
    {
      break;
    }
    total += read;
  }

  ArenaPopBy(arena, size - total);
  return {{ptr, total}, total == size};
}

// returns how many bytes were written, less than data.count on failure.
inline usize Win32_FileWrite(Win32_File file, usize offset, Slice<u8> data)
{
  assert(file.handle != nullptr && "file isn't open");

  usize total = 0;
  for (; total < data.count;)
  {
    usize position = offset + total;
    OVERLAPPED overlapped = {};
    overlapped.Offset = static_cast<DWORD>(position);
    overlapped.OffsetHigh = static_cast<DWORD>(position >> 32);

    DWORD chunk = Minimum(data.count - total, WIN32_FILE_IO_CHUNK_SIZE);
    DWORD written = 0;
    if (!WriteFile(file.handle, data.ptr + total, chunk, &written, &overlapped) || written == 0)
    {
      break;
    }
    total += written;
  }
  return total;
}

inline bool Win32_FileSetSize(Win32_File file, usize size)
{
  assert(file.handle != nullptr && "file isn't open");
  FILE_END_OF_FILE_INFO endOfFile = {};
  endOfFile.EndOfFile.QuadPart = static_cast<LONGLONG>(size);
  return SetFileInformationByHandle(file.handle, FileEndOfFileInfo, &endOfFile, sizeof(endOfFile));
}

[[nodiscard]]
inline Pair<Slice<u8>, bool> Win32_ReadEntireFile(
    Arena_Allocator *arena, String path,
    usize alignment = WIN32_FILE_READ_DEFAULT_ALIGNMENT)
{
  auto [opened, ok] = Win32_FileOpen(path);
  if (!ok)
  {
    return {};
  }
  // lambdas can't capture structured bindings in C++17
  Win32_File file = opened;
  Defer(Win32_FileClose(&file););

  usize size = Win32_FileSize(file);
  usize rollback = ArenaPosition(arena);
  auto [bytes, readOk] = Win32_FileRead(arena, file, 0, size, alignment);
  if (!readOk)
  {
    ArenaPopTo(arena, rollback);
    return {};
  }
  return {bytes, true};
}

inline bool Win32_WriteEntireFile(String path, Slice<u8> data)
{
  auto [file, ok] = Win32_FileOpen(path, File_Mode_Flag_Write, File_Share_Flag_None);
  if (!ok)
  {
    return false;
  }

  bool written = Win32_FileSetSize(file, 0) && Win32_FileWrite(file, 0, data) == data.count;
  // e.g. when a network drive can't flush the data
  bool closed = Win32_FileClose(&file);
  return written && closed;
}