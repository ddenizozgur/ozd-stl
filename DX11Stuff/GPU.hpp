#pragma once

#include "../Base/ArenaAllocator.hpp"
#include "../Base/Core.hpp"
#include "../Win32Stuff/Com.hpp"
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")

struct DX11_Instance
{
  IDXGIFactory6 *factory6;
  IDXGIInfoQueue *infoQueue; // nullptr when the debug layer is off
};

// IDXGIFactory6 requires Windows 10 1803.
[[nodiscard]]
inline DX11_Instance DX11_CreateInstance(bool debug = IS_DEBUG)
{
  UINT flags = 0;
  IDXGIInfoQueue *infoQueue = nullptr;
  if (debug)
  {
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&infoQueue))))
    {
      flags = DXGI_CREATE_FACTORY_DEBUG;
      // also breaks on D3D11 messages
      if (IsDebuggerPresent())
      {
        infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      }
    }
    else
    {
      fprintf(stderr, "'Graphics Tools' optional Windows feature not installed, the debug layer is off\n");
    }
  }

  IDXGIFactory6 *factory6 = nullptr;
  CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory6));
  return {factory6, infoQueue};
}

// for a device created somewhere else
[[nodiscard]]
inline IDXGIFactory6 *DX11_Factory6FromDevice(ID3D11Device *device)
{
  IDXGIDevice *dxgiDevice = nullptr;
  device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
  Defer(dxgiDevice->Release(););

  IDXGIAdapter *adapter = nullptr;
  dxgiDevice->GetAdapter(&adapter);
  Defer(adapter->Release(););

  IDXGIFactory6 *factory6 = nullptr;
  adapter->GetParent(IID_PPV_ARGS(&factory6));
  return factory6;
}

//
// Device
//
[[nodiscard]]
inline HRESULT DX11_CreateDeviceAndImmediateContext_(IDXGIAdapter1 *adapter1, UINT createFlags,
                                                     ID3D11Device **device, ID3D11DeviceContext **context)
{
  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
  HRESULT hRes = D3D11CreateDevice(adapter1, D3D_DRIVER_TYPE_UNKNOWN, nullptr, createFlags,
                                   &featureLevel, 1, D3D11_SDK_VERSION, device, nullptr, context);
  if (FAILED(hRes) && (createFlags & D3D11_CREATE_DEVICE_DEBUG))
  {
    DXGI_ADAPTER_DESC1 desc = {};
    adapter1->GetDesc1(&desc);
    fprintf(stderr, "creating the D3D11 device with the debug layer failed on %ls, retrying without it\n", desc.Description);

    createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
    hRes = D3D11CreateDevice(adapter1, D3D_DRIVER_TYPE_UNKNOWN, nullptr, createFlags,
                             &featureLevel, 1, D3D11_SDK_VERSION, device, nullptr, context);
  }
  return hRes;
}

struct DX11_GPU
{
  ID3D11Device1 *device1;
  ID3D11DeviceContext1 *immediateContext1;
  IDXGIAdapter1 *adapter1;
};

[[nodiscard]]
inline Pair<DX11_GPU, bool> DX11_CreateGPU(DX11_Instance instance,
                                           DXGI_GPU_PREFERENCE preference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                           bool forceWarp = false)
{
  auto factory6 = instance.factory6;
  UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  if (instance.infoQueue != nullptr)
  {
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
  }

  ID3D11Device *baseDevice = nullptr;
  ID3D11DeviceContext *baseImmediateContext = nullptr;
  IDXGIAdapter1 *adapter1 = nullptr;
  HRESULT hRes = E_FAIL;

  if (!forceWarp)
  {
    factory6->EnumAdapterByGpuPreference(0, preference, IID_PPV_ARGS(&adapter1));
    hRes = DX11_CreateDeviceAndImmediateContext_(adapter1, createFlags, &baseDevice, &baseImmediateContext);
  }

  if (FAILED(hRes))
  {
    Win32_ComSafeRelease(&adapter1);
    factory6->EnumWarpAdapter(IID_PPV_ARGS(&adapter1));
    hRes = DX11_CreateDeviceAndImmediateContext_(adapter1, createFlags, &baseDevice, &baseImmediateContext);
  }

  if (FAILED(hRes))
  {
    Win32_ComSafeRelease(&adapter1);
    return {};
  }
  Defer(baseDevice->Release();
        baseImmediateContext->Release(););

  ID3D11Device1 *device1 = nullptr;
  ID3D11DeviceContext1 *immediateContext1 = nullptr;
  baseDevice->QueryInterface(IID_PPV_ARGS(&device1));
  baseImmediateContext->QueryInterface(IID_PPV_ARGS(&immediateContext1));
  return {{device1, immediateContext1, adapter1}, true};
}

[[nodiscard]]
inline DXGI_ADAPTER_DESC1 DX11_GetAdapterDesc1(DX11_GPU gpu)
{
  DXGI_ADAPTER_DESC1 desc = {};
  gpu.adapter1->GetDesc1(&desc);
  return desc;
}

[[nodiscard]]
inline bool DX11_IsWarpGPU(DX11_GPU gpu)
{
  return DX11_GetAdapterDesc1(gpu).Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
}

[[nodiscard]]
inline bool DX11_IsDebugGPU(DX11_GPU gpu)
{
  return gpu.device1->GetCreationFlags() & D3D11_CREATE_DEVICE_DEBUG;
}

inline void DX11_ReleaseGPU(DX11_GPU *gpu)
{
  gpu->immediateContext1->ClearState();
  gpu->immediateContext1->Flush();
  Win32_ComSafeRelease(&gpu->immediateContext1);
  Win32_ComSafeRelease(&gpu->device1);
  Win32_ComSafeRelease(&gpu->adapter1);
}

[[nodiscard]]
inline const char *
DX11_InfoQueueSeverityName_(DXGI_INFO_QUEUE_MESSAGE_SEVERITY severity)
{
  switch (severity)
  {
  case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION:
    return "CORRUPTION";
  case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:
    return "ERROR";
  case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:
    return "WARNING";
  case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO:
    return "INFO";
  default:
    return "MESSAGE";
  }
}

inline void DX11_FlushInfoQueue_(IDXGIInfoQueue *queue)
{
  UINT64 count = queue->GetNumStoredMessagesAllowedByRetrievalFilters(DXGI_DEBUG_ALL);
  for (UINT64 i = 0; i < count; i += 1)
  {
    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch););

    SIZE_T size = 0;
    queue->GetMessage(DXGI_DEBUG_ALL, i, nullptr, &size);

    void *messageBytes = ArenaPushWithAlignment(scratch.arena, size, alignof(DXGI_INFO_QUEUE_MESSAGE));
    auto message = static_cast<DXGI_INFO_QUEUE_MESSAGE *>(messageBytes);
    queue->GetMessage(DXGI_DEBUG_ALL, i, message, &size);

    auto producer = message->Producer == DXGI_DEBUG_D3D11 ? "D3D11" : "DXGI";
    fprintf(stderr, "%s %s: %s\n", producer, DX11_InfoQueueSeverityName_(message->Severity), message->pDescription);
  }
  queue->ClearStoredMessages(DXGI_DEBUG_ALL);
}

// prints the debug layer messages since the last call.
inline void DX11_FlushInfoQueue(DX11_Instance instance)
{
  if (instance.infoQueue != nullptr)
  {
    DX11_FlushInfoQueue_(instance.infoQueue);
  }
}

// Release it last, with the debug layer on it prints every D3D11 and DXGI object still alive.
inline void DX11_ReleaseInstance(DX11_Instance *instance)
{
  Win32_ComSafeRelease(&instance->factory6);
  if (instance->infoQueue != nullptr)
  {
    DX11_FlushInfoQueue_(instance->infoQueue);

    IDXGIDebug1 *debug = nullptr;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
    auto rloFlags = static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    debug->ReportLiveObjects(DXGI_DEBUG_ALL, rloFlags);
    debug->Release();

    DX11_FlushInfoQueue_(instance->infoQueue);
  }
  Win32_ComSafeRelease(&instance->infoQueue);
}
