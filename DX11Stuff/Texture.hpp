#pragma once

#include "../Base/Image.hpp"
#include "GPU.hpp"

struct DX11_Texture2D
{
  ID3D11ShaderResourceView *view;
};

[[nodiscard]]
inline Vector2<u32> DX11_GetTextureSize_(ID3D11View *view)
{
  ID3D11Resource *resource = nullptr;
  view->GetResource(&resource);
  Defer(resource->Release(););

  D3D11_TEXTURE2D_DESC desc = {};
  static_cast<ID3D11Texture2D *>(resource)->GetDesc(&desc);
  return {desc.Width, desc.Height};
}

// TODO: find a better way to handle mips, generateMips uses the immediate context
[[nodiscard]]
inline Pair<DX11_Texture2D, bool>
DX11_CreateTexture2D(DX11_GPU gpu, Slice<u8> pixels, Vector2<u32> size, DXGI_FORMAT format,
                     bool generateMips = false, bool updatable = false)
{
  usize pixelCount = static_cast<usize>(size.width) * size.height;
  assert(pixelCount != 0 && pixels.count % pixelCount == 0 && "pixels don't match with size");

  auto rowPitch = static_cast<UINT>(pixels.count / pixelCount * size.width);
  D3D11_TEXTURE2D_DESC desc = {
    .Width = size.width,
    .Height = size.height,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = format,
    .SampleDesc = {.Count = 1},
    .Usage = D3D11_USAGE_IMMUTABLE,
    .BindFlags = D3D11_BIND_SHADER_RESOURCE,
  };
  if (updatable)
  {
    desc.Usage = D3D11_USAGE_DEFAULT;
  }
  D3D11_SUBRESOURCE_DATA subresource = {.pSysMem = pixels.ptr, .SysMemPitch = rowPitch};
  D3D11_SUBRESOURCE_DATA *initialData = &subresource;
  if (generateMips)
  {
    // GenerateMips DRAWS each smaller copy, so the texture needs to be a render target
    desc.MipLevels = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    // level 0 is uploaded after creation
    initialData = nullptr;
  }

  ID3D11Texture2D *texture = nullptr;
  if (FAILED(gpu.device1->CreateTexture2D(&desc, initialData, &texture)))
  {
    return {};
  }
  Defer(texture->Release(););

  DX11_Texture2D result = {};
  if (FAILED(gpu.device1->CreateShaderResourceView(texture, nullptr, &result.view)))
  {
    return {};
  }

  if (generateMips)
  {
    gpu.immediateContext1->UpdateSubresource(texture, 0, nullptr, pixels.ptr, rowPitch, 0);
    gpu.immediateContext1->GenerateMips(result.view);
  }
  return {result, true};
}

[[nodiscard]]
inline Vector2<u32> DX11_GetTexture2DSize(DX11_Texture2D texture)
{
  return DX11_GetTextureSize_(texture.view);
}

[[nodiscard]]
inline DXGI_FORMAT DX11_TextureFormatFromImage(Image_Format format, bool srgb)
{
  switch (format)
  {
  case Image_Format_RGBA8:
  case Image_Format_RGBA8Premultiplied:
    return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
  }

  assert(!srgb && "only RGBA8 has an sRGB version");
  switch (format)
  {
  case Image_Format_Gray8:
    return DXGI_FORMAT_R8_UNORM;
  case Image_Format_RGBA16:
    return DXGI_FORMAT_R16G16B16A16_UNORM;
  case Image_Format_Gray16:
    return DXGI_FORMAT_R16_UNORM;
  case Image_Format_RGBA16Float:
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  default:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  }
}

// the texture must be created updatable.
inline void DX11_UpdateTexture2D(DX11_GPU gpu, DX11_Texture2D texture,
                                 Vector2<u32> position, Vector2<u32> size,
                                 Slice<u8> pixels, u32 rowPitch)
{
  ID3D11Resource *resource = nullptr;
  texture.view->GetResource(&resource);
  Defer(resource->Release(););

  D3D11_BOX box = {
    .left = position.x,
    .top = position.y,
    .front = 0,
    .right = position.x + size.width,
    .bottom = position.y + size.height,
    .back = 1,
  };
  gpu.immediateContext1->UpdateSubresource(resource, 0, &box, pixels.ptr, rowPitch, 0);
}

inline void DX11_ReleaseTexture2D(DX11_Texture2D *texture)
{
  Win32_ComSafeRelease(&texture->view);
}
