#pragma once

#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Texture.hpp"
#include "../Win32Stuff/GlyphAtlas.hpp"

using R_Kind = u32;
enum : R_Kind
{
  R_Kind_Rect,
  R_Kind_Glyph,
  R_Kind_Image,
};

struct R_Instance
{
  Vector2<f32> position; // top-left in pixels
  Vector2<f32> size;
  Vector2<f32> atlasPosition;
  Vector4<u8> color; // RGBA, straight alpha
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

struct R_Batch
{
  Fixed_List<R_Instance> instances;
  DX11_Transient_Buffer gpuInstances;
};

[[nodiscard]]
inline Pair<R_Batch, bool> R_InitBatch(DX11_GPU gpu, Slice<R_Instance> buffer)
{
  R_Batch batch = {.instances = FixedListInit(buffer)};
  auto [gpuInstances, gpuInstancesOk] = DX11_CreateTransientBuffer(
    gpu, static_cast<u32>(buffer.count), sizeof(R_Instance));
  batch.gpuInstances = gpuInstances;
  return {batch, gpuInstancesOk};
}

inline void R_ReleaseBatch(R_Batch *batch)
{
  DX11_ReleaseTransientBuffer(&batch->gpuInstances);
  FixedListClear(&batch->instances);
}

inline void R_Flush(DX11_GPU gpu, R_Batch *batch)
{
  if (batch->instances.count == 0)
  {
    return;
  }
  auto instanceBytes = SliceAsBytes(SliceFromFixedList(batch->instances));
  DX11_UpdateTransientBuffer(gpu, batch->gpuInstances, instanceBytes);
  auto instanceCount = static_cast<u32>(batch->instances.count);
  gpu.immediateContext1->DrawInstanced(4, instanceCount, 0, 0);
  FixedListClear(&batch->instances);
}

inline void R_Push(DX11_GPU gpu, R_Batch *batch, R_Instance instance)
{
  if (FixedListAppend(&batch->instances, {&instance, 1}) == 0)
  {
    R_Flush(gpu, batch);
    FixedListAppend(&batch->instances, {&instance, 1});
  }
}

inline void R_PushRect(DX11_GPU gpu, R_Batch *batch,
                       Vector2<f32> position, Vector2<f32> size,
                       Vector4<u8> color, f32 cornerRadius = 0, f32 borderThickness = 0)
{
  R_Instance instance = {
    .position = position,
    .size = size,
    .color = color,
    .kind = R_Kind_Rect,
    .cornerRadius = cornerRadius,
    .borderThickness = borderThickness,
  };
  R_Push(gpu, batch, instance);
}

inline void R_PushImage(DX11_GPU gpu, R_Batch *batch,
                        Vector2<f32> position, Vector2<f32> size,
                        Vector4<u8> color = {255, 255, 255, 255},
                        f32 cornerRadius = 0, f32 borderThickness = 0)
{
  R_Instance instance = {
    .position = position,
    .size = size,
    .color = color,
    .kind = R_Kind_Image,
    .cornerRadius = cornerRadius,
    .borderThickness = borderThickness,
  };
  R_Push(gpu, batch, instance);
}

inline void R_UploadAtlas(DX11_GPU gpu, Glyph_Atlas *atlas, DX11_Texture2D atlasTexture)
{
  auto [position, size] = GlyphAtlasTakeDirty(atlas);
  if (size.width == 0)
  {
    return;
  }
  usize start = static_cast<usize>(position.y) * atlas->size.width + position.x;
  auto pixels = SliceFromRange(atlas->coverage, start, atlas->coverage.count);
  DX11_UpdateTexture2D(gpu, atlasTexture, position, size, pixels, atlas->size.width);
}

// pen is on the baseline
inline Vector2<f32> R_PushText(DX11_GPU gpu, R_Batch *batch, Glyph_Atlas *atlas, DX11_Texture2D atlasTexture,
                               Win32_DWrite dwrite, Vector2<f32> pen, Win32_DWrite_Font font, f32 fontSize,
                               Vector4<u8> color, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  for (auto shaped : Win32_DWriteShapeText(scratch.arena, dwrite, font, fontSize, text, codeLigatures))
  {
    u32 resetCount = atlas->resetCount;
    auto [glyph, glyphOk] = Win32_DWriteGlyphFromAtlas(atlas, dwrite, font, fontSize, shaped.index);
    if (atlas->resetCount != resetCount)
    {
      // the GPU texture still has the old glyphs, draw their instances before uploading the new atlas
      R_Flush(gpu, batch);
    }
    R_UploadAtlas(gpu, atlas, atlasTexture);

    if (glyphOk && glyph.size.width != 0)
    {
      f32 left = (pen.x + shaped.offset.x) + glyph.offset.x;
      f32 top = (pen.y + shaped.offset.y) + glyph.offset.y;

      R_Instance instance = {
        .position = {left, top},
        .size = {static_cast<f32>(glyph.size.width), static_cast<f32>(glyph.size.height)},
        .atlasPosition = {static_cast<f32>(glyph.position.x), static_cast<f32>(glyph.position.y)},
        .color = color,
        .kind = R_Kind_Glyph,
      };
      R_Push(gpu, batch, instance);
    }
    pen.x += RoundHalfDownF32(shaped.advance);
  }
  return pen;
}