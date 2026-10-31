#pragma once

#include "../Base/String.hpp"
#include "GPU.hpp"
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

inline constexpr const char *DX11_VERTEX_SHADER_TARGET = "vs_5_0";
inline constexpr const char *DX11_PIXEL_SHADER_TARGET = "ps_5_0";
inline constexpr const char *DX11_COMPUTE_SHADER_TARGET = "cs_5_0";

[[nodiscard]]
inline Pair<Slice<u8>, bool>
DX11_CompileShaderFromFile(Arena_Allocator *arena, String path,
                           const char *entry, const char *target,
                           Slice<D3D_SHADER_MACRO> defines = {})
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if IS_DEBUG
  flags |= D3DCOMPILE_DEBUG;
#endif

  // D3DCompile reads defines until a {nullptr, nullptr} entry
  auto terminatedDefines = ArenaPushSlice<D3D_SHADER_MACRO>(scratch.arena, defines.count + 1);
  SliceCopyInto(terminatedDefines, defines);
  terminatedDefines[defines.count] = {};

  String16 path16 = Win32_String16From(scratch.arena, path);
  ID3DBlob *bytecode = nullptr, *messages = nullptr;
  HRESULT result = D3DCompileFromFile(path16.ptr, terminatedDefines.ptr,
                                      D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                      entry, target, flags, 0, &bytecode, &messages);
  if (messages != nullptr)
  {
    fprintf(stderr, "%.*s", static_cast<i32>(messages->GetBufferSize()),
            static_cast<const char *>(messages->GetBufferPointer()));
    messages->Release();
  }
  if (FAILED(result))
  {
    fprintf(stderr, "compiling %.*s failed\n", StringArgs(path));
    Win32_ComSafeRelease(&bytecode);
    return {};
  }
  Defer(bytecode->Release(););

  Slice<u8> bytes = {static_cast<u8 *>(bytecode->GetBufferPointer()), bytecode->GetBufferSize()};
  Slice<u8> copy = ArenaCloneSlice(arena, bytes);
  return {copy, copy.ptr != nullptr};
}

[[nodiscard]]
inline ID3D11VertexShader *DX11_CreateVertexShader(DX11_GPU gpu, Slice<u8> bytecode)
{
  ID3D11VertexShader *shader = nullptr;
  gpu.device1->CreateVertexShader(bytecode.ptr, bytecode.count, nullptr, &shader);
  return shader;
}

[[nodiscard]]
inline ID3D11PixelShader *DX11_CreatePixelShader(DX11_GPU gpu, Slice<u8> bytecode)
{
  ID3D11PixelShader *shader = nullptr;
  gpu.device1->CreatePixelShader(bytecode.ptr, bytecode.count, nullptr, &shader);
  return shader;
}

[[nodiscard]]
inline ID3D11ComputeShader *DX11_CreateComputeShader(DX11_GPU gpu, Slice<u8> bytecode)
{
  ID3D11ComputeShader *shader = nullptr;
  gpu.device1->CreateComputeShader(bytecode.ptr, bytecode.count, nullptr, &shader);
  return shader;
}
