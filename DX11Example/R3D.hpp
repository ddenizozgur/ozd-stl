#pragma once

// TODO: maybe reversed Z: projection near=1 far=0, clear depth to 0, D3D11_COMPARISON_GREATER

#include "../Base/File.hpp"
#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Presets.hpp"
#include "../DX11Stuff/Shader.hpp"
#include "../Misc/GLTF.hpp"

using Diffuse_Kind = u32;
enum : Diffuse_Kind
{
  Diffuse_Kind_Lambert,
  Diffuse_Kind_Half_Lambert,
  Diffuse_Kind_COUNT,
};

[[nodiscard]]
inline String StringFromDiffuseKind(Diffuse_Kind kind)
{
  switch (kind)
  {
  case Diffuse_Kind_Lambert:
    return StringLit("Lambert");
  case Diffuse_Kind_Half_Lambert:
    return StringLit("Half Lambert");
  default:
    assert(false && "unknown diffuse kind");
    return StringLit("Lambert");
  }
}

struct alignas(16) R3D_Frame_Constants
{
  Matrix4x4 viewProjectionMatrix;
  bool showAlbedo = true;
  Vector3<f32> directionToLight;
  f32 lightIntensity = 4;
  Vector3<f32> lightColor = {1, 0.96f, 0.9f};
  f32 ambient = 0.15f;
  Diffuse_Kind diffuseKind;
};

struct alignas(16) R3D_Mesh_Constants
{
  Matrix4x4 modelMatrix;
};

struct R3D_State
{
  DX11_GPU gpu;

  ID3D11VertexShader *vertexShader;
  ID3D11PixelShader *pixelShader;
  ID3D11RasterizerState *rasterizerCullBack;
  ID3D11DepthStencilState *depthLess;
  ID3D11SamplerState *samplerLinearWrap;
  DX11_Constant_Buffer frameConstants;
  DX11_Constant_Buffer meshConstants;
};

[[nodiscard]]
inline Pair<R3D_State, bool> R3D_Init(DX11_GPU gpu)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto shaderPath = Win32_PathFromExe(scratch.arena, StringLit("../Shaders/R3D.hlsl"));
  auto [vsBytes, vsOk] = DX11_CompileShaderFromFile(scratch.arena, shaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [psBytes, psOk] = DX11_CompileShaderFromFile(scratch.arena, shaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto [frameConstants, frameConstantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(R3D_Frame_Constants));
  auto [meshConstants, meshConstantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(R3D_Mesh_Constants));

  R3D_State r3d = {
    .gpu = gpu,
    .vertexShader = DX11_CreateVertexShader(gpu, vsBytes),
    .pixelShader = DX11_CreatePixelShader(gpu, psBytes),
    .rasterizerCullBack = DX11_CreateRasterizerState(gpu, DX11_RASTERIZER_DESC_CULL_BACK_CCW),
    .depthLess = DX11_CreateDepthState(gpu, DX11_DEPTH_DESC_LESS),
    .samplerLinearWrap = DX11_CreateSamplerState(gpu, DX11_SAMPLER_DESC_LINEAR_WRAP),
    .frameConstants = frameConstants,
    .meshConstants = meshConstants,
  };
  return {r3d, vsOk && psOk && frameConstantsOk && meshConstantsOk};
}

inline void R3D_Release(R3D_State *r3d)
{
  DX11_ReleaseConstantBuffer(&r3d->meshConstants);
  DX11_ReleaseConstantBuffer(&r3d->frameConstants);
  Win32_ComSafeRelease(&r3d->samplerLinearWrap);
  Win32_ComSafeRelease(&r3d->depthLess);
  Win32_ComSafeRelease(&r3d->rasterizerCullBack);
  Win32_ComSafeRelease(&r3d->pixelShader);
  Win32_ComSafeRelease(&r3d->vertexShader);
}

struct R3D_Mesh
{
  DX11_Storage_Buffer positions;
  DX11_Storage_Buffer texcoords;
  DX11_Storage_Buffer normals;
  DX11_Index_Buffer indices;
  DX11_Texture2D albedo;
};

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
  auto texcoords = GLTF_LoadTexcoords(scratch.arena, primitive, 0);
  auto normals = GLTF_LoadNormals(scratch.arena, primitive);
  auto indices = GLTF_LoadIndices(scratch.arena, primitive);
  auto [albedo, albedoOk] = GLTF_LoadAlbedo(scratch.arena, primitive, Image_Format_RGBA8, gltfPath);
  if (!albedoOk)
  { // TODO:
    return {};
  }

  auto [positionsGpu, positionsGpuOk] = DX11_CreateStorageBuffer(gpu, SliceAsBytes(positions), sizeof(Vector3<f32>));
  auto [texcoordsGpu, texcoordsGpuOk] = DX11_CreateStorageBuffer(gpu, SliceAsBytes(texcoords), sizeof(Vector2<f32>));
  auto [normalsGpu, normalsGpuOk] = DX11_CreateStorageBuffer(gpu, SliceAsBytes(normals), sizeof(Vector3<f32>));
  auto [indicesGpu, indicesGpuOk] = DX11_CreateIndexBuffer(gpu, indices);
  auto [albedoGpu, albedoGpuOk] = DX11_CreateTexture2D(gpu, albedo.pixels, albedo.size,
                                                       DX11_TextureFormatFromImage(albedo.format, true),
                                                       true, false);
  return {{positionsGpu, texcoordsGpu, normalsGpu, indicesGpu, albedoGpu},
          positionsGpuOk && texcoordsGpuOk && normalsGpuOk && indicesGpuOk && albedoGpuOk};
}

inline void R3D_ReleaseMesh(R3D_Mesh *mesh)
{
  DX11_ReleaseStorageBuffer(&mesh->positions);
  DX11_ReleaseStorageBuffer(&mesh->texcoords);
  DX11_ReleaseStorageBuffer(&mesh->normals);
  DX11_ReleaseIndexBuffer(&mesh->indices);
  DX11_ReleaseTexture2D(&mesh->albedo);
}

inline void R3D_DrawMesh(R3D_State *r3d, R3D_Mesh mesh, R3D_Mesh_Constants constants)
{
  DX11_UpdateConstantBuffer(r3d->gpu, r3d->meshConstants, SliceAsBytes(SliceMake(&constants, 1)));

  auto context = r3d->gpu.immediateContext1;
  ID3D11ShaderResourceView *vertexViews[] = {mesh.positions.view, mesh.texcoords.view, mesh.normals.view};
  context->VSSetShaderResources(0, 3, vertexViews);
  context->PSSetShaderResources(3, 1, &mesh.albedo.view);
  context->IASetIndexBuffer(mesh.indices.buffer, DXGI_FORMAT_R32_UINT, 0);
  context->DrawIndexed(DX11_GetIndexCount(mesh.indices), 0, 0);
}

inline void R3D_Bind(R3D_State *r3d, R3D_Frame_Constants constants)
{
  DX11_UpdateConstantBuffer(r3d->gpu, r3d->frameConstants, SliceAsBytes(SliceMake(&constants, 1)));

  auto context = r3d->gpu.immediateContext1;
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->VSSetShader(r3d->vertexShader, nullptr, 0);
  ID3D11Buffer *vertexConstants[] = {r3d->frameConstants.buffer, r3d->meshConstants.buffer};
  context->VSSetConstantBuffers(0, 2, vertexConstants);
  context->PSSetShader(r3d->pixelShader, nullptr, 0);
  context->PSSetConstantBuffers(0, 1, &r3d->frameConstants.buffer);
  context->PSSetSamplers(0, 1, &r3d->samplerLinearWrap);
  context->RSSetState(r3d->rasterizerCullBack);
  context->OMSetDepthStencilState(r3d->depthLess, 0);
  context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}
