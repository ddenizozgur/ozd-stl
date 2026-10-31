#pragma once

#include "../Base/Slice.hpp"
#include "GPU.hpp"
#include <string.h>

// StructuredBuffer<T> in HLSL, filled once at creation
struct DX11_Storage_Buffer
{
  ID3D11ShaderResourceView *view;
};

// StructuredBuffer<T> in HLSL, rewritten by the CPU
struct DX11_Transient_Buffer
{
  ID3D11ShaderResourceView *view;
};

struct DX11_Constant_Buffer
{
  ID3D11Buffer *buffer;
};

struct DX11_Index_Buffer
{
  ID3D11Buffer *buffer;
};

[[nodiscard]]
inline ID3D11ShaderResourceView *
DX11_CreateStructuredView_(DX11_GPU gpu, D3D11_BUFFER_DESC desc, const void *data)
{
  D3D11_SUBRESOURCE_DATA initialData = {.pSysMem = data};

  ID3D11Buffer *buffer = nullptr;
  if (FAILED(gpu.device1->CreateBuffer(&desc, data != nullptr ? &initialData : nullptr, &buffer)))
  {
    return nullptr;
  }
  Defer(buffer->Release(););

  ID3D11ShaderResourceView *view = nullptr;
  gpu.device1->CreateShaderResourceView(buffer, nullptr, &view);
  return view;
}

[[nodiscard]]
inline bool DX11_WriteBuffer_(DX11_GPU gpu, ID3D11Resource *buffer, Slice<u8> bytes)
{
  D3D11_MAPPED_SUBRESOURCE mapped = {};
  if (FAILED(gpu.immediateContext1->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
  {
    return false;
  }
  memcpy(mapped.pData, bytes.ptr, bytes.count);
  gpu.immediateContext1->Unmap(buffer, 0);
  return true;
}

[[nodiscard]]
inline u32 DX11_GetStructuredElementCount_(ID3D11ShaderResourceView *view)
{
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  view->GetDesc(&desc);
  return desc.Buffer.NumElements;
}

//
// Storage
//

// stride must be alignas(4), up to 2048 bytes
[[nodiscard]]
inline Pair<DX11_Storage_Buffer, bool>
DX11_CreateStorageBuffer(DX11_GPU gpu, Slice<u8> bytes, u32 stride)
{
  D3D11_BUFFER_DESC desc = {
      .ByteWidth = static_cast<UINT>(bytes.count),
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = stride,
  };

  auto view = DX11_CreateStructuredView_(gpu, desc, bytes.ptr);
  return {{view}, view != nullptr};
}

[[nodiscard]]
inline u32 DX11_GetStorageBufferElementCount(DX11_Storage_Buffer buffer)
{
  return DX11_GetStructuredElementCount_(buffer.view);
}

inline void DX11_ReleaseStorageBuffer(DX11_Storage_Buffer *buffer)
{
  Win32_ComSafeRelease(&buffer->view);
}

//
// Transient
//

// stride must be alignas(4), up to 2048 bytes
[[nodiscard]]
inline Pair<DX11_Transient_Buffer, bool>
DX11_CreateTransientBuffer(DX11_GPU gpu, u32 capacity, u32 stride)
{
  D3D11_BUFFER_DESC desc = {
      .ByteWidth = capacity * stride,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = stride,
  };

  auto view = DX11_CreateStructuredView_(gpu, desc, nullptr);
  return {{view}, view != nullptr};
}

[[nodiscard]]
inline u32 DX11_GetTransientBufferCapacity(DX11_Transient_Buffer buffer)
{
  return DX11_GetStructuredElementCount_(buffer.view);
}

// Replaces the contents, bytes past bytes.count become undefined.
inline bool DX11_UpdateTransientBuffer(DX11_GPU gpu, DX11_Transient_Buffer buffer, Slice<u8> bytes)
{
  ID3D11Resource *resource = nullptr;
  buffer.view->GetResource(&resource);
  Defer(resource->Release(););

#if IS_DEBUG
  D3D11_BUFFER_DESC desc = {};
  static_cast<ID3D11Buffer *>(resource)->GetDesc(&desc);
  assert(bytes.count <= desc.ByteWidth && "more data than the buffer's capacity");
  assert(bytes.count % desc.StructureByteStride == 0 && "bytes aren't a whole number of elements");
#endif

  return DX11_WriteBuffer_(gpu, resource, bytes);
}

inline void DX11_ReleaseTransientBuffer(DX11_Transient_Buffer *buffer)
{
  Win32_ComSafeRelease(&buffer->view);
}

//
// Constant
//

// size must be alignas(16)
[[nodiscard]]
inline Pair<DX11_Constant_Buffer, bool>
DX11_CreateConstantBuffer(DX11_GPU gpu, u32 size)
{
  D3D11_BUFFER_DESC desc = {
      .ByteWidth = size,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
  };

  ID3D11Buffer *buffer = nullptr;
  gpu.device1->CreateBuffer(&desc, nullptr, &buffer);
  return {{buffer}, buffer != nullptr};
}

inline bool DX11_UpdateConstantBuffer(DX11_GPU gpu, DX11_Constant_Buffer buffer, Slice<u8> bytes)
{
#if IS_DEBUG
  D3D11_BUFFER_DESC desc = {};
  buffer.buffer->GetDesc(&desc);
  assert(bytes.count == desc.ByteWidth && "size differs from the buffer's");
#endif

  return DX11_WriteBuffer_(gpu, buffer.buffer, bytes);
}

inline void DX11_ReleaseConstantBuffer(DX11_Constant_Buffer *buffer)
{
  Win32_ComSafeRelease(&buffer->buffer);
}

//
// Index
//
[[nodiscard]]
inline Pair<DX11_Index_Buffer, bool>
DX11_CreateIndexBuffer(DX11_GPU gpu, Slice<u32> indices)
{
  D3D11_BUFFER_DESC desc = {
      .ByteWidth = static_cast<UINT>(indices.count * sizeof(u32)),
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_INDEX_BUFFER,
  };
  D3D11_SUBRESOURCE_DATA data = {.pSysMem = indices.ptr};

  ID3D11Buffer *buffer = nullptr;
  gpu.device1->CreateBuffer(&desc, &data, &buffer);
  return {{buffer}, buffer != nullptr};
}

[[nodiscard]]
inline u32 DX11_GetIndexCount(DX11_Index_Buffer buffer)
{
  D3D11_BUFFER_DESC desc = {};
  buffer.buffer->GetDesc(&desc);
  return desc.ByteWidth / sizeof(u32);
}

inline void DX11_ReleaseIndexBuffer(DX11_Index_Buffer *buffer)
{
  Win32_ComSafeRelease(&buffer->buffer);
}
