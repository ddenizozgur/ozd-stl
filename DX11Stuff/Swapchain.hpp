#pragma once

#include "../Base/Math.hpp"
#include "../Win32Stuff/Waitable.hpp"
#include "GPU.hpp"

using DX11_Swapchain_Flags = u32;
enum : DX11_Swapchain_Flags
{
  DX11_Swapchain_Flag_None = 0,
  DX11_Swapchain_Flag_Srgb = 1 << 0,
};

struct DX11_Swapchain
{
  IDXGISwapChain2 *dxgiSwapchain2;
  ID3D11RenderTargetView *backbufferView;
  Win32_Waitable frameLatency; // signaled when the swapchain can take a new frame
};

[[nodiscard]]
inline ID3D11RenderTargetView *
DX11_CreateBackbufferView_(ID3D11Device1 *device1, IDXGISwapChain2 *swapchain2, DXGI_FORMAT viewFormat)
{
  ID3D11Texture2D *backbuffer = nullptr;
  if (FAILED(swapchain2->GetBuffer(0, IID_PPV_ARGS(&backbuffer))))
  {
    return nullptr;
  }
  Defer(backbuffer->Release(););

  D3D11_RENDER_TARGET_VIEW_DESC desc = {
    .Format = viewFormat,
    .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
  };

  ID3D11RenderTargetView *view = nullptr;
  device1->CreateRenderTargetView(backbuffer, &desc, &view);
  return view;
}

[[nodiscard]]
inline Pair<DX11_Swapchain, bool>
DX11_CreateSwapchain(IDXGIFactory6 *factory6, DX11_GPU gpu, HWND hWnd,
                     DX11_Swapchain_Flags flags = DX11_Swapchain_Flag_None)
{
  BOOL allowsTearing = false;
  UINT swapchainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  factory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowsTearing, sizeof(allowsTearing));
  if (allowsTearing)
  {
    swapchainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }

  DXGI_SWAP_CHAIN_DESC1 desc = {
    .Format = DXGI_FORMAT_B8G8R8A8_UNORM, // flip model buffers can't be sRGB, only their views
    .SampleDesc = {.Count = 1},
    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
    .BufferCount = 2,
    .Scaling = DXGI_SCALING_STRETCH,
    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
    .Flags = swapchainFlags,
  };

  IDXGISwapChain1 *swapchain1 = nullptr;
  if (FAILED(factory6->CreateSwapChainForHwnd(gpu.device1, hWnd, &desc, nullptr, nullptr, &swapchain1)))
  {
    return {};
  }
  Defer(swapchain1->Release(););

  IDXGISwapChain2 *swapchain2 = nullptr;
  swapchain1->QueryInterface(IID_PPV_ARGS(&swapchain2));

  factory6->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
  swapchain2->SetMaximumFrameLatency(1);
  Win32_Waitable frameLatency = {swapchain2->GetFrameLatencyWaitableObject()};

  DXGI_FORMAT viewFormat = (flags & DX11_Swapchain_Flag_Srgb) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
                                                              : DXGI_FORMAT_B8G8R8A8_UNORM;
  ID3D11RenderTargetView *backbufferView = DX11_CreateBackbufferView_(gpu.device1, swapchain2, viewFormat);
  if (backbufferView == nullptr)
  {
    Win32_CloseWaitable(&frameLatency);
    Win32_ComSafeRelease(&swapchain2);
    return {};
  }

  return {{swapchain2, backbufferView, frameLatency}, true};
}

[[nodiscard]]
inline DXGI_SWAP_CHAIN_DESC1 DX11_GetSwapchainDesc1(DX11_Swapchain swapchain)
{
  DXGI_SWAP_CHAIN_DESC1 desc = {};
  swapchain.dxgiSwapchain2->GetDesc1(&desc);
  return desc;
}

[[nodiscard]]
inline Vector2<u32> DX11_GetSwapchainSize(DX11_Swapchain swapchain)
{
  auto desc = DX11_GetSwapchainDesc1(swapchain);
  return {desc.Width, desc.Height};
}

// zero size keeps the old buffers.
inline bool DX11_ResizeSwapchain(DX11_GPU gpu, DX11_Swapchain *swapchain, Vector2<u32> size)
{
  DXGI_SWAP_CHAIN_DESC1 desc = DX11_GetSwapchainDesc1(*swapchain);
  bool minimized = size.x == 0 || size.y == 0;
  bool sameSize = size.x == desc.Width && size.y == desc.Height;
  if (minimized || sameSize)
  {
    return true;
  }

  D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
  swapchain->backbufferView->GetDesc(&viewDesc);

  gpu.immediateContext1->OMSetRenderTargets(0, nullptr, nullptr);
  Win32_ComSafeRelease(&swapchain->backbufferView);
  gpu.immediateContext1->Flush();

  HRESULT result = swapchain->dxgiSwapchain2->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, desc.Flags);
  // after a failed resize this points at the old buffers again
  swapchain->backbufferView = DX11_CreateBackbufferView_(gpu.device1, swapchain->dxgiSwapchain2, viewDesc.Format);
  return SUCCEEDED(result) && swapchain->backbufferView != nullptr;
}

// blocks until the swapchain can take a new frame.
// Call it before handling input, so the frame starts as late as possible.
inline Wait_Result DX11_WaitForBetterLatency(DX11_Swapchain swapchain, u32 timeoutMs = 1000)
{
  return Win32_Wait(swapchain.frameLatency, timeoutMs);
}

// false when the device was lost.
inline bool DX11_PresentSwapchain(DX11_Swapchain swapchain, u32 syncInterval = 1)
{
  UINT presentFlags = 0;
  BOOL allowsTearing = DX11_GetSwapchainDesc1(swapchain).Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  if (syncInterval == 0 && allowsTearing)
  {
    presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
  }
  return SUCCEEDED(swapchain.dxgiSwapchain2->Present(syncInterval, presentFlags));
}

inline void DX11_ReleaseSwapchain(DX11_Swapchain *swapchain)
{
  Win32_ComSafeRelease(&swapchain->backbufferView);
  Win32_CloseWaitable(&swapchain->frameLatency);
  Win32_ComSafeRelease(&swapchain->dxgiSwapchain2);
}