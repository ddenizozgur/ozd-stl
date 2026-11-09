#pragma once

#include "../Base/File.hpp"
#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Presets.hpp"
#include "../DX11Stuff/Shader.hpp"
#include "../DX11Stuff/Texture.hpp"
#include "../Misc/GlyphAtlas.hpp"

using R_Kind = u32;
enum : R_Kind
{
  R_Kind_Rect,
  R_Kind_Glyph,
  R_Kind_Image,
};

struct R_Instance
{
  Vector2<f32> dstPosition;
  Vector2<f32> dstSize;
  Vector2<f32> srcPosition;
  Vector2<f32> srcSize;
  Vector4<u8> color; // RGBA
  R_Kind kind;
  f32 cornerRadius;
  f32 borderThickness;
};

struct alignas(16) R_Constants
{
  Vector4<f32> gammaRatios;
  Vector2<f32> screenSize;
  f32 enhancedContrast;
};

inline constexpr auto R_INSTANCE_CAPACITY = static_cast<u32>(Kilobytes(32) / sizeof(R_Instance));
inline constexpr Vector2<u32> R_ATLAS_SIZE = {1024, 1024};

struct R_State
{
  DX11_GPU gpu;
  Win32_DWrite dwrite;
  Win32_DWrite_Blend_Params blendParams;

  ID3D11VertexShader *vertexShader;
  ID3D11PixelShader *pixelShader;
  ID3D11SamplerState *samplerLinearWrap;
  ID3D11BlendState *blendPremultiplied;
  DX11_Constant_Buffer constants;

  Fixed_List<R_Instance> instances;
  DX11_Transient_Buffer gpuInstances;
  DX11_Texture2D image;

  Glyph_Atlas atlas;
  DX11_Texture2D atlasTexture;
};

[[nodiscard]]
inline Pair<R_State, bool> R_Init(Arena_Allocator *arena, DX11_GPU gpu, Win32_DWrite dwrite)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  auto shaderPath = Win32_PathFromExe(scratch.arena, StringLit("../Shaders/R.hlsl"));
  auto [vsBytes, vsOk] = DX11_CompileShaderFromFile(scratch.arena, shaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [psBytes, psOk] = DX11_CompileShaderFromFile(scratch.arena, shaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto [constants, constantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(R_Constants));
  auto [gpuInstances, gpuInstancesOk] = DX11_CreateTransientBuffer(gpu, R_INSTANCE_CAPACITY, sizeof(R_Instance));
  auto [atlas, atlasOk] = GlyphAtlasInit(arena, R_ATLAS_SIZE);
  auto [atlasTexture, atlasTextureOk] = DX11_CreateTexture2D(gpu, atlas.coverage, atlas.size, DXGI_FORMAT_R8_UNORM, false, true);

  R_State r = {
    .gpu = gpu,
    .dwrite = dwrite,
    .blendParams = Win32_DWriteBlendParams(dwrite),
    .vertexShader = DX11_CreateVertexShader(gpu, vsBytes),
    .pixelShader = DX11_CreatePixelShader(gpu, psBytes),
    .samplerLinearWrap = DX11_CreateSamplerState(gpu, DX11_SAMPLER_DESC_LINEAR_WRAP),
    .blendPremultiplied = DX11_CreateBlendState(gpu, DX11_BLEND_DESC_PREMULTIPLIED_ALPHA),
    .constants = constants,
    .instances = FixedListInit(ArenaPushSlice<R_Instance>(arena, R_INSTANCE_CAPACITY)),
    .gpuInstances = gpuInstances,
    .atlas = atlas,
    .atlasTexture = atlasTexture,
  };
  return {r, vsOk && psOk && constantsOk && gpuInstancesOk && atlasOk && atlasTextureOk};
}

inline void R_Release(R_State *r)
{
  DX11_ReleaseTexture2D(&r->atlasTexture);
  GlyphAtlasRelease(&r->atlas);
  DX11_ReleaseTransientBuffer(&r->gpuInstances);
  DX11_ReleaseConstantBuffer(&r->constants);
  Win32_ComSafeRelease(&r->blendPremultiplied);
  Win32_ComSafeRelease(&r->samplerLinearWrap);
  Win32_ComSafeRelease(&r->pixelShader);
  Win32_ComSafeRelease(&r->vertexShader);
}

inline void R_Bind(R_State *r, Vector2<f32> screenSize)
{
  R_Constants constantsFrame = {
    .gammaRatios = r->blendParams.gammaRatios,
    .screenSize = screenSize,
    .enhancedContrast = r->blendParams.enhancedContrast,
  };
  DX11_UpdateConstantBuffer(r->gpu, r->constants, SliceAsBytes(SliceMake(&constantsFrame, 1)));

  auto context = r->gpu.immediateContext1;
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  context->VSSetShader(r->vertexShader, nullptr, 0);
  context->VSSetConstantBuffers(0, 1, &r->constants.buffer);
  context->VSSetShaderResources(0, 1, &r->gpuInstances.view);
  context->PSSetShader(r->pixelShader, nullptr, 0);
  context->PSSetConstantBuffers(0, 1, &r->constants.buffer);
  context->PSSetShaderResources(1, 1, &r->atlasTexture.view);
  context->PSSetSamplers(0, 1, &r->samplerLinearWrap);
  context->RSSetState(nullptr);
  context->OMSetBlendState(r->blendPremultiplied, nullptr, 0xFFFFFFFF);

  r->image = {};
}

inline void R_Flush(R_State *r)
{
  if (r->instances.count == 0)
  {
    return;
  }
  auto instanceBytes = SliceAsBytes(SliceFromFixedList(r->instances));
  DX11_UpdateTransientBuffer(r->gpu, r->gpuInstances, instanceBytes);
  auto instanceCount = static_cast<u32>(r->instances.count);
  r->gpu.immediateContext1->DrawInstanced(4, instanceCount, 0, 0);
  FixedListClear(&r->instances);
}

inline void R_Push(R_State *r, R_Instance instance)
{
  if (FixedListAppend(&r->instances, {&instance, 1}) == 0)
  {
    R_Flush(r);
    FixedListAppend(&r->instances, {&instance, 1});
  }
}

inline void R_PushRect(R_State *r, Rect<f32> dstRect, Vector4<u8> color,
                       f32 cornerRadius = 0, f32 borderThickness = 0)
{
  R_Instance instance = {
    .dstPosition = dstRect.position,
    .dstSize = dstRect.size,
    .color = color,
    .kind = R_Kind_Rect,
    .cornerRadius = cornerRadius,
    .borderThickness = borderThickness,
  };
  R_Push(r, instance);
}

inline void R_PushImage(R_State *r, DX11_Texture2D image, Rect<f32> dstRect, Rect<f32> srcRect,
                        Vector4<u8> color = {255, 255, 255, 255}, f32 cornerRadius = 0, f32 borderThickness = 0)
{
  if (r->image.view != image.view)
  {
    R_Flush(r);
    r->image = image;
    r->gpu.immediateContext1->PSSetShaderResources(2, 1, &image.view);
  }

  R_Instance instance = {
    .dstPosition = dstRect.position,
    .dstSize = dstRect.size,
    .srcPosition = srcRect.position,
    .srcSize = srcRect.size,
    .color = color,
    .kind = R_Kind_Image,
    .cornerRadius = cornerRadius,
    .borderThickness = borderThickness,
  };
  R_Push(r, instance);
}

inline void R_UploadAtlas(R_State *r)
{
  auto dirty = GlyphAtlasTakeDirty(&r->atlas);
  if (dirty.width == 0)
  {
    return;
  }
  usize start = static_cast<usize>(dirty.y) * r->atlas.size.width + dirty.x;
  auto pixels = SliceFromRange(r->atlas.coverage, start, r->atlas.coverage.count);
  DX11_UpdateTexture2D(r->gpu, r->atlasTexture, dirty, pixels, r->atlas.size.width);
}

// pen is on the baseline
inline Vector2<f32> R_PushText(R_State *r, Vector2<f32> pen, Win32_DWrite_Font font, f32 fontSize,
                               Vector4<u8> color, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  for (auto shaped : Win32_DWriteShapeText(scratch.arena, r->dwrite, font, fontSize, text, codeLigatures))
  {
    u32 resetCount = r->atlas.resetCount;
    auto [glyph, glyphOk] = Win32_DWriteGlyphFromAtlas(&r->atlas, r->dwrite, font, fontSize, shaped.index);
    if (r->atlas.resetCount != resetCount)
    {
      // the GPU texture still has the old glyphs, draw their instances before uploading the new atlas
      R_Flush(r);
    }
    R_UploadAtlas(r);

    if (glyphOk && glyph.size.width != 0)
    {
      f32 left = (pen.x + shaped.offset.x) + glyph.offset.x;
      f32 top = (pen.y + shaped.offset.y) + glyph.offset.y;

      R_Instance instance = {
        .dstPosition = {left, top},
        .dstSize = Vector2Cast<f32>(glyph.size),
        .srcPosition = Vector2Cast<f32>(glyph.position),
        .srcSize = Vector2Cast<f32>(glyph.size),
        .color = color,
        .kind = R_Kind_Glyph,
      };
      R_Push(r, instance);
    }
    pen.x += RoundHalfDownF32(shaped.advance);
  }
  return pen;
}