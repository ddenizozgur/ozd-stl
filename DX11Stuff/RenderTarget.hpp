#pragma once

#include "../Base/Math.hpp"
#include "Swapchain.hpp"

struct DX11_Render_Target
{
  ID3D11RenderTargetView *renderView;   // output, SV_Target in HLSL
  ID3D11ShaderResourceView *sampleView; // input, Texture2D in HLSL
};

struct DX11_Depth_Target
{
  ID3D11DepthStencilView *view;
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

//
// Render Target
//
[[nodiscard]]
inline Pair<DX11_Render_Target, bool>
DX11_CreateRenderTarget(DX11_GPU gpu, Vector2<u32> size, DXGI_FORMAT format)
{
  D3D11_TEXTURE2D_DESC desc = {
      .Width = size.width,
      .Height = size.height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = format,
      .SampleDesc = {.Count = 1},
      .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };

  ID3D11Texture2D *texture = nullptr;
  if (FAILED(gpu.device1->CreateTexture2D(&desc, nullptr, &texture)))
  {
    return {};
  }
  Defer(texture->Release(););

  DX11_Render_Target target = {};
  gpu.device1->CreateRenderTargetView(texture, nullptr, &target.renderView);
  gpu.device1->CreateShaderResourceView(texture, nullptr, &target.sampleView);
  if (target.renderView == nullptr || target.sampleView == nullptr)
  {
    Win32_ComSafeRelease(&target.renderView);
    Win32_ComSafeRelease(&target.sampleView);
    return {};
  }
  return {target, true};
}

[[nodiscard]]
inline Vector2<u32> DX11_GetRenderTargetSize(DX11_Render_Target target)
{
  return DX11_GetTextureSize_(target.renderView);
}

inline void DX11_ReleaseRenderTarget(DX11_Render_Target *target)
{
  Win32_ComSafeRelease(&target->renderView);
  Win32_ComSafeRelease(&target->sampleView);
}

//
// Depth Target
//
// format: DXGI_FORMAT_D32_FLOAT, D24_UNORM_S8_UINT to also have a stencil, or D16_UNORM to save memory
[[nodiscard]]
inline Pair<DX11_Depth_Target, bool>
DX11_CreateDepthTarget(DX11_GPU gpu, Vector2<u32> size, DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT)
{
  D3D11_TEXTURE2D_DESC desc = {
      .Width = size.width,
      .Height = size.height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = format,
      .SampleDesc = {.Count = 1},
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
  };

  ID3D11Texture2D *texture = nullptr;
  if (FAILED(gpu.device1->CreateTexture2D(&desc, nullptr, &texture)))
  {
    return {};
  }
  Defer(texture->Release(););

  DX11_Depth_Target target = {};
  gpu.device1->CreateDepthStencilView(texture, nullptr, &target.view);
  return {target, target.view != nullptr};
}

[[nodiscard]]
inline Vector2<u32> DX11_GetDepthTargetSize(DX11_Depth_Target target)
{
  return DX11_GetTextureSize_(target.view);
}

inline void DX11_ReleaseDepthTarget(DX11_Depth_Target *target)
{
  Win32_ComSafeRelease(&target->view);
}

//
// Render Pass
//
inline void DX11_SetRenderPass_(DX11_GPU gpu,
                                Slice<ID3D11RenderTargetView *> renderTargetViews,
                                DX11_Depth_Target depthTarget, Vector2<u32> size)
{
  auto context = gpu.immediateContext1;
  context->OMSetRenderTargets(static_cast<UINT>(renderTargetViews.count),
                              renderTargetViews.ptr, depthTarget.view);

  D3D11_VIEWPORT viewport = {
      .Width = static_cast<f32>(size.width),
      .Height = static_cast<f32>(size.height),
      .MaxDepth = 1,
  };
  context->RSSetViewports(1, &viewport);
}

inline void DX11_SetBackbufferRenderPass(DX11_GPU gpu,
                                         DX11_Swapchain swapchain,
                                         DX11_Depth_Target depthTarget = {})
{
  auto size = DX11_GetSwapchainSize(swapchain);
  DX11_SetRenderPass_(gpu, {&swapchain.backbufferView, 1}, depthTarget, size);
}

// all must be the same size as each other and as depth.
inline void DX11_SetRenderPass(DX11_GPU gpu,
                               Slice<DX11_Render_Target> renderTargets,
                               DX11_Depth_Target depthTarget = {})
{
  assert(renderTargets.count <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
         "D3D11 binds at most 8 render targets");

  ID3D11RenderTargetView *renderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
  for (usize i = 0; i < renderTargets.count; i += 1)
  {
    renderTargetViews[i] = renderTargets[i].renderView;
  }

  Vector2<u32> size = renderTargets.count > 0
                          ? DX11_GetRenderTargetSize(renderTargets[0])
                          : DX11_GetDepthTargetSize(depthTarget);
  DX11_SetRenderPass_(gpu, {renderTargetViews, renderTargets.count}, depthTarget, size);
}

//
// Clear
//
inline void DX11_ClearRenderTarget(DX11_GPU gpu, DX11_Render_Target target, const f32 (&color)[4])
{
  gpu.immediateContext1->ClearRenderTargetView(target.renderView, color);
}

inline void DX11_ClearBackbuffer(DX11_GPU gpu, DX11_Swapchain swapchain, const f32 (&color)[4])
{
  gpu.immediateContext1->ClearRenderTargetView(swapchain.backbufferView, color);
}

inline void DX11_ClearDepthTarget(DX11_GPU gpu, DX11_Depth_Target target, f32 depth = 1)
{
  gpu.immediateContext1->ClearDepthStencilView(target.view, D3D11_CLEAR_DEPTH, depth, 0);
}

// only for depth targets with a stencil, e.g. D24_UNORM_S8_UINT
inline void DX11_ClearStencil(DX11_GPU gpu, DX11_Depth_Target target, u8 stencil = 0)
{
  gpu.immediateContext1->ClearDepthStencilView(target.view, D3D11_CLEAR_STENCIL, 1, stencil);
}
