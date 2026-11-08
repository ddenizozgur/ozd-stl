#pragma once

// COM must be initialized on the calling thread.

#include "../Base/ArenaAllocator.hpp"
#include "../Base/Image.hpp"
#include "Com.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs")

using Image_File_Format = u32;
enum : Image_File_Format
{
  Image_File_Format_Png,
  Image_File_Format_Jpeg,
  Image_File_Format_Tiff,
};

[[nodiscard]]
inline const GUID &Win32_WicPixelFormatFrom_(Image_Format format)
{
  switch (format)
  {
  case Image_Format_BGRA8:
    return GUID_WICPixelFormat32bppBGRA;
  case Image_Format_BGRA8Premultiplied:
    return GUID_WICPixelFormat32bppPBGRA;
  case Image_Format_RGBA8Premultiplied:
    return GUID_WICPixelFormat32bppPRGBA;
  case Image_Format_Gray8:
    return GUID_WICPixelFormat8bppGray;
  case Image_Format_RGBA16:
    return GUID_WICPixelFormat64bppRGBA;
  case Image_Format_Gray16:
    return GUID_WICPixelFormat16bppGray;
  case Image_Format_RGBA16Float:
    return GUID_WICPixelFormat64bppRGBAHalf;
  case Image_Format_RGBA32Float:
    return GUID_WICPixelFormat128bppRGBAFloat;
  default:
    return GUID_WICPixelFormat32bppRGBA;
  }
}

[[nodiscard]]
inline const GUID &Win32_WicContainerFrom_(Image_File_Format format)
{
  switch (format)
  {
  case Image_File_Format_Jpeg:
    return GUID_ContainerFormatJpeg;
  case Image_File_Format_Tiff:
    return GUID_ContainerFormatTiff;
  default:
    return GUID_ContainerFormatPng;
  }
}

[[nodiscard]]
inline IWICImagingFactory *Win32_CreateWicFactory_()
{
  IWICImagingFactory *factory = nullptr;
  auto hRes = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
  assert(SUCCEEDED(hRes) && "COM isn't initialized on this thread, call Win32_InitComOnThread first");
  (void)hRes;
  return factory;
}

// Float formats come out linear, the others keep the file's gamma.
// false when the bytes aren't an image Windows can read.
[[nodiscard]]
inline Pair<Image, bool>
Win32_WicDecodeImageAs(Arena_Allocator *arena, Slice<u8> fileBytes, Image_Format dstFormat)
{
  assert(fileBytes.count <= MAXDWORD && "WIC reads at most 4 GiB from memory");

  IWICImagingFactory *factory = Win32_CreateWicFactory_();
  Defer(factory->Release(););

  IWICStream *stream = nullptr;
  factory->CreateStream(&stream);
  Defer(stream->Release(););
  stream->InitializeFromMemory(fileBytes.ptr, static_cast<DWORD>(fileBytes.count));

  IWICBitmapDecoder *decoder = nullptr;
  if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)))
  {
    return {};
  }
  Defer(decoder->Release(););

  IWICBitmapFrameDecode *frame = nullptr;
  if (FAILED(decoder->GetFrame(0, &frame)))
  {
    return {};
  }
  Defer(frame->Release(););

  IWICFormatConverter *converter = nullptr;
  factory->CreateFormatConverter(&converter);
  Defer(converter->Release(););
  const GUID &wicFormat = Win32_WicPixelFormatFrom_(dstFormat);
  if (FAILED(converter->Initialize(frame, wicFormat, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom)))
  {
    return {};
  }

  Vector2<u32> size = {};
  converter->GetSize(&size.width, &size.height);
  u32 bytesPerPixel = ImageFormatBytesPerPixel(dstFormat);
  usize rollback = ArenaPosition(arena);
  auto pixels = ArenaPushSlice<u8>(arena, static_cast<usize>(size.width) * size.height * bytesPerPixel);
  if (pixels.ptr == nullptr)
  {
    return {};
  }

  // truncated file fails here
  UINT stride = size.width * bytesPerPixel;
  if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.count), pixels.ptr)))
  {
    ArenaPopTo(arena, rollback);
    return {};
  }
  return {{pixels, size, dstFormat}, true};
}

// Returns the bytes of the file, the pixels are converted to the closest format fileFormat can store.
// JPEG drops alpha.
[[nodiscard]]
inline Pair<Slice<u8>, bool>
Win32_WicEncodeImageAs(Arena_Allocator *arena, Image image, Image_File_Format fileFormat)
{
  IWICImagingFactory *factory = Win32_CreateWicFactory_();
  Defer(factory->Release(););

  const GUID &wicFormat = Win32_WicPixelFormatFrom_(image.format);
  UINT stride = image.size.width * ImageFormatBytesPerPixel(image.format);
  IWICBitmap *bitmap = nullptr;
  if (FAILED(factory->CreateBitmapFromMemory(image.size.width, image.size.height, wicFormat, stride,
                                             static_cast<UINT>(image.pixels.count), image.pixels.ptr, &bitmap)))
  {
    return {};
  }
  Defer(bitmap->Release(););

  IStream *stream = nullptr;
  CreateStreamOnHGlobal(nullptr, TRUE, &stream);
  Defer(stream->Release(););

  IWICBitmapEncoder *encoder = nullptr;
  factory->CreateEncoder(Win32_WicContainerFrom_(fileFormat), nullptr, &encoder);
  Defer(encoder->Release(););
  encoder->Initialize(stream, WICBitmapEncoderNoCache);

  IWICBitmapFrameEncode *frame = nullptr;
  encoder->CreateNewFrame(&frame, nullptr);
  Defer(frame->Release(););
  frame->Initialize(nullptr);
  frame->SetSize(image.size.width, image.size.height);
  // the encoder changes it to the closest format it can store, WriteSource converts to that
  WICPixelFormatGUID frameFormat = wicFormat;
  frame->SetPixelFormat(&frameFormat);
  if (FAILED(frame->WriteSource(bitmap, nullptr)) || FAILED(frame->Commit()) || FAILED(encoder->Commit()))
  {
    return {};
  }

  STATSTG stat = {};
  stream->Stat(&stat, STATFLAG_NONAME);
  auto bytes = ArenaPushSlice<u8>(arena, stat.cbSize.QuadPart);
  if (bytes.ptr == nullptr)
  {
    return {};
  }
  stream->Seek({}, STREAM_SEEK_SET, nullptr);
  stream->Read(bytes.ptr, static_cast<ULONG>(bytes.count), nullptr);
  return {bytes, true};
}
