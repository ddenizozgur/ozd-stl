#pragma once

#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Texture.hpp"
#include "../Win32Stuff/GlyphAtlas.hpp"

using UI_Kind = u32;
enum : UI_Kind
{
  UI_Kind_Rect,
  UI_Kind_Glyph,
  UI_Kind_Image,
};

struct UI_Instance
{
  Vector2<f32> position; // top-left in pixels
  Vector2<f32> size;
  Vector2<f32> atlasPosition;
  Vector4<u8> color; // RGBA, straight alpha
  UI_Kind kind;
};

struct alignas(16) UI_Constants
{
  Vector4<f32> gammaRatios;
  Vector2<f32> screenSize;
  f32 enhancedContrast;
};

struct UI_Batch
{
  Fixed_List<UI_Instance> instances;
  DX11_Transient_Buffer gpuInstances;
};

[[nodiscard]]
inline Pair<UI_Batch, bool> UI_InitBatch(DX11_GPU gpu, Slice<UI_Instance> buffer)
{
  UI_Batch batch = {.instances = FixedListInit(buffer)};
  auto [gpuInstances, gpuInstancesOk] = DX11_CreateTransientBuffer(
    gpu, static_cast<u32>(buffer.count), sizeof(UI_Instance));
  batch.gpuInstances = gpuInstances;
  return {batch, gpuInstancesOk};
}

inline void UI_ReleaseBatch(UI_Batch *batch)
{
  DX11_ReleaseTransientBuffer(&batch->gpuInstances);
  FixedListClear(&batch->instances);
}

inline void UI_Flush(DX11_GPU gpu, UI_Batch *batch)
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

inline void UI_Push(DX11_GPU gpu, UI_Batch *batch, UI_Instance instance)
{
  if (FixedListAppend(&batch->instances, {&instance, 1}) == 0)
  {
    UI_Flush(gpu, batch);
    FixedListAppend(&batch->instances, {&instance, 1});
  }
}

inline void UI_PushRect(DX11_GPU gpu, UI_Batch *batch, Vector2<f32> position, Vector2<f32> size, Vector4<u8> color)
{
  UI_Push(gpu, batch, {.position = position, .size = size, .color = color, .kind = UI_Kind_Rect});
}

inline void UI_PushImage(DX11_GPU gpu, UI_Batch *batch, Vector2<f32> position, Vector2<f32> size, Vector4<u8> color = {255, 255, 255, 255})
{
  UI_Push(gpu, batch, {.position = position, .size = size, .color = color, .kind = UI_Kind_Image});
}

// sends the atlas area written since the last upload to the GPU
inline void UI_UploadAtlas(DX11_GPU gpu, Glyph_Atlas *atlas, DX11_Texture2D atlasTexture)
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
inline Vector2<f32> UI_PushText(DX11_GPU gpu, UI_Batch *batch, Glyph_Atlas *atlas, DX11_Texture2D atlasTexture,
                                Win32_DWrite dwrite, Vector2<f32> pen, Win32_DWrite_Font font, f32 fontSize,
                                Vector4<u8> color, String text)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  for (auto shaped : Win32_DWriteShapeText(scratch.arena, dwrite, font, fontSize, text))
  {
    u32 resetCount = atlas->resetCount;
    auto [glyph, glyphOk] = Win32_DWriteGlyphFromAtlas(atlas, dwrite, font, fontSize, shaped.index);
    if (atlas->resetCount != resetCount)
    {
      // the GPU texture still has the old glyphs, draw their instances before uploading the new atlas
      UI_Flush(gpu, batch);
    }
    UI_UploadAtlas(gpu, atlas, atlasTexture);

    if (glyphOk && glyph.size.width != 0)
    {
      f32 left = (pen.x + shaped.offset.x) + glyph.offset.x;
      f32 top = (pen.y + shaped.offset.y) + glyph.offset.y;

      UI_Instance instance = {
        .position = {left, top},
        .size = {static_cast<f32>(glyph.size.width), static_cast<f32>(glyph.size.height)},
        .atlasPosition = {static_cast<f32>(glyph.position.x), static_cast<f32>(glyph.position.y)},
        .color = color,
        .kind = UI_Kind_Glyph,
      };
      UI_Push(gpu, batch, instance);
    }
    pen.x += RoundHalfDownF32(shaped.advance);
  }
  return pen;
}