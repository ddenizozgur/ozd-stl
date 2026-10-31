#pragma once

#include "../Base/Core.hpp"
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")

template <typename T>
inline void Win32_ComSafeRelease(T **object)
{
  if (*object != nullptr)
  {
    (*object)->Release();
  }
  *object = nullptr;
}

// IDXGIFactory6 requires Windows 10 1803.
[[nodiscard]]
inline IDXGIFactory6 *DX11_CreateFactory6(bool debug = IS_DEBUG)
{
  UINT flags = 0;
  if (debug)
  {
    IDXGIInfoQueue *infoQueue = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&infoQueue))))
    {
      flags = DXGI_CREATE_FACTORY_DEBUG;
      infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
      infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      infoQueue->Release();
    }
    else
    {
      fprintf(stderr, "'Graphics Tools' optional Windows feature not installed, the DXGI debug layer is off\n");
    }
  }

  IDXGIFactory6 *factory6 = nullptr;
  CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory6));
  return factory6;
}

// for a device created somewhere else.
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
inline HRESULT DX11_CreateDeviceAndImmediateContext_(IDXGIAdapter1 *adapter, UINT createFlags,
                                                     ID3D11Device **device, ID3D11DeviceContext **context)
{
  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
  HRESULT result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, createFlags,
                                     &featureLevel, 1, D3D11_SDK_VERSION, device, nullptr, context);
  if (FAILED(result) && (createFlags & D3D11_CREATE_DEVICE_DEBUG))
  {
    fprintf(stderr, "creating the D3D11 device with the debug layer failed, retrying without it\n");
    createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
    result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, createFlags,
                               &featureLevel, 1, D3D11_SDK_VERSION, device, nullptr, context);
  }
  return result;
}

using DX11_Device_Flags = u32;
enum : DX11_Device_Flags
{
  DX11_Device_Flag_None = 0,
  DX11_Device_Flag_Debug = 1 << 0,
  DX11_Device_Flag_SingleThreaded = 1 << 1,
  DX11_Device_Flag_ForceWarp = 1 << 2, // software renderer
#if IS_DEBUG
  DX11_Device_Flags_Default = DX11_Device_Flag_Debug,
#else
  DX11_Device_Flags_Default = DX11_Device_Flag_None,
#endif
};

struct DX11_GPU
{
  ID3D11Device1 *device1;
  ID3D11DeviceContext1 *immediateContext1;
  IDXGIAdapter1 *adapter1;
};

[[nodiscard]]
inline Pair<DX11_GPU, bool>
DX11_CreateGPU(IDXGIFactory6 *factory,
               DX11_Device_Flags flags = DX11_Device_Flags_Default,
               DXGI_GPU_PREFERENCE preference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE)
{
  UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  if (flags & DX11_Device_Flag_Debug)
  {
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
  }
  if (flags & DX11_Device_Flag_SingleThreaded)
  {
    createFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
  }

  ID3D11Device *baseDevice = nullptr;
  ID3D11DeviceContext *baseImmediateContext = nullptr;
  IDXGIAdapter1 *adapter1 = nullptr;
  HRESULT result = E_FAIL;

  if (!(flags & DX11_Device_Flag_ForceWarp))
  {
    factory->EnumAdapterByGpuPreference(0, preference, IID_PPV_ARGS(&adapter1));
    result = DX11_CreateDeviceAndImmediateContext_(adapter1, createFlags, &baseDevice, &baseImmediateContext);
  }

  if (FAILED(result))
  {
    Win32_ComSafeRelease(&adapter1);
    factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1));
    result = DX11_CreateDeviceAndImmediateContext_(adapter1, createFlags, &baseDevice, &baseImmediateContext);
  }

  if (FAILED(result))
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

  if (device1->GetCreationFlags() & D3D11_CREATE_DEVICE_DEBUG)
  {
    ID3D11InfoQueue *infoQueue = nullptr;
    device1->QueryInterface(IID_PPV_ARGS(&infoQueue));
    infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
    infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
    infoQueue->Release();
  }

  return {{device1, immediateContext1, adapter1}, true};
}

// name, vendor/device IDs, video memory sizes, flags
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

// Prints COM objects that were never released to the debugger output.
// Call it after releasing the device and the factory. Needs the 'Graphics Tools' optional Windows feature.
inline void DX11_ReportLiveObjects()
{
  IDXGIDebug1 *debug = nullptr;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
  {
    auto rloFlags = static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    debug->ReportLiveObjects(DXGI_DEBUG_ALL, rloFlags);
    debug->Release();
  }
}
