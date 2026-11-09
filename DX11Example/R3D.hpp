#pragma once

// TODO: maybe reversed Z: projection near=1 far=0, clear depth to 0, D3D11_COMPARISON_GREATER

#include "../Base/File.hpp"
#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Presets.hpp"
#include "../DX11Stuff/Shader.hpp"
#include "../Misc/GLTF.hpp"

struct alignas(16) R3D_Constants
{
  Matrix4x4 toClip;
};

struct R3D_Mesh
{
  DX11_Storage_Buffer positions;
  DX11_Index_Buffer indices;
};

struct R3D_State
{
  DX11_GPU gpu;

  ID3D11VertexShader *vertexShader;
  ID3D11PixelShader *pixelShader;
  ID3D11RasterizerState *rasterizerCullBack;
  ID3D11DepthStencilState *depthLess;
  DX11_Constant_Buffer constants;
};

[[nodiscard]]
inline Pair<R3D_State, bool> R3D_Init(DX11_GPU gpu)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto shaderPath = Win32_PathFromExe(scratch.arena, StringLit("../Shaders/R3D.hlsl"));
  auto [vsBytes, vsOk] = DX11_CompileShaderFromFile(scratch.arena, shaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [psBytes, psOk] = DX11_CompileShaderFromFile(scratch.arena, shaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto [constants, constantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(R3D_Constants));

  R3D_State r3d = {
    .gpu = gpu,
    .vertexShader = DX11_CreateVertexShader(gpu, vsBytes),
    .pixelShader = DX11_CreatePixelShader(gpu, psBytes),
    .rasterizerCullBack = DX11_CreateRasterizerState(gpu, DX11_RASTERIZER_DESC_CULL_BACK_CCW),
    .depthLess = DX11_CreateDepthState(gpu, DX11_DEPTH_DESC_LESS),
    .constants = constants,
  };
  return {r3d, vsOk && psOk && constantsOk};
}

inline void R3D_Release(R3D_State *r3d)
{
  DX11_ReleaseConstantBuffer(&r3d->constants);
  Win32_ComSafeRelease(&r3d->depthLess);
  Win32_ComSafeRelease(&r3d->rasterizerCullBack);
  Win32_ComSafeRelease(&r3d->pixelShader);
  Win32_ComSafeRelease(&r3d->vertexShader);
}

// loads the first primitive of the first mesh
[[nodiscard]]
inline Pair<R3D_Mesh, bool> R3D_LoadMesh(DX11_GPU gpu, String gltfPath)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto pathCStr = CStringFrom(scratch.arena, gltfPath);

  cgltf_options options = {};
  cgltf_data *data = nullptr;
  if (cgltf_parse_file(&options, pathCStr, &data) != cgltf_result_success)
  {
    return {};
  }
  Defer(cgltf_free(data););

  if (cgltf_load_buffers(&options, data, pathCStr) != cgltf_result_success ||
      cgltf_validate(data) != cgltf_result_success)
  {
    return {};
  }

  auto primitive = &data->meshes[0].primitives[0];
  auto positions = GLTF_LoadPositions(scratch.arena, primitive);
  auto indices = GLTF_LoadIndices(scratch.arena, primitive);

  auto [positionsGpu, positionsOk] = DX11_CreateStorageBuffer(gpu, SliceAsBytes(positions), sizeof(Vector3<f32>));
  auto [indicesGpu, indicesOk] = DX11_CreateIndexBuffer(gpu, indices);
  return {{positionsGpu, indicesGpu}, positionsOk && indicesOk};
}

inline void R3D_ReleaseMesh(R3D_Mesh *mesh)
{
  DX11_ReleaseIndexBuffer(&mesh->indices);
  DX11_ReleaseStorageBuffer(&mesh->positions);
}

inline void R3D_Bind(R3D_State *r3d)
{
  auto context = r3d->gpu.immediateContext1;
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->VSSetShader(r3d->vertexShader, nullptr, 0);
  context->VSSetConstantBuffers(0, 1, &r3d->constants.buffer);
  context->PSSetShader(r3d->pixelShader, nullptr, 0);
  context->RSSetState(r3d->rasterizerCullBack);
  context->OMSetDepthStencilState(r3d->depthLess, 0);
  context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

inline void R3D_DrawMesh(R3D_State *r3d, R3D_Mesh mesh, const Matrix4x4 &viewProjection, const Matrix4x4 &world)
{
  R3D_Constants constants = {.toClip = viewProjection * world};
  DX11_UpdateConstantBuffer(r3d->gpu, r3d->constants, SliceAsBytes(SliceMake(&constants, 1)));

  auto context = r3d->gpu.immediateContext1;
  context->VSSetShaderResources(0, 1, &mesh.positions.view);
  context->IASetIndexBuffer(mesh.indices.buffer, DXGI_FORMAT_R32_UINT, 0);
  context->DrawIndexed(DX11_GetIndexCount(mesh.indices), 0, 0);
}